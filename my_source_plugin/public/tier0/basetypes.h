//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef BASETYPES_H
#define BASETYPES_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/valve_off.h"
#ifdef _LINUX
#include <ctype.h> // isspace()
#include <sys/types.h> // size_t
#endif
#include <cmath> // For fabsf
#include "tier0/valve_on.h"


//-----------------------------------------------------------------------------
// Basic fixed-width types
//-----------------------------------------------------------------------------
typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned long long uint64;
typedef signed long long int64;


//-----------------------------------------------------------------------------
// We also need a boolean type that is the same size on all platforms
//-----------------------------------------------------------------------------
typedef int8 bool_t;
#ifndef __cplusplus
	#ifndef bool
		#define bool bool_t
	#endif
	#ifndef false
		#define false 0
	#endif
	#ifndef true
		#define true 1
	#endif
#endif


//-----------------------------------------------------------------------------
// Define a type that can be used to store a pointer or an integer
//-----------------------------------------------------------------------------
#if defined( _WIN64 ) || defined( __x86_64__ ) || defined( _LP64 )
	typedef unsigned long long uintp; // unsigned integer that can hold a pointer
	typedef signed long long intp; // signed integer that can hold a pointer
#else
	typedef unsigned int uintp; // unsigned integer that can hold a pointer
	typedef signed int intp; // signed integer that can hold a pointer
#endif


//-----------------------------------------------------------------------------
// Other type defines
//-----------------------------------------------------------------------------
#ifndef NULL
	#define NULL 0
#endif

#ifndef MAX_PATH
	#define MAX_PATH 260
#endif

// string_t definition, commonly an int or unsigned int
typedef int string_t;
#define INVALID_STRING_INDEX -1


// Placeholder for edict_t (actual definition is in edict.h in the SDK)
// struct edict_s {}; // Already forward declared, providing minimal definition here for now
// No, edict_t is usually a pointer to an incomplete type until edict.h is included.
// For now, the forward declaration of edict_s and typedef edict_t is enough.

// Placeholder for CCommand (actual definition is more complex, usually in const.h or similar)
// class CCommand // Already forward declared, providing minimal definition here
// {
// public:
	// These are just examples, the real CCommand is more complex
//	int ArgC() const { return 0; }
//	const char *ArgS() const { return ""; } // Get all args as a single string
//	const char *operator[]( int nIndex ) const { return ""; } // Get specific arg
//	const char *Arg( int nIndex ) const { return ""; } // Get specific arg
// };
// Minimal definition for CCommand as it's used as a parameter type
class CCommand
{
public:
	int ArgC() const { return 0; }
	const char **ArgV() const { return nullptr; }
	const char *ArgS() const { return ""; }
	const char *GetCommandString() const { return ""; }
	const char *operator[]( int nIndex ) const { return ""; }
	const char *Arg( int nIndex ) const { return ""; }
};


// Placeholder for netadr_t (actual definition is in netadr.h in the SDK)
typedef struct netadr_s
{
	unsigned char	ip[4];
	unsigned short	port;
	// char			name[32]; // Optional: sometimes present
	// int				type;     // Optional: sometimes present (NA_IP, NA_LOOPBACK, etc.)
} netadr_t;


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct edict_s; // Tentative forward declaration
typedef struct edict_s edict_t;

class CCommand; // Forward declaration for CCommand
class Vector;
class Vector2D;
class QAngle;
class RadianEuler;
class Quaternion;
class matrix3x4_t;
class VMatrix;


