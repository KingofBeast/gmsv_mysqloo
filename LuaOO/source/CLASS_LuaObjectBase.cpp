#include "CLASS_LuaObjectBase.h"

#include <string.h>
#include <stdarg.h>
#include <sstream>
#ifdef LINUX
#include <stdio.h>
#endif

LuaObjectBase::LuaObjectBase(const LuaClassInfo& classInfo, lua_State* state)
	: m_luaState(state)
	, m_classInfo(classInfo)
	, m_enableGC(true)
	, m_gcRefCount(0)
	, m_markedForDeletion(false)
{
	LuaOO::instance()->allocated(this);
}

LuaObjectBase::~LuaObjectBase()
{
	LuaOO::instance()->released(this);
}

bool LuaObjectBase::deleteIfReady()
{
	if (!m_markedForDeletion)
		return false;
	delete this;
	return true;
}

void LuaObjectBase::markForDeletion()
{
	m_markedForDeletion = true;
}

int LuaObjectBase::toString()
{
	if (!m_luaState)
		return 0;

	char str[64];
	#ifdef LINUX
	snprintf(str, sizeof(m_classInfo.className()) + sizeof(this), "[%s:%08X]", m_classInfo.className(), this);
	#else
	sprintf_s(str, "[%s:%08X]", m_classInfo.className(), this);
	#endif
	MLUA->PushString(str);
	return 1;
}

bool LuaObjectBase::isValid(bool bError)
{
	return true;
}

void LuaObjectBase::poll()
{
}

void LuaObjectBase::enableGC(bool enable)
{
	m_enableGC = enable;
}

bool LuaObjectBase::canDelete()
{
	return m_enableGC;
}

void LuaObjectBase::pushObject()
{
	if (!m_luaState)
		return;

#ifdef FULL_USER_DATA
	m_gcRefCount++;
	GarrysMod::Lua::UserData* ud = (GarrysMod::Lua::UserData*)MLUA->NewUserdata(sizeof(GarrysMod::Lua::UserData));
	ud->data = this;
	ud->type = m_classInfo.typeId();
	MLUA->ReferencePush( m_classInfo.m_metatable );
	MLUA->SetMetaTable(-2);
#else
	if (latestRef())
	{
		latestRef()->Push();
	}
	else
	{
		ILuaObject* metaTable = m_luaInterface->GetNewTable();
		metaTable->SetMember("__gc", LuaObjectBase::gcDeleteWrapper );
		metaTable->SetMember("__tostring", LuaObjectBase::toStringWrapper);

		ILuaObject* object = m_luaInterface->GetNewTable();
		object->SetMemberUserDataLite("_this", this);
		const std::vector<LuaBoundFunction*>& list = m_classInfo.functionList();
		for (std::vector<LuaBoundFunction*>::const_iterator it = list.begin(); 
				it != list.end(); 
				++it)
		{
			object->SetMember( (*it)->functionName(), (*it)->function() );
		}
		object->SetMetaTable(metaTable);

		object->Push();
    
		metaTable->UnReference();
		object->UnReference();
	}
#endif
}

LuaObjectBase* LuaObjectBase::getFromObject(lua_State* state, int object, bool error)
{
#ifdef FULL_USER_DATA
	// Make sure it's some user data!
	LUA->ReferencePush(object);
	int type = LUA->GetType(-1);
	if(type != GarrysMod::Lua::Type::USERDATA && type <= GarrysMod::Lua::Type::COUNT)
	{
		if (error)
		{
			LUA->ThrowError( "Invalid object! (not userdata)\n" );
		}
		return 0;
	}

	// Check the object
	GarrysMod::Lua::UserData* a = (GarrysMod::Lua::UserData*)LUA->GetUserdata(-1);
	LuaObjectBase* data = (LuaObjectBase*)(a->data);
	if (!checkValidity(state, type, data, error))
		return 0;
	
	// Just a simple cast required
	return reinterpret_cast<LuaObjectBase*>(a);
#else
	void* userData = object->GetMemberUserDataLite("_this");

	if (!checkValidity(luaInterface, 0, (LuaObjectBase*)userData, error))
		return 0;

	LuaObjectBase* baseObject = reinterpret_cast<LuaObjectBase*>(userData);
	baseObject->luaRef(object);
	return baseObject;
#endif
}

LuaObjectBase* LuaObjectBase::getFromStack(lua_State* state, int position, bool error)
{
#ifdef FULL_USER_DATA
	// Check the object
	if (!checkValidity(state, position, error))
		return 0;

	// Just a simple cast required
	GarrysMod::Lua::UserData* obj = (GarrysMod::Lua::UserData* )LUA->GetUserdata(position);
	LuaObjectBase* luaobj = (LuaObjectBase*)(obj->data);
	return luaobj;
#else
	ILuaObject* object = luaInterface->GetObject(position);
	if (!object)
		return 0;

	return getFromObject(luaInterface, object, error);
#endif
}

