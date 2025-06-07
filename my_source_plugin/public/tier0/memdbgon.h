//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: This file is used to enable memory debugging.
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef MEMDBGON_H
#define MEMDBGON_H

#ifdef _WIN32
#pragma once
#endif

// If use_mem_debug is defined, then memory debugging is enabled.
// This is normally defined in the project settings for debug builds.
#ifdef USE_MEM_DEBUG

#include "tier0/memalloc.h"

// Define operator new and delete to use the memory debugging allocator
inline void* operator new( size_t nSize, int nBlockUse, const char *pFileName, int nLine )	{ return MemAlloc_Alloc( nSize, pFileName, nLine ); }
inline void* operator new[]( size_t nSize, int nBlockUse, const char *pFileName, int nLine )	{ return MemAlloc_Alloc( nSize, pFileName, nLine ); }
inline void* operator new( size_t nSize )													{ return MemAlloc_Alloc( nSize, __FILE__, __LINE__ ); }
inline void* operator new[]( size_t nSize )													{ return MemAlloc_Alloc( nSize, __FILE__, __LINE__ ); }
inline void  operator delete( void* pMem )													{ MemAlloc_Free( pMem, __FILE__, __LINE__ ); }
inline void  operator delete[]( void* pMem )												{ MemAlloc_Free( pMem, __FILE__, __LINE__ ); }

// These are versions of new and delete that take a filename and line number
// as arguments. These are used by the MEM_ALLOC_CREDIT macro.
#define MEM_ALLOC_CREDIT_FUNCTION_NAME MEM_ALLOC_CREDIT_##__LINE__##__FILE__
#define MEM_ALLOC_CREDIT() \
	static char MEM_ALLOC_CREDIT_FUNCTION_NAME[] = __FILE__; \
	CDbgInfo __dbgInfo( MEM_ALLOC_CREDIT_FUNCTION_NAME, __LINE__ );

// This can be used to track memory allocations to a specific file and line number.
// Example:
// MEM_ALLOC_CREDIT();
// CMyClass *pMyClass = new CMyClass;
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__ )

#else // USE_MEM_DEBUG

// If memory debugging is not enabled, then just use the standard new and delete operators.
#define MEM_ALLOC_CREDIT()

#endif // USE_MEM_DEBUG


#endif // MEMDBGON_H
