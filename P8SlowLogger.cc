#include "P8SlowLogger.hh"
#include <math.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/time.h>
#include <vector> 
#include <string>
#include <cstdlib>
#include <algorithm>
using namespace std;

//the difference in seconds between two timevals
double seconds_difference(const struct timeval &now,const struct timeval &before);


P8SlowLoggerSensor::P8SlowLoggerSensor()
{
	last_reading.timestamp.tv_sec=0;
	last_reading.timestamp.tv_usec=0;
}

//
P8SlowLoggerSensor_Cal::P8SlowLoggerSensor_Cal()
{
	last_reading.timestamp.tv_sec=0;
	last_reading.timestamp.tv_usec=0;
	logx=false;
	logy=false;
}
//INTERPOLATION BETWEEN TWO POINTS TO CALIBRATE
void P8SlowLoggerSensor_Cal::setCalibrationValues(string calfile)
{
//	string temp_name = name.append(".calib");
//	ifstream fin(temp_name.c_str());//name.append(".calib"));
	ifstream fin(calfile.c_str());
	if(!fin.good()) cerr<< "cannot open calib file" << calfile << endl;
	string line;

	while(getline(fin,line))					
	{
		if(line=="") continue;
		if(line[0] == '#') {
			if(line.substr(1,line.size()-1)=="logx") logx=true;
			if(line.substr(1,line.size()-1)=="logy") logy=true;
			if(line.size()>5&&(line.substr(1,5)=="units")) {
				stringstream ss(line.substr(6,line.size()-1));
				ss >> units;
			}				
			continue;
		} else if(line[0] == '!') 
		{ 
			stringstream s(line);
			string temp; 
			s >> temp;
			units = temp.replace(0,1,"");
			continue; 	
		}
		/*
		if(line[0] == '$') 
		{ 
			stringstream s(line);
			string temp; 
			s >> temp;
			xaxis = temp.replace(0,1,"");
			continue; 	
		}
		*/
		stringstream ss(line);
		double x,y;
		ss >> x >> y;
		calibration_lookup.push_back(make_pair(x,y));
//		lookup_x.push_back(atof(x.c_str()));
//		lookup_y.push_back(atof(y.c_str()));
	}	
	//sort by x values (then by y values)
	sort(calibration_lookup.begin(),calibration_lookup.end());
	fin.close();
}
double P8SlowLoggerSensor_Cal::getCalibratedValue(double orig_value)
{
	/*
	double calibrated_value;
	double slope;
	double intcpt;
	int index;
	double cal_x; 
	if(xaxis == "log")
	{
		cal_x = log(orig_value);
	} 
	else if (xaxis == "log10")
	{
		cal_x = log10(orig_value);
	}
	else if (xaxis == "exp")
	{
		cal_x = exp(orig_value);
	}
	else if(xaxis == "linear")
	{
		cal_x = orig_value; 
	}
	for(vector<double>::const_iterator it = lookup_x.begin(); it != lookup_x.end(); it++)
	{
		if(cal_x <= *it) break;
		//if(orig_value <= *it) break;
		index++;
	}	

	slope = (lookup_y[index]-lookup_y[index-1])/(lookup_x[index]-lookup_x[index-1]);
	intcpt = lookup_y[index] - slope * lookup_x[index]; 
	calibrated_value = slope * cal_x + intcpt; 
	return calibrated_value; 
	*/
	double x=orig_value;
	double x1,x2,y1,y2;
	if(orig_value<calibration_lookup[0].first) {
		x1=calibration_lookup[0].first;
		x2=calibration_lookup[1].first;
		y1=calibration_lookup[0].second;
		y2=calibration_lookup[1].second;	
	} else
	for(size_t i=0;(i+1)<calibration_lookup.size();i++) {
		if(((orig_value>calibration_lookup[i].first)&&
		   (orig_value<calibration_lookup[i+1].first))||
				(i+2==calibration_lookup.size())) {
			x1=calibration_lookup[i].first;
			x2=calibration_lookup[i+1].first;
			y1=calibration_lookup[i].second;
			y2=calibration_lookup[i+1].second;
			break;
		}
	}
//cerr << "name " << name << "x is " << x << " x1 " << x1 << " y1 " << y1 << " x2 " << x2 << " y2 " << y2 << endl; 
	if(logx) {
		x=log(x);
		x1=log(x1);
		x2=log(x2);
	}
	if(logy) {
		y1=log(y1);
		y2=log(y2);
	}
	double y=y1+(y2-y1)*(x-x1)/(x2-x1);
	if(logy) y=exp(y);
	return y;
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
	for(list<P8SlowLoggerSensor_Cal>::iterator it=sensors_cal.begin();it!=sensors_cal.end();it++)
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
		for(list<P8SlowLoggerSensor_Cal>::iterator it=sensors_cal.begin();it!=sensors_cal.end();it++)		//LOOP OVER CALIBRATED SENSORS
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
/*	
//PRINTS CALIBRATED SENSOR READING TO A STRING**********************
string P8SlowLogger::printCalibratedSensorReading(SensorReading &reading)
{
	char outline[1024];
	sprintf(outline, "%g %s", reading.value, reading.units.c_str()); 
	return string(outline); 	
}
//
*/
JSONObject P8SlowLogger::getJSONReading(SensorReading &reading) {
	JSONObject ret;
	ret["sensor_name"]=reading.sensor_name;
	long long mseconds=((long long)reading.timestamp.tv_sec)*1000;
	mseconds+=reading.timestamp.tv_usec/1000;
	ret["timestamp_mseconds"].setIntValue(mseconds);
	char reading_timestamp[256];
	strftime(reading_timestamp,256,"%Y-%m-%d %H:%M:%S",localtime(&reading.timestamp.tv_sec));
	ret["timestamp_localstring"].setStringValue(reading_timestamp);
	ret["value"].setDoubleValue(reading.value);
	ret["units"].setStringValue(reading.units);
	ret["precision"].setDoubleValue(reading.precision);
	if(reading.has_error) {
		ret["has_error"].setBoolValue(true);
		ret["error_value"].setStringValue(reading.error_value);
	}
	if(reading.is_calibrated) {
		ret["uncalibrated_value"].setDoubleValue(reading.orig_value);
		ret["uncalibrated_units"].setStringValue(reading.orig_units);
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
//OVERLOAD THE DOLOG FUNCTION TO HAVE P8SLOWLOGGERSENSOR_CAL PARAMETER	
void P8SlowLogger::doLog(P8SlowLoggerSensor_Cal &sensor)
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
	reading.units=sensor.units;								//
	reading.is_calibrated=true;
	reading.orig_value=reading.value;
	reading.orig_units=sensor.orig_units;
	reading.value = sensor.getCalibratedValue(reading.value);				//REPLACES VALUE WITH CALIBRATED VALUE 
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
		} 
		else if(type=="SENSOR_CAL")
		{
			string name,command;
			double minlts,maxlts,minch;

			ss >> name;
			string com;
			getline(ss,com,'\"'); //read command in quotes
			getline(ss,com,'\"');
			string un;
			string mylogfile;
			string mycalfile;
		    ss >> minlts >> maxlts >> minch >> un >> mylogfile >> mycalfile;
			cerr << "adding sensor " << name << endl;
			cout << "adding sensor " << name << endl;
			P8SlowLoggerSensor_Cal toadd;
			toadd.address=SensorAddress("",com);
			toadd.name=name;
			toadd.min_log_time_spacing=minlts;
			toadd.max_log_time_spacing=maxlts;
			toadd.min_change_for_logging=minch;
			toadd.orig_units=un;
			toadd.last_reading.precision=4;
			toadd.log_name=mylogfile;
			toadd.setCalibrationValues(mycalfile);				//SETS CALIBRATION 
			sensors_cal.push_back(toadd);
		}else if (type=="LOGFILE") 
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
	
/* TODO
void P8SlowLogger::load_config_file_json(string fname)
{
	ifstream fin(fname.c_str());
	if(!fin.good()) {
		cerr << "error loading file " << fname << endl;
		exit(0);
	}
	JSONObject config;
	fin >> config;
	fin.close();
	couchdb.setServer(config["database"].getObjectValue()["host"]);
	couchdb.setPort(config["database"].getObjectValue()["port"]);
	couchdb.setDBName(config["database"].getObjectValue()["dbname"]);

}
*/
