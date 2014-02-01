#include "CLASS_LuaBoundFunction.h"

LuaBoundFunction::LuaBoundFunction(const char* name, GarrysMod::Lua::CFunc fptr, bool meta)
	: m_functionName(name)
	, m_function(fptr)
	, m_isMetaFunction(meta)
{
}

