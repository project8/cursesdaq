#include "P8InstrumentWrangler.hh"
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
using namespace std;

P8InstrumentWrangler instrument_wrangler;


P8InstrumentWrangler::~P8InstrumentWrangler()
{
	for(list<P8Instrument*>::iterator it=instruments.begin();it!=instruments.end();it++)
		delete (*it);
	instruments.clear();
}
	
P8Instrument *P8InstrumentWrangler::connectToInstrument(string ipv4address,int gpib_address,string name)
{
	//cout << "connectToInstrument (" << ipv4address << "," << gpib_address << "," << name << ") called" << endl;
	for(list<P8Instrument*>::iterator it=instruments.begin();it!=instruments.end();it++)
		if((*it)->instrument_name==name)
		{	
			if(((*it)->IPv4address!=ipv4address)||((*it)->gpib_address)!=gpib_address)
			{
				stringstream ss;
				ss << "Device with name " << name << " was requested with address " << ipv4address << " " << gpib_address << " but already exists with address " << (*it)->IPv4address << " " << (*it)->gpib_address;
				last_error=ss.str();
				return NULL;
			}
			return (*it);
		}

	P8PrologixGPIBDevice *device=prologix_wrangler.connectToDevice(ipv4address,gpib_address);
	if(device==NULL)
	{
		stringstream ss;
		ss << "unable to connect to device " << ipv4address << " " << gpib_address << ": " << prologix_wrangler.last_error;
		last_error=ss.str();
		return NULL;
	}
	P8Instrument *ret=createInstrument(device,name);
	if(ret!=NULL) instruments.push_back(ret);
	return ret;
}
	
P8Instrument *P8InstrumentWrangler::createInstrument(P8PrologixGPIBDevice *device,string name)
{
	if(device==NULL) {cerr << "error, null device in createInstrument" << endl; return NULL;}
	if(device->getConnection()==NULL) {cerr << "error, device has NULL connection in createInstrument" << endl; return NULL;}
	P8Instrument *ret=NULL;
	string deviceid=device->getID();
	if((deviceid.size()>4)&&(deviceid.substr(0,4)=="LSCI"))
	{
		ret=new P8GenericLakeshoreInstrument(device->getConnection()->GetIPv4Address(),device->getGPIBAddress(),name,deviceid,device);
	} else
	if((deviceid.size()>22)&&(deviceid.substr(0,22)=="HEWLETT-PACKARD,E3631A"))
	{
		ret=new P8E3631(device->getConnection()->GetIPv4Address(),device->getGPIBAddress(),name,device);
	} else
	{
//		stringstream ss;
		//ss << "Unable to identify device " << deviceid << " that is to be " << name " returning generic device" << endl;
		//return NULL;
		cerr << "Unable to identify device " << deviceid << " that is to be " << name << " returning generic device" << endl;
		ret=new P8Instrument(device->getConnection()->GetIPv4Address(),device->getGPIBAddress(),name,device);
		return ret;
	}
	return ret;
}
	
SensorReading P8InstrumentWrangler::takeReading(const SensorAddress &sensor)
{
	return executeReadingScript(sensor);
}

//--------------------P8GenericLakeshoreInstrument----
//
P8GenericLakeshoreInstrument::P8GenericLakeshoreInstrument(string ip,int gpib,string name,string id,P8PrologixGPIBDevice *dev) : P8Instrument(ip,gpib,name,dev)
{
	sensor_defs=NULL;
	stringstream ss(id);
	string manu,model,serial;
	getline(ss,manu,',');
	getline(ss,model,',');
	getline(ss,serial,',');
	if(model.substr(0,8)=="MODEL218")
	{
		read_command="SRDG?";
		n_sensors=8;
		precision=2;
		units="Ohms";
		sensor_defs=new string[n_sensors];
		for(int i=0;i<n_sensors;i++)
		{
			char nm[16];
			sprintf(nm,"%d",i+1);
			sensor_defs[i]=string(nm);
		}
	} else
	if(model.substr(0,8)=="MODEL475")
	{
		read_command="RDGFIELD?";
		n_sensors=0;
		precision=2;
		units="G";
		sensor_defs=new string[1];
		sensor_defs[0]="";
	} else
	if(model.substr(0,8)=="MODEL340")
	{
		read_command="SRDG?";
		n_sensors=2;
		precision=2;
		units="Ohms";
		sensor_defs=new string[2];
		sensor_defs[0]="A";
		sensor_defs[1]="B";
	} else
	{
		cerr << "unrecognized device that is supposed to be lakeshore: " << model << endl;
	}
}
	
