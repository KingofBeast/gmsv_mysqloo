#ifndef _CLASS_DATABASE_H_
#define _CLASS_DATABASE_H_

#include "MODULE_LuaOO.h"
#include "LIBRARY_MySql.h"
#include "CLASS_Mutex.h"
#include <vector>

class ConnectThread;
class Query;

enum
{
	DATABASE_CONNECTED = 0,
	DATABASE_CONNECTING,
	DATABASE_NOT_CONNECTED,
	DATABASE_INTERNAL_ERROR,
};

class Database :
	public LuaObjectBaseTemplate<Database, 215>
{
	LUA_OBJECT
public:
	Database(lua_State* state);
	virtual ~Database(void);

	virtual void poll();
	virtual bool canDelete();

	void setHost(const char* host);
	void setUserPassword(const char* user, const char* pass);
	void setDatabase(const char* database);
	void setPort(unsigned int port);
	void setUnixSocket(const char* socket);
	void setClientFlag(unsigned int port);

	int connect();
	int query();
	int escape();
	int abortAllQueries();
	int status();
	int wait();
	int serverInfo();
	int serverVersion();
	int hostInfo();

	MYSQL* lockHandle();
	void unlockHandle();

	void setRunning(Query*);
private:
	void checkQueries();

	Mutex m_sqlMutex;
	MYSQL* m_sql;
	ConnectThread* m_connectionThread;
	std::vector<Query*> m_runningQueries;
};

#endif //_CLASS_DATABASE_H_
