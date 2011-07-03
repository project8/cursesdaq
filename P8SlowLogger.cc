#include "P8SlowLogger.hh"
#include <math.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/time.h>
using namespace std;

//the difference in seconds between two timevals
double seconds_difference(const struct timeval &now,const struct timeval &before);


P8SlowLoggerSensor::P8SlowLoggerSensor()
{
	last_reading.timestamp.tv_sec=0;
	last_reading.timestamp.tv_usec=0;
}

//retrieve the last reading from a sensor named sensor name
SensorReading P8SlowLogger::getLastReading(string sensor_name)
{
	SensorReading ret;
	ret.has_error=true;
	ret.error_value="Sensor Not Found";
	for(list<P8SlowLoggerSensor>::iterator it=sensors.begin();it!=sensors.end();it++)
	{
		if((*it).name==sensor_name)
		{
			(*it).access_mutex.Lock();
			ret=(*it).last_reading;
			(*it).access_mutex.UnLock();
			break;
		}
	}
	return ret;
}

//the main thread of the logger.  periodically calls doLog
int P8SlowLogger::run()
{
	running=true;
	while(running)
	{
		for(list<P8SlowLoggerSensor>::iterator it=sensors.begin();it!=sensors.end();it++)
		{
			doLog((*it));
		}
		usleep(sleep_time_usec);
	}
	return 0;
}
	
//prints out a sensor reading to a string
string P8SlowLogger::printSensorReading(SensorReading &reading)
{
	char reading_timestamp[256];
	strftime(reading_timestamp,256,"%Y-%m-%d %H:%M:%S",localtime(&reading.timestamp.tv_sec));
	char outline[1024];
	if(!reading.has_error)
		sprintf(outline,"%-32s %s.%06d %g %s",reading.sensor_name.c_str(),reading_timestamp,(int)reading.timestamp.tv_usec,reading.value,reading.units.c_str());
	else
		sprintf(outline,"#%-32s %s.%06d Error: %s",reading.sensor_name.c_str(),reading_timestamp,(int)reading.timestamp.tv_usec,reading.error_value.c_str());
	return string(outline);
}
	
JSONObject P8SlowLogger::getJSONReading(SensorReading &reading) {
	JSONObject ret;
	ret["sensor_name"]=reading.sensor_name;
//TODO check this is the right timestamp format
	ret["timestamp_seconds"].setIntValue(reading.timestamp.tv_sec);
	ret["timestamp_useconds"].setIntValue(reading.timestamp.tv_usec);
	ret["value"]=reading.value;
	ret["units"]=reading.units;
	ret["precision"]=reading.precision;
	if(reading.has_error) {
		ret["has_error"].setBoolValue(true);
		ret["error_value"].setStringValue(reading.error_value);
	}
	return ret;
}
	
//checks if its time to read a sensor, if so, reads it and logs it
void P8SlowLogger::doLog(P8SlowLoggerSensor &sensor)
{
	//if its been too short a time, return
	struct timeval now;
	gettimeofday(&now,NULL);
	double timesincelast=seconds_difference(now,sensor.last_reading.timestamp);
	if(timesincelast<sensor.min_log_time_spacing) return;
	//take a reading
	//cerr << "taking reading on " << sensor.name << endl;
	SensorReading reading=instrument_wrangler.takeReading(sensor.address);
	reading.sensor_name=sensor.name;
	reading.units=sensor.units;
	//if the reading hasn't changed significantly, skip it unless beyond max long spacing
	if(timesincelast<sensor.max_log_time_spacing)
	{
		if(reading.has_error)
		{
			if(reading.error_value==sensor.last_reading.error_value) return;
		} else
		{
			double valchange=fabs(sensor.last_reading.value-reading.value);
			if(valchange<sensor.min_change_for_logging) return;
		}
	}
	//record the reading
	sensor.access_mutex.Lock();
	sensor.last_reading=reading;
	sensor.access_mutex.UnLock();
	string outfilename=getLogFileNameFromName(sensor.log_name);
	string outline=printSensorReading(reading);
	//print the reading to the log file
	ofstream out(outfilename.c_str(),ios::app);
	out << outline << endl;
	out.close();
	if(couch_logging_on) {
		JSONObject document=getJSONReading(reading);
		couchdb_mutex.Lock();
		if(!couchdb.sendDocument(document)) {
		if(couchdb.getLastCurlError()!="")
			cerr << "Curl error with database access: " << couchdb.getLastCurlError() << endl;
		else
			cerr << "Database responded with error: " << couchdb.getLastCouchResult() << endl;
		}
		couchdb_mutex.UnLock();
	}
}
	
