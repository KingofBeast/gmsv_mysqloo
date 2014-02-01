#ifndef _CLASS_CONNECTTHREAD_H_
#define _CLASS_CONNECTTHREAD_H_

#include "CLASS_Thread.h"
#include "CLASS_Mutex.h"
#include <string>

class Database;

class ConnectThread :
	public Thread
{
public:

	enum { CONNECTION_FINISHED = 0 };

	ConnectThread(Database* dbase);
	virtual ~ConnectThread(void);

	void setHost(const char* host);
	void setUserPassword(const char* user, const char* pass);
	void setDatabase(const char* database);
	void setPort(unsigned int port);
	void setUnixSocket(const char* socket);
	void setClientFlag(unsigned int flag);

	bool wasSuccessful();
	std::string error();

	virtual bool init();
	virtual int run();
	virtual void exit();
private:
	Database* m_mysql;

	std::string m_error;
	bool m_success;

	std::string m_host;
	std::string m_user;
	std::string m_pass;
	std::string m_database;
	std::string m_unixSocket;
	int m_port;
	int m_clientFlag;

	Mutex m_dataMutex;
};

#endif //_CLASS_CONNECTTHREAD_H_
