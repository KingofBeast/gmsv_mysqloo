
#include "CLASS_MutexLocker.h"
#include "CLASS_Mutex.h"

MutexLocker::MutexLocker(Mutex& mutex)
	: m_mutex(mutex)
{
	m_mutex.lock();
}

MutexLocker::~MutexLocker()
{
	m_mutex.unLock();
}
