//this is a thread that periodically logs sensors in plaintext

#pragma once
#include "P8InstrumentWrangler.hh"
#include "Thread.h"
#include "CouchDBInterface.hh"

class P8SlowLoggerSensor;

class P8SlowLogger : public Thread
{
public:
	P8SlowLogger() : log_file_name("./log") {sleep_time_usec=10000; couch_logging_on=false;};

	void load_config_file(string fname);
	SensorReading getLastReading(string sensor_name);
	double calibratedValue(SensorReading &reading);		//calibrates sensor reading
	string getLogFileName();
	string getLogFileNameFromStub(string stub);
	string getLogFileNameFromName(string name);
	string printSensorReading(SensorReading &reading);
	JSONObject getJSONReading(SensorReading &reading);

//	string printCalibratedSensorReading(SensorReading &reading); 	//PRINTS CALIBRATED READING 
	list<P8SlowLoggerSensor_Cal> sensors_cal; 			//LISTS CALIBRATED SENSORS

	//call start_thread() to start
	void stop() {running=false;};

	list<P8SlowLoggerSensor> sensors;
	string log_file_name;
	map<string,string> log_file_names;
	long sleep_time_usec;

	bool running;
private:
	virtual int run();
	void doLog(P8SlowLoggerSensor &sensor);
	void doLog(P8SlowLoggerSensor_Cal &sensor);			//Overload function for calibrated sensor
	
	P8Mutex couchdb_mutex;
	CouchDBInterface couchdb;
	bool couch_logging_on;
};

//This represents a sensor to be logged
class P8SlowLoggerSensor
{
public:
	P8SlowLoggerSensor();

	SensorAddress address;
	string name;
	SensorReading last_reading;
	double min_log_time_spacing;
	double max_log_time_spacing;
	double min_change_for_logging;
	string units;
	string log_name;
	
	P8Mutex access_mutex;
};

//COPY OF P8SLOWLOGGERSENSOR

class P8SlowLoggerSensor_Cal
{
public:
	P8SlowLoggerSensor_Cal();

	SensorAddress address;
	string name;
	SensorReading last_reading;
	double min_log_time_spacing;
	double max_log_time_spacing;
	double min_change_for_logging;
	string units;
	string log_name;

	vector<double> lookup_x;
	vector<double> lookup_y; 	
	
	void setCalibrationValues();
	double getCalibratedValue(double orig_value); 
//	double slope;
//	double intcpt; 
	P8Mutex access_mutex;
};