//-----------------------------------------------------------------------------
// class CUtlMemory
//-----------------------------------------------------------------------------
// A growable memory class.
// The amount of memory allocated is stored in m_nAllocationCount.
// The amount of memory used is stored in m_nGrowSize.
// The memory is allocated in blocks of m_nGrowSize.
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
	void AssumeMemory( T *pMemory, int nSize );

	// Fast swap
	void Swap( CUtlMemory< T, I > &mem );

	// Switches the buffer from an external buffer to an internal one
	void ConvertToGrowableMemory( int nGrowSize );

	// Size
	int NumAllocated() const;
	int Count() const;

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

	// Trail 'i' to the end of the list.
	void AddToTail( I i );

	// Add an element to the tail.
	I AddToTail();
	I AddToTail( const T& src );

	// Add elements to the tail.
	I AddMultipleToTail( int num );
	I AddMultipleToTail( int num, const T *pToCopy );

	// Insert an element before the given index
	I InsertBefore( I before );
	I InsertBefore( I before, const T& src );

	// Insert elements before the given index
	I InsertMultipleBefore( I before, int num );
	I InsertMultipleBefore( I before, int num, const T *pToCopy );

	// Insert an element after the given index
	I InsertAfter( I after );
	I InsertAfter( I after, const T& src );

	// Removes an element from the list
	void FastRemove( I elem );	// doesn't preserve order
	void Remove( I elem );		// preserves order
	void RemoveMultiple( I elem, int num ); // preserves order
	void RemoveAll();			// doesn't preserve order (fast)

	// Find an element ( XFINDSIMPLEMEM() in CUtlSymbolTable )
	I Find( const T& src ) const;

	// Does an element exist?
	bool HasElement( const T& src ) const;

	// Makes sure we have enough memory allocated to store a requested # of elements
	bool CheckAllocation( int numElements );

	// Makes sure we have enough memory allocated to store a requested # of elements
	// Use this instead of CheckAllocation if you are going to call Base() and then
	// write into the memory. It will preserve the contents of the memory if it has
	// to reallocate.
	bool CheckAndPreserveAllocation( int numElements );

	// Element access for the head of the list
	T& Head();
	const T& Head() const;

	// Element access for the tail of the list
	T& Tail();
	const T& Tail() const;

	// Get the count of elements in the list
	static int ElementCount();

	// Returns the invalid index
	static I InvalidIndex();

	// Is an index valid?
	bool IsIdxValid( I i ) const;

protected:
	// Grows the memory
	void GrowSelf( int num );

	// For easier access to the elements
	T* m_pMemory;

	// Number of elements allocated
	int m_nAllocationCount;

	// Number of elements currently in use
	int m_nGrowSize;
};

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
// Vector class
//-----------------------------------------------------------------------------
class Vector
{
public:
	// Members
	float x, y, z;

	// Construction/destruction
	Vector(void);
	Vector(float X, float Y, float Z);
	Vector(const float *clr);

	// Initialization
	void Init(float ix=0.0f, float iy=0.0f, float iz=0.0f);

	// Got any NaN's?
	bool IsValid() const;

	// array access...
	float operator[](int i) const;
	float& operator[](int i);

	// Base address...
	float* Base();
	float const* Base() const;

	// Initialization methods
	void Random( float minVal, float maxVal );

	// equality
	bool operator==(const Vector& v) const;
	bool operator!=(const Vector& v) const;

	// arithmetic operations
	Vector&	operator+=(const Vector& v);
	Vector&	operator-=(const Vector& v);
	Vector&	operator*=(const Vector& v);
	Vector&	operator*=(float s);
	Vector&	operator/=(const Vector& v);
	Vector&	operator/=(float s);

	// negate the vector
	Vector	operator-() const;

	// Get the vector's magnitude.
	float	Length() const;

	// Get the vector's magnitude squared.
	float	LengthSqr(void) const;

	// return true if this vector is (0,0,0) within tolerance
	bool IsZero( float tolerance = 0.01f ) const;

	// Normalize in place and return the old length.
	float	NormalizeInPlace();

	// Compare length.
	bool	IsLengthGreaterThan( float val ) const;
	bool	IsLengthLessThan( float val ) const;

	// Get the distance from this vector to the other one.
	float	DistTo(const Vector &vOther) const;

	// Get the distance from this vector to the other one squared.
	float	DistToSqr(const Vector &vOther) const;

	// Copy
	void	CopyToArray(float* rgfl) const;

	// Multiply, add, and assign to this (ie: *this = a + b * scalar). This
	// is about 12% faster than the actual vector equation (because it's done per-component
	// rather than per-vector).
	void	MulAdd(const Vector& a, const Vector& b, float scalar);

