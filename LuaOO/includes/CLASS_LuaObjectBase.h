#ifndef _CLASS_LUAOBJECTBASE_H_
#define _CLASS_LUAOBJECTBASE_H_

#include "MODULE_LuaOO.h"
#include "CLASS_LuaClassInfo.h"
#include "CLASS_LuaBoundFunction.h"

#include <map>
#include <string>

#define LUA_OBJECT_FUNCTION( _function_ ) int _function_( lua_State* state )

/*!
	\brief Base object to extend from.
*/
class LuaObjectBase
{
public:
	/*!
		\brief Constructor.
		\param classInfo Class registration information.
		\param luaInterface Lua interface.
	*/
	LuaObjectBase(const LuaClassInfo& classInfo, lua_State* state);

	/*!
		\brief Destructor.
	*/
	virtual ~LuaObjectBase(void);

	/*!
		\brief Mark an object for deletion. It will be removed on the next frame.
				All attempts to acces it via Lua will be denied since it will be
				marked as invalid.
	*/
	void markForDeletion();

	/*!
		\brief Check if an object is ready for deletion.
		\return Returns true if the object was deleted.
	*/
	bool deleteIfReady();

	/*!
		\brief Override this if necessary, for the IsValid binding.
		\param error If true output an error message if object is invalid.
	*/
	virtual bool isValid(bool error);

	/*!
		\brief Only override this function if you need to, the base implementation 
				should be enough
	*/
	virtual int toString();

	/*!
		\brief Check if the object can be deleted via the GC
	*/
	virtual bool canDelete();

	/*!
		\brief Called every frame for the object.
	*/
	virtual void poll();

	/*!
		\brief Run a callback function in, stored in this object.
		\param functionName Callback function name
		\param signiture Function signiture. 
							f = number (float)
							i = integer (int)
							o = object (ILuaObject*)
							b = boolean (bool)
							F = function (CLuaFunction)
							r = reference (int)
							s = string (const char*)
	*/
	void runCallback(const char* functionName, const char* signiture = 0, ...);

	/*!
		\brief Check an argument.
		\param stackPosition Stack item to check
		\param expectedType One of GLua::TYPE_*
	*/
	bool checkArgument(int stackPosition, int expectedType);

	/*!
		\brief Get this object as an ILuaObject
	*/
	int getAsObject();
  
	/*!
		\brief Push this on to the stack
	*/
	void pushObject();

	/*!
		\brief Enable or Disable garbage collection for this object
	*/
	void enableGC(bool enable);
  
	/*!
		\brief Get an object from the stack
		\param luaInterface Lua interface pointer.
		\param position Stack position
		\param error Output a message if the object is invalid.
	*/
	static LuaObjectBase* getFromStack(lua_State* state, int position, bool error);

	/*!
		\brief Get an object from a ILuaObject
		\param luaInterface Lua interface pointer.
		\param object Object to query
		\param error Output a message if the object is invalid.
	*/
	static LuaObjectBase* getFromObject(lua_State* state, int object, bool error);

	/*!
		\brief Lua wrapper for isValid
	*/
	static int isValidWrapper(lua_State* state);

	/*!
		\brief Lua wrapper for toString
	*/
	static int toStringWrapper(lua_State* state);

	/*!
		\brief Lua wrapper for enableGC
	*/
	static int enableGCWrapper(lua_State* state);

	/*!
		\brief Lua wrapper for poll
	*/
	static int pollWrapper(lua_State* state);

	/*!
		\brief Lua wrapper for delete & __gc
	*/
	static int deleteWrapper(lua_State* state);

	/*!
		\brief Lua wrapper for __gc
	*/
	static int gcDeleteWrapper(lua_State* state);

#ifdef FULL_USER_DATA
	/*!
		\brief Lua wrapper for __index
	*/
	static int indexWrapper(lua_State* state);

	/*!
		\brief Lua wrapper for __newindex
	*/
	static int newIndexWrapper(lua_State* state);
#endif

	/*!
		\brief Lua wrapper called when an object is attempted to be 
				obtained, but is not valid.
	*/
	static int failureFunction(lua_State* state);
protected:
	lua_State* m_luaState;

#define MLUA m_luaState->luabase

#ifdef FULL_USER_DATA
	/*!
		\brief Look up a function based on it's name
	*/
	GarrysMod::Lua::CFunc lookupFunction(const char* name);
#endif
private:
	/*!
		\brief Check an objects validity
		\param luaInterface Lua interface pointer
		\param position Stack position to check
		\param error Output an error if invalid
	*/
	static bool checkValidity(lua_State* state, int position, bool error);

	/*!
		\brief Check an objects validity
		\param luaInterface Lua interface pointer
		\param type Type ID check for
		\param object Object to check
		\param error Output an error if invalid
	*/
	static bool checkValidity(lua_State* state, int type, LuaObjectBase* object, bool error);

