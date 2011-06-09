/*
 * P8Mutex.cc
 *
 * created on: Jan 17, 2011
 * author: dlfurse
 */

#include "P8Mutex.hh"

P8Mutex::P8Mutex()
{
    pthread_mutex_init(&fMutex,0);
}

P8Mutex::~P8Mutex()
{
    pthread_mutex_destroy(&fMutex);
}

int P8Mutex::Lock()
{
    return pthread_mutex_lock(&fMutex);
}
int P8Mutex::TryLock()
{
    return pthread_mutex_trylock(&fMutex);
}
int P8Mutex::UnLock()
{
    return pthread_mutex_unlock(&fMutex);
}
