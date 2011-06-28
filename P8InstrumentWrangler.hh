#pragma once
#include "P8PrologixWrangler.hh"

class SensorAddress;
class SensorReading;

class P8Instrument
{
public:
	P8Instrument() {};
	P8Instrument(string ip,int gpib,string nm,P8PrologixGPIBDevice *dev) 
		{instrument_name=nm; IPv4address=ip; gpib_address=gpib; prologix_device=dev;};
	virtual ~P8Instrument() {};

	virtual void sendCommand(string command);
	virtual SensorReading sendQuery(string query,const SensorAddress &address);
	virtual SensorReading takeReading(const SensorAddress &address);
	virtual bool hasSensor(const SensorAddress &address) {return true;};
	virtual list<SensorAddress> listSensors() {return list<SensorAddress>();};

	string instrument_name;
	string IPv4address;
	int gpib_address;

	P8PrologixGPIBDevice *prologix_device;
};

class P8GenericLakeshoreInstrument : public P8Instrument
{
public:
	P8GenericLakeshoreInstrument(string ip,int gpib,string name,string id,P8PrologixGPIBDevice *dev);
	virtual ~P8GenericLakeshoreInstrument() {delete [] sensor_defs;};

//	virtual SensorReading takeReading(const SensorAddress &address);
	virtual bool hasSensor(const SensorAddress &address);
	virtual list<SensorAddress> listSensors();

	map<int,string> read_commands;
	//Some specific ones:
	//LS340 - SRDG? <A or B> //LS475 - RDGFIELD?
	//LS218 - SRDG? <n>

	int precision;
	string units;

	//depricated
	string read_command;
	string *sensor_defs;
	int n_sensors; //0 means 1 sensor, and no number in query

};

class P8E3631 : public P8Instrument
{
public:
	P8E3631(string ip,int gpib,string nm,P8PrologixGPIBDevice *dev) : P8Instrument(ip,gpib,nm,dev) {};
	virtual ~P8E3631() {};

};

class P8InstrumentWrangler
{
public:
	virtual ~P8InstrumentWrangler();

	P8Instrument *connectToInstrument(string IPv4address,int gpib_address,string name);
	P8Instrument *createInstrument(P8PrologixGPIBDevice *device,string name);
	P8Instrument *getInstrument(string name);
	SensorReading takeReading(const SensorAddress &sensor);
	SensorReading executeReadingScript(const SensorAddress &sensor);

	list<P8Instrument*> instruments;
	string last_error;
};

extern P8InstrumentWrangler instrument_wrangler;

class SensorAddress
{
public:
	SensorAddress() {};
	SensorAddress(string iname,string command) {instrument_name=iname; read_command=command;};
	string instrument_name; //depricated
	string read_command; //these are the commands sent to instruments
};

class SensorReading
{
public:
	SensorReading();
	SensorAddress address;
	string sensor_name;
	double value;
	int precision;
	string units;
	struct timeval timestamp;
	bool has_error;
	string error_value;
};

ostream &operator<<(ostream &out,const SensorReading &d);
