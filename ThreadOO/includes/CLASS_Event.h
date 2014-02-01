#ifndef _CLASS_EVENT_H_
#define _CLASS_EVENT_H_

#include "PlatformSpecific.h"

/*!
	\brief Event object
*/
class Event
{
#ifdef WIN32
	HANDLE m_event;
#elif LINUX
	pthread_mutex_t m_mutex;
	bool m_signal;
#else
#error Unhandled Platform!
#endif
public:
	/*!
		\brief Constructor
	*/
	Event();

	/*!
		\brief Destructor
	*/
	~Event();

	/*!
		\brief Signal the event
	*/
	void signal();

	/*!
		\brief Clear the signal
	*/
	void clear();

	/*!
		\brief Wait (forever) for the signal to be set
	*/
	bool wait();

	/*!
		\brief Check if the signal is set.
	*/
	bool poll();
};

#endif //_CLASS_EVENT_H