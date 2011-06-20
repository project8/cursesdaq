/*
 * P8EthernetSocket.cc
 *
 * created on: Jan 12, 2011
 * author: dlfurse
 */

#include "P8EthernetSocket.hh"

#include <iostream>
//#include <poll.h>
#include <sys/select.h>
#include <errno.h>
using std::cerr;
using std::endl;

P8EthernetSocket::P8EthernetSocket() :
    P8Socket(),
    fConnected(false),
    fIPv4Address(""),
    fPort(""),
    fSocket(0),
    fAddressInfo(0)
{
	connection_timeout_seconds=5;
}

P8EthernetSocket::~P8EthernetSocket()
{
    if( fConnected == true )
    {
        Disconnect();
    }
}

bool P8EthernetSocket::Connect()
{
    if( fConnected == false )
    {
        int ReturnValue;

        //make and initialize a temporary addrinfo struct for hints
        addrinfo Hints;
        Hints.ai_flags = 0;
        Hints.ai_family = AF_UNSPEC;
        Hints.ai_socktype = SOCK_STREAM;
        Hints.ai_protocol = 0;
        Hints.ai_addrlen = 0;
        Hints.ai_addr = 0;
        Hints.ai_canonname = 0;
        Hints.ai_next = 0;

        //get address information for the given port
        ReturnValue = getaddrinfo(fIPv4Address.c_str(), fPort.c_str(), &Hints, &fAddressInfo);
        if( ReturnValue != 0 )
        {
            cerr << "error getting information for address/port <" << fIPv4Address << ":" << fPort << ">: " << gai_strerror(ReturnValue) << endl;
            return false;
        }

        //make a socket for the resolved address
        ReturnValue = socket(fAddressInfo->ai_family, fAddressInfo->ai_socktype, fAddressInfo->ai_protocol);
        if( ReturnValue == -1 )
        {
            cerr << "error creating socket for " << fIPv4Address << endl;
            return false;
        }
        else
        {
            fSocket = ReturnValue;
        }

		//make the socket nonblocking
		fcntl(fSocket,F_SETFL,fcntl(fSocket,F_GETFL,0) | O_NONBLOCK);

		fd_set mysocketset;
		FD_ZERO(&mysocketset);
		FD_SET(fSocket,&mysocketset);
		struct timeval wait_time;
		wait_time.tv_sec=connection_timeout_seconds;
		wait_time.tv_usec=0;
        ReturnValue = connect(fSocket, fAddressInfo->ai_addr, fAddressInfo->ai_addrlen);
		int select_ret=select(1,&mysocketset,NULL,NULL,&wait_time);
		//make the socket blocking again
		fcntl(fSocket,F_SETFL,fcntl(fSocket,F_GETFL,0) ^ O_NONBLOCK);

		if(select_ret==0) {
			cerr << "timed out while trying to connect to " << fIPv4Address << endl;
			errno=ETIMEDOUT;
			return false;
		}
		if(select_ret==-1) {
			cerr << "select returned error trying to connect to " << fIPv4Address << endl;
			return false;
		}
	
		/*
        ReturnValue = connect(fSocket, fAddressInfo->ai_addr, fAddressInfo->ai_addrlen);
        if( ReturnValue == -1 )
        {
            cerr << "error creating connection to " << fIPv4Address << endl;
            return false;
        }
		*/

        fConnected = true;
        return true;
    }

    return false;
}

bool P8EthernetSocket::Disconnect()
{
    if( fConnected == true )
    {
        //close the socket and free the memory the address info list takes up
        close(fSocket);
        freeaddrinfo(fAddressInfo);

        fConnected = false;
        return true;
    }
    return false;
}
		
string P8EthernetSocket::ReceiveLine(long timeout_msec)
{
	string ret;
	size_t max_chunk_size=256;
	char chunk[256];

	size_t rec_len;
	//keep pulling data as long as it is coming
	while(Receive_with_timeout(chunk,max_chunk_size,&rec_len,timeout_msec))
	{
		ret+=string(chunk,rec_len);
		//if the last character is an end of line, break
		if((ret[ret.size()-1]==13)||(ret[ret.size()-1]==10))
		{
			//trim off end of line characters before returning
			while((ret[ret.size()-1]==13)||(ret[ret.size()-1]==10))
				ret=ret.substr(0,ret.size()-1);
			return ret;
		}
	}
	//in case of timeout or fail, return blank string
	return string("");
}

bool P8EthernetSocket::Send(const char* command, const size_t command_length) const
{
    size_t BytesSent = 0;
    int SendReturn = 0;

    while( BytesSent < command_length )
    {
#ifdef MSG_NOSIGNAL
    	SendReturn = send(fSocket, command+BytesSent, command_length-BytesSent, MSG_NOSIGNAL);
#endif
#ifdef MSG_HAVEMORE
    	SendReturn = send(fSocket, command+BytesSent, command_length-BytesSent, MSG_HAVEMORE);
#endif
        if( SendReturn == -1 )
        {
            cerr << "error occurred sending data to " << fIPv4Address << endl;
            return false;
        }
        BytesSent += SendReturn;
    }

    return true;
}

bool P8EthernetSocket::Receive(char* data, const size_t data_length, size_t* received_length) const
{
    int RecvReturn = 0;

    RecvReturn = recv(fSocket, data, data_length, 0);
    if( RecvReturn <= 0 )
    {
        cerr << "error occurred receiving data from " << fIPv4Address << endl;
        *received_length = 0;
        return false;
    }

    *received_length = RecvReturn;
    return true;
}
		
bool P8EthernetSocket::dataWaiting()
{
	struct timeval timeout_struct;
	memset(&timeout_struct,0,sizeof(timeout_struct));
	fd_set socket_set;
	FD_ZERO(&socket_set);
	FD_SET(fSocket,&socket_set);
	select(fSocket+1,&socket_set,NULL,NULL,&timeout_struct);
	if(FD_ISSET(fSocket,&socket_set))
		return true;
	return false;
}

bool P8EthernetSocket::Receive_with_timeout(char* data, const size_t data_length, size_t* received_length,long timeout_msec) const
{
//	struct pollfd topoll;
//	topoll.fd=fSocket;
//	topoll.events=POLLIN|POLLNVAL;

//	if(poll(&topoll,1,timeout_msec))
	struct timeval timeout_struct;
	timeout_struct.tv_sec=timeout_msec/1000;
	timeout_struct.tv_usec=(timeout_msec%1000)*1000;
//	cerr << "timout struct is " << timeout_struct.tv_sec << " seconds and " << timeout_struct.tv_usec << " microseconds " << endl;
	fd_set socket_set;
	FD_ZERO(&socket_set);
	FD_SET(fSocket,&socket_set);
	select(fSocket+1,&socket_set,NULL,NULL,&timeout_struct);
	if(FD_ISSET(fSocket,&socket_set))
		return Receive(data,data_length,received_length);
	return false;
}
