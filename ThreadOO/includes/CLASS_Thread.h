#ifndef _CLASS_THREAD_H_
#define _CLASS_THREAD_H_

#include "PlatformSpecific.h"
#include <vector>
#include <map>

#include "CLASS_Mutex.h"

void msleep(unsigned int milli);
 
/*!
	\brief Thread class
*/
class Thread
{
public:
	static Thread* currentThread();
	/*!
		\brief Generic event notification
	*/
	typedef struct
	{
		unsigned int id;
		void* data;
	} EventData;

	/*!
		\brief Constructor
	*/
	Thread(void);

	/*!
		\brief Destructor
	*/
	virtual ~Thread(void);

	/*!
		\brief Start the thread
	*/
	virtual bool start();

	/*!
		\brief Wait for the thread to complete
	*/
	virtual void wait();
  
	/*!
		\brief Called before the thread is run
	*/
	virtual bool init();

	/*!
		\brief Activity to perform
	*/
	virtual int run() = 0;

	/*!
		\brief Called after the thread has run
	*/
	virtual void exit();

	/*!
		\brief Check if the thread is still running
	*/
	inline bool isRunning() { return m_running; }

	/*!
		\brief Get the full list of posted events.
	*/
	void getEvents(std::vector<EventData>& events);

	/*!
		\brief Get a single event.
	*/
	bool getEvent(EventData& event);

	/*!
		\brief Post an event to the list
	*/
	void postEvent(int eventID, void* data = 0);

	/*!
		\brief Check if there are any events.
	*/
	bool hasEvents();

private:
	/*!
		\brief Clear any variables when the thread finishes
	*/
	void done();
	bool m_running;

#ifdef WIN32
	typedef HANDLE ThreadHandleType;
	DWORD m_threadID;
	/*!
		\brief Thread entry point (windows)
	*/
	static DWORD WINAPI threadProc(void* p);
#elif LINUX
	typedef pthread_t ThreadHandleType;

	static void* threadProc(void* p);
#else
#error Unhandled Platform!
#endif

	ThreadHandleType m_thread;
	static Mutex s_activeThreadMutex;
	static std::map<ThreadHandleType, Thread*> s_activeThreads;
	typedef std::map<ThreadHandleType, Thread*>::iterator ThreadIterator;

	Mutex m_eventList;
	std::vector<EventData> m_events;

	void setActive(bool b);
};

#endif //_CLASS_THREAD_H_