bool LuaObjectBase::checkValidity(lua_State* state, int type, LuaObjectBase* object, bool error)
{
#ifdef FULL_USER_DATA
	// Make sure we recognise the type number
	if (!LuaOO::instance()->typeRegistered(type))
	{
		if (error)
			LUA->ThrowError("Invalid object! (unknown type)\n");
		return false;
	}
#endif

  // Check for a NULL pointer
	if (object == 0)
	{
		if (error)
			LUA->ThrowError("Invalid object! (null)\n");
		return false;
	}

  // Make sure it's something we've allocated
	if (!LuaOO::instance()->hasAllocated(object))
	{
		if (error)
			LUA->ThrowError("Invalid object! (unreferenced)\n");
		return false;
	}

#ifdef FULL_USER_DATA
  // Check the type (this should always be correct)
	if (object->m_classInfo.typeId() != type)
	{
		if (error)
			LUA->ThrowError("Invalid object! (types are different)\n");
		return false;
	}
#endif

  // Check the interface pointer (this should always be correct)
	if (object->m_luaState != state)
	{
		if (error)
			LUA->ThrowError("Invalid object! (interface pointer is different)\n");
		return false;
	}

  if (object->m_markedForDeletion)
  {
		if (error)
			LUA->ThrowError("Invalid object! (about to be deleted)\n");
		return false;
  }

	return true;
}

bool LuaObjectBase::checkValidity(lua_State* state, int position, bool error)
{
#ifdef FULL_USER_DATA
// Make sure it's some user data!
	int type = LUA->GetType(position);
	if(type != GarrysMod::Lua::Type::USERDATA && type <= GarrysMod::Lua::Type::COUNT)
	{
		if (error)
		{
			LUA->ThrowError( "Invalid object! (not userdata)\n" );
		}
		return 0;
	}

// Acually do the check
	GarrysMod::Lua::UserData*  obj = (GarrysMod::Lua::UserData*)LUA->GetUserdata(position);
	LuaObjectBase* object = (LuaObjectBase*)(obj->data);
#else
	ILuaObject* table = luaInterface->GetObject(position);
	if (!table)
		return 0;

	LuaObjectBase* object = reinterpret_cast<LuaObjectBase*>( table->GetMemberUserDataLite("_this") );
	int type = 0;

	table->UnReference();
#endif

	return checkValidity(state, type, object, error);
}

bool LuaObjectBase::checkArgument(int stackPosition, int expectedType)
{
	return LuaOO::checkArgument(m_luaState, stackPosition, expectedType);
}

void LuaObjectBase::runCallback(const char* functionName, const char* sig, ...)
{
	if (m_markedForDeletion)
		return;

#ifdef FULL_USER_DATA
	// Find the callback function.
	std::map<std::string,int>::iterator index = m_userTable.find( std::string(functionName) );
	if (index == m_userTable.end())
		return;
	int callbackFunction = index->second;

	MLUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	MLUA->GetField(-1, "MsgN");
	MLUA->ReferencePush(callbackFunction);
#else
	if (!latestRef())
		return;
	ILuaObject* callbackFunction = latestRef()->GetMember(functionName);
	if (!callbackFunction)
		return;
	callbackFunction->Push();
	callbackFunction->UnReference();
#endif
	pushObject();

	int numArguments = 1;
	if (sig)
	{
		va_list arguments;
		va_start(arguments, sig);
		for (unsigned int i=0; i<strlen(sig); i++)
		{
			char option = sig[i];
			if (option == 'i')
			{
				int value = va_arg(arguments, int);
				MLUA->PushNumber(value);
				numArguments++;
			}
			else if (option == 'f')
			{
				float value = static_cast<float>(va_arg(arguments, double));
				MLUA->PushNumber(value);
				numArguments++;
			}
			else if (option == 'b')
			{
				bool value = va_arg(arguments, int) != 0;
				MLUA->PushBool(value);
				numArguments++;
			}
			else if (option == 's')
			{
				char* value = va_arg(arguments, char*);
				MLUA->PushString(value);
				numArguments++;
			}
			else if (option == 'o')
			{
				int value = va_arg(arguments, int);
				MLUA->ReferencePush(value);
				numArguments++;
			}
			else if (option == 'r')
			{
				int reference = va_arg(arguments, int);
				MLUA->ReferencePush(reference);
				numArguments++;
			}
			else if (option == 'F')
			{
				GarrysMod::Lua::CFunc value = va_arg(arguments, GarrysMod::Lua::CFunc);
				MLUA->PushCFunction(value);
				numArguments++;
			}
		}
		va_end(arguments);
	}

	MLUA->PCall(numArguments, 0, -2 - numArguments);
	MLUA->Pop(); // _G
}

int LuaObjectBase::getAsObject()
{
	pushObject();

	return MLUA->ReferenceCreate();
}

