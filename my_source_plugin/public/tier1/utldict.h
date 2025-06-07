//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef UTLDICT_H
#define UTLDICT_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier0/basetypes.h"
#include "tier1/utlvector.h"
#include "tier1/utlsymbol.h"


//-----------------------------------------------------------------------------
// Purpose: A dictionary mapping from A to B.
//-----------------------------------------------------------------------------
// If you are using this with a symbol type, then an invalid symbol is -1.
// If you are using this with a pointer type, then an invalid pointer is NULL.
// If you are using this with a CUtlString type, then an invalid string is an empty string.
//-----------------------------------------------------------------------------
template <class T, class I = int>
class CUtlDict
{
public:
	// constructor, destructor
	// pass allocator to use when constructing contained CUtlVectors
	CUtlDict( int growSize = 0, int initSize = 0, bool bCaseInsensitive = false );
	CUtlDict( IAppSystem *pAlloc, int growSize = 0, int initSize = 0, bool bCaseInsensitive = false ); // use this constructor if you want to use a specific memory allocator for the CUtlVectors
	~CUtlDict();

	// gets particular elements
	T&			Element( I i );
	const T&	Element( I i ) const;
	T&			operator[]( I i );
	const T&	operator[]( I i ) const;

	// Gets the number of elements in the dictionary
	unsigned int Count() const;

	// Is element index valid?
	bool IsValidIndex( I i ) const;

	// Returns the global invalid index
	static I InvalidIndex();

	// Returns the elements in the order they were inserted
	const CUtlVector<T>& GetElements() const;

	// Returns the string table for this dictionary
	CUtlSymbolTable& GetStringTable();
	const CUtlSymbolTable& GetStringTable() const;

	// Looks up an element, adds if not found.
	// Returns the index of the element.
	I Insert( const char *pName, const T& element );
	I Insert( const char *pName ); // Element is constructed using default constructor

	// Finds an element and returns its index or InvalidIndex() if not found.
	I Find( const char *pName ) const;

	// Remove an element from the dictionary.
	void RemoveAt( I i );
	void Remove( const char *pName );
	void RemoveAll();

	// Purges the dictionary and calls delete on each element in it.
	void PurgeAndDeleteElements();

	// Iteration methods
	I First() const;
	I Next( I i ) const;

	// Get the name of an element
	const char *GetElementName( I i ) const;

	// If this is a dictionary of CUtlString, then this can be used to make the dictionary case insensitive
	void SetCaseInsensitive( bool bCaseInsensitive );

protected:
	struct DictElement_t
	{
		CUtlSymbol	m_Name;
		T			m_Element;

		DictElement_t() {}
		DictElement_t( CUtlSymbol name, const T &element ) : m_Name( name ), m_Element( element ) {}
	};

	CUtlVector< DictElement_t > m_Elements;
	CUtlSymbolTable				m_StringTable;
	bool						m_bCaseInsensitive;
};


//-----------------------------------------------------------------------------
// CUtlDict inline functions
//-----------------------------------------------------------------------------
template <class T, class I>
inline CUtlDict<T,I>::CUtlDict( int growSize, int initSize, bool bCaseInsensitive ) :
	m_Elements( growSize, initSize ),
	m_StringTable( 0, 256, bCaseInsensitive ), // 256 initial buckets in string table
	m_bCaseInsensitive( bCaseInsensitive )
{
}

template <class T, class I>
inline CUtlDict<T,I>::CUtlDict( IAppSystem *pAlloc, int growSize, int initSize, bool bCaseInsensitive ) :
	m_Elements( pAlloc, growSize, initSize ),
	m_StringTable( 0, 256, bCaseInsensitive ), // 256 initial buckets in string table
	m_bCaseInsensitive( bCaseInsensitive )
{
}

template <class T, class I>
inline CUtlDict<T,I>::~CUtlDict()
{
	Purge();
}

template <class T, class I>
inline T& CUtlDict<T,I>::Element( I i )
{
	return m_Elements[i].m_Element;
}

template <class T, class I>
inline const T& CUtlDict<T,I>::Element( I i ) const
{
	return m_Elements[i].m_Element;
}

template <class T, class I>
inline T& CUtlDict<T,I>::operator[]( I i )
{
	return Element(i);
}

