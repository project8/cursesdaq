#include "P8PrologixWrangler.hh"
#include "P8InstrumentWrangler.hh"
#include <errno.h>
#include <iostream>
#include <stdio.h>
using namespace std;

int main(int argc,char *argv[])
{
	string address[2];
	address[0]="10.0.0.3";
	address[1]="10.0.0.4";
	P8PrologixConnection *connection[2];
	connection[0]=NULL;
	connection[1]=NULL;


	for(int i=0;i<2;i++)
	{
		cerr << "Connecting to " << address[i] << endl;
		connection[i]=prologix_wrangler.connectToBox(address[i]);
		if(connection[i]==NULL)
		{
			cerr << "FAILED: " << prologix_wrangler.last_error << endl;
			continue;
		}
		cerr << "Scanning " << address[i] << endl;
		map<int,string> devices=connection[i]->scan_for_devices(1,4);
		for(map<int,string>::iterator it=devices.begin();it!=devices.end();it++)
		{
			cerr << "Found " << (*it).second << " on gpib address " << (*it).first << endl;
		}
	}
	/*
	int gpib[2];
	gpib[0]=1;
	gpib[1]=2;
	cerr << "connecting to devices" << endl;
	for(int i=0;i<4;i++)
	{
		char name[256];
		sprintf(name,"Device_%d",i);
		P8Instrument *inst=NULL;

		if((inst=instrument_wrangler.connectToInstrument(address[i/2],i%2,"Device_1"))==NULL)
			cerr << "failed: " << instrument_wrangler.last_error << endl;
		else
			cerr << "success! " << inst->prologix_device->getID() << endl;
	}

	for(int i=0;i<4;i++)
	{
		char name[256];
		sprintf(name,"Device_%d",i);
		cerr << "reading " << name << " sensor 1" << endl;
		cerr << instrument_wrangler.takeReading(SensorAddress(name,1)) << endl;
	}
	*/
}
