//Interface for the Prologix GPIB-Ethernet Converter
//4/19/2011 - Gray Rybka

#pragma once
#include "P8EthernetSocket.hh"
#include <list>
#include <map>
using namespace std;

//This is an ethernet connection to a prologix box
class P8PrologixConnection : public P8EthernetSocket
{
public:
	P8PrologixConnection();
	P8PrologixConnection(string ipv4address);
	virtual bool Connect();

	virtual ~P8PrologixConnection() {};

	//high-level interface to talk to the various devices
	map<int,string> scan_for_devices(int address_start,int address_stop);
	void send_gpib_command(string command,int gpib_address);
	string send_gpib_query(string command,int gpib_address);


	//mid-level commands that talk to the box itself
	bool send_box_command(string command);
	string send_box_query(string command);
	int lock_box() {return box_claimed.Lock();}; //keepin it threadsafe
	int unlock_box() {return box_claimed.UnLock();};
	void set_query_timeout(int ms); //milliseconds
	int get_query_timeout() {return query_timeout_ms;};

	static string escape_special_characters(const string &in);

private:
	//low level functions that actually do the work, but would be
	//thread unsafe if anyone called them carelessly
	bool send_box_command_nolock(string command);
	string send_box_query_nolock(string command);
	bool set_address_nolock(int addr);
	//this keeps multiple threads from interfering with each other
	P8Mutex box_claimed;
	//the last GPIB address spoken to, to save time in unnecessary switching
	int last_address;
	//how long to wait for a query to respond, both from the prologix box and ethernet socket
	int query_timeout_ms;
};

//this is a GPIB device attached to a prologix box connection
class P8PrologixGPIBDevice
{
public:
	P8PrologixGPIBDevice() {};
	P8PrologixGPIBDevice(P8PrologixConnection *c,int gpibid,string n) 
		{setConnection(c); setGPIBAddress(gpibid); setName(n);};
        

	string getAddressString();
	void setGPIBAddress(int n) {gpib_address=n;};
	int getGPIBAddress() const {return gpib_address;};
	void setConnection(P8PrologixConnection *c) {connection=c;};
	P8PrologixConnection *getConnection() const {return connection;};
	string getName() const {return name;};
	void setName(string n) {name=n;};
	void sendCommand(string s)
		{connection->send_gpib_command(s,getGPIBAddress());}
	string sendQuery(string s)
		{return connection->send_gpib_query(s,getGPIBAddress());}

	string getID();

private:
	P8PrologixConnection *connection;
	int gpib_address;
	string name;
};

class P8PrologixWrangler
{
public:
	~P8PrologixWrangler();
	P8PrologixConnection *connectToBox(const string &ipv4address);
	P8PrologixGPIBDevice *connectToDevice(const string &ipv4address,int gpib_address);

	list<P8PrologixConnection*> open_sockets;
	list<P8PrologixGPIBDevice*> open_devices;

	string last_error;
};

extern P8PrologixWrangler prologix_wrangler;