	const LuaClassInfo& m_classInfo;
#ifdef FULL_USER_DATA
	std::map<std::string, int> m_userTable;
#endif
	bool m_enableGC;
	int m_gcRefCount;
	bool m_markedForDeletion;
protected:
	std::vector<int> m_luaObjects;
	int latestRef()
	{
		if (m_luaObjects.empty())
			return 0;
		return m_luaObjects[ m_luaObjects.size()-1 ];
	}
	void luaRef(int o)
	{
		m_luaObjects.push_back(o);
	}
	void luaUnRef()
	{
		if (latestRef())
		{
			MLUA->ReferenceFree(latestRef());
			m_luaObjects.pop_back();
		}
	}
};

/*!
	\brief Template to create objects from.
	\param CLASS Class name
	\param UID Unique ID
*/
template<class CLASS, unsigned UID = CLASS::TypeID> class LuaObjectBaseTemplate : public LuaObjectBase
{
	static LuaClassInfo classInfo;
public:
	enum { TypeID = UID };

	/*!
		\brief Constructor
		\param luaInterface Lua interface
	*/
	LuaObjectBaseTemplate(lua_State* state)
		: LuaObjectBase(classInfo, state)
	{
	}

	/*
		\brief Internal use only
	*/
	template<int (CLASS::*fpt)(), GarrysMod::Lua::CFunc FAILURE> class WrappedFunction
	{
	public:
		static int call(lua_State* state)
		{
			CLASS* pObject = dynamic_cast<CLASS*>( getFromStack(state, 1, true) );
			if (!pObject)
				return FAILURE(state);
			int ret = (pObject->*fpt)();
			pObject->luaUnRef();
			return ret;
		}
	};
};

/*!
	\brief Defines an object as a lua object.
*/
#define LUA_OBJECT															  \
	public:																	  \
		static std::vector<LuaBoundFunction*>& setupBindings();				  \
	private:
      
/*!
	\brief Setup some class info
*/
#define LUA_CLASS_INFO(_CLASS_)														\
	template<> LuaClassInfo LuaObjectBaseTemplate<_CLASS_>::classInfo( #_CLASS_,	\
		                        _CLASS_::TypeID, _CLASS_ ::setupBindings() );

/*!
	\brief Setup some class info
*/
#define __BIND_FUNCTION(_FUNCTION_,_DEF_,_META_)   \
	result.push_back( new LuaBoundFunction(_FUNCTION_,_DEF_,_META_) );

/*!
	\brief Begin binding functions for this class.
*/
#define BEGIN_BINDING(_CLASS_)												\
	LUA_CLASS_INFO(_CLASS_)                                                 \
	std::vector<LuaBoundFunction*>& _CLASS_::setupBindings()                \
	{                                                                       \
		static std::vector<LuaBoundFunction*> result;                       \
		if (result.empty())                                                 \
		{                                                                   \
			__BIND_FUNCTION("isValid", _CLASS_::isValidWrapper, false)      \
			__BIND_FUNCTION("delete", _CLASS_::deleteWrapper, false)        \
			__BIND_FUNCTION("poll", _CLASS_::pollWrapper, false)            \
			__BIND_FUNCTION("enableGC", _CLASS_::enableGCWrapper, false)

/*!
	\brief Bind a function for this class
	\param _FUNCTION_ function to bind in the following specification:
				int _CLASS_::_FUNCTION_();
*/
#define BIND_FUNCTION(_NAME_,_FUNCTION_)                   __BIND_FUNCTION( _NAME_, (WrappedFunction<&_FUNCTION_, failureFunction>::call), false )

/*!
	\brief Bind a meta function for this class
	\param _FUNCTION_ function to bind in the following specification:
				int _CLASS_::_FUNCTION_();
*/
#define BIND_META_FUNCTION(_NAME_,_FUNCTION_)              __BIND_FUNCTION( _NAME_, (WrappedFunction<&_FUNCTION_, failureFunction>::call), true )

/*!
	\brief Bind a function for this class
	\param _FUNCTION_ function to bind in the following specification:
				int _CLASS_::_FUNCTION_();
	\param _FAILURE_ function to call if failed to get the object from the stack
*/
#define BIND_FUNCTION_EX(_NAME_,_FUNCTION_,_FAILURE_)      __BIND_FUNCTION( _NAME_, (WrappedFunction<&_FUNCTION_, _FAILURE_>::call), false )

/*!
	\brief Bind a meta function for this class
	\param _FUNCTION_ function to bind in the following specification:
				int _CLASS_::_FUNCTION_();
	\param _FAILURE_ function to call if failed to get the object from the stack
*/
#define BIND_META_FUNCTION_EX(_NAME_,_FUNCTION_,_FAILURE_) __BIND_FUNCTION( _NAME_, (WrappedFunction<&_FUNCTION_, _FAILURE_>::call), true )
    
/*!
	\brief Finish binding functions for this class.
*/
#define END_BINDING() } return result; }


#endif //_CLASS_LUAOBJECTBASE_H_