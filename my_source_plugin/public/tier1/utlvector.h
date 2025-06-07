//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef UTLVECTOR_H
#define UTLVECTOR_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier0/basetypes.h"
#include "tier1/utlmemory.h"


//-----------------------------------------------------------------------------
// The CUtlVector class:
// A growable array class which doubles in size by default.
// It will always keep all elements consecutive in memory, and may move the
// elements around in memory (via a realloc) when elements are added or removed.
//-----------------------------------------------------------------------------
template< class T >
class CUtlVector
{
public:
	typedef T ElemType_t;
	typedef T* iterator;
	typedef const T* const_iterator;

	// constructor, destructor
	CUtlVector( int growSize = 0, int initSize = 0 );
	CUtlVector( T* pMemory, int allocationCount, int numElements = 0 );
	~CUtlVector();

	// copy constructor
	CUtlVector( const CUtlVector<T> &other );
	CUtlVector<T>& operator=( const CUtlVector<T> &other );

	// element access
	T& operator[]( int i );
	const T& operator[]( int i ) const;
	T& Element( int i );
	const T& Element( int i ) const;
	T& Head();
	const T& Head() const;
	T& Tail();
	const T& Tail() const;

	// Gets the base address (can change when adding elements!)
	T* Base()								{ return m_Memory.Base(); }
	const T* Base() const					{ return m_Memory.Base(); }
	// Returns the number of elements in the vector
	int Count() const;
	// Is element index valid?
	bool IsValidIndex( int i ) const;
	static int InvalidIndex();

	// Adds an element, uses default constructor
	int AddToHead();
	int AddToTail();
	int InsertBefore( int elem );
	int InsertAfter( int elem );

	// Adds an element, uses copy constructor
	int AddToHead( const T& src );
	int AddToTail( const T& src );
	int InsertBefore( int elem, const T& src );
	int InsertAfter( int elem, const T& src );

	// Adds multiple elements, uses default constructor
	int AddMultipleToHead( int num );
	int AddMultipleToTail( int num );
	int InsertMultipleBefore( int elem, int num );
	int InsertMultipleAfter( int elem, int num );

	// Adds multiple elements, uses copy constructor
	int AddMultipleToHead( int num, const T *pToCopy );
	int AddMultipleToTail( int num, const T *pToCopy );
	int InsertMultipleBefore( int elem, int num, const T *pToCopy );
	int InsertMultipleAfter( int elem, int num, const T *pToCopy );

	// Calls RemoveAll() then AddMultipleToTail.
	void CopyFromArray( const T *pArray, int arraySize );

	// Fast swap
	void Swap( CUtlVector< T > &vec );

	// Add the specified array to the tail.
	int AddVectorToTail( CUtlVector<T> const &src );

	// Finds an element ( XFINDSIMPLE() is used typicaly )
	int Find( const T& src ) const;

	bool HasElement( const T& src ) const;

	// Makes sure we have enough memory allocated to store a requested # of elements
	void EnsureCapacity( int num );

	// Makes sure we have at least this many elements
	void EnsureCount( int num );

	// Element removal
	void FastRemove( int elem );	// doesn't preserve order
	void Remove( int elem );		// preserves order
	void RemoveMultiple( int elem, int num );	// preserves order
	void RemoveAll();				// doesn't preserve order (fast)

	// Memory deallocation
	void Purge();

	// Purges the list and calls delete on each element in it.
	void PurgeAndDeleteElements();

	// Compacts the vector to the number of elements actually in use
	void Compact();

	// Set the size by which the vector grows
	void SetGrowSize( int size )			{ m_Memory.SetGrowSize( size ); }
	int GetGrowSize() const					{ return m_Memory.GetGrowSize(); }

	// Returns the number of elements allocated for growth
	int NumAllocated() const;

	// Sorting
	typedef int (__cdecl *SortFunc_t)(const T *, const T *);
	void Sort( SortFunc_t sortFunc );

	// allow iteration
	iterator begin()						{ return Base(); }
	const_iterator begin() const			{ return Base(); }
	iterator end()							{ return Base() + Count(); }
	const_iterator end() const				{ return Base() + Count(); }

	// Calls constructor for all elements
	void CallConstructors( T *pElements, int nCount );

