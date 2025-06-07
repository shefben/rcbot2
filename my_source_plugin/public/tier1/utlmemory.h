//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef UTLMEMORY_H
#define UTLMEMORY_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier0/basetypes.h" // For CUtlMemory, CUtlVector, etc.
#include "tier0/memalloc.h" // For IMemAlloc


//-----------------------------------------------------------------------------
// The CUtlMemory class:
// A growable memory class.
//-----------------------------------------------------------------------------
template< class T, class I = int >
class CUtlMemory
{
public:
	// constructor, destructor
	CUtlMemory( int nGrowSize = 0, int nInitSize = 0 );
	CUtlMemory( T* pMemory, int numElements );
	CUtlMemory( const T* pMemory, int numElements );
	~CUtlMemory();

	// Set the size by which the memory grows
	void Init( int nGrowSize = 0, int nInitSize = 0 );

	class Iterator_t
	{
	public:
		Iterator_t( I i ) : index( i ) {}
		I index;

		bool operator==( const Iterator_t it ) const	{ return index == it.index; }
		bool operator!=( const Iterator_t it ) const	{ return index != it.index; }
	};
	Iterator_t First() const						{ return Iterator_t( IsIdxValid(0) ? 0 : InvalidIndex() ); }
	Iterator_t Next( const Iterator_t &it ) const	{ return Iterator_t( IsIdxValid(it.index + 1) ? it.index + 1 : InvalidIndex() ); }
	I GetIndex( const Iterator_t &it ) const		{ return it.index; }
	bool IsIdxAfter( I i, const Iterator_t &it ) const { return i > it.index; }
	bool IsValidIterator( const Iterator_t &it ) const { return IsIdxValid( it.index ); }
	Iterator_t InvalidIterator() const				{ return Iterator_t( InvalidIndex() ); }

	// element access
	T& operator[]( I i );
	const T& operator[]( I i ) const;
	T& Element( I i );
	const T& Element( I i ) const;

	// Can we use this memory?
	bool IsValid() const;

	// Gets the base address (can change when adding elements!)
	T* Base();
	const T* Base() const;

	// Attaches the buffer to external memory....
	void SetExternalBuffer( T* pMemory, int numElements );
	void SetExternalBuffer( const T* pMemory, int numElements );
	// Takes ownership of the passed memory, including freeing it when this instance is deleted.
	void AssumeMemory( T *pMemory, int nSize, int nAllocCount );

	// Fast swap
	void Swap( CUtlMemory< T, I > &mem );

	// Switches the buffer from an external buffer to an internal one
	void ConvertToGrowableMemory( int nGrowSize );

	// Size
	int NumAllocated() const;
	int Count() const; // Number of elements currently in use

	// Grows the memory, if necessary
	void Grow( int num = 1 );

	// Makes sure we've got at least this much memory
	void EnsureCapacity( int num );

	// Memory deallocation
	void Purge();

	// Purge all but the given number of elements
	void Purge( int numElements );

	// is the memory externally allocated?
	bool IsExternallyAllocated() const;

	// Set the size by which the memory grows
	void SetGrowSize( int size );

	// Allocates a new block of memory, copies the old data, and frees the old memory
	bool Alloc( int nSize );

	// Invalid index
	static I InvalidIndex() { return (I)-1; }

	// Is an index valid?
	bool IsIdxValid( I i ) const;

protected:
	enum MemoryFlags_t
	{
		EXTERNAL_BUFFER_MARKER = (1 << 0),
		EXTERNAL_CONST_BUFFER_MARKER = (1 << 1),
	};

	// Grows the memory
	void GrowSelf( int num );

	// For easier access to the elements
	T* m_pMemory;

	// Number of elements allocated
	int m_nAllocationCount;

	// Number of elements currently in use
	// (This is actually m_nGrowSize in CUtlMemory from basetypes.h,
	//  but it's used as the count of elements in use here, similar to CUtlVector's m_Size)
	int m_nElements;

	// How much to grow by?
	int m_nGrowSize;

	// Memory allocation flags
	uint8 m_nMemoryFlags;

