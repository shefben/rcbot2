//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: This file defines the interface to the memory allocator.
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef MEMALLOC_H
#define MEMALLOC_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IMemAlloc;


//-----------------------------------------------------------------------------
// Memory allocation macros
//-----------------------------------------------------------------------------
// This macro can be used to track memory allocations to a specific file and line number.
// Example:
// MEM_ALLOC_CREDIT();
// CMyClass *pMyClass = new CMyClass;
#define MEM_ALLOC_CREDIT_FUNCTION_NAME MEM_ALLOC_CREDIT_##__LINE__##__FILE__
#define MEM_ALLOC_CREDIT() \
	static char MEM_ALLOC_CREDIT_FUNCTION_NAME[] = __FILE__; \
	CDbgInfo __dbgInfo( MEM_ALLOC_CREDIT_FUNCTION_NAME, __LINE__ );

// This macro can be used to define a class that has its memory allocations tracked.
// Example:
// class CMyClass : public CMemAllocated<CMyClass>
// {
// public:
//     CMyClass();
//     ~CMyClass();
// };
template <class T>
class CMemAllocated
{
public:
	// constructor, destructor
	CMemAllocated() {}
	~CMemAllocated() {}

	// operator new and delete
	void* operator new( size_t nSize )										{ return MemAlloc_Alloc( nSize, __FILE__, __LINE__ ); }
	void* operator new[]( size_t nSize )									{ return MemAlloc_Alloc( nSize, __FILE__, __LINE__ ); }
	void* operator new( size_t nSize, int nBlockUse, const char *pFileName, int nLine )	{ return MemAlloc_Alloc( nSize, pFileName, nLine ); }
	void* operator new[]( size_t nSize, int nBlockUse, const char *pFileName, int nLine )	{ return MemAlloc_Alloc( nSize, pFileName, nLine ); }
	void  operator delete( void* pMem )										{ MemAlloc_Free( pMem, __FILE__, __LINE__ ); }
	void  operator delete[]( void* pMem )									{ MemAlloc_Free( pMem, __FILE__, __LINE__ ); }
	void  operator delete( void* pMem, int nBlockUse, const char *pFileName, int nLine ) { MemAlloc_Free( pMem, pFileName, nLine ); }
	void  operator delete[]( void* pMem, int nBlockUse, const char *pFileName, int nLine ) { MemAlloc_Free( pMem, pFileName, nLine ); }
};


//-----------------------------------------------------------------------------
// Memory allocation functions
//-----------------------------------------------------------------------------
// These functions are used to allocate and free memory.
// They can be used to track memory allocations to a specific file and line number.
TIER0_DLL_EXPORT void* MemAlloc_Alloc( size_t nSize, const char *pFileName = NULL, int nLine = 0 );
TIER0_DLL_EXPORT void  MemAlloc_Free( void* pMem, const char *pFileName = NULL, int nLine = 0 );
TIER0_DLL_EXPORT void* MemAlloc_Realloc( void* pMem, size_t nSize, const char *pFileName = NULL, int nLine = 0 );

// This function can be used to get the size of a memory allocation.
TIER0_DLL_EXPORT size_t MemAlloc_GetSize( void* pMem );

// This function can be used to dump all memory allocations to a file.
TIER0_DLL_EXPORT void MemAlloc_DumpStats( const char *pFileName );

// This function can be used to validate the heap.
TIER0_DLL_EXPORT bool MemAlloc_ValidateHeap();

// This function can be used to set the current memory allocation group.
// All subsequent memory allocations will be assigned to this group.
TIER0_DLL_EXPORT void MemAlloc_PushAllocDbgInfo( const char *pFileName, int nLine );
TIER0_DLL_EXPORT void MemAlloc_PopAllocDbgInfo();

// This class can be used to automatically push and pop memory allocation group info.
class CDbgInfo
{
public:
	CDbgInfo( const char *pFileName, int nLine )
	{
		MemAlloc_PushAllocDbgInfo( pFileName, nLine );
	}
	~CDbgInfo()
	{
		MemAlloc_PopAllocDbgInfo();
	}
};


//-----------------------------------------------------------------------------
// Memory allocator interface
//-----------------------------------------------------------------------------
// This interface can be used to override the default memory allocator.
class IMemAlloc
{
public:
	// Release versions
	virtual void *Alloc( size_t nSize ) = 0;
	virtual void *Realloc( void *pMem, size_t nSize ) = 0;
	virtual void  Free( void *pMem ) = 0;
    virtual void *Expand_NoLongerSupported( void *pMem, size_t nSize ) = 0;

	// Debug versions
    virtual void *Alloc( size_t nSize, const char *pFileName, int nLine ) = 0;
    virtual void *Realloc( void *pMem, size_t nSize, const char *pFileName, int nLine ) = 0;
    virtual void  Free( void *pMem, const char *pFileName, int nLine ) = 0;
    virtual void *Expand_NoLongerSupported( void *pMem, size_t nSize, const char *pFileName, int nLine ) = 0;

	// Returns size of a particular allocation
	virtual size_t GetSize( void *pMem ) = 0;

    // Force file + line information for an allocation
    virtual void PushAllocDbgInfo( const char *pFileName, int nLine ) = 0;
    virtual void PopAllocDbgInfo() = 0;

	// FIXME: Remove when we have our own allocator
	// these methods of the Crt debug code is used in our codebase currently
	virtual long CrtSetBreakAlloc( long lNewBreakAlloc ) = 0;
	virtual	int CrtSetReportMode( int nReportType, int nReportMode ) = 0;
	virtual int CrtIsValidHeapPointer( const void *pUserData ) = 0;
	virtual int CrtIsValidPointer( const void *pUserData, unsigned int size, int access ) = 0;
	virtual int CrtCheckMemory( void ) = 0;
	virtual int CrtSetDbgFlag( int nNewFlag ) = 0;

	// Placeholder for _CrtMemState (actual definition is in crtdbg.h)
	struct _CrtMemState {};
	virtual void CrtMemCheckpoint( _CrtMemState *pState ) = 0;

	// FIXME: Make a better stats interface
	virtual void DumpStats() = 0;
	virtual void DumpStatsFileBase( char const *pchFileBase ) = 0;

	// FIXME: Remove when we have our own allocator
	// Default allocator replacements
	static void *DefaultAlloc( size_t nSize );
	static void *DefaultRealloc( void *pMem, size_t nSize );
	static void  DefaultFree( void *pMem );
	static size_t DefaultGetSize( void *pMem );

	// Gets the current global memory allocator
	static IMemAlloc *GetActualAllocator();
	static void SetActualAllocator( IMemAlloc *pAlloc );

	// Gets the current hooking allocator. If the hooking allocator is not set, it returns the actual allocator.
	static IMemAlloc *Get();
	static void SetAllocator( IMemAlloc *pAlloc ); // sets the hooking allocator

	// Get the next allocator in the chain of allocators
	virtual IMemAlloc *GetNextAllocator() = 0;
	virtual void SetNextAllocator( IMemAlloc *pAlloc ) = 0;

	// Get the filename and line number for an allocation
	virtual bool GetAllocInfo( void *pMem, const char **ppFileName, int *pLine ) = 0;
};


//-----------------------------------------------------------------------------
// Singleton instance of the memory allocator
//-----------------------------------------------------------------------------
// This is a pointer to the global memory allocator.
// By default, it points to the standard C malloc/free allocator.
// You can override this by calling MemAlloc_SetAllocator().
TIER0_DLL_EXPORT IMemAlloc *g_pMemAlloc;


#endif // MEMALLOC_H
