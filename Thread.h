#pragma once
#include <pthread.h>

class Thread
{
public:
	Thread() : m_thread(0) {};
	virtual ~Thread() {};

	int start_thread();
	int join(int &retval);

	pthread_t getThreadID() {return m_thread;};
private:
	pthread_t m_thread;

	virtual int run()=0; //This is what must be implemented
	static void *runWrapper(void *threadArg);
};

class Mutex
{
public:
	Mutex() {pthread_mutex_init(&my_mutex,NULL);};
	~Mutex() {pthread_mutex_destroy(&my_mutex);};
	int lock() {return pthread_mutex_lock(&my_mutex);};
	int unlock() {return pthread_mutex_unlock(&my_mutex);};
	int trylock() {return pthread_mutex_trylock(&my_mutex);};

	pthread_mutex_t getMutexID() {return my_mutex;};
private:
	pthread_mutex_t my_mutex;
};
