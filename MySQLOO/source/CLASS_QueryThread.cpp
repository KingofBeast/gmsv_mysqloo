#include "CLASS_QueryThread.h"
#include "CLASS_MutexLocker.h"
#include "CLASS_DataRow.h"
#include "CLASS_Database.h"

#include <sstream>
#include <string.h>

QueryThread::QueryThread(Database* dbase)
	: Thread()
	, m_database( dbase )
	, m_affectedRows(0)
	, m_lastInsert(0)
{
	m_error = false;
	m_abort = false;
}

QueryThread::~QueryThread(void)
{
}

void QueryThread::setQuery(const char* query)
{
	m_query = std::string(query);
}

void QueryThread::abort()
{
	MutexLocker lock(m_resultInfo);
	m_abort = true;
}

std::string QueryThread::error()
{
	MutexLocker lock(m_resultInfo);
	return m_errorText;
}

std::vector<std::string> QueryThread::columns()
{
	MutexLocker lock(m_resultInfo);
	return m_columns;
}

std::vector<int> QueryThread::columnTypes()
{
	MutexLocker lock(m_resultInfo);
	return m_columnType;
}

long QueryThread::lastInsertId()
{
	MutexLocker lock(m_resultInfo);

	return (long)m_lastInsert;
}

long QueryThread::affectedRows()
{
	MutexLocker lock(m_resultInfo);

	return (long)m_affectedRows;
}

bool QueryThread::checkAbort()
{
	MutexLocker lock(m_resultInfo);
	return m_abort;
}

bool QueryThread::init()
{
	mysql_thread_init();
	return true;
}

void QueryThread::exit()
{
	mysql_thread_end();
}

int QueryThread::run()
{
	// Note: The m_abort flag does not actually abort, after the mysql_query function:
	// MySql documentation states:
	//    "When using mysql_use_result(), you must execute mysql_fetch_row() until
	//     a NULL value is returned, otherwise, the unfetched rows are returned as 
	//     part of the result set for your next query. The C API gives the error Commands 
	//     out of sync; you can't run this command now if you forget to do this!"
	// In otherwords: once we've started, we can't stop. So what we actually do is run the 
	// query in full, and just not post any events if this flag is set.

	MYSQL* sql = 0;

	{
		MutexLocker lock(m_resultInfo);
		m_columns.clear();
		m_columnType.clear();
	}

	if (checkAbort())
	{
		MutexLocker lock(m_resultInfo);
		m_abort = false;
		postEvent(QUERY_ABORTED);
		return 0;
	}

	sql = m_database->lockHandle();
	if (mysql_query(sql, m_query.c_str()) != 0)
	{
		MutexLocker lock(m_resultInfo);
		m_error = true;
		m_errorText = mysql_error(sql);
		postEvent( QUERY_ERROR );

		m_database->unlockHandle();
			return 0;
	}

	MYSQL_RES* pResult = mysql_use_result(sql);
	if (!pResult)
	{
		if (checkAbort())
		{
			m_database->unlockHandle();
			return 0;
		}

		if(mysql_errno(sql))
		{
			MutexLocker lock(m_resultInfo);
			m_error = true;
			m_errorText = mysql_error(sql);
			postEvent( QUERY_ERROR );
		}
		else
		{
			MutexLocker lock(m_resultInfo);
			m_error = false;
			m_lastInsert = mysql_insert_id(sql);
			m_affectedRows = mysql_affected_rows(sql);
			postEvent( QUERY_SUCCESS_NO_DATA );
		}
		m_database->unlockHandle();
		return 0;
	}

	int numColumns = mysql_num_fields(pResult);
  
	{
		{
			MutexLocker lock(m_resultInfo);
			for(int i = 0; i < numColumns; i++)
			{
				MYSQL_FIELD* field = mysql_fetch_field_direct(pResult, i);
				if (field && field->name && strlen(field->name))
				{
					m_columns.push_back( std::string(field->name) );

					if (field->type == MYSQL_TYPE_TINY || field->type == MYSQL_TYPE_SHORT || field->type == MYSQL_TYPE_LONG || field->type == MYSQL_TYPE_LONG)
						m_columnType.push_back( INTEGER );
					else if (field->type == MYSQL_TYPE_FLOAT || field->type == MYSQL_TYPE_DOUBLE)
						m_columnType.push_back( FLOATING_POINT );
					else
						m_columnType.push_back( STRING );
				}
				else
				{
					std::stringstream col;
					col << (i+1);

					m_columns.push_back( col.str() );
					m_columnType.push_back( STRING );
				}
			}
		}

		if (!checkAbort())
			postEvent( QUERY_COLUMNS );
	}

	MYSQL_ROW CurrentRow = mysql_fetch_row(pResult);
	while (CurrentRow)
	{
		if (checkAbort())
		{
			CurrentRow = mysql_fetch_row(pResult);
			continue;
		}

		DataRow* row = new DataRow;

		for(int i = 0; i < numColumns; i++)
		{
			if (CurrentRow[i])
			{
				row->push_back( new std::string( CurrentRow[i] ) );
			}
			else
			{
				row->push_back( 0 );
			}
		}    

		postEvent( QUERY_DATA, row );

		CurrentRow = mysql_fetch_row(pResult);
	}

	if(mysql_errno(sql))
	{
		{
			MutexLocker lock(m_resultInfo);
			m_error = true;
			m_errorText = mysql_error(sql);
		}
		if (!checkAbort())
			postEvent( QUERY_ERROR );
		else
		{
			postEvent( QUERY_ABORTED );

			MutexLocker lock(m_resultInfo);
			m_abort = false;
		}
	}
	else
	{
		{
			MutexLocker lock(m_resultInfo);
			m_error = false;
			m_lastInsert = mysql_insert_id(sql);
			m_affectedRows = mysql_affected_rows(sql);
		}
		if (!checkAbort())
			postEvent( QUERY_SUCCESS );
		else
		{
			postEvent( QUERY_ABORTED );

			MutexLocker lock(m_resultInfo);
			m_abort = false;
		}
	}
		
	mysql_free_result(pResult);

	while (mysql_next_result(sql) == 0) {
		pResult = mysql_store_result(sql);
		mysql_free_result(pResult);
	}

	m_database->unlockHandle();

	return 0;
}