	// The allocator to use.
	IMemAlloc *m_pAllocator;

	void SetAllocator( IMemAlloc *pAllocator );
};


//-----------------------------------------------------------------------------
// CUtlMemory inline functions
//-----------------------------------------------------------------------------
template< class T, class I >
inline CUtlMemory<T,I>::CUtlMemory( int nGrowSize, int nInitAllocationCount ) :
	m_pMemory(0),
	m_nAllocationCount( nInitAllocationCount ),
	m_nElements(0),
	m_nGrowSize( nGrowSize ),
	m_nMemoryFlags(0),
	m_pAllocator( g_pMemAlloc )
{
	if (m_nAllocationCount)
	{
		MEM_ALLOC_CREDIT();
		m_pMemory = (T*)m_pAllocator->Alloc( m_nAllocationCount * sizeof(T) );
	}
}

template< class T, class I >
inline CUtlMemory<T,I>::CUtlMemory( T* pMemory, int numElements ) :
	m_pMemory(pMemory),
	m_nAllocationCount( numElements ),
	m_nElements(numElements), // Assume all elements are in use
	m_nGrowSize(0),
	m_nMemoryFlags(EXTERNAL_BUFFER_MARKER),
	m_pAllocator( g_pMemAlloc )
{
}

template< class T, class I >
inline CUtlMemory<T,I>::CUtlMemory( const T* pMemory, int numElements ) :
	m_pMemory((T*)pMemory), // const cast is safe here, we won't modify it if EXTERNAL_CONST_BUFFER_MARKER is set
	m_nAllocationCount( numElements ),
	m_nElements(numElements), // Assume all elements are in use
	m_nGrowSize(0),
	m_nMemoryFlags(EXTERNAL_BUFFER_MARKER | EXTERNAL_CONST_BUFFER_MARKER),
	m_pAllocator( g_pMemAlloc )
{
}

template< class T, class I >
inline CUtlMemory<T,I>::~CUtlMemory()
{
	Purge();
}

template< class T, class I >
inline void CUtlMemory<T,I>::Init( int nGrowSize /*= 0*/, int nInitSize /*= 0*/ )
{
	Purge();

	m_nGrowSize = nGrowSize;
	m_nAllocationCount = nInitSize;
	m_nElements = 0;
	m_nMemoryFlags = 0;
	if (m_nAllocationCount)
	{
		MEM_ALLOC_CREDIT();
		m_pMemory = (T*)m_pAllocator->Alloc( m_nAllocationCount * sizeof(T) );
	}
	else
	{
		m_pMemory = 0;
	}
}

template< class T, class I >
inline T& CUtlMemory<T,I>::operator[]( I i )
{
	Assert( !IsExternallyAllocated() || !(m_nMemoryFlags & EXTERNAL_CONST_BUFFER_MARKER) ); // Can't write to const buffer
	Assert( IsIdxValid(i) );
	return m_pMemory[i];
}

template< class T, class I >
inline const T& CUtlMemory<T,I>::operator[]( I i ) const
{
	Assert( IsIdxValid(i) );
	return m_pMemory[i];
}

template< class T, class I >
inline T& CUtlMemory<T,I>::Element( I i )
{
	Assert( !IsExternallyAllocated() || !(m_nMemoryFlags & EXTERNAL_CONST_BUFFER_MARKER) ); // Can't write to const buffer
	Assert( IsIdxValid(i) );
	return m_pMemory[i];
}

template< class T, class I >
inline const T& CUtlMemory<T,I>::Element( I i ) const
{
	Assert( IsIdxValid(i) );
	return m_pMemory[i];
}

template< class T, class I >
inline bool CUtlMemory<T,I>::IsValid() const
{
	return m_pMemory != NULL;
}

template< class T, class I >
inline T* CUtlMemory<T,I>::Base()
{
	Assert( !IsExternallyAllocated() || !(m_nMemoryFlags & EXTERNAL_CONST_BUFFER_MARKER) ); // Can't write to const buffer
	return m_pMemory;
}

template< class T, class I >
inline const T* CUtlMemory<T,I>::Base() const
{
	return m_pMemory;
}

