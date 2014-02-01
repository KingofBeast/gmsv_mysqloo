#ifndef _CLASS_MUTEX_H_
#define _CLASS_MUTEX_H_

#include "PlatformSpecific.h"

class Thread;

/*!
	\brief Mutex object
*/
class Mutex
{
	Thread* m_lockedBy;
#ifdef WIN32
	CRITICAL_SECTION m_mutex;
#elif LINUX
	pthread_mutex_t m_mutex;
#else
#error Unhandled Platform!
#endif
public:
	/*!
		\brief Constructor
	*/
	Mutex();

	/*!
		\brief Destructor
	*/
	~Mutex();

	/*!
		\brief Lock the mutex
	*/
	void lock();

	/*!
		\brief Unlock the mutex
	*/
	void unLock();
};

#endif //_CLASS_MUTEX_H_