	// Calls destructor for all elements
	void CallDestructors( T *pElements, int nCount );

protected:
	// Can't copy this unless we explicitly do it!
	CUtlMemory<T> m_Memory;
	int m_Size;

	// For easier access to the elements
	// it's really an array or an array of pointers
	T* m_pElements;

	void GrowVector( int num = 1 );

	// Shifts elements....
	void ShiftElementsRight( int elem, int num = 1 );
	void ShiftElementsLeft( int elem, int num = 1 );
};


//-----------------------------------------------------------------------------
// CUtlVector inline functions
//-----------------------------------------------------------------------------
template< typename T >
inline CUtlVector<T>::CUtlVector( int growSize, int initSize ) :
	m_Memory( growSize, initSize ), m_Size( 0 )
{
	m_pElements = m_Memory.Base();
}

template< typename T >
inline CUtlVector<T>::CUtlVector( T* pMemory, int allocationCount, int numElements ) :
	m_Memory( pMemory, allocationCount ), m_Size( numElements )
{
	m_pElements = m_Memory.Base();
}

template< typename T >
inline CUtlVector<T>::~CUtlVector()
{
	Purge();
}

template< typename T >
inline CUtlVector<T>::CUtlVector( const CUtlVector<T> &other )
{
	m_Memory.SetGrowSize( other.m_Memory.GetGrowSize() );
	CopyFromArray( other.Base(), other.Count() );
	m_pElements = m_Memory.Base();
}

template< typename T >
inline CUtlVector<T>& CUtlVector<T>::operator=( const CUtlVector<T> &other )
{
	if ( this == &other )
		return *this;

	m_Memory.SetGrowSize( other.m_Memory.GetGrowSize() );
	CopyFromArray( other.Base(), other.Count() );
	m_pElements = m_Memory.Base();
	return *this;
}

template< typename T >
inline T& CUtlVector<T>::operator[]( int i )
{
	Assert( IsValidIndex(i) );
	return Base()[i];
}

template< typename T >
inline const T& CUtlVector<T>::operator[]( int i ) const
{
	Assert( IsValidIndex(i) );
	return Base()[i];
}

template< typename T >
inline T& CUtlVector<T>::Element( int i )
{
	Assert( IsValidIndex(i) );
	return Base()[i];
}

template< typename T >
inline const T& CUtlVector<T>::Element( int i ) const
{
	Assert( IsValidIndex(i) );
	return Base()[i];
}

template< typename T >
inline T& CUtlVector<T>::Head()
{
	Assert( m_Size > 0 );
	return Base()[0];
}

template< typename T >
inline const T& CUtlVector<T>::Head() const
{
	Assert( m_Size > 0 );
	return Base()[0];
}

template< typename T >
inline T& CUtlVector<T>::Tail()
{
	Assert( m_Size > 0 );
	return Base()[m_Size - 1];
}

template< typename T >
inline const T& CUtlVector<T>::Tail() const
{
	Assert( m_Size > 0 );
	return Base()[m_Size - 1];
}

template< typename T >
inline int CUtlVector<T>::Count() const
{
	return m_Size;
}

template< typename T >
inline bool CUtlVector<T>::IsValidIndex( int i ) const
{
	return (i >= 0) && (i < m_Size);
}

template< typename T >
inline int CUtlVector<T>::InvalidIndex()
{
	return -1;
}

template< typename T >
inline int CUtlVector<T>::AddToHead()
{
	return InsertBefore(0);
}

template< typename T >
inline int CUtlVector<T>::AddToTail()
{
	return InsertBefore(m_Size);
}

template< typename T >
inline int CUtlVector<T>::InsertBefore( int elem )
{
	// Can insert at the end
	Assert( (elem == m_Size) || IsValidIndex(elem) );

	GrowVector();
	ShiftElementsRight(elem);
	Construct( &Element(elem) );
	return elem;
}

template< typename T >
inline int CUtlVector<T>::InsertAfter( int elem )
{
	Assert( IsValidIndex(elem) );
	return InsertBefore(elem + 1);
}

template< typename T >
inline int CUtlVector<T>::AddToHead( const T& src )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || (&src < Base()) || (&src >= (Base() + NumAllocated())) );
	return InsertBefore( 0, src );
}

