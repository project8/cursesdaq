/*
 * P8Socket.hh
 *
 * created on: Jan 12, 2011
 * author: dlfurse
 */

#ifndef P8SOCKET_HH_
#define P8SOCKET_HH_

#include <cstddef>

/*
 * this is a really, really low-level model for an external communication link
 */
class P8Socket
{
    public:
        P8Socket();
        virtual ~P8Socket();

        virtual bool Connect() = 0;
        virtual bool Disconnect() = 0;

        virtual bool Send(const char* command, const size_t command_length) const = 0;
        virtual bool Receive(char* data, const size_t data_length, size_t* received_length) const = 0;
};

#endif /* P8SOCKET_HH_ */
