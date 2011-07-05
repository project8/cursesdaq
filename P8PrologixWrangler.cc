#include "P8PrologixWrangler.hh"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <errno.h>
#include <string.h>

#define PROLOGIX_DEBUG_ON
	
//-------------------P8PrologixConnection-------------
P8PrologixConnection::P8PrologixConnection()
{
	SetPort("1234");
	last_address=-1;
	query_timeout_ms=1000;
}

P8PrologixConnection::P8PrologixConnection(string ipv4address)
{
	SetPort("1234");
	SetIPv4Address(ipv4address);
	last_address=-1;
	query_timeout_ms=1000;
}
	
bool P8PrologixConnection::Connect()
{
	lock_box();
	//cennect to the ethernet socket
	if(!P8EthernetSocket::Connect()) return false;
	//perform some initialization functions
	bool good=true;
	good&=send_box_command_nolock("++mode 1");
	good&=send_box_command_nolock("++auto 0");
	good&=send_box_command_nolock("++eoi 1");
	good&=send_box_command_nolock("++eos 3");
	//TODO check version is compatible
	unlock_box();
	//setup the read timeout (keep outside of lock)
	set_query_timeout(get_query_timeout());
	return good;
}
	
void P8PrologixConnection::set_query_timeout(int ms) //milliseconds
{
	query_timeout_ms=ms;
	char command[256];
	sprintf(command,"++read_tmo_ms %d",query_timeout_ms);
	send_box_command(command);
}
	
bool P8PrologixConnection::set_address_nolock(int addr)
{
	if(last_address==addr) return true;
	char command[256];
	sprintf(command,"++addr %d\n",addr);
	last_address=addr;
	return send_box_command_nolock(command);
}

bool P8PrologixConnection::send_box_command_nolock(string command)
{
	if(command[command.size()-1]!='\n')
		command.append("\n");
#ifdef PROLOGIX_DEBUG_ON
	cerr << "Sending command |" << command.substr(0,command.size()-1) << "| to box" << endl;
#endif
	return Send(command);
}

bool P8PrologixConnection::send_box_command(string command)
{
	lock_box();
	bool ret=send_box_command_nolock(command);
	unlock_box();
	return ret;
}

string P8PrologixConnection::send_box_query_nolock(string command)
{
	//remove any data that might be clogging the ethernet socket
	while(dataWaiting())
	{
		cerr << "Warning, data waiting on prologix socket.  Clearing" << endl;
		size_t t;
		char trash[256];
		Receive(trash,256,&t);
	}
	//always append line feed to message
	if(command[command.size()-1]!='\n')
		command.append("\n");

	#ifdef PROLOGIX_DEBUG_ON
		cerr << "Sending query |" << command.substr(0,command.size()-1) << "| to box" << endl;
	#endif

	Send(command);
	Send("++read 10\n"); //read until line feed
	string ret=ReceiveLine(query_timeout_ms);
#ifdef PROLOGIX_DEBUG_ON
	cerr << "received response |" << ret << "|" << endl;
#endif
	return ret;

}

string P8PrologixConnection::send_box_query(string command)
{
	lock_box();
	string ret=send_box_query_nolock(command);
    unlock_box();
	return ret;
}

void P8PrologixConnection::send_gpib_command(string command,int gpib_address)
{
	lock_box();
   	set_address_nolock(gpib_address);
	string escaped_command=escape_special_characters(command);
	send_box_command_nolock(escaped_command);
	unlock_box();
}

string P8PrologixConnection::send_gpib_query(string command,int gpib_address)
{
	lock_box();
	set_address_nolock(gpib_address);
	string escaped_command=escape_special_characters(command);
	string ret=send_box_query_nolock(escaped_command);
	unlock_box();
	return ret;
}

map<int,string> P8PrologixConnection::scan_for_devices(int address_start,int address_stop)
{
	lock_box();
	last_address=-1;
	map<int,string> ret;
	for(int i=address_start;i<address_stop;i++)
	{
		set_address_nolock(i);
		string s=send_box_query_nolock("*IDN?");
		if(s!="") ret[i]=s;
	}
	unlock_box();
	return ret;
}
	
//the little prologix boxes need certain characters escaped when sent
//so they can be passed on to the GPIB device.  this function
//escapes those characters
string P8PrologixConnection::escape_special_characters(const string &in)
{
	string out;
	for(size_t i=0;i<in.size();i++)
	{
		char c=in[i];
		if((c==13)||(c==10)||(c==27)||(c==43))
			out.push_back((char)27);
		out.push_back(c);
	}
	if(out[out.size()-1]!='\n')
		out.push_back('\n');
	return out;
}
	
//---------------------P8PrologixGPIBDevice-------------

string P8PrologixGPIBDevice::getID()
{
	return connection->send_gpib_query("*IDN?",getGPIBAddress());
}
	
string P8PrologixGPIBDevice::getAddressString()
{
	stringstream addrwriter;
	if(connection)
		addrwriter << connection->GetIPv4Address() << "-" << connection->GetPort() << "-";
	addrwriter << getGPIBAddress();
	return addrwriter.str();
}
	
//---------------------P8PrologixWrangler---------------
P8PrologixWrangler prologix_wrangler;

P8PrologixWrangler::~P8PrologixWrangler()
{
	for(list<P8PrologixGPIBDevice*>::iterator it=open_devices.begin();it!=open_devices.end();it++)
		delete (*it);
	open_devices.clear();
	for(list<P8PrologixConnection*>::iterator it=open_sockets.begin();it!=open_sockets.end();it++)
		delete (*it);
	open_sockets.clear();
}

P8PrologixConnection *P8PrologixWrangler::connectToBox(const string &ipv4address)
{
	for(list<P8PrologixConnection*>::iterator it=open_sockets.begin();it!=open_sockets.end();it++)
		if((*it)->GetIPv4Address()==ipv4address)
			return (*it);
	P8PrologixConnection *ret=new P8PrologixConnection(ipv4address);
	if(ret->Connect()) 
	{
		open_sockets.push_back(ret);
		return ret;
	} else
	{
		last_error="Prologix-Ethernet Connection Failed: "+string(strerror(errno));
	}
	delete ret;
	return NULL;
}

P8PrologixGPIBDevice *P8PrologixWrangler::connectToDevice(const string &ipv4address,int gpib_address)
{
	last_error="";
	#ifdef PROLOGIX_DEBUG_ON
	cerr << "connectToDevice( " << ipv4address << "," << gpib_address <<") called" << endl;
	#endif
	for(list<P8PrologixGPIBDevice*>::iterator it=open_devices.begin();it!=open_devices.end();it++)
		if(((*it)->getConnection()->GetIPv4Address()==ipv4address)&&((*it)->getGPIBAddress()==gpib_address))
		{
	#ifdef PROLOGIX_DEBUG_ON
			cerr << "found device " << (*it) << " , returning" << endl;
	#endif
			return (*it);
		}
	P8PrologixConnection *connection=connectToBox(ipv4address);
	if(connection==NULL) return NULL;
	P8PrologixGPIBDevice *ret=new P8PrologixGPIBDevice(connection,gpib_address,"");
/*
	ret->setName(ret->getID());
	if(ret->getName()=="")
	{
		delete ret;
		stringstream ss;
		ss << "No device found at address " << ipv4address << " " << gpib_address << endl;
		last_error=ss.str();
		return NULL;
	}
*/
	open_devices.push_back(ret);
	#ifdef PROLOGIX_DEBUG_ON
			cerr << "created device " << ret << " , returning" << endl;
	#endif

	return ret;
}
