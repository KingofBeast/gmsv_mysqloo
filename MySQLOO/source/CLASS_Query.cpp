#include "CLASS_Query.h"
#include "CLASS_QueryThread.h"
#include "CLASS_Database.h"
#include "CLASS_DataRow.h"

#ifdef LINUX
#include <stdlib.h>
#endif

Query::Query(Database* dbase, lua_State* state)
	: LuaObjectBaseTemplate<Query>(state)
	, m_database(dbase)
{
	m_options = OPTION_NAMED_FIELDS | OPTION_INTERPRET_DATA | OPTION_CACHE;
	m_status = QUERY_NOT_RUNNING;
	m_queryThread = new QueryThread( m_database );
}

Query::~Query(void)
{
	reset();
	delete m_queryThread;
	m_queryThread = 0;
}

BEGIN_BINDING(Query)
	BIND_FUNCTION("start", Query::start)
	BIND_FUNCTION("isRunning", Query::isRunning)
	BIND_FUNCTION("getData", Query::getData)
	BIND_FUNCTION("abort", Query::abort)
	BIND_FUNCTION("lastInsert", Query::lastInsert)
	BIND_FUNCTION("status", Query::status)
	BIND_FUNCTION("affectedRows", Query::affectedRows)
	BIND_FUNCTION("setOption", Query::setOption)
	BIND_FUNCTION("wait", Query::wait)
	BIND_FUNCTION("error", Query::error)
END_BINDING()

bool Query::canDelete()
{
	if (m_queryThread->isRunning())
		return false;
	if (m_queryThread->hasEvents())
		return false;
	return LuaObjectBaseTemplate<Query>::canDelete();
}

void Query::setQuery(const char* query)
{
	m_queryThread->setQuery(query);
}

void Query::reset()
{
	if(m_queryThread->checkAbort())
		m_queryThread->abort();

		m_queryThread->wait();

		std::vector<Thread::EventData> events;
		m_queryThread->getEvents(events);

	for( std::vector<Thread::EventData>::iterator eventIterator = events.begin();
		eventIterator != events.end();
		++eventIterator)
	{
		if (eventIterator->id == QueryThread::QUERY_DATA)
		{
			DataRow* row = reinterpret_cast<DataRow*>(eventIterator->data);
			delete row;
		}
	}

	m_columns.clear();
	for( std::vector<DataRow*>::iterator rowIterator = m_allRows.begin();
		rowIterator != m_allRows.end();
		++rowIterator)
	{
		DataRow* row = reinterpret_cast<DataRow*>(*rowIterator);
		delete row;
	}
	m_allRows.clear();
}

int Query::start()
{
	reset();
	m_database->setRunning(this);
	m_queryThread->start();
	m_status = QUERY_RUNNING;
	return 0;
}

bool Query::threadRunning()
{
	if (!m_queryThread)
		return false;

	return m_queryThread->isRunning();
}

int Query::isRunning()
{
	if (!m_queryThread)
		return 0;
  
	MLUA->PushBool( m_queryThread->isRunning() );
	return 1;
}

int Query::lastInsert()
{
	if (!m_queryThread)
		return 0;

	MLUA->PushNumber( m_queryThread->lastInsertId() );
	return 1;
}

int Query::affectedRows()
{
	if (!m_queryThread)
		return 0;

	MLUA->PushNumber( m_queryThread->affectedRows() );
	return 1;
}

int Query::setOption()
{
	if (!checkArgument(2, GarrysMod::Lua::Type::NUMBER))
		return 0;
  
	bool set = true;
	int option = (int)MLUA->GetNumber(2);
	if (option != OPTION_NUMERIC_FIELDS && 
		option != OPTION_NAMED_FIELDS && 
		option != OPTION_INTERPRET_DATA &&
		option != OPTION_CACHE)
	{
		MLUA->ThrowError("Invalid option");
		return 0;
	}

	if (MLUA->Top() >= 3)
	{
		if (!checkArgument(3, GarrysMod::Lua::Type::BOOL))
			return 0;
		set = MLUA->GetBool(3);
	}
  
	if (set)
	{
		m_options |= option;
	}
	else
	{
		m_options &= ~option;
	}
	return 0;
}

int Query::getData()
{
	MLUA->CreateTable();
  
	float rowNumber = 1;
	for( std::vector<DataRow*>::iterator it = m_allRows.begin();
		it != m_allRows.end();
		++it)
	{
		int rowObject = rowToLua( *it );
		MLUA->PushNumber(rowNumber);
		MLUA->ReferencePush(rowObject);
		MLUA->SetTable(-3);
		MLUA->ReferenceFree(rowObject);
	    
		rowNumber++;
	}

	return 1;
}

