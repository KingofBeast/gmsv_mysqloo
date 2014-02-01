#ifndef _CLASS_MUTEXLOCKER_H_
#define _CLASS_MUTEXLOCKER_H_

class Mutex;

/*!
	\brief Auto lock mutex
*/
class MutexLocker
{
	Mutex& m_mutex;
public:
	/*!
		\brief Lock a mutex on construction.
	*/
	MutexLocker(Mutex& mutex);

	/*!
		\brief Unlock a mutex on destruction.
	*/
	~MutexLocker();
};

#endif //_CLASS_MUTEX_H_