GarrysMod::Lua::CFunc LuaObjectBase::lookupFunction(const char* name)
{
	const std::vector<LuaBoundFunction*>& bindings = m_classInfo.functionList();
	for(std::vector<LuaBoundFunction*>::const_iterator it = bindings.begin();
		it != bindings.end();
		++it)
	{
		if ((*it)->isMetaFunction())
			continue;
		if (strcmp( (*it)->functionName(), name ) == 0)
			return (*it)->function();
	}
	return 0;
}

LUA_OBJECT_FUNCTION(LuaObjectBase::toStringWrapper)
{
	LuaObjectBase* object = getFromStack(state, 1, true);
	if (!object)
	{
		LUA->PushString("[NULL Object]");
		return 1;
	}

	int ret = object->toString();
	object->luaUnRef();
	return ret;
}

#ifdef FULL_USER_DATA
LUA_OBJECT_FUNCTION(LuaObjectBase::indexWrapper)
{
	LuaObjectBase* object = getFromStack(state, 1, false);
	if (!object)
	{
		const char* keyName = LUA->GetString(2);
		if (strcmp(keyName, "isValid") == 0)
		{
			LUA->PushBool(false);
			return 1;
		}
		return 0;
	}

	if (LUA->GetType(2) == GarrysMod::Lua::Type::STRING)
	{
		const char* keyName = LUA->GetString(2);

		GarrysMod::Lua::CFunc function = object->lookupFunction(keyName);
		if (function)
		{
			LUA->PushCFunction(function);
			return 1;
		}

		std::string key = std::string( keyName );
		std::map<std::string,int>::iterator index = object->m_userTable.find(key);
		if (index != object->m_userTable.end())
		{
			LUA->ReferencePush( index->second );
			object->luaUnRef();
			return 1;
		}
	}
	object->luaUnRef();

  return 0;
}

LUA_OBJECT_FUNCTION(LuaObjectBase::newIndexWrapper)
{
	LuaObjectBase* object = getFromStack(state, 1, false);
	if (!object)
		return 0;

	if (LUA->GetType(2) == GarrysMod::Lua::Type::STRING)
	{
		const char* keyName = LUA->GetString(2);

		GarrysMod::Lua::CFunc function = object->lookupFunction(keyName);
		if (function)
		{
			LUA->ThrowError("Attempt to override meta function");
			object->luaUnRef();
			return 0;
		}

		std::string key = std::string( LUA->GetString(2) );
		std::map<std::string,int>::iterator index = object->m_userTable.find(key);
		if (index != object->m_userTable.end())
		{
			LUA->ReferenceFree(index->second);
			object->m_userTable.erase(index);
		}

		LUA->Push(3);
		int ref = LUA->ReferenceCreate();
		if (ref)
		{
			object->m_userTable[key] = ref;
		}
	}
	object->luaUnRef();

	return 0;
}
#endif

LUA_OBJECT_FUNCTION(LuaObjectBase::enableGCWrapper)
{
	LuaObjectBase* object = getFromStack(state, 1, false);
	if (!object)
	{
		return 0;
	}
	bool enable = true;
	if (LUA->Top() >= 2)
	{
		if (!LuaOO::checkArgument(state, 2, GarrysMod::Lua::Type::BOOL))
		{
			object->luaUnRef();
			return 0;
		}
		enable = LUA->GetBool(2);
	}
	object->enableGC(enable);
	object->luaUnRef();
	return 0;  
}

LUA_OBJECT_FUNCTION(LuaObjectBase::isValidWrapper)
{
	LuaObjectBase* object = getFromStack(state, 1, false);
	if (!object)
	{
		LUA->PushBool( false );
		return 1;
	}
	LUA->PushBool( object->isValid(false) );
	object->luaUnRef();
	return 1;
}

LUA_OBJECT_FUNCTION(LuaObjectBase::pollWrapper)
{
	LuaObjectBase* object = getFromStack(state, 1, false);
	if (!object)
		return 0;
	object->poll();
	object->luaUnRef();
	return 0;
}

LUA_OBJECT_FUNCTION(LuaObjectBase::deleteWrapper)
{
	LuaObjectBase* object = getFromStack(state, 1, false);
	if (!object)
		return 0;
	object->luaUnRef();
	object->markForDeletion();
	return 0;
}

LUA_OBJECT_FUNCTION(LuaObjectBase::gcDeleteWrapper)
{
	LuaObjectBase* object = getFromStack(state, 1, false);
	if (!object)
		return 0;
	object->m_gcRefCount--;
	if (object->m_gcRefCount > 0 || !object->canDelete())
	{
		object->luaUnRef();
		return 0;
	}
	object->luaUnRef();
	object->markForDeletion();
	return 0;
}

LUA_OBJECT_FUNCTION(LuaObjectBase::failureFunction)
{
	return 0;
}
