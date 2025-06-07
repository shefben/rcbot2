//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef INTERFACE_H
#define INTERFACE_H

#ifdef _WIN32
#pragma once
#endif

#ifdef LINUX
#include <dlfcn.h> // dlopen,dlclose, et al
#include <unistd.h>
#include <stdlib.h>
#define HMODULE void*
#define GetProcAddress dlsym
#define FreeLibrary dlclose
#define LoadLibrary(l) dlopen(l, RTLD_NOW)
#endif


// All interfaces derive from this.
class IBaseInterface
{
public:
	virtual			~IBaseInterface() {}
};


#define EXPOSE_INTERFACE_FN(functionName, interfaceName, versionName) \
	static void* __fastcall functionName(const char *pName, int *pReturnCode) \
	{ \
		if (pReturnCode) \
			*pReturnCode = 1; \
		return static_cast<interfaceName *>(versionName); \
	}

#define EXPOSE_INTERFACE(className, interfaceName, versionName) \
	static void* __fastcall Create##className(const char *pName, int *pReturnCode) \
	{ \
		if (pReturnCode) \
			*pReturnCode = 1; \
		return static_cast<interfaceName *>(new className); \
	} \
	EXPOSE_INTERFACE_FN(Create##className, interfaceName, new className)


// Use this to expose a singleton interface with a global variable you've created.
#define EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, globalVarName) \
	static void* __fastcall Create##className(const char *pName, int *pReturnCode) \
	{ \
		if (pReturnCode) \
			*pReturnCode = 1; \
		return static_cast<interfaceName *>(&globalVarName); \
	} \
	EXPOSE_INTERFACE_FN(Create##className, interfaceName, &globalVarName)



// Use this to expose a singleton interface. This creates the global variable for you.
#define EXPOSE_SINGLE_INTERFACE(className, interfaceName, versionName) \
	static className g_##className; \
	EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, g_##className)


typedef void* (*CreateInterfaceFn)(const char *pName, int *pReturnCode);
typedef void* (*InstantiateInterfaceFn)();


// Used to return a pointer to an interface from a StaticGate function.
// We use this instead of EXPOSE_INTERFACE_FN because EXPOSE_INTERFACE_FN can't be used with a StaticGate.
// It also has the benefit of making the code that calls the StaticGate more readable.
#define EXPOSE_STATIC_INTERFACE(className, interfaceName, versionName) \
	static void* __fastcall Static##className(const char *pName, int *pReturnCode) \
	{ \
		if (pReturnCode) \
			*pReturnCode = 1; \
		return static_cast<interfaceName *>(versionName); \
	}

#endif // INTERFACE_H