//returns the default log file name
string P8SlowLogger::getLogFileName()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	struct tm *tvtm=localtime(&(tv.tv_sec));
	char fname[512];
	char date_as_char[256];
	strftime(date_as_char,256,"%Y-%m-%d",tvtm);
	sprintf(fname,"%s_%s.log",log_file_name.c_str(),date_as_char);
	return string(fname);
}

//adds date and .log to log file stub
string P8SlowLogger::getLogFileNameFromStub(string stub)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	struct tm *tvtm=localtime(&(tv.tv_sec));
	char fname[512];
	char date_as_char[256];
	strftime(date_as_char,256,"%Y-%m-%d",tvtm);
	sprintf(fname,"%s_%s.log",stub.c_str(),date_as_char);
	return string(fname);
}

//retrieves the specific log file a sensor is asking for
string P8SlowLogger::getLogFileNameFromName(string name)
{
	map<string,string>::iterator found=log_file_names.find(name);
	if(found==log_file_names.end()) return getLogFileNameFromStub("unknown_log");
	return getLogFileNameFromStub((*found).second);
}

//loads a configuration file with instruments, sensors, and logfile definitions
void P8SlowLogger::load_config_file(string fname)
{
	ifstream fin(fname.c_str());
	if(!fin.good()) cerr << "cannot open config file " << fname << endl;
	string line;
	while(getline(fin,line))
	{
		if(line=="") continue;
		if(line[0]=='#') continue;
		stringstream ss(line);
		string type;
		ss >> type;
		if(type=="INSTRUMENT")
		{
			string name,ip;
			int gpib;
			ss >> name >> ip >> gpib;
			cerr << "Connecting to instrument: " << name << " @ " << ip << " " << gpib << endl;
			cout << "Connecting to instrument: " << name << " @ " << ip << " " << gpib << endl;
			if(instrument_wrangler.connectToInstrument(ip,gpib,name)==NULL)
			{
				cerr << "Could not connect: " << instrument_wrangler.last_error << endl;
				cout << "Could not connect: " << instrument_wrangler.last_error << endl;
			}
			else {
				cerr << "Connected" << endl;
				cout << "Connected" << endl;
			}
		} else
		if(type=="SENSOR")
		{
			string name,command;
			double minlts,maxlts,minch;

			ss >> name;
			string com;
			getline(ss,com,'\"'); //read command in quotes
			getline(ss,com,'\"');
			string un;
			string mylogfile;
		    ss >> minlts >> maxlts >> minch >> un >> mylogfile;
			cerr << "adding sensor " << name << endl;
			cout << "adding sensor " << name << endl;
			P8SlowLoggerSensor toadd;
			toadd.address=SensorAddress("",com);
			toadd.name=name;
			toadd.min_log_time_spacing=minlts;
			toadd.max_log_time_spacing=maxlts;
			toadd.min_change_for_logging=minch;
			toadd.units=un;
			toadd.last_reading.precision=4;
			toadd.log_name=mylogfile;
			sensors.push_back(toadd);
		} else if (type=="LOGFILE") 
		{
			string name,filename;
			ss >> name >> filename;
			log_file_names[name]=filename;
		} else if (type=="DATABASE")
		{ 
			string host,port,dbname;
			ss >> host >> port >> dbname;
			couchdb.setServer(host);
			couchdb.setPort(port);
			couchdb.setDBName(dbname);
			couch_logging_on=true;
		} else
		{
			cerr << "unrecognized type in config file: " << type << endl;
		}
	}
	fin.close();
	
	//debug enumerate instruments
	cerr << "enumerating instruments " << endl;
	cout << "enumerating instruments " << endl;
	for(list<P8Instrument*>::iterator it=instrument_wrangler.instruments.begin();it!=instrument_wrangler.instruments.end();it++)
	{
		cerr << "Instrument: |" << (*it)->instrument_name << "|" << endl;
		cout << "Instrument: |" << (*it)->instrument_name << "|" << endl;
	//	list<SensorAddress> addresses=(*it)->listSensors();
	}
}

//returs the difference between two timevals in seconds
double seconds_difference(const struct timeval &now,const struct timeval &before)
{
	double ret=0;
	ret+=((double)now.tv_sec)-((double)before.tv_sec);
	ret+=1e-6*(((double)now.tv_usec)-((double)before.tv_usec));
	return ret;
}
