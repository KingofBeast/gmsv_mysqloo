
#include "CLASS_Event.h"
#include "CLASS_Thread.h"

Event::Event()
{
#ifdef WIN32
	m_event = CreateEvent(0, TRUE, FALSE, 0);
#elif LINUX
	m_signal = false;
	pthread_mutex_init(&m_mutex, NULL);
#else
#error Unhandled Platform!
#endif
}

Event::~Event()
{
#ifdef WIN32
	CloseHandle(m_event);
#elif LINUX
	pthread_mutex_destroy(&m_mutex);
#else
#error Unhandled Platform!
#endif
}

void Event::signal()
{
#ifdef WIN32
	SetEvent(m_event);
#elif LINUX
	pthread_mutex_lock(&m_mutex);
	m_signal = true;
	pthread_mutex_unlock(&m_mutex);
#else
#error Unhandled Platform!
#endif
}

void Event::clear()
{
#ifdef WIN32
	ResetEvent(m_event);
#elif LINUX
	pthread_mutex_lock(&m_mutex);
	m_signal = false;
	pthread_mutex_unlock(&m_mutex);
#else
#error Unhandled Platform!
#endif
}

bool Event::wait()
{
#ifdef WIN32
	return (WaitForSingleObject(m_event, INFINITE) == WAIT_OBJECT_0);
#elif LINUX
	while (true)
	{
		pthread_mutex_lock(&m_mutex);
		if (m_signal)
		{
			pthread_mutex_unlock(&m_mutex);
			break;
		}
		pthread_mutex_unlock(&m_mutex);

		msleep(100);
	}
	return true;
#else
#error Unhandled Platform!
#endif
}

bool Event::poll()
{
#ifdef WIN32
	return (WaitForSingleObject(m_event, 0) == WAIT_OBJECT_0);
#elif LINUX
	pthread_mutex_lock(&m_mutex);
	if (m_signal)
	{
		pthread_mutex_unlock(&m_mutex);
		return true;
	}
	return false;
#else
#error Unhandled Platform!
#endif
}