template< class T, class I >
inline void CUtlMemory<T,I>::SetExternalBuffer( T* pMemory, int numElements )
{
	Purge();

	m_pMemory = pMemory;
	m_nAllocationCount = numElements;
	m_nElements = numElements; // Assume all elements are in use
	m_nMemoryFlags = EXTERNAL_BUFFER_MARKER;
}

template< class T, class I >
inline void CUtlMemory<T,I>::SetExternalBuffer( const T* pMemory, int numElements )
{
	Purge();

	m_pMemory = (T*)pMemory; // const cast is safe here, we won't modify it if EXTERNAL_CONST_BUFFER_MARKER is set
	m_nAllocationCount = numElements;
	m_nElements = numElements; // Assume all elements are in use
	m_nMemoryFlags = EXTERNAL_BUFFER_MARKER | EXTERNAL_CONST_BUFFER_MARKER;
}

template< class T, class I >
inline void CUtlMemory<T,I>::AssumeMemory( T *pMemory, int numElements, int nAllocCount )
{
	Purge();
	m_pMemory = pMemory;
	m_nElements = numElements;
	m_nAllocationCount = nAllocCount;
	m_nMemoryFlags = 0; // Not external, we own this memory
}


template< class T, class I >
void CUtlMemory<T,I>::Swap( CUtlMemory<T,I> &mem )
{
	V_swap( m_nGrowSize, mem.m_nGrowSize );
	V_swap( m_pMemory, mem.m_pMemory );
	V_swap( m_nAllocationCount, mem.m_nAllocationCount );
	V_swap( m_nElements, mem.m_nElements );
	V_swap( m_nMemoryFlags, mem.m_nMemoryFlags );
	V_swap( m_pAllocator, mem.m_pAllocator );
}

template< class T, class I >
void CUtlMemory<T,I>::ConvertToGrowableMemory( int nGrowSize )
{
	if ( !IsExternallyAllocated() )
		return;

	m_nGrowSize = nGrowSize;
	if (m_nAllocationCount)
	{
		MEM_ALLOC_CREDIT();
		T *pNewMemory = (T*)m_pAllocator->Alloc( m_nAllocationCount * sizeof(T) );
		memcpy( pNewMemory, m_pMemory, m_nAllocationCount * sizeof(T) );
		m_pMemory = pNewMemory;
	}
	else
	{
		m_pMemory = NULL;
	}
	m_nMemoryFlags = 0;
}

template< class T, class I >
inline int CUtlMemory<T,I>::NumAllocated() const
{
	return m_nAllocationCount;
}

template< class T, class I >
inline int CUtlMemory<T,I>::Count() const
{
	return m_nElements;
}

template< class T, class I >
inline void CUtlMemory<T,I>::Grow( int num )
{
	Assert( num > 0 );

	if ( IsExternallyAllocated() )
	{
		Assert(0); // Can't grow an externally allocated buffer
		return;
	}

	// Make sure we have at least num elements allocated
	EnsureCapacity( m_nElements + num );
	m_nElements += num; // Increase the count of elements in use
}

template< class T, class I >
inline void CUtlMemory<T,I>::EnsureCapacity( int num )
{
	if ( m_nAllocationCount >= num )
		return;

	if ( IsExternallyAllocated() )
	{
		// Can't grow an externally allocated buffer
		Assert(0);
		return;
	}

	MEM_ALLOC_CREDIT();
	GrowSelf( num - m_nAllocationCount );
}

template< class T, class I >
inline void CUtlMemory<T,I>::Purge()
{
	if ( !IsExternallyAllocated() )
	{
		if (m_pMemory)
		{
			m_pAllocator->Free( (void*)m_pMemory );
			m_pMemory = 0;
		}
	}
	m_nAllocationCount = 0;
	m_nElements = 0;
	m_nMemoryFlags = 0; // Reset flags
}