int Query::status()
{
	MLUA->PushNumber(m_status);
	return 1;
}

int Query::error()
{
	MLUA->PushString( m_queryThread->error().c_str() );
	return 1;
}

int Query::abort()
{
	if (!m_queryThread)
		return 0;
	m_queryThread->abort();
	return 0;
}

int Query::wait()
{
	if (!m_queryThread)
		return 0;
	if (!m_queryThread->isRunning())
		return 0;
	m_queryThread->wait();// Block, wait for the query to complete.
	poll(); // Dispatch any events once finished.
	return 0;
}

void Query::dataToLua(const std::string* data, int row, unsigned int column)
{
	MLUA->ReferencePush(row);
	if (column < m_columns.size())
	{
		int type = m_columnTypes[column];

		if (type == QueryThread::STRING || (!testOption(OPTION_INTERPRET_DATA)))
		{
			if (testOption(OPTION_NUMERIC_FIELDS))
			{
				MLUA->PushNumber((float)column+1); 
				MLUA->PushString(data->c_str());
				MLUA->SetTable(-3);
			}
			if (testOption(OPTION_NAMED_FIELDS))
			{
				MLUA->PushString(data->c_str());
				MLUA->SetField(-2, m_columns[column].c_str());
			}
		}
		else if (type == QueryThread::INTEGER)
		{
			int value = atoi( data->c_str() );
			if (testOption(OPTION_NUMERIC_FIELDS))
			{
				MLUA->PushNumber((float)column+1);
				MLUA->PushNumber((float)value);
				MLUA->SetTable(-3);
			}
			if (testOption(OPTION_NAMED_FIELDS))
			{
				MLUA->PushNumber((float)value);
				MLUA->SetField(-2, m_columns[column].c_str());
			}
		}	
		else if (type == QueryThread::FLOATING_POINT)
		{
			double value = atof( data->c_str() );
			if (testOption(OPTION_NUMERIC_FIELDS))
			{
				MLUA->PushNumber((float)column+1);
				MLUA->PushNumber((float)value);
				MLUA->SetTable(-3);
			}
			if (testOption(OPTION_NAMED_FIELDS))
			{
				MLUA->PushNumber((float)value);
				MLUA->SetField(-2, m_columns[column].c_str());
			}
		}
	}
	else
	{
		if (testOption(OPTION_NUMERIC_FIELDS))
		{
			MLUA->PushNumber((float)column+1);
			MLUA->PushString(data->c_str());
			MLUA->SetTable(-3);
		}
	}
	MLUA->Pop();
}

int Query::rowToLua(DataRow* row)
{
	MLUA->CreateTable();
	int rowObject = MLUA->ReferenceCreate();
  
	unsigned int column = 0;
	for( DataRow::iterator it = row->begin();
		it != row->end();
		++it)
	{
		if (*it)
		{
			dataToLua(*it, rowObject, column);
		}

	column++;
	}

	return rowObject;
}

void Query::poll()
{
	if (!m_queryThread)
		return;

	std::vector<Thread::EventData> events;
	m_queryThread->getEvents(events);

	int dataref = 0;
	for( std::vector<Thread::EventData>::iterator it = events.begin();
		it != events.end();
		++it)
	{
		switch (it->id)
		{
			case QueryThread::QUERY_ABORTED:
				runCallback("onAborted");
				m_status = QUERY_ABORTED;
				break;

			case QueryThread::QUERY_ERROR:
				if (m_queryThread->checkAbort())
					continue;
				m_status = QUERY_COMPLETE;
				runCallback("onError", "ss", m_queryThread->error().c_str(), m_queryThread->getQuery().c_str());
				break;

			case QueryThread::QUERY_SUCCESS:
			case QueryThread::QUERY_SUCCESS_NO_DATA:
				if (m_queryThread->checkAbort())
					continue;
				m_status = QUERY_COMPLETE;
				getData();
				dataref = MLUA->ReferenceCreate();
				runCallback("onSuccess", "r", dataref);
				break;

			case QueryThread::QUERY_COLUMNS:
				if (m_queryThread->checkAbort())
					continue;
				m_columns = m_queryThread->columns();
				m_columnTypes = m_queryThread->columnTypes();
				break;

			case QueryThread::QUERY_DATA:
				{
					m_status = QUERY_READING_DATA;
					DataRow* row = reinterpret_cast<DataRow*>(it->data);
					if (!row)
						break;

					if (testOption(OPTION_CACHE))
						m_allRows.push_back(row);

					if (m_queryThread->checkAbort())
						continue;

					int dataChunk = rowToLua(row);
					runCallback("onData", "o", dataChunk);
					MLUA->ReferenceFree(dataChunk);
					if (!testOption(OPTION_CACHE))
						delete row;
				}
		}
	}
}