template <class T, class I>
inline const T& CUtlDict<T,I>::operator[]( I i ) const
{
	return Element(i);
}

template <class T, class I>
inline unsigned int CUtlDict<T,I>::Count() const
{
	return m_Elements.Count();
}

template <class T, class I>
inline bool CUtlDict<T,I>::IsValidIndex( I i ) const
{
	return m_Elements.IsValidIndex(i);
}

template <class T, class I>
inline I CUtlDict<T,I>::InvalidIndex()
{
	return CUtlVector<DictElement_t>::InvalidIndex();
}

template <class T, class I>
inline const CUtlVector<T>& CUtlDict<T,I>::GetElements() const
{
	// This is a bit of a hack, but it works because the layout of DictElement_t is the same as T
	// (assuming T doesn't have a CUtlSymbol as its first member).
	// If T does have a CUtlSymbol as its first member, then this will probably crash.
	// TODO: Find a better way to do this.
	return *(reinterpret_cast<const CUtlVector<T>*>(&m_Elements));
}

template <class T, class I>
inline CUtlSymbolTable& CUtlDict<T,I>::GetStringTable()
{
	return m_StringTable;
}

template <class T, class I>
inline const CUtlSymbolTable& CUtlDict<T,I>::GetStringTable() const
{
	return m_StringTable;
}

template <class T, class I>
inline I CUtlDict<T,I>::Insert( const char *pName, const T& element )
{
	// If the name is already in the dictionary, just update the element
	I idx = Find( pName );
	if ( idx != InvalidIndex() )
	{
		m_Elements[idx].m_Element = element;
		return idx;
	}

	// Otherwise, add a new element
	CUtlSymbol sym = m_StringTable.AddString( pName );
	return m_Elements.AddToTail( DictElement_t( sym, element ) );
}

template <class T, class I>
inline I CUtlDict<T,I>::Insert( const char *pName )
{
	// If the name is already in the dictionary, just return its index
	I idx = Find( pName );
	if ( idx != InvalidIndex() )
		return idx;

	// Otherwise, add a new element (constructed using default constructor)
	CUtlSymbol sym = m_StringTable.AddString( pName );
	return m_Elements.AddToTail( DictElement_t( sym, T() ) );
}

template <class T, class I>
inline I CUtlDict<T,I>::Find( const char *pName ) const
{
	if ( !pName )
		return InvalidIndex();

	CUtlSymbol sym = m_StringTable.Find( pName );
	if ( sym.IsValid() )
	{
		int c = m_Elements.Count();
		for ( int i = 0; i < c; ++i )
		{
			if ( m_Elements[i].m_Name == sym )
				return i;
		}
	}

	return InvalidIndex();
}

template <class T, class I>
inline void CUtlDict<T,I>::RemoveAt( I i )
{
	Assert( IsValidIndex(i) );
	m_Elements.FastRemove(i);
}

template <class T, class I>
inline void CUtlDict<T,I>::Remove( const char *pName )
{
	I idx = Find( pName );
	if ( idx != InvalidIndex() )
	{
		RemoveAt(idx);
	}
}

template <class T, class I>
inline void CUtlDict<T,I>::RemoveAll()
{
	m_Elements.RemoveAll();
}

template <class T, class I>
inline void CUtlDict<T,I>::PurgeAndDeleteElements()
{
	for( int i=0; i < m_Elements.Count(); i++ )
	{
		delete Element(i);
	}
	Purge();
}

template <class T, class I>
inline I CUtlDict<T,I>::First() const
{
	if ( Count() > 0 )
		return 0;
	return InvalidIndex();
}

template <class T, class I>
inline I CUtlDict<T,I>::Next( I i ) const
{
	if ( !IsValidIndex(i) )
		return InvalidIndex();
	++i;
	if ( i < Count() )
		return i;
	return InvalidIndex();
}

template <class T, class I>
inline const char *CUtlDict<T,I>::GetElementName( I i ) const
{
	Assert( IsValidIndex(i) );
	return m_StringTable.String( m_Elements[i].m_Name );
}

template <class T, class I>
inline void CUtlDict<T,I>::SetCaseInsensitive( bool bCaseInsensitive )
{
	m_bCaseInsensitive = bCaseInsensitive;
	m_StringTable.SetCaseInsensitive( bCaseInsensitive );
}


#endif // UTLDICT_H
