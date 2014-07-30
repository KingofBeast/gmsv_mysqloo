#ifndef _CLASS_QUERY_THREAD_H
#define _CLASS_QUERY_THREAD_H
#include "CLASS_Thread.h"
#include "LIBRARY_MySql.h"
#include <string>

class DataRow;
class Database;

class QueryThread :
	public Thread
{
public:
	enum
	{
		QUERY_ERROR = 0,
		QUERY_SUCCESS_NO_DATA,
		QUERY_DATA,
		QUERY_SUCCESS,
		QUERY_COLUMNS,
		QUERY_ABORTED,
	};
	enum
	{
		INTEGER = 0,
		FLOATING_POINT,
		STRING,
	};
	struct QueryDataInfo {
		int statement;
		DataRow* row;
	};

	QueryThread(Database* dbase);
	virtual ~QueryThread(void);

	MYSQL_RES* pResult;

	void setQuery(const char* query);
	std::string getQuery(){return m_query;}
	void abort();

	bool checkAbort();
	std::string error();
	long lastInsertId();
	long affectedRows();
	std::vector<std::string> columns();
	std::vector<int> columnTypes();

	virtual bool init();
	virtual int run();
	virtual void exit();
private:
	std::string m_query;
	Database* m_database;

	Mutex m_resultInfo;
	bool m_error;
	std::string m_errorText;
	std::vector<std::string> m_columns;
	std::vector<int> m_columnType;
	my_ulonglong m_lastInsert;
	my_ulonglong m_affectedRows;
	bool m_abort;
};

#endif //_CLASS_QUERY_THREAD_H