	// Dot product.
	float	Dot(const Vector& vOther) const;

	// assignment
	Vector& operator=(const Vector &vOther);

	// 2d
	float	Length2D( void ) const;
	float	Length2DSqr( void ) const;

	// Add a vector to another vector and return the result in a new vector.
	Vector	operator+(const Vector& v) const;
	Vector	operator-(const Vector& v) const;
	Vector	operator*(const Vector& v) const;
	Vector	operator/(const Vector& v) const;
	Vector	operator*(float fl) const;
	Vector	operator/(float fl) const;

	// Cross product between two vectors.
	Vector	Cross(const Vector &vOther) const;

	// Min/max operations.
	Vector	Min(const Vector &vOther) const;
	Vector	Max(const Vector &vOther) const;
};


//-----------------------------------------------------------------------------
// Vector2D class
//-----------------------------------------------------------------------------
class Vector2D
{
public:
	// Members
	float x, y;

	// Construction/destruction
	Vector2D(void);
	Vector2D(float X, float Y);
	Vector2D(const float *pFloat);

	// Initialization
	void Init(float ix=0.0f, float iy=0.0f);

	// Got any NaN's?
	bool IsValid() const;

	// array access...
	float operator[](int i) const;
	float& operator[](int i);

	// Base address...
	float* Base();
	float const* Base() const;

	// Initialization methods
	void Random( float minVal, float maxVal );

	// equality
	bool operator==(const Vector2D& v) const;
	bool operator!=(const Vector2D& v) const;

	// arithmetic operations
	Vector2D&	operator+=(const Vector2D& v);
	Vector2D&	operator-=(const Vector2D& v);
	Vector2D&	operator*=(const Vector2D& v);
	Vector2D&	operator*=(float s);
	Vector2D&	operator/=(const Vector2D& v);
	Vector2D&	operator/=(float s);

	// negate the vector
	Vector2D	operator-() const;

	// Get the vector's magnitude.
	float	Length() const;

	// Get the vector's magnitude squared.
	float	LengthSqr(void) const;

	// return true if this vector is (0,0) within tolerance
	bool IsZero( float tolerance = 0.01f ) const;

	// Normalize in place and return the old length.
	float	NormalizeInPlace();

	// Compare length.
	bool	IsLengthGreaterThan( float val ) const;
	bool	IsLengthLessThan( float val ) const;

	// Get the distance from this vector to the other one.
	float	DistTo(const Vector2D &vOther) const;

	// Get the distance from this vector to the other one squared.
	float	DistToSqr(const Vector2D &vOther) const;

	// Copy
	void	CopyToArray(float* rgfl) const;

	// Multiply, add, and assign to this (ie: *this = a + b * scalar). This
	// is about 12% faster than the actual vector equation (because it's done per-component
	// rather than per-vector).
	void	MulAdd(const Vector2D& a, const Vector2D& b, float scalar);

	// Dot product.
	float	Dot(const Vector2D& vOther) const;

	// assignment
	Vector2D& operator=(const Vector2D &vOther);

	// Add a vector to another vector and return the result in a new vector.
	Vector2D	operator+(const Vector2D& v) const;
	Vector2D	operator-(const Vector2D& v) const;
	Vector2D	operator*(const Vector2D& v) const;
	Vector2D	operator/(const Vector2D& v) const;
	Vector2D	operator*(float fl) const;
	Vector2D	operator/(float fl) const;

	// Min/max operations.
	Vector2D	Min(const Vector2D &vOther) const;
	Vector2D	Max(const Vector2D &vOther) const;
};


//-----------------------------------------------------------------------------
// Radian Euler angle class
//-----------------------------------------------------------------------------
class RadianEuler
{
public:
	// Members
	float x, y, z;

	// Construction/destruction
	RadianEuler(void);
	RadianEuler(float X, float Y, float Z);
	RadianEuler(Quaternion const &q);	// initialize from a quaternion
	RadianEuler(QAngle const &angles);	// initialize from a QAngle

	// Initialization
	void Init(float ix=0.0f, float iy=0.0f, float iz=0.0f);

