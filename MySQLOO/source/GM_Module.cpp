
#include "GarrysMod/Lua/Interface.h"

#include "CLASS_Database.h"
#include "CLASS_Query.h"

#define VERSION "8.1"

int DatabaseConnect(lua_State* state)
{
	/* Arguments:
		1 = host
		2 = username
		3 = password
		4 = database
		5 = port
		6 = unixSocket
		7 = clientFlag
	*/
  
	int nNumParameters = LUA->Top();
	if (!LuaOO::checkArgument(state, 1, GarrysMod::Lua::Type::STRING)) 
		return 0;
	if (!LuaOO::checkArgument(state, 2, GarrysMod::Lua::Type::STRING)) 
		return 0;
	if (!LuaOO::checkArgument(state, 3, GarrysMod::Lua::Type::STRING)) 
		return 0;

	const char* host = LUA->GetString(1);
	const char* user = LUA->GetString(2);
	const char* pass = LUA->GetString(3);
	const char* database = 0;
	unsigned int port = 0;
	const char* unixSocket = 0;
	unsigned int clientflag = 0;

	if (nNumParameters > 3)
	{
		if (!LuaOO::checkArgument(state, 4, GarrysMod::Lua::Type::STRING)) 
			return 0;
		database = LUA->GetString(4);
	}
	if (nNumParameters > 4)
	{
		if (!LuaOO::checkArgument(state, 5, GarrysMod::Lua::Type::NUMBER))
			return 0;
		port = (unsigned int)LUA->GetNumber(5);
	}
	if (nNumParameters > 5)  
	{
		if (!LuaOO::checkArgument(state, 6, GarrysMod::Lua::Type::STRING)) 
			return 0;
		unixSocket = LUA->GetString(6);
	}
	if (nNumParameters > 6)  
	{
		if (!LuaOO::checkArgument(state, 7,GarrysMod::Lua::Type::NUMBER)) 
			return 0;
		clientflag = (unsigned int)LUA->GetNumber(7);
	}

	Database* dbase = new Database(state);

	dbase->setHost(host);
	dbase->setUserPassword(user,pass);
	dbase->setDatabase(database);
	dbase->setPort(port);
	dbase->setUnixSocket(unixSocket);
	dbase->setClientFlag(clientflag);

	dbase->pushObject();
	return 1;
}

GMOD_MODULE_OPEN()
{
	if (mysql_library_init(0,NULL,NULL))
	{
		LUA->ThrowError("Error: Couldn't initialize libmysql!");
		return 0;
	}

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);

	LuaOO::instance()->registerPollingFunction(state, "MySqlOO::Poll");
	LuaOO::instance()->registerClasses(state);

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->CreateTable();

			LUA->PushString(VERSION); LUA->SetField(-2, "VERSION");
			LUA->PushNumber((float)mysql_get_client_version()); LUA->SetField(-2, "MYSQL_VERSION");
			LUA->PushString(mysql_get_client_info()); LUA->SetField(-2, "MYSQL_INFO");

			LUA->PushNumber((float)DATABASE_CONNECTED); LUA->SetField(-2, "DATABASE_CONNECTED");
			LUA->PushNumber((float)DATABASE_CONNECTING); LUA->SetField(-2, "DATABASE_CONNECTING");
			LUA->PushNumber((float)DATABASE_NOT_CONNECTED); LUA->SetField(-2, "DATABASE_NOT_CONNECTED");
			LUA->PushNumber((float)DATABASE_INTERNAL_ERROR); LUA->SetField(-2, "DATABASE_INTERNAL_ERROR");

			LUA->PushNumber((float)QUERY_NOT_RUNNING); LUA->SetField(-2, "QUERY_NOT_RUNNING");
			LUA->PushNumber((float)QUERY_RUNNING); LUA->SetField(-2, "QUERY_RUNNING");
			LUA->PushNumber((float)QUERY_READING_DATA); LUA->SetField(-2, "QUERY_READING_DATA");
			LUA->PushNumber((float)QUERY_COMPLETE); LUA->SetField(-2, "QUERY_COMPLETE");
			LUA->PushNumber((float)QUERY_ABORTED); LUA->SetField(-2, "QUERY_ABORTED");

			LUA->PushNumber((float)OPTION_NUMERIC_FIELDS); LUA->SetField(-2, "OPTION_NUMERIC_FIELDS");
			LUA->PushNumber((float)OPTION_NAMED_FIELDS); LUA->SetField(-2, "OPTION_NAMED_FIELDS");
			LUA->PushNumber((float)OPTION_INTERPRET_DATA); LUA->SetField(-2, "OPTION_INTERPRET_DATA");
			LUA->PushNumber((float)OPTION_CACHE); LUA->SetField(-2, "OPTION_CACHE");

			LUA->PushNumber((float)CLIENT_MULTI_RESULTS); LUA->SetField(-2, "CLIENT_MULTI_RESULTS");
			LUA->PushNumber((float)CLIENT_MULTI_STATEMENTS); LUA->SetField(-2, "CLIENT_MULTI_STATEMENTS");

			LUA->PushCFunction(DatabaseConnect); LUA->SetField(-2, "connect");

		LUA->SetField(-2, "mysqloo");
	LUA->Pop();

	return 0;
}

GMOD_MODULE_CLOSE()
{
	LuaOO::shutdown();

	mysql_library_end();
	return 0;
}
