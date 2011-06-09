/*
 * P8Mutex.hh
 *
 * created on: Jan 17, 2011
 * author: dlfurse
 */

#ifndef P8MUTEX_HH_
#define P8MUTEX_HH_

#include <pthread.h>

/*!
 *  @class P8Mutex
 *  @author Dan Furse
 *
 *  @brief This is a very bare-bones encapsulation of the POSIX mutex into a class.
 *
 */
class P8Mutex
{
    public:
        /// Constructor
        P8Mutex();

        /// Destructor
        virtual ~P8Mutex();

        /// Locks the mutex
        int Lock();

        /// Does a dry-run lock of the mutex
        int TryLock();

        /// Unlocks the mutex
        int UnLock();

    private:
        /// The raw POSIX mutex id
        pthread_mutex_t fMutex;
};

#endif /* P8MUTEX_HH_ */