	// Conversion to qangle
	QAngle ToQAngle(void) const;
	bool IsValid() const;
	void Invalidate();

	// array access...
	float operator[](int i) const;
	float& operator[](int i);

	// Base address...
	float* Base();
	float const* Base() const;

	// equality
	bool operator==(const RadianEuler& v) const;
	bool operator!=(const RadianEuler& v) const;

	// arithmetic operations
	RadianEuler&	operator+=(const RadianEuler& v);
	RadianEuler&	operator-=(const RadianEuler& v);
	RadianEuler&	operator*=(float s);
	RadianEuler&	operator/=(float s);

	// Get the vector's magnitude.
	float	Length() const;

	// Get the vector's magnitude squared.
	float	LengthSqr(void) const;

	// assignment
	RadianEuler& operator=(const RadianEuler &vOther);

	// Add a vector to another vector and return the result in a new vector.
	RadianEuler	operator+(const RadianEuler& v) const;
	RadianEuler	operator-(const RadianEuler& v) const;
	RadianEuler	operator*(float fl) const;
	RadianEuler	operator/(float fl) const;
};


//-----------------------------------------------------------------------------
// Quaternion class
//-----------------------------------------------------------------------------
class Quaternion				// same data-layout as engine's vec4_t,
{								//		which is a float[4]
public:
	// Members
	float x, y, z, w;

	// Construction/destruction
	Quaternion(void);
	Quaternion(float X, float Y, float Z, float W);
	Quaternion(RadianEuler const &angle);	// initialize from a RadianEuler
	Quaternion(QAngle const &angle);		// initialize from a QAngle
	Quaternion(matrix3x4_t const &matrix);	// initialize from a matrix

	// Initialization
	void Init(float ix=0.0f, float iy=0.0f, float iz=0.0f, float iw=0.0f);

	// Got any NaN's?
	bool IsValid() const;
	void Invalidate();

	// array access...
	float operator[](int i) const;
	float& operator[](int i);

	// Base address...
	float* Base();
	float const* Base() const;

	// equality
	bool operator==(const Quaternion& v) const;
	bool operator!=(const Quaternion& v) const;

	// arithmetic operations
	Quaternion&	operator+=(const Quaternion& v);
	Quaternion&	operator-=(const Quaternion& v);
	Quaternion&	operator*=(const Quaternion& v);
	Quaternion&	operator*=(float s);
	Quaternion&	operator/=(const Quaternion& v);
	Quaternion&	operator/=(float s);

	// Get the vector's magnitude.
	float	Length() const;

	// Get the vector's magnitude squared.
	float	LengthSqr(void) const;

	// Normalize in place and return the old length.
	float	NormalizeInPlace();

	// Conjugate
	Quaternion Conjugate() const;

	// To matrix
	void ToMatrix( matrix3x4_t& matrix ) const;

	// To euler angles
	RadianEuler ToEulerAngles() const;
	QAngle ToQAngle() const;

	// assignment
	Quaternion& operator=(const Quaternion &vOther);

	// Add a vector to another vector and return the result in a new vector.
	Quaternion	operator+(const Quaternion& v) const;
	Quaternion	operator-(const Quaternion& v) const;
	Quaternion	operator*(const Quaternion& v) const; // Quaternion multiplication
	Quaternion	operator*(float fl) const;
	Quaternion	operator/(float fl) const;

	// dot product
	float Dot( const Quaternion &vOther ) const;
};


//-----------------------------------------------------------------------------
// QAngle class
//-----------------------------------------------------------------------------
class QAngle
{
public:
	// Members
	float x, y, z;

	// Construction/destruction
	QAngle(void);
	QAngle(float X, float Y, float Z);
//	QAngle(RadianEuler const &angles);	// Initialize from a radian euler angle
	QAngle(Quaternion const &quat);		// Initialize from a quaternion

	// Initialization
	void Init(float ix=0.0f, float iy=0.0f, float iz=0.0f);
	void Random( float minVal, float maxVal );

	// Got any NaN's?
	bool IsValid() const;
	void Invalidate();

