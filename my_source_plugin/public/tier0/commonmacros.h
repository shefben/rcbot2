//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef COMMONMACROS_H
#define COMMONMACROS_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Macro to get the number of elements in a static array
//-----------------------------------------------------------------------------
#define NELEMS(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))


//-----------------------------------------------------------------------------
// Macro to get the offset of a member in a struct
//-----------------------------------------------------------------------------
#define MEMBER_OFFSET(structName, memberName) ((int)(size_t)&(((structName *)0)->memberName))


//-----------------------------------------------------------------------------
// Macro to get the size of a member in a struct
//-----------------------------------------------------------------------------
#define MEMBER_SIZE(structName, memberName) (sizeof(((structName *)0)->memberName))


//-----------------------------------------------------------------------------
// Macro to get a pointer to the containing struct from a pointer to a member
//-----------------------------------------------------------------------------
#define PARENT_STRUCT_FROM_MEMBER(structName, memberName, memberPtr) \
	((structName *)((char *)(memberPtr) - MEMBER_OFFSET(structName, memberName)))


//-----------------------------------------------------------------------------
// Macro to generate a unique name
//-----------------------------------------------------------------------------
#define UNIQUE_NAME_JOIN( name, line ) name##line
#define UNIQUE_NAME_LINE( name, line ) UNIQUE_NAME_JOIN( name, line )
#define UNIQUE_NAME( name ) UNIQUE_NAME_LINE( name, __LINE__ )


//-----------------------------------------------------------------------------
// Macro to align a value to a given power of 2
//-----------------------------------------------------------------------------
#define ALIGN_VALUE( val, alignment ) ( ( val + alignment - 1 ) & ~( alignment - 1 ) )


//-----------------------------------------------------------------------------
// Macro to check if a value is a power of 2
//-----------------------------------------------------------------------------
#define IS_POWER_OF_2( val ) ( ( ( val ) > 0 ) && ( ( ( val ) & ( ( val ) - 1 ) ) == 0 ) )


//-----------------------------------------------------------------------------
// Macro to get the absolute value of a number
//-----------------------------------------------------------------------------
#define ABS( x ) ( ( (x) < 0 ) ? -(x) : (x) )


//-----------------------------------------------------------------------------
// Macro to swap two values
//-----------------------------------------------------------------------------
#define SWAP( a, b, temp ) ( ( temp ) = ( a ), ( a ) = ( b ), ( b ) = ( temp ) )


//-----------------------------------------------------------------------------
// Macro to define a bitwise enum
//-----------------------------------------------------------------------------
#define DEFINE_ENUM_BITWISE_OPERATORS( Type ) \
	inline Type operator|( Type a, Type b ) { return Type( int( a ) | int( b ) ); } \
	inline Type operator&( Type a, Type b ) { return Type( int( a ) & int( b ) ); } \
	inline Type operator^( Type a, Type b ) { return Type( int( a ) ^ int( b ) ); } \
	inline Type operator~( Type a ) { return Type( ~int( a ) ); } \
	inline Type& operator|=( Type& a, Type b ) { return a = a | b; } \
	inline Type& operator&=( Type& a, Type b ) { return a = a & b; } \
	inline Type& operator^=( Type& a, Type b ) { return a = a ^ b; }


//-----------------------------------------------------------------------------
// Macro to define a simple factory
//-----------------------------------------------------------------------------
#define DEFINE_SIMPLE_FACTORY( BaseClass, DerivedClass ) \
	static BaseClass* Create##DerivedClass() { return new DerivedClass; }


//-----------------------------------------------------------------------------
// Macro to define a factory that takes one argument
//-----------------------------------------------------------------------------
#define DEFINE_FACTORY_ARG1( BaseClass, DerivedClass, Arg1Type, Arg1Name ) \
	static BaseClass* Create##DerivedClass( Arg1Type Arg1Name ) { return new DerivedClass( Arg1Name ); }


//-----------------------------------------------------------------------------
// Macro to define a factory that takes two arguments
//-----------------------------------------------------------------------------
#define DEFINE_FACTORY_ARG2( BaseClass, DerivedClass, Arg1Type, Arg1Name, Arg2Type, Arg2Name ) \
	static BaseClass* Create##DerivedClass( Arg1Type Arg1Name, Arg2Type Arg2Name ) { return new DerivedClass( Arg1Name, Arg2Name ); }


//-----------------------------------------------------------------------------
// Macro to define a factory that takes three arguments
//-----------------------------------------------------------------------------
#define DEFINE_FACTORY_ARG3( BaseClass, DerivedClass, Arg1Type, Arg1Name, Arg2Type, Arg2Name, Arg3Type, Arg3Name ) \
	static BaseClass* Create##DerivedClass( Arg1Type Arg1Name, Arg2Type Arg2Name, Arg3Type Arg3Name ) { return new DerivedClass( Arg1Name, Arg2Name, Arg3Name ); }


//-----------------------------------------------------------------------------
// Macro to define a factory that takes four arguments
//-----------------------------------------------------------------------------
#define DEFINE_FACTORY_ARG4( BaseClass, DerivedClass, Arg1Type, Arg1Name, Arg2Type, Arg2Name, Arg3Type, Arg3Name, Arg4Type, Arg4Name ) \
	static BaseClass* Create##DerivedClass( Arg1Type Arg1Name, Arg2Type Arg2Name, Arg3Type Arg3Name, Arg4Type Arg4Name ) { return new DerivedClass( Arg1Name, Arg2Name, Arg3Name, Arg4Name ); }


//-----------------------------------------------------------------------------
// Macro to define a factory that takes five arguments
//-----------------------------------------------------------------------------
#define DEFINE_FACTORY_ARG5( BaseClass, DerivedClass, Arg1Type, Arg1Name, Arg2Type, Arg2Name, Arg3Type, Arg3Name, Arg4Type, Arg4Name, Arg5Type, Arg5Name ) \
	static BaseClass* Create##DerivedClass( Arg1Type Arg1Name, Arg2Type Arg2Name, Arg3Type Arg3Name, Arg4Type Arg4Name, Arg5Type Arg5Name ) { return new DerivedClass( Arg1Name, Arg2Name, Arg3Name, Arg4Name, Arg5Name ); }


#endif // COMMONMACROS_H