template< class T, class I >
inline void CUtlMemory<T,I>::Purge( int numElements )
{
	Assert( numElements >= 0 );

	if( numElements > m_nAllocationCount )
	{
		// Ensure this isn't a grow request in disguise.
		Assert( numElements <= m_nAllocationCount );
		return;
	}

	// If we have zero elements, just Nuke the whole thing.
	if( numElements == 0 )
	{
		Purge();
		return;
	}

	if ( IsExternallyAllocated() )
	{
		// Can't shrink an externally allocated buffer.
		// This is a no-op.
		m_nElements = MIN( m_nElements, numElements ); // Update element count if shrinking
		return;
	}

	if ( numElements == m_nAllocationCount )
	{
		m_nElements = MIN( m_nElements, numElements ); // Update element count if shrinking
		return;
	}


	if ( !m_pMemory )
	{
		// Allocation count is ALREADY 0
		return;
	}

	MEM_ALLOC_CREDIT();
	T* pNewMem = (T*)m_pAllocator->Alloc( numElements * sizeof(T) );
	// Copy the data if we're shrinking
	if ( numElements < m_nElements )
	{
		memcpy( pNewMem, m_pMemory, numElements * sizeof(T) );
	}
	else if ( m_nElements <= numElements ) // If we are "purging" to a larger size or same, copy all existing elements
	{
		memcpy( pNewMem, m_pMemory, m_nElements * sizeof(T) );
	}


	m_pAllocator->Free( (void*)m_pMemory );
	m_pMemory = pNewMem;
	m_nAllocationCount = numElements;
	m_nElements = MIN( m_nElements, numElements ); // Update element count
}

template< class T, class I >
inline bool CUtlMemory<T,I>::IsExternallyAllocated() const
{
	return (m_nMemoryFlags & EXTERNAL_BUFFER_MARKER) != 0;
}

template< class T, class I >
inline void CUtlMemory<T,I>::SetGrowSize( int size )
{
	m_nGrowSize = size;
}

template< class T, class I >
inline bool CUtlMemory<T,I>::Alloc( int nSize )
{
	Purge();
	m_nGrowSize = 0; // Don't auto-grow if we're allocating a specific size
	EnsureCapacity( nSize );
	m_nElements = nSize; // All allocated elements are considered in use
	return (m_pMemory != NULL);
}

template< class T, class I >
inline bool CUtlMemory<T,I>::IsIdxValid( I i ) const
{
	// GCC warns if I is unsigned type and we do i < 0.
	// Then it warns that the rams < 0 is bogus. Chronium only
	//  uses signed types for I, so I'm not sure what to do here.
	long long val = i;
	return (val >= 0) && (val < m_nElements); // Check against elements in use, not allocated count
}

template< class T, class I >
void CUtlMemory<T,I>::GrowSelf( int num )
{
	Assert( !IsExternallyAllocated() );

	if ( m_pMemory == NULL && m_nAllocationCount == 0 ) // First allocation
	{
		// If m_nGrowSize is set, then calculate the initial allocation size
		// from m_nGrowSize. Otherwise, just allocate what's requested.
		if ( m_nGrowSize )
		{
			m_nAllocationCount = MAX( m_nGrowSize, num );
		}
		else
		{
			m_nAllocationCount = num;
		}
	}
	else
	{
		// If m_nGrowSize is set, then double the memory. Otherwise, just allocate what's requested.
		if ( m_nGrowSize )
		{
			m_nAllocationCount += MAX( m_nGrowSize, num );
		}
		else
		{
			m_nAllocationCount += num;
		}
	}

	if ( m_pMemory )
	{
		m_pMemory = (T*)m_pAllocator->Realloc( m_pMemory, m_nAllocationCount * sizeof(T) );
	}
	else
	{
		MEM_ALLOC_CREDIT();
		m_pMemory = (T*)m_pAllocator->Alloc( m_nAllocationCount * sizeof(T) );
	}
}

template< class T, class I >
void CUtlMemory<T,I>::SetAllocator( IMemAlloc *pAllocator )
{
	Assert( pAllocator );
	// Cannot change allocator if memory is already allocated
	Assert( m_nAllocationCount == 0 && m_pMemory == NULL );
	m_pAllocator = pAllocator;
}


#endif // UTLMEMORY_H