	// array access...
	float operator[](int i) const;
	float& operator[](int i);

	// Base address...
	float* Base();
	float const* Base() const;

	// equality
	bool operator==(const QAngle& v) const;
	bool operator!=(const QAngle& v) const;

	// arithmetic operations
	QAngle&	operator+=(const QAngle& v);
	QAngle&	operator-=(const QAngle& v);
	QAngle&	operator*=(float s);
	QAngle&	operator/=(float s);

	// Get the vector's magnitude.
	float	Length() const;

	// Get the vector's magnitude squared.
	float	LengthSqr(void) const;

	// assignment
	QAngle& operator=(const QAngle &vOther);

	// Add a vector to another vector and return the result in a new vector.
	QAngle	operator+(const QAngle& v) const;
	QAngle	operator-(const QAngle& v) const;
	QAngle	operator*(float fl) const;
	QAngle	operator/(float fl) const;

	// Convert to vector.
	void VectorVectors( Vector *pForward, Vector *pRight, Vector *pUp ) const;
};


//-----------------------------------------------------------------------------
// matrix3x4_t class
//-----------------------------------------------------------------------------
class matrix3x4_t
{
public:
	// Members
	float m_flMatVal[3][4];

	// constructor
	matrix3x4_t() {}

	// initialize from a quaternion
	matrix3x4_t( Quaternion const& q );
	matrix3x4_t( RadianEuler const& angles );
	matrix3x4_t( QAngle const& angles, Vector const& position );
	matrix3x4_t( Vector const& xAxis, Vector const& yAxis, Vector const& zAxis, Vector const& vecOrigin );

	// initialization
	void Init( Quaternion const& q );
	void Init( RadianEuler const& angles );
	void Init( QAngle const& angles, Vector const& position );
	void Init( Vector const& xAxis, Vector const& yAxis, Vector const& zAxis, Vector const& vecOrigin );

	// array access
	float* operator[]( int i )				{ return m_flMatVal[i]; }
	const float* operator[]( int i ) const	{ return m_flMatVal[i]; }
	float* Base()							{ return &m_flMatVal[0][0]; }
	const float* Base() const				{ return &m_flMatVal[0][0]; }

	// Get and set the basis vectors
	Vector GetRow( int i ) const			{ return Vector(m_flMatVal[i][0], m_flMatVal[i][1], m_flMatVal[i][2]); }
	Vector GetColumn( int i ) const			{ return Vector(m_flMatVal[0][i], m_flMatVal[1][i], m_flMatVal[2][i]); }
	void SetRow( int i, const Vector &v )	{ m_flMatVal[i][0] = v.x; m_flMatVal[i][1] = v.y; m_flMatVal[i][2] = v.z; }
	void SetColumn( int i, const Vector &v ){ m_flMatVal[0][i] = v.x; m_flMatVal[1][i] = v.y; m_flMatVal[2][i] = v.z; }

	// Get and set the position
	Vector GetOrigin() const					{ return Vector(m_flMatVal[0][3], m_flMatVal[1][3], m_flMatVal[2][3]); }
	void SetOrigin( const Vector &p )		{ m_flMatVal[0][3] = p.x; m_flMatVal[1][3] = p.y; m_flMatVal[2][3] = p.z; }

	// Get and set the angles
	void GetAngles( QAngle &angles ) const;
	void SetAngles( const QAngle &angles );
	void GetAngles( RadianEuler &angles ) const;
	void SetAngles( const RadianEuler &angles );

	// Invalidate the matrix
	void Invalidate( void );
	bool IsValid( void ) const;

	// Matrix operations
	void ConcatTransforms( const matrix3x4_t &in1, const matrix3x4_t &in2 );
	void TransformVector( const Vector &vIn, Vector &vOut ) const;
	void InverseTransformVector( const Vector &vIn, Vector &vOut ) const;
	void RotateVector( const Vector &vIn, Vector &vOut ) const;
	void InverseRotateVector( const Vector &vIn, Vector &vOut ) const;

