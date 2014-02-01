#include "CLASS_Thread.h"
#include "CLASS_MutexLocker.h"

Mutex Thread::s_activeThreadMutex;
std::map<Thread::ThreadHandleType, Thread*> Thread::s_activeThreads;

#ifdef WIN32
void msleep(unsigned int milli)
{
	SleepEx(milli, FALSE);
}

DWORD WINAPI Thread::threadProc(void* p)
{
	Thread* thread = reinterpret_cast<Thread*>(p);
	if (!thread)
		return 0;

	thread->setActive(true);
	int result = 0;
	if (thread->init())
	{
		result = thread->run();
		thread->exit();
	}
	thread->done();
	thread->setActive(false);

	return result;
}
#elif LINUX
void msleep(unsigned int milli)
{
	sched_yield();

	struct timespec timeOut,remains;
	timeOut.tv_sec  = (milli / 1000);
	timeOut.tv_nsec = (milli % 1000) * 1000000;
	nanosleep(&timeOut, &remains);
}

void* Thread::threadProc(void* p)
{
	Thread* thread = reinterpret_cast<Thread*>(p);
	if (!thread)
		return 0;
  
	thread->setActive(true);
	int result = 0;
	if (thread->init())
	{
		result = thread->run();
		thread->exit();
	}
	thread->done();
	thread->setActive(false);

	return (void*)result;
}
#else
#error Unhandled Platform!
#endif

void Thread::setActive(bool b)
{
	s_activeThreadMutex.lock();
	if (b)
	{
		s_activeThreads[m_thread] = this;
	}
	else
	{
		ThreadIterator iterator = s_activeThreads.find(m_thread);
		if (iterator != s_activeThreads.end())
			s_activeThreads.erase(iterator);
	}
	s_activeThreadMutex.unLock();
}

Thread* Thread::currentThread()
{
#ifdef WIN32
	HANDLE currentThread = GetCurrentThread();
#elif LINUX
	pthread_t currentThread = pthread_self();
#else
#error Unhandled Platform!
#endif

	s_activeThreadMutex.lock();
	ThreadIterator iterator = s_activeThreads.find(currentThread);
	if (iterator == s_activeThreads.end())
	{
		s_activeThreadMutex.unLock();
		return 0;
	}
	Thread* activeThread = (*iterator).second;
	s_activeThreadMutex.unLock();

	return activeThread;
}

Thread::Thread(void)
{
	m_running = false;
#ifdef WIN32
	m_thread = 0;
	m_threadID = 0;
#elif LINUX
	m_thread = 0;
#else
#error Unhandled Platform!
#endif
}

Thread::~Thread(void)
{
	wait();
}

bool Thread::start()
{
	if (isRunning())
		return false;

	m_running = true;

#ifdef WIN32
	m_thread = CreateThread(0, 0, &Thread::threadProc, this, CREATE_SUSPENDED, &m_threadID );
	if (m_thread == 0 || m_thread == INVALID_HANDLE_VALUE)
		return false;

	ResumeThread(m_thread);
#elif LINUX
	int error = pthread_create(&m_thread, NULL, &threadProc, this);
	if (error != 0)
		return false;
#else
#error Unhandled Platform!
#endif

	return true;
}

void Thread::done()
{
#ifdef WIN32
	m_thread = 0;
	m_threadID = 0;
#elif LINUX
	pthread_detach(m_thread);
	m_thread = 0;
#else
#error Unhandled Platform!
#endif
	m_running = false;
}

void Thread::wait()
{
	if (!isRunning())
		return;
  
#ifdef WIN32
	WaitForSingleObject(m_thread, INFINITE);
#elif LINUX
	pthread_join(m_thread, NULL);
#else
#error Unhandled Platform!
#endif
}

void Thread::postEvent(int eventID, void* data)
{
	EventData ed = {0};
	ed.id = eventID;
	ed.data = data;

	MutexLocker lock(m_eventList);
	m_events.push_back(ed);
}

void Thread::getEvents(std::vector<EventData>& events)
{
	MutexLocker lock(m_eventList);
	events = m_events;
	m_events.clear();
}

bool Thread::getEvent(EventData& event)
{
	MutexLocker lock(m_eventList);
	if (m_events.empty())
		return false;

	event = *m_events.begin();
	m_events.erase( m_events.begin() );
	return true;
}

bool Thread::hasEvents()
{
	MutexLocker lock(m_eventList);
	return !m_events.empty();
}

bool Thread::init()
{
	return true;
}

void Thread::exit()
{
}