//depricated
SensorReading P8Instrument::takeReading(const SensorAddress &address)
{
	SensorReading ret;
	/*
	gettimeofday(&ret.timestamp,NULL);
	
	string reply=prologix_device->sendQuery(address.read_command);

	if(reply!="")
	{
		ret.value=atof(reply.c_str());
		ret.has_error=false;
	} else
	{
		ret.has_error=true;
		stringstream ss;
		ss << "blank reply returned from instrument.  TODO add further error analysis here." << endl;
		ret.error_value=ss.str();
	}
	*/
	return ret;
}

void P8Instrument::sendCommand(string command) 
{
	prologix_device->sendCommand(command);
}

SensorReading P8Instrument::sendQuery(string query,const SensorAddress &address)
{
	SensorReading ret;
	gettimeofday(&ret.timestamp,NULL);
	
	string reply=prologix_device->sendQuery(query);

	if(reply!="")
	{
		//trim off non numbers
		while((reply.size()!=0)&&(!isdigit(reply[0])))
			reply=reply.substr(1,reply.size()-1);
		ret.value=atof(reply.c_str());
		ret.has_error=false;
	} else
	{
		ret.has_error=true;
		stringstream ss;
		ss << "blank reply returned from instrument.  TODO add further error analysis here." << endl;
		ret.error_value=ss.str();
	}
	return ret;
}


bool P8GenericLakeshoreInstrument::hasSensor(const SensorAddress &address)
{
	return true;
	/*
	if(n_sensors==0) return true;
	if(address.sensor_number>=0)
	if((address.sensor_number<n_sensors)) 
		return true;
	return false;
	*/
}

list<SensorAddress> P8GenericLakeshoreInstrument::listSensors()
{
	list<SensorAddress> ret;
	/*
	if(n_sensors==0)
	{
		ret.push_back(SensorAddress(instrument_name,0));
	} else
	{
		for(int i=0;i<n_sensors;i++)
			ret.push_back(SensorAddress(instrument_name,i+1));
	}
	*/
	return ret;
}


ostream &operator<<(ostream &out,const SensorReading &d)
{
	char timestr[256];
	strftime(timestr,256,"%Y_%m_%d %H:%M:%S",localtime(&d.timestamp.tv_sec));
	if(d.has_error)
		out << "#" << d.sensor_name << " ERROR " << d.error_value;
	else
		out << d.sensor_name << " " << d.value << " " << d.units << " @ " << timestr;
	return out;
}

//------------SensorReading-----------

SensorReading::SensorReading()
{
	sensor_name="UNDEFINED";
	value=0;
	precision=2;
	units="UNDEFINED";
	has_error=false;
	error_value="UNDEFINED";
}

SensorReading P8InstrumentWrangler::executeReadingScript(const SensorAddress &sensor)
{
	//TODO init a mutex here
	//scripting format:
	//INSTRUMENT_NAME: GPIB_COMMAND;
	// -or-
	//INSTRUMENT_NAME? GPIB_QUERY;
	SensorReading ret;
	string command;
	string instrument;
	bool instrument_error=false;;
	stringstream commandstream(sensor.read_command);
	while(getline(commandstream,command,';')) {
		if(command=="") continue;
		size_t colonpos=command.find(':');
		instrument=command.substr(0,colonpos);
		bool isquery=false;
		if(command[colonpos+1]=='?') {isquery=true; colonpos++;}
			else isquery=false;
		string inst_command=command.substr(colonpos+1,string::npos);
		while((inst_command.size()>0)&&(inst_command[0]==' ')) inst_command=inst_command.substr(1,inst_command.size()-1); //trim off leading spaces
		if(inst_command.size()==0) cerr << "warning, empty instrument command for insturment " << instrument << endl;
		//TODO call instruments here
		P8Instrument *pinstrument=getInstrument(instrument);
		if(pinstrument==NULL) {instrument_error=true; break;}
		if(isquery)
			ret=pinstrument->sendQuery(inst_command,sensor);
		else
			pinstrument->sendCommand(inst_command);
	}
	//TODO end a mutex here
	if(instrument_error==true) {
		gettimeofday(&ret.timestamp,NULL);
		ret.address=sensor;
		ret.has_error=true;
		stringstream ss;
		ss << "Could not find instrument: " << instrument;
		ret.error_value=ss.str();
	}
	return ret;

}

P8Instrument *P8InstrumentWrangler::getInstrument(string name)
{
	list<P8Instrument*>::iterator it;
	for(it=instruments.begin();it!=instruments.end();it++)
		if((*it)->instrument_name==name)
			return (*it);
	return NULL;
}
