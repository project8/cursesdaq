/*
 * P8EthernetSocket.hh
 *
 * created on: Jan 12, 2011
 * author: dlfurse
 */

#ifndef ETHERNETSOCKET_HH_
#define ETHERNETSOCKET_HH_

#include "P8Socket.hh"
#include "P8Mutex.hh"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include <cstring>

#include <string>
using std::string;
#include <sstream>
using std::stringstream;

/*
 * this is a model of an ethernet connection to a particular ip address.
 * once a connection is established, one can send and receive generic messages to this address.
 */
class P8EthernetSocket :
    public P8Socket
{
    public:
        P8EthernetSocket();
        virtual ~P8EthernetSocket();

        virtual bool Connect();
        virtual bool Disconnect();

        virtual bool Send(const char* command, const size_t command_length) const;
		virtual bool Send(const string &s) const {return Send(s.c_str(),s.size());};

		virtual bool dataWaiting();
        virtual bool Receive(char* data, const size_t data_length, size_t* received_length) const;
		virtual	bool Receive_with_timeout(char* data, const size_t data_length, size_t* received_length,long timeout_msec) const;
		virtual string ReceiveLine(long timeout_msec=1000);

        void SetIPv4Address(const string& addr);
        const string& GetIPv4Address() const;

        void SetPort(const string& port);
        const string& GetPort() const;

		void SetTimeout(int timeout_seconds)
		{ connection_timeout_seconds=timeout_seconds;};
		int GetTimeout() {return connection_timeout_seconds;};

    private:
		P8Mutex in_use;
        bool fConnected;
        string fIPv4Address;
        string fPort;
        int fSocket;
        addrinfo* fAddressInfo;
		int connection_timeout_seconds;
};


inline void P8EthernetSocket::SetIPv4Address(const string& addr)
{
    fIPv4Address = addr;
    return;
}
inline const string& P8EthernetSocket::GetIPv4Address() const
{
    return fIPv4Address;
}
inline void P8EthernetSocket::SetPort(const string& port)
{
    fPort = port;
    return;
}
inline const string& P8EthernetSocket::GetPort() const
{
    return fPort;
}

#endif /* ETHERNETSOCKET_HH_ */