	// transform a AABB
	void TransformAABB( const Vector &vecMinsIn, const Vector &vecMaxsIn, Vector &vecMinsOut, Vector &vecMaxsOut ) const;
	void InverseTransformAABB( const Vector &vecMinsIn, const Vector &vecMaxsIn, Vector &vecMinsOut, Vector &vecMaxsOut ) const;

	// For rotations only
	void Inverse( matrix3x4_t& matInv ) const;
};


//-----------------------------------------------------------------------------
// VMatrix class
//-----------------------------------------------------------------------------
class VMatrix
{
public:
	// Members
	float		m[4][4];

	// Construction.
	VMatrix();
	VMatrix(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33
		);

	// Creates a matrix where the X axis = forward
	// the Y axis = left, and the Z axis = up
	VMatrix( const Vector& forward, const Vector& left, const Vector& up );

	// Construct from a 3x4 matrix
	VMatrix( const matrix3x4_t& matrix3x4 );

	// Set the values in the matrix.
	void		Init(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33
		);


	// Initialize from a 3x4
	void		Init( const matrix3x4_t& matrix3x4 );

	// array access
	float* operator[]( int i )				{ return m[i]; }
	const float* operator[]( int i ) const	{ return m[i]; }
	float* Base()							{ return &m[0][0]; }
	const float* Base() const				{ return &m[0][0]; }

	void		SetLeft( const Vector &vLeft );
	void		SetUp( const Vector &vUp );
	void		SetForward( const Vector &vForward );

	void		GetBasisVectors( Vector &vForward, Vector &vLeft, Vector &vUp ) const;
	void		SetBasisVectors( const Vector &vForward, const Vector &vLeft, const Vector &vUp );

	// Get/set the translation.
	Vector		GetTranslation( void ) const;
	void		SetTranslation( const Vector &vTrans );

	void		PreTranslate( const Vector &vTrans );
	void		PostTranslate( const Vector &vTrans );

	matrix3x4_t& As3x4();
	const matrix3x4_t& As3x4() const;
	void		CopyFrom3x4( const matrix3x4_t &m3x4 );
	void		Set3x4( matrix3x4_t& matrix3x4 ) const;

	bool		operator==(const VMatrix& src) const;
	bool		operator!=(const VMatrix& src) const { return !(*this == src); }

	// Multiply by scalars
	VMatrix		operator*(float fl) const;
	VMatrix&	operator*=(float fl);

	// Transpose.
	VMatrix		Transpose() const;

	// Multiply.
	VMatrix		operator*(const VMatrix &vOther) const;
	VMatrix&	operator*=(const VMatrix &vOther);

	// Add.
	VMatrix		operator+(const VMatrix &vOther) const;
	VMatrix&	operator+=(const VMatrix &vOther);

	// Zero.
	void		Zero();

	// Identity.
	void		Identity();
	bool		IsIdentity() const;

	// Computes an inverse.
	bool		InverseGeneral( VMatrix &vInverse ) const;

	// Computes the inverse of a rotation matrix
	VMatrix		InverseTR() const;

	// Get the scale of the matrix
	Vector		GetScale() const;

	// (Fast) multiply by a vector (override appropriate scale terms)
	Vector		operator*(const Vector &vVec) const;

	// Multiply by a vector (divides by w, assumes input w is 1).
	Vector		VMul4x3(const Vector &vVec) const;

	// Multiply by the upper 3x3 part of the matrix.
	Vector		VMul3x3(const Vector &vVec) const;

	// Apply the inverse transpose of the 3x3 part of the matrix to the vector.
	Vector		VMul3x3Transpose(const Vector &vVec) const;

	// Apply the inverse of the 3x3 part of the matrix to the vector.
	Vector		ApplyRotation(const Vector &vVec) const;

	// Multiply by a 4D vector.
	Vector4D	operator*(const Vector4D &vVec) const;

	// Rotate a vector using the 3x3 part of the matrix.
	Vector		GetRotation(const Vector &vVec) const;

	// Get the matrix an orthonormal basis
	VMatrix		GetOrthoBase() const;

