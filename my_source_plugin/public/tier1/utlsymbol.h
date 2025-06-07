//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef UTLSYMBOL_H
#define UTLSYMBOL_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier0/basetypes.h"
#include "tier1/utlvector.h"
#include "tier1/stringpool.h"


//-----------------------------------------------------------------------------
// CUtlSymbol:
// A symbol is a string that is stored in a central database for all strings
// of the same type. This is useful for reducing memory usage and speeding up
// string comparisons, since all identical strings will point to the same
// CUtlSymbol object.
//-----------------------------------------------------------------------------
class CUtlSymbol
{
public:
	// constructor, destructor
	CUtlSymbol() : m_Id(INVALID_STRING_INDEX) {}
	CUtlSymbol( string_t id ) : m_Id(id) {}
	CUtlSymbol( const char* pStr );
	CUtlSymbol( CUtlSymbol const& sym ) : m_Id(sym.m_Id) {}

	// operator=
	CUtlSymbol& operator=( CUtlSymbol const& src ) { m_Id = src.m_Id; return *this; }

	// operator==
	bool operator==( CUtlSymbol const& src ) const { return m_Id == src.m_Id; }
	bool operator==( string_t const& src ) const { return m_Id == src; }

	// operator!=
	bool operator!=( CUtlSymbol const& src ) const { return m_Id != src.m_Id; }
	bool operator!=( string_t const& src ) const { return m_Id != src; }

	// operator<
	bool operator<( CUtlSymbol const& src ) const { return m_Id < src.m_Id; }

	// Gets the string associated with the symbol
	operator const char*() const;
	const char* String() const;

	// Is it valid?
	bool IsValid() const { return m_Id != INVALID_STRING_INDEX; }

	// Gets at the symbol
	string_t Get() const { return m_Id; }

	// Gets at the symbol (for debugging)
	unsigned short ToDebugShort() const;

	// Returns a CUtlSymbol for an already-interned string
	static CUtlSymbol Test( const char *pStr );

	// Returns the global string pool
	static CStringPool *StringPool();

private:
	string_t   m_Id;
};


//-----------------------------------------------------------------------------
// CUtlSymbolTable:
// A table of symbols. This is a CUtlVector of CUtlSymbol objects, but it
// also has a CStringPool that is used to store the actual strings.
//-----------------------------------------------------------------------------
class CUtlSymbolTable
{
public:
	// constructor, destructor
	CUtlSymbolTable( int growSize = 0, int initSize = 0, bool caseInsensitive = false );
	~CUtlSymbolTable();

	// Finds a string in the table, adds it if not present
	// Returns the symbol for the string
	CUtlSymbol AddString( const char* pString );

	// Finds a string in the table
	// Returns INVALID_STRING_INDEX if not present
	CUtlSymbol Find( const char* pString ) const;

	// Gets the string associated with a symbol
	const char* String( CUtlSymbol id ) const;

	// Is the symbol valid?
	bool IsValid( CUtlSymbol id ) const;

	// Removes all strings from the table
	void RemoveAll();

	// Returns the number of strings in the table
	int GetNumStrings() const;

	// If this is a case insensitive symbol table, then this can be used to make it case sensitive or vice versa
	void SetCaseInsensitive( bool bCaseInsensitive );

private:
	// The string pool stores all of the actual strings
	CStringPool m_StringPool;

	// This is a list of all of the symbols in the table
	// (This is basically a CUtlVector<string_t>, but we don't want to include utlvector.h here)
	// TODO: Can we just use a CUtlVector<string_t> here?
	struct SymbolTableEntry_t
	{
		string_t	m_Id;
		// We don't need to store the string pointer here, because it's already in the string pool.
		// However, it might be useful for debugging.
		// const char *m_pDebugString;
	};
	CUtlVector<SymbolTableEntry_t> m_Strings;
};


//-----------------------------------------------------------------------------
// CUtlSymbol inline functions
//-----------------------------------------------------------------------------
inline CUtlSymbol::CUtlSymbol( const char* pStr )
{
	m_Id = StringPool()->FindHetKan(pStr);
	if ( m_Id == INVALID_STRING_INDEX )
	{
		m_Id = StringPool()->Add(pStr);
	}
}

inline CUtlSymbol::operator const char*() const
{
	return String();
}

inline const char* CUtlSymbol::String() const
{
	return StringPool()->Get(m_Id);
}

inline unsigned short CUtlSymbol::ToDebugShort() const
{
	return (unsigned short)m_Id;
}

inline CUtlSymbol CUtlSymbol::Test( const char *pStr )
{
	return CUtlSymbol( StringPool()->FindHetKan(pStr) );
}

inline CStringPool *CUtlSymbol::StringPool()
{
	static CStringPool g_StringPool( false ); // false = case sensitive
	return &g_StringPool;
}


//-----------------------------------------------------------------------------
// CUtlSymbolTable inline functions
//-----------------------------------------------------------------------------
inline CUtlSymbolTable::CUtlSymbolTable( int growSize, int initSize, bool caseInsensitive ) :
	m_StringPool( caseInsensitive ),
	m_Strings( growSize, initSize )
{
}

inline CUtlSymbolTable::~CUtlSymbolTable()
{
}

inline CUtlSymbol CUtlSymbolTable::AddString( const char* pString )
{
	// First, see if the string is already in the table
	CUtlSymbol id = Find( pString );
	if ( id.IsValid() )
		return id;

	// If not, add it to the string pool and the table
	string_t newId = m_StringPool.Add( pString );
	SymbolTableEntry_t entry;
	entry.m_Id = newId;
	m_Strings.AddToTail( entry );
	return CUtlSymbol( newId );
}

inline CUtlSymbol CUtlSymbolTable::Find( const char* pString ) const
{
	return CUtlSymbol( m_StringPool.FindHetKan(pString) );
}

inline const char* CUtlSymbolTable::String( CUtlSymbol id ) const
{
	return m_StringPool.Get( id.Get() );
}

inline bool CUtlSymbolTable::IsValid( CUtlSymbol id ) const
{
	return m_StringPool.IsValidIndex( id.Get() );
}

inline void CUtlSymbolTable::RemoveAll()
{
	m_StringPool.FreeAll();
	m_Strings.RemoveAll();
}

inline int CUtlSymbolTable::GetNumStrings() const
{
	return m_Strings.Count();
}

inline void CUtlSymbolTable::SetCaseInsensitive( bool bCaseInsensitive )
{
	m_StringPool.SetCaseInsensitive( bCaseInsensitive );
}


#endif // UTLSYMBOL_H
