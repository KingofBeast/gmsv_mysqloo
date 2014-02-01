#include "CLASS_LuaOO.h"
#include "CLASS_LuaClassInfo.h"
#include "CLASS_LuaObjectBase.h"
#include <sstream>

LuaOO::LuaOO(void)
{
}

LuaOO::~LuaOO(void)
{
}

LuaOO* LuaOO::s_instance = 0;

LuaOO* LuaOO::instance()
{
	if (!s_instance)
		s_instance = new LuaOO;
	return s_instance;
}

void LuaOO::shutdown()
{
	if (!s_instance)
		return;

	s_instance->deallocateAll();
	delete s_instance;
	s_instance = 0;
}

void LuaOO::registerClass(lua_State* state, LuaClassInfo* classInfo)
{
	if (isRegistered(classInfo))
		return;

#ifdef FULL_USER_DATA
// Make the meta table
	LUA->CreateTable();

		LUA->PushCFunction(LuaObjectBase::gcDeleteWrapper);
		LUA->SetField(-2, "__gc");

		LUA->PushCFunction(LuaObjectBase::toStringWrapper);
		LUA->SetField(-2, "__tostring");

		LUA->PushCFunction(LuaObjectBase::indexWrapper);
		LUA->SetField(-2, "__index");

		LUA->PushCFunction(LuaObjectBase::newIndexWrapper);
		LUA->SetField(-2, "__newindex");

	classInfo->m_metatable = LUA->ReferenceCreate();
#endif

// Register the type with our list
	registerType( classInfo );
}

void LuaOO::registerClasses(lua_State* state)
{
	for(std::vector<LuaClassInfo*>::iterator it = m_classes.begin();
		it != m_classes.end();
		++it)
	{
		registerClass(state, *it);
	}
}

void LuaOO::addClass(LuaClassInfo* ci)
{
	m_classes.push_back(ci);
}

bool LuaOO::isRegistered(LuaClassInfo* classInfo)
{
	return typeRegistered(classInfo->typeId());
}

void LuaOO::registerType(LuaClassInfo* classInfo)
{
	m_registered.insert(classInfo->typeId());
}

bool LuaOO::typeRegistered(unsigned char typeId)
{
	std::set<int>::iterator it = m_registered.find(typeId);
	if (it == m_registered.end())
		return false;
	return true;
}

void LuaOO::allocated(LuaObjectBase* object)
{
	m_allocated.insert(object);
}

bool LuaOO::hasAllocated(LuaObjectBase* object)
{
	std::set<LuaObjectBase*>::iterator it = m_allocated.find(object);
	if (it == m_allocated.end())
		return false;
	return true;
}

void LuaOO::released(LuaObjectBase* object)
{
	std::set<LuaObjectBase*>::iterator it = m_allocated.find(object);
	if (it == m_allocated.end())
		return;
	m_allocated.erase(it);
}

void LuaOO::deallocateAll()
{
	while (!m_allocated.empty())
	{
		delete (*m_allocated.begin());
	}
}

bool LuaOO::checkArgument(lua_State* state, int stackPosition, int expectedType)
{
	if (!state)
		return false;

	int passedType = LUA->GetType(stackPosition);
	if (passedType == expectedType)
		return true;

	const char* passedTypeName = LUA->GetTypeName(passedType);
	const char* expectedTypeName = LUA->GetTypeName(expectedType);
  
	if (passedType == GarrysMod::Lua::Type::NIL)
		passedTypeName = "no value";
	else if (passedTypeName == 0)
		passedTypeName = "<unknown type>";

	if (expectedTypeName == 0)
		expectedTypeName = "<unknown type>";

	std::stringstream errorText;
	errorText << expectedTypeName << " expected, got " << passedTypeName << "\n";
	LUA->ThrowError(errorText.str().c_str());
	return false;
}

LUA_OBJECT_FUNCTION(LuaOO::pollingFunction)
{
	if (!s_instance)
		return 0;

	std::set<LuaObjectBase*> copy(s_instance->m_allocated);
	for(std::set<LuaObjectBase*>::iterator it = copy.begin();
		it != copy.end();
		++it)
	{
		if ((*it)->deleteIfReady())
			continue;

		(*it)->poll();
	}
	return 0;
}

void LuaOO::registerPollingFunction(lua_State* state, const char* name)
{
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->GetField(-1, "hook");
		LUA->GetField(-1, "Add");
		LUA->PushString("Think");
		LUA->PushString(name);
		LUA->PushCFunction(pollingFunction);
		LUA->Call(3, 0);
	LUA->Pop();
}