	// Decompose the matrix into translation, rotation, and scale components
	bool		Decompose( Vector *pvecTranslate, QAngle *pqaRotate, Vector *pvecScale ) const;
};


//-----------------------------------------------------------------------------
// Vector4D class ( x, y, z, w )
//-----------------------------------------------------------------------------
class Vector4D
{
public:
	// Members
	float x, y, z, w;

	// Construction/destruction
	Vector4D(void);
	Vector4D(float X, float Y, float Z, float W);
	Vector4D(const float *pFloat);

	// Initialization
	void Init(float ix=0.0f, float iy=0.0f, float iz=0.0f, float iw=0.0f);

	// Got any NaN's?
	bool IsValid() const;

	// array access...
	float operator[](int i) const;
	float& operator[](int i);

	// Base address...
	float* Base();
	float const* Base() const;

	// equality
	bool operator==(const Vector4D& v) const;
	bool operator!=(const Vector4D& v) const;

	// arithmetic operations
	Vector4D&	operator+=(const Vector4D& v);
	Vector4D&	operator-=(const Vector4D& v);
	Vector4D&	operator*=(const Vector4D& v);
	Vector4D&	operator*=(float s);
	Vector4D&	operator/=(const Vector4D& v);
	Vector4D&	operator/=(float s);

	// negate the vector
	Vector4D	operator-() const;

	// Get the vector's magnitude.
	float	Length() const;

	// Get the vector's magnitude squared.
	float	LengthSqr(void) const;

	// return true if this vector is (0,0,0,0) within tolerance
	bool IsZero( float tolerance = 0.01f ) const;

	// Normalize in place and return the old length.
	float	NormalizeInPlace();

	// assignment
	Vector4D& operator=(const Vector4D &vOther);

	// Add a vector to another vector and return the result in a new vector.
	Vector4D	operator+(const Vector4D& v) const;
	Vector4D	operator-(const Vector4D& v) const;
	Vector4D	operator*(const Vector4D& v) const;
	Vector4D	operator/(const Vector4D& v) const;
	Vector4D	operator*(float fl) const;
	Vector4D	operator/(float fl) const;

	// Dot product.
	float	Dot(const Vector4D& vOther) const;
};


//-----------------------------------------------------------------------------
// Utility functions
//-----------------------------------------------------------------------------

// Compare two floats for equality
inline bool FloatsAreEqual( float f1, float f2, float epsilon = 1e-6f )
{
	return ( fabsf( f1 - f2 ) < epsilon );
}

// Is a number a power of two?
inline bool IsPowerOfTwo( uint32 x )
{
	return ( x & ( x - 1 ) ) == 0;
}

// Get the next power of two greater than or equal to x
inline uint32 NextPowerOfTwo( uint32 x )
{
	x -= 1;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

// Swap two values
template <class T>
inline void V_swap( T& x, T& y )
{
	T temp = x;
	x = y;
	y = temp;
}

// Min and Max functions
template <class T>
inline T const& V_min( T const& x, T const& y )
{
	return (x < y) ? x : y;
}

template <class T>
inline T const& V_max( T const& x, T const& y )
{
	return (x > y) ? x : y;
}

// Clamp a value to a range
template <class T>
inline T V_clamp( T const& x, T const& minVal, T const& maxVal )
{
	if ( x < minVal )
		return minVal;
	else if ( x > maxVal )
		return maxVal;
	else
		return x;
}


//-----------------------------------------------------------------------------
// class FourVectors (aligned to 16 bytes)
//-----------------------------------------------------------------------------
class FourVectors
{
public:
	Vector m_v[4];

public:
	FourVectors() {}
	FourVectors( const Vector &v0, const Vector &v1, const Vector &v2, const Vector &v3 )
	{
		m_v[0] = v0; m_v[1] = v1; m_v[2] = v2; m_v[3] = v3;
	}

	// array access
	Vector& operator[]( int i )				{ return m_v[i]; }
	const Vector& operator[]( int i ) const	{ return m_v[i]; }

	// convert to array of floats
	float *Base() { return (float *)this; }
	const float *Base() const { return (const float *)this; }
};


#endif // BASETYPES_H
