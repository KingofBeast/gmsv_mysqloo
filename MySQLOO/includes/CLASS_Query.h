#ifndef _CLASS_QUERY_H_
#define _CLASS_QUERY_H_

#include "CLASS_LuaObjectBase.h"
#include "CLASS_QueryThread.h"
#include "LIBRARY_MySql.h"

class QueryThread;
class Database;
class DataRow;

enum
{
	QUERY_NOT_RUNNING = 0,
	QUERY_RUNNING,
	QUERY_READING_DATA,
	QUERY_COMPLETE,
	QUERY_ABORTED,
};

enum
{
	OPTION_NUMERIC_FIELDS = 1,
	OPTION_NAMED_FIELDS   = 2,
	OPTION_INTERPRET_DATA = 4,
	OPTION_CACHE          = 8,
};

class Query :
	public LuaObjectBaseTemplate<Query,216>
{
	LUA_OBJECT
public:
	Query(Database* dbase, lua_State* state);
	virtual ~Query(void);

	void setQuery(const char* query);

	virtual void poll();
	virtual bool canDelete();

	bool threadRunning();

	int start();
	int isRunning();
	int getData();
	int abort();
	int lastInsert();
	int affectedRows();
	int status();
	int setOption();
	int wait();
	int error();
private:
	inline bool testOption(int option)
	{
		return (m_options & option) == option;
	}

	Database* m_database;
	QueryThread* m_queryThread;
	std::vector<std::string> m_columns;
	std::vector<int> m_columnTypes;
	std::vector<QueryThread::QueryDataInfo*> m_allRows;
	int m_status;
	int m_options;

	void reset();

	void dataToLua(const std::string* data, int row, unsigned int column);
	int rowToLua(DataRow* row);
};

#endif //_CLASS_QUERY_H_