template< typename T >
inline int CUtlVector<T>::AddToTail( const T& src )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || (&src < Base()) || (&src >= (Base() + NumAllocated())) );
	return InsertBefore( m_Size, src );
}

template< typename T >
inline int CUtlVector<T>::InsertBefore( int elem, const T& src )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || (&src < Base()) || (&src >= (Base() + NumAllocated())) );

	// Can insert at the end
	Assert( (elem == m_Size) || IsValidIndex(elem) );

	GrowVector();
	ShiftElementsRight(elem);
	CopyConstruct( &Element(elem), src );
	return elem;
}

template< typename T >
inline int CUtlVector<T>::InsertAfter( int elem, const T& src )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || (&src < Base()) || (&src >= (Base() + NumAllocated())) );
	Assert( IsValidIndex(elem) );
	return InsertBefore( elem + 1, src );
}

template< typename T >
inline int CUtlVector<T>::AddMultipleToHead( int num )
{
	return InsertMultipleBefore( 0, num );
}

template< typename T >
inline int CUtlVector<T>::AddMultipleToTail( int num )
{
	return InsertMultipleBefore( m_Size, num );
}

template< typename T >
inline int CUtlVector<T>::InsertMultipleBefore( int elem, int num )
{
	if( num == 0 )
		return elem;

	// Can insert at the end
	Assert( (elem == m_Size) || IsValidIndex(elem) );

	GrowVector(num);
	ShiftElementsRight(elem, num);

	// Invoke default constructors
	for ( int i = 0; i < num; ++i )
		Construct( &Element(elem+i) );

	return elem;
}

template< typename T >
inline int CUtlVector<T>::InsertMultipleAfter( int elem, int num )
{
	Assert( IsValidIndex(elem) );
	return InsertMultipleBefore( elem + 1, num );
}

template< typename T >
inline int CUtlVector<T>::AddMultipleToHead( int num, const T *pToCopy )
{
	return InsertMultipleBefore( 0, num, pToCopy );
}

template< typename T >
inline int CUtlVector<T>::AddMultipleToTail( int num, const T *pToCopy )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || !pToCopy || (pToCopy + num <= Base()) || (pToCopy >= (Base() + NumAllocated())) );
	return InsertMultipleBefore( m_Size, num, pToCopy );
}

template< typename T >
inline int CUtlVector<T>::InsertMultipleBefore( int elem, int num, const T *pToCopy )
{
	if( num == 0 )
		return elem;

	// Can insert at the end
	Assert( (elem == m_Size) || IsValidIndex(elem) );

	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || !pToCopy || (pToCopy + num <= Base()) || (pToCopy >= (Base() + NumAllocated())) );

	GrowVector(num);
	ShiftElementsRight(elem, num);

	// Invoke default constructors
	if ( !pToCopy )
	{
		for ( int i = 0; i < num; ++i )
			Construct( &Element(elem+i) );
	}
	else
	{
		for ( int i = 0; i < num; ++i )
			CopyConstruct( &Element(elem+i), pToCopy[i] );
	}
	return elem;
}

template< typename T >
inline int CUtlVector<T>::InsertMultipleAfter( int elem, int num, const T *pToCopy )
{
	Assert( IsValidIndex(elem) );
	return InsertMultipleBefore( elem + 1, num, pToCopy );
}

template< typename T >
void CUtlVector<T>::CopyFromArray( const T *pArray, int arraySize )
{
	// Can't copy something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || !pArray || (pArray + arraySize <= Base()) || (pArray >= (Base() + NumAllocated())) );

	RemoveAll();
	EnsureCapacity( arraySize );
	for( int i = 0; i < arraySize; i++ )
	{
		CopyConstruct( &Element(i), pArray[i] );
	}
	m_Size = arraySize;
}

template< typename T >
void CUtlVector<T>::Swap( CUtlVector< T > &vec )
{
	m_Memory.Swap( vec.m_Memory );
	V_swap( m_Size, vec.m_Size );
	V_swap( m_pElements, vec.m_pElements );
}

template< typename T >
int CUtlVector<T>::AddVectorToTail( CUtlVector<T> const &src )
{
	Assert( &src != this );

	int base = Count();

	// Make space.
	int nElems = src.Count();
	EnsureCapacity( base + nElems );

	// Copy the elements.
	for ( int i = 0; i < nElems; i++ )
	{
		CopyConstruct( &Element( base + i ), src[i] );
	}
	m_Size += nElems;
	return base;
}

