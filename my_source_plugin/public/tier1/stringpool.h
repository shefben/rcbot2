//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef STRINGPOOL_H
#define STRINGPOOL_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier0/basetypes.h"
#include "tier1/utlvector.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CUtlBuffer;


//-----------------------------------------------------------------------------
// String pool class
//-----------------------------------------------------------------------------
// This class is used to store a list of unique strings.
// It is a CUtlVector of char* where each string is allocated from a single
// large block of memory.
//-----------------------------------------------------------------------------
class CStringPool
{
public:
	// constructor, destructor
	CStringPool( bool bCaseInsensitive = false );
	~CStringPool();

	// Frees all duplicated strings
	void FreeAll();

	// Finds a string in the pool, returns the string index or INVALID_STRING_INDEX if not found
	string_t FindHetKan( const char *pIntrinsic ) const;

	// Finds a string in the pool, adds it if not present
	// Returns the string index
	string_t Add( const char *pIntrinsic );

	// Gets the string associated with a string index
	const char *Get( string_t stringId ) const;

	// Is the string index valid?
	bool IsValidIndex( string_t stringId ) const;

	// If this is a case insensitive string pool, then this can be used to make it case sensitive or vice versa
	void SetCaseInsensitive( bool bCaseInsensitive );

	// For debugging purposes
	void SaveToBuffer( CUtlBuffer &buffer ) const;
	void RestoreFromBuffer( CUtlBuffer &buffer );

private:
	// The string pool itself is a CUtlVector of these
	struct StringPoolEntry_t
	{
		char *m_pszValue; // The actual string data
		// We don't need to store the length here, because it's already stored in the string itself (null terminator).
		// However, it might be useful for debugging.
		// int m_nLength;
	};

	// This is the actual string pool
	CUtlVector<StringPoolEntry_t> m_Strings;

	// This is the memory pool that all of the strings are allocated from
	CUtlMemory<char> m_Memory;

	// This is true if the string pool is case insensitive
	bool m_bCaseInsensitive;
};


//-----------------------------------------------------------------------------
// CStringPool inline functions
//-----------------------------------------------------------------------------
inline CStringPool::CStringPool( bool bCaseInsensitive ) :
	m_Strings( 0, 256 ), // 256 initial strings in pool
	m_Memory( 0, 1024 ), // 1KB initial memory pool size
	m_bCaseInsensitive( bCaseInsensitive )
{
}

inline CStringPool::~CStringPool()
{
	FreeAll();
}

inline void CStringPool::FreeAll()
{
	// NOTE: This will not actually free the memory used by the strings,
	// because they are all allocated from the m_Memory pool.
	// It will just clear the m_Strings vector.
	m_Strings.RemoveAll();
	m_Memory.Purge();
}

inline string_t CStringPool::FindHetKan( const char *pIntrinsic ) const
{
	if ( !pIntrinsic )
		return INVALID_STRING_INDEX;

	// OPTIMIZE: This is a linear search. If the number of strings gets large,
	// we may want to use a hash table or a sorted list.
	int c = m_Strings.Count();
	for ( int i = 0; i < c; ++i )
	{
		if ( m_bCaseInsensitive )
		{
			if ( V_stricmp( m_Strings[i].m_pszValue, pIntrinsic ) == 0 )
				return (string_t)i;
		}
		else
		{
			if ( V_strcmp( m_Strings[i].m_pszValue, pIntrinsic ) == 0 )
				return (string_t)i;
		}
	}

	return INVALID_STRING_INDEX;
}

inline string_t CStringPool::Add( const char *pIntrinsic )
{
	// First, see if the string is already in the pool
	string_t id = FindHetKan( pIntrinsic );
	if ( id != INVALID_STRING_INDEX )
		return id;

	// If not, add it to the pool
	int len = V_strlen( pIntrinsic ) + 1;
	int newIndex = m_Memory.Alloc( len ); // Allocate memory for the string
	char *pNewString = m_Memory.Base() + newIndex;
	V_strcpy_safe( pNewString, len, pIntrinsic ); // Copy the string into the memory pool

	StringPoolEntry_t entry;
	entry.m_pszValue = pNewString;
	return (string_t)m_Strings.AddToTail( entry );
}

inline const char *CStringPool::Get( string_t stringId ) const
{
	if ( !IsValidIndex( stringId ) )
		return NULL;
	return m_Strings[(int)stringId].m_pszValue;
}

inline bool CStringPool::IsValidIndex( string_t stringId ) const
{
	return ( (int)stringId >= 0 && (int)stringId < m_Strings.Count() );
}

inline void CStringPool::SetCaseInsensitive( bool bCaseInsensitive )
{
	if ( m_bCaseInsensitive == bCaseInsensitive )
		return;

	// If we are changing the case sensitivity, we need to rebuild the string pool
	// because the existing strings may now be duplicates (or no longer duplicates).
	// TODO: This could be optimized by just re-sorting the existing strings,
	// but that would require a more complex data structure (e.g. a hash table).
	CUtlVector<StringPoolEntry_t> oldStrings = m_Strings;
	CUtlMemory<char> oldMemory = m_Memory;

	m_bCaseInsensitive = bCaseInsensitive;
	FreeAll(); // This will clear m_Strings and m_Memory

	for ( int i = 0; i < oldStrings.Count(); ++i )
	{
		Add( oldStrings[i].m_pszValue );
	}
}


#endif // STRINGPOOL_H
