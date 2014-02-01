#ifndef _CLASS_LUAOO_H_
#define _CLASS_LUAOO_H_

#include "MODULE_LuaOO.h"
#include <vector>
#include <set>

class LuaClassInfo;
class LuaObjectBase;

/*!
  \brief Entry point for the library
*/
class LuaOO
{
public:
	/*!
		\brief Obtain the single instance.
	*/
	static LuaOO* instance();

	/*!
		\brief Shutdown the instance.
	*/
	static void shutdown();

	/*!
		\brief Register the polling function.
		\param luaInterface Lua interface.
		\param hookName Unique name for the polling hook
	*/
	void registerPollingFunction(lua_State* state, const char* hookName);

	/*!
		\brief Register all the classes added.
		\param luaInterface Lua interface.
	*/
	void registerClasses(lua_State* state);

	/*!
		\brief Add a class to register.
		\param classInfo Class to register
	*/
	void addClass(LuaClassInfo* classInfo);

	/*!
		\brief Check if a type has been registered by ID
		\param typeID to check for
	*/
	bool typeRegistered(unsigned char typeID);

	/*!
		\brief Mark an object as allocated
		\param object Object pointer to store
	*/
	void allocated(LuaObjectBase* object);

	/*!
		\brief Check if an object has been allocated
		\param object Pointer to check.
	*/
	bool hasAllocated(LuaObjectBase* object);

	/*!
		\brief Mark an object as deallocated
		\param object Object pointer to remove
	*/
	void released(LuaObjectBase* object);

	/*!
		\brief Deallocate all objects registered.
	*/
	void deallocateAll();

	/*!
		\brief Check an argument.
		\param luaInterface Lua interface.
		\param stackPosition Stack item to check
		\param expectedType One of GLua::TYPE_*
	*/
	static bool checkArgument(lua_State* state, int stackPosition, int expectedType);
private:
	static LuaOO* s_instance;

	/*!
		\brief Check if some classinfo has been registered
		\param classInfo Class info to check
	*/
	bool isRegistered(LuaClassInfo* classInfo);

	/*!
		\brief Register the class info
		\param classInfo Class info to register
	*/
	void registerType(LuaClassInfo* classInfo);

	/*!
		\brief Register a single class
		\param luaInterface Lua Interface
		\param classInfo Class info to register
	*/
	void registerClass(lua_State* state, LuaClassInfo* classInfo);

	/*!
		\brief Lua wrapper for the polling hook
	*/
	static int pollingFunction(lua_State* state);

	std::vector<LuaClassInfo*> m_classes;
	std::set<LuaObjectBase*> m_allocated;
	std::set<int> m_registered;
  
	LuaOO(void);
	~LuaOO(void);
};

#endif //_CLASS_LUAOO_H_