template< typename T >
inline int CUtlVector<T>::Find( const T& src ) const
{
	for ( int i = 0; i < Count(); ++i )
	{
		if ( Element(i) == src )
			return i;
	}
	return -1;
}

template< typename T >
inline bool CUtlVector<T>::HasElement( const T& src ) const
{
	return ( Find(src) >= 0 );
}

template< typename T >
inline void CUtlVector<T>::EnsureCapacity( int num )
{
	m_Memory.EnsureCapacity(num);
	m_pElements = m_Memory.Base();
}

template< typename T >
inline void CUtlVector<T>::EnsureCount( int num )
{
	if (Count() < num)
	{
		AddMultipleToTail( num - Count() );
	}
}

template< typename T >
inline void CUtlVector<T>::FastRemove( int elem )
{
	Assert( IsValidIndex(elem) );

	Destruct( &Element(elem) );
	if (m_Size > 0)
	{
		if ( elem != m_Size -1 )
			memcpy( &Element(elem), &Element(m_Size-1), sizeof(T) );
		--m_Size;
	}
}

template< typename T >
inline void CUtlVector<T>::Remove( int elem )
{
	Destruct( &Element(elem) );
	ShiftElementsLeft(elem);
	--m_Size;
}

template< typename T >
inline void CUtlVector<T>::RemoveMultiple( int elem, int num )
{
	Assert( elem >= 0 );
	Assert( elem + num <= Count() );

	for (int i = elem + num; --i >= elem; )
		Destruct(&Element(i));

	ShiftElementsLeft(elem, num);
	m_Size -= num;
}

template< typename T >
inline void CUtlVector<T>::RemoveAll()
{
	for (int i = m_Size; --i >= 0; )
	{
		Destruct(&Element(i));
	}
	m_Size = 0;
}

template< typename T >
inline void CUtlVector<T>::Purge()
{
	RemoveAll();
	m_Memory.Purge();
	m_pElements = m_Memory.Base();
}

template< typename T >
inline void CUtlVector<T>::PurgeAndDeleteElements()
{
	for( int i=0; i < m_Size; i++ )
	{
		delete Element(i);
	}
	Purge();
}

template< typename T >
inline void CUtlVector<T>::Compact()
{
	m_Memory.Purge( m_Size );
}

template< typename T >
inline int CUtlVector<T>::NumAllocated() const
{
	return m_Memory.NumAllocated();
}

template< typename T >
void CUtlVector<T>::Sort( SortFunc_t sortFunc )
{
	qsort( Base(), Count(), sizeof(T), (int (__cdecl *)(const void *, const void *))sortFunc );
}

template< typename T >
void CUtlVector<T>::CallConstructors( T *pElements, int nCount )
{
	for( int i = 0; i < nCount; ++i, ++pElements )
	{
		Construct( pElements );
	}
}

template< typename T >
void CUtlVector<T>::CallDestructors( T *pElements, int nCount )
{
	for( int i = 0; i < nCount; ++i, ++pElements )
	{
		Destruct( pElements );
	}
}

template< typename T >
void CUtlVector<T>::GrowVector( int num )
{
	if (m_Size + num > NumAllocated())
	{
		m_Memory.Grow( m_Size + num - NumAllocated() );
	}

	m_Size += num;
	m_pElements = m_Memory.Base();
}

template< typename T >
void CUtlVector<T>::ShiftElementsRight( int elem, int num )
{
	Assert( IsValidIndex(elem) || (elem == m_Size) );
	int numToMove = m_Size - elem - num;
	if ((numToMove > 0) && (num > 0))
		memmove( &Element(elem+num), &Element(elem), numToMove * sizeof(T) );
}

template< typename T >
void CUtlVector<T>::ShiftElementsLeft( int elem, int num )
{
	Assert( IsValidIndex(elem) || (elem == m_Size) );
	int numToMove = m_Size - elem - num;
	if ((numToMove > 0) && (num > 0))
	{
		memmove( &Element(elem), &Element(elem+num), numToMove * sizeof(T) );

#ifdef _DEBUG
		memset( &Element(m_Size-num), 0xDD, num * sizeof(T) );
#endif
	}
}


#endif // UTLVECTOR_H
