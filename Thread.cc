#include "Thread.h"

void *Thread::runWrapper(void *threadArg)
{
	Thread *p=(Thread*)threadArg;
	int retval=p->run();
	return (void*)retval;
}

int Thread::start_thread()
{
	return pthread_create(&m_thread,(pthread_attr_t*)NULL,runWrapper,this);
}

int Thread::join(int &retval)
{
	int *rvptr=&retval;
	return pthread_join(m_thread,(void**)(&rvptr));
}
