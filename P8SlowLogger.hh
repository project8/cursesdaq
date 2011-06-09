//this is a thread that periodically logs sensors in plaintext

#pragma once
#include "P8InstrumentWrangler.hh"
#include "Thread.h"

class P8SlowLoggerSensor;

class P8SlowLogger : public Thread
{
public:
	P8SlowLogger() : log_file_name("./log") {sleep_time_usec=10000;};

	void load_config_file(string fname);
	SensorReading getLastReading(string sensor_name);
	string getLogFileName();
	string printSensorReading(SensorReading &reading);

	//call start_thread() to start
	void stop() {running=false;};

	list<P8SlowLoggerSensor> sensors;
	string log_file_name;
	long sleep_time_usec;

	bool running;
private:
	virtual int run();
	void doLog(P8SlowLoggerSensor &sensor);
};

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
	
	P8Mutex access_mutex;
};
