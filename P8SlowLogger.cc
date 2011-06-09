#include "P8SlowLogger.hh"
#include <math.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/time.h>
using namespace std;

double seconds_difference(const struct timeval &now,const struct timeval &before);


P8SlowLoggerSensor::P8SlowLoggerSensor()
{
	last_reading.timestamp.tv_sec=0;
	last_reading.timestamp.tv_usec=0;
}

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
	
string P8SlowLogger::printSensorReading(SensorReading &reading)
{
	char reading_timestamp[256];
	strftime(reading_timestamp,256,"%Y-%m-%d %H:%M:%S",localtime(&reading.timestamp.tv_sec));
	char outline[512];
	sprintf(outline,"%-32s %s.%06d %g %s",reading.sensor_name.c_str(),reading_timestamp,(int)reading.timestamp.tv_usec,reading.value,reading.units.c_str());
	return string(outline);
}
	
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
	sensor.access_mutex.Lock();
	sensor.last_reading=reading;
	sensor.access_mutex.UnLock();
	string outfilename=getLogFileName();
	string outline=printSensorReading(reading);
//	char reading_timestamp[256];
//	strftime(reading_timestamp,256,"%Y-%m-%d %H:%M:%S",localtime(&reading.timestamp.tv_sec));
//	char outline[512];
//	sprintf(outline,"%-32s %s.%06d %g %s",reading.sensor_name.c_str(),reading_timestamp,(int)reading.timestamp.tv_usec,reading.value,reading.units.c_str());
	ofstream out(outfilename.c_str(),ios::app);
	//out << reading << endl;
	out << outline << endl;
	out.close();
}
	
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
			cerr << "Connecting to instrument: " << name << endl;
			if(instrument_wrangler.connectToInstrument(ip,gpib,name)==NULL)
				cerr << "Could not connect: " << instrument_wrangler.last_error << endl;
			else cerr << "Connected" << endl;
		} else
		if(type=="SENSOR")
		{
			string name,inst,command;
			double minlts,maxlts,minch;

			ss >> name >> inst;
			string com;
			getline(ss,com,'\"'); //read command in quotes
			getline(ss,com,'\"');
			string un;
		    ss >> minlts >> maxlts >> minch >> un;
			cerr << "adding sensor " << name << endl;
			P8SlowLoggerSensor toadd;
			toadd.address=SensorAddress(inst,com);
			toadd.name=name;
			toadd.min_log_time_spacing=minlts;
			toadd.max_log_time_spacing=maxlts;
			toadd.min_change_for_logging=minch;
			toadd.units=un;
			toadd.last_reading.precision=4;
			sensors.push_back(toadd);
		} else
		{
			cerr << "unrecognized type in config file: " << type << endl;
		}
	}
	fin.close();
	
	//debug enumerate instruments
	cerr << "enumerating instruments " << endl;
	for(list<P8Instrument*>::iterator it=instrument_wrangler.instruments.begin();it!=instrument_wrangler.instruments.end();it++)
	{
		cerr << "Instrument: |" << (*it)->instrument_name << "|" << endl;
	//	list<SensorAddress> addresses=(*it)->listSensors();
	}
}

double seconds_difference(const struct timeval &now,const struct timeval &before)
{
	double ret=0;
	ret+=((double)now.tv_sec)-((double)before.tv_sec);
	ret+=1e-6*(((double)now.tv_usec)-((double)before.tv_usec));
	return ret;
}
