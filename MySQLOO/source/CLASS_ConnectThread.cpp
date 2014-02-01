#include "CLASS_ConnectThread.h"
#include "CLASS_MutexLocker.h"
#include "CLASS_Database.h"

std::string safeStrToStr(const char* str)
{
	if (str)
		return std::string(str);
	else
		return std::string();
}

const char* stringOrNull(const std::string& str)
{
	if (str.empty())
		return 0;
	return str.c_str();
}

ConnectThread::ConnectThread(Database* dbase)
	: Thread()
	, m_mysql(dbase)
	, m_clientFlag(0)
	, m_port(0)
{
}

ConnectThread::~ConnectThread(void)
{
}

bool ConnectThread::init()
{
	mysql_thread_init();
	return true;
}

void ConnectThread::exit()
{
	mysql_thread_end();
}

void ConnectThread::setHost(const char* host)
{
	MutexLocker dataLock(m_dataMutex);
	m_host = safeStrToStr(host);
}

void ConnectThread::setUserPassword(const char* user, const char* pass)
{
	MutexLocker dataLock(m_dataMutex);
	m_user = safeStrToStr(user);
	m_pass = safeStrToStr(pass);
}

void ConnectThread::setDatabase(const char* db)
{
	MutexLocker dataLock(m_dataMutex);
	m_database = safeStrToStr(db);
}

void ConnectThread::setPort(unsigned int port)
{
	MutexLocker dataLock(m_dataMutex);
	m_port = port;
}

void ConnectThread::setUnixSocket(const char* socket)
{
	MutexLocker dataLock(m_dataMutex);
	m_unixSocket = safeStrToStr(socket);
}

void ConnectThread::setClientFlag(unsigned int flag)
{
	MutexLocker dataLock(m_dataMutex);
	m_clientFlag = flag;
}

bool ConnectThread::wasSuccessful()
{
	MutexLocker dataLock(m_dataMutex);
	return m_success;
}

std::string ConnectThread::error()
{
	MutexLocker dataLock(m_dataMutex);
	return m_error;
}

int ConnectThread::run()
{
	MYSQL* sql = m_mysql->lockHandle();
	if (!mysql_real_connect(sql, stringOrNull(m_host), stringOrNull(m_user), stringOrNull(m_pass), stringOrNull(m_database), m_port, stringOrNull(m_unixSocket), m_clientFlag))
	{
		MutexLocker dataLock(m_dataMutex);
		m_success = false;
		m_error = std::string( mysql_error(sql) );
		postEvent( CONNECTION_FINISHED );

		m_mysql->unlockHandle();
		return 0;
	}
	else
	{
		MutexLocker dataLock(m_dataMutex);
		m_success = true;
		m_error = "";
		postEvent( CONNECTION_FINISHED );

		m_mysql->unlockHandle();
		return 0;
	}
}