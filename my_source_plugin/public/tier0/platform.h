//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/valve_off.h"
#ifdef _LINUX
#include <stdint.h> // intptr_t
#include <wchar.h> // wchar_t
#endif
#include "tier0/valve_on.h"

//-----------------------------------------------------------------------------
// Old-school defines we don't want to use moving forward
//-----------------------------------------------------------------------------
#define PLATFORM_WINDOWS 1
#define PLATFORM_X360 2
#define PLATFORM_PS3 3
#define PLATFORM_LINUX 4
#define PLATFORM_OSX 5
#define PLATFORM_ANDROID 6
#define PLATFORM_IOS 7


//-----------------------------------------------------------------------------
// Platform constants
//-----------------------------------------------------------------------------
#if defined( _WIN32 ) && !defined( _X360 )
	#define IsPlatformWindows() true
	#define IsPlatformX360() false
	#define IsPlatformPS3() false
	#define IsPlatformLinux() false
	#define IsPlatformOSX() false
	#define IsPlatformPosix() false
	#define IsPlatformAndroid() false
	#define IsPlatformIOS() false
	#define PLATFORM_IS_WINDOWS
#elif defined( _X360 )
	#define IsPlatformWindows() false
	#define IsPlatformX360() true
	#define IsPlatformPS3() false
	#define IsPlatformLinux() false
	#define IsPlatformOSX() false
	#define IsPlatformPosix() false
	#define IsPlatformAndroid() false
	#define IsPlatformIOS() false
	#define PLATFORM_IS_X360
#elif defined( PS3 )
	#define IsPlatformWindows() false
	#define IsPlatformX360() false
	#define IsPlatformPS3() true
	#define IsPlatformLinux() false
	#define IsPlatformOSX() false
	#define IsPlatformPosix() true // next two lines should be fixed
	#define IsPlatformAndroid() false
	#define IsPlatformIOS() false
	#define PLATFORM_IS_PS3
#elif defined( LINUX )
	#define IsPlatformWindows() false
	#define IsPlatformX360() false
	#define IsPlatformPS3() false
	#define IsPlatformLinux() true
	#define IsPlatformOSX() false
	#define IsPlatformPosix() true
	#define IsPlatformAndroid() false
	#define IsPlatformIOS() false
	#define PLATFORM_IS_LINUX
#elif defined( OSX )
	#define IsPlatformWindows() false
	#define IsPlatformX360() false
	#define IsPlatformPS3() false
	#define IsPlatformLinux() false
	#define IsPlatformOSX() true
	#define IsPlatformPosix() true
	#define IsPlatformAndroid() false
	#define IsPlatformIOS() false
	#define PLATFORM_IS_OSX
#elif defined( ANDROID )
	#define IsPlatformWindows() false
	#define IsPlatformX360() false
	#define IsPlatformPS3() false
	#define IsPlatformLinux() true // Android is based on Linux
	#define IsPlatformOSX() false
	#define IsPlatformPosix() true
	#define IsPlatformAndroid() true
	#define IsPlatformIOS() false
	#define PLATFORM_IS_ANDROID
#elif defined( IOS )
	#define IsPlatformWindows() false
	#define IsPlatformX360() false
	#define IsPlatformPS3() false
	#define IsPlatformLinux() false
	#define IsPlatformOSX() false // iOS is not OS X
	#define IsPlatformPosix() true
	#define IsPlatformAndroid() false
	#define IsPlatformIOS() true
	#define PLATFORM_IS_IOS
#else
	#error "Unsupported platform!"
#endif


//-----------------------------------------------------------------------------
// CPU constants
//-----------------------------------------------------------------------------
// Forcing x64 for now due to preprocessor issues with Makefile define
#define IsCPUByteSwapped() false // x64 is little endian
#define IsCPUX86() false
#define IsCPUX64() true
#define IsCPUPPC() false
#define IsCPUARM() false
/*
#if defined( _M_IX86 )
	#define IsCPUByteSwapped() false // x86 is little endian
	#define IsCPUX86() true
	#define IsCPUX64() false
	#define IsCPUPPC() false
	#define IsCPUARM() false
#elif defined( _M_X64 )
	#define IsCPUByteSwapped() false // x64 is little endian
	#define IsCPUX86() false
	#define IsCPUX64() true
	#define IsCPUPPC() false
	#define IsCPUARM() false
#elif defined( _M_PPC )
	#define IsCPUByteSwapped() true // PPC is big endian
	#define IsCPUX86() false
	#define IsCPUX64() false
	#define IsCPUPPC() true
	#define IsCPUARM() false
#elif defined( __arm__ ) || defined( __thumb__ )
	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		#define IsCPUByteSwapped() true
	#else
		#define IsCPUByteSwapped() false
	#endif
	#define IsCPUX86() false
	#define IsCPUX64() false
	#define IsCPUPPC() false
	#define IsCPUARM() true
#else
	#error "Unsupported CPU!"
#endif
*/


//-----------------------------------------------------------------------------
// Compiler constants
//-----------------------------------------------------------------------------
#if defined( _MSC_VER )
	#define IsCompilerMSVC() true
	#define IsCompilerGCC() false
	#define IsCompilerPS3() false
	#define IsCompilerClang() false
	#define COMPILER_IS_MSVC
#elif defined( __GNUC__ )
	#define IsCompilerMSVC() false
	#define IsCompilerGCC() true
	#define IsCompilerPS3() false
	#if __clang__
		#define IsCompilerClang() true
	#else
		#define IsCompilerClang() false
	#endif
	#define COMPILER_IS_GCC
#elif defined( SN_TARGET_PS3 )
	#define IsCompilerMSVC() false
	#define IsCompilerGCC() false
	#define IsCompilerPS3() true
	#define IsCompilerClang() false
	#define COMPILER_IS_PS3SNC
#else
	#error "Unsupported compiler!"
#endif


//-----------------------------------------------------------------------------
// OS version constants
//-----------------------------------------------------------------------------
#ifdef _WIN32
	#ifndef _WIN32_WINNT
		// Target Windows XP SP2 or later by default. For a different minimum requirement, define _WIN32_WINNT to the appropriate value.
		// See http://msdn.microsoft.com/en-us/library/aa383745(VS.85).aspx
		#define _WIN32_WINNT 0x0502
	#endif
#endif


//-----------------------------------------------------------------------------
// Numerical representation
//-----------------------------------------------------------------------------
#define ExecuteNTimes( nTimes, x ) \
	{ \
		static int __executeCount=0; \
		if ( __executeCount < nTimes ) \
		{ \
			x; \
			__executeCount++; \
		} \
	}

#define ExecuteOnce( x ) ExecuteNTimes( 1, x )

// Pad a number so it lies on a N byte boundary.
// So PAD_NUMBER(0,4) is 0 and PAD_NUMBER(1,4) is 4
#define PAD_NUMBER( number, boundary ) \
	( ( (number) + ((boundary)-1) ) & ~((boundary)-1) )

// Pad a pointer so it lies on a N byte boundary.
// So PAD_POINTER(0,4) is 0 and PAD_POINTER(1,4) is 4.
#define PAD_POINTER( pPointer, boundary ) \
	( (void *)PAD_NUMBER( (uintp)pPointer, boundary ) )

// Use this to align variables.
// USAGE: ALIGN(16) int i;
// This will make "i" have 16-byte alignment.
#ifdef _MSC_VER
	#define ALIGN( numBytes ) __declspec(align(numBytes))
#elif __GNUC__
	#define ALIGN( numBytes ) __attribute__ ((aligned (numBytes)))
#else
	#error "Unsupported compiler for ALIGN macro!"
#endif

// Used to step into the debugger
#if defined( _WIN32 ) && !defined( _X360 )
	#define DebuggerBreak() __debugbreak()
#elif defined( _X360 )
	#define DebuggerBreak() DebugBreak()
#elif LINUX
	#if defined __i386__ || defined __x86_64__
		#define DebuggerBreak() __asm__ __volatile__( "int $3" )
	#else
		#define DebuggerBreak() ((void (*)())0)()
	#endif
#elif defined( OSX )
	#if defined __i386__ || defined __x86_64__
		#define DebuggerBreak() __asm__ __volatile__( "int $3" )
	#else
		#define DebuggerBreak() ((void (*)())0)()
	#endif
#elif defined( PS3 )
	#define DebuggerBreak() __asm__ volatile ( "tw 31,1,1" )
#else
	#error "Unsupported platform for DebuggerBreak macro!"
#endif

// For branch prediction hints
#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
	#define PREDICT_TRUE(exp)	__builtin_expect((exp), 1)
	#define PREDICT_FALSE(exp)	__builtin_expect((exp), 0)
#else
	#define PREDICT_TRUE(exp)	(exp)
	#define PREDICT_FALSE(exp)	(exp)
#endif


//-----------------------------------------------------------------------------
// DLL export and import definitions
//-----------------------------------------------------------------------------
#ifndef DLL_EXPORT
	#ifdef _WIN32
		#define DLL_EXPORT extern "C" __declspec( dllexport )
	#elif __GNUC__ >= 4
		#define DLL_EXPORT extern "C" __attribute__ ((visibility("default")))
	#else
		#define DLL_EXPORT extern "C"
	#endif
#endif

#ifndef DLL_IMPORT
	#ifdef _WIN32
		#define DLL_IMPORT extern "C" __declspec( dllimport )
	#else
		#define DLL_IMPORT extern "C"
	#endif
#endif

// Used to define a function as external
#ifndef EXTERN_FUNC
	#ifdef _WIN32
		#define EXTERN_FUNC extern
	#else
		#define EXTERN_FUNC
	#endif
#endif

// Used for dll exporting and importing
#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)

// Factory used to instance objects from the same DLL
#define Sys_GetFactory( libName ) GetProcAddress( (HMODULE)GetModuleHandle( libName ), "CreateInterface" )
#define Sys_GetFactoryThis() Sys_GetFactory( NULL )


//-----------------------------------------------------------------------------
// Types
//-----------------------------------------------------------------------------

// This can be used to ensure the size of an enum.
// Use int32 for enums that need to be stored in files.
// Use int8 for enums that are used frequently in structures.
#if !defined( ENUM_BITFIELD_TYPE )
	#if IsCompilerGCC()
		#define ENUM_BITFIELD_TYPE( enumName, fieldName, bitSize ) enumName fieldName : bitSize
	#else
		#define ENUM_BITFIELD_TYPE( enumName, fieldName, bitSize ) enumName fieldName
	#endif
#endif

#if !defined(FORCEINLINE)
	#if defined(_MSC_VER)
		#define FORCEINLINE __forceinline
	#elif defined(__GNUC__) && __GNUC__ > 3 // Also ensure we're using C++ for GCC
		// Clang also defines __GNUC__ (as 4)
		#if defined(__clang__)
			#if __has_attribute(always_inline)
				#define FORCEINLINE inline __attribute__((always_inline))
			#else
				#define FORCEINLINE inline
			#endif
		#else
			#define FORCEINLINE inline __attribute__((always_inline))
		#endif
	#else
		#define FORCEINLINE inline
	#endif
#endif

// Pass this to functions that are expecting a varargs list that you don't have.
#define VA_ARGS_EMPTY nullptr

// Use this to specify that a function is a varargs function.
// (This is not actually necessary in C++, but it doesn't hurt and makes it more clear.)
#define VARARGS ...

// Use this to declare a function as using the C calling convention.
#ifdef _WIN32
	#define  STDCALL				__stdcall
	#define  FASTCALL				__fastcall
	#define  FORCEINLINE_FASTCALL	FORCEINLINE __fastcall
#else
	#define  STDCALL
	#define  FASTCALL
	#define  FORCEINLINE_FASTCALL	FORCEINLINE
#endif

// Use this to declare a function as deprecated
#ifdef _MSC_VER
	#define DECL_DEPRECATED __declspec(deprecated)
#elif __GNUC__
	#define DECL_DEPRECATED __attribute__ ((deprecated))
#else
	#define DECL_DEPRECATED
#endif

// Use this to declare a function as having a return value that should not be ignored.
#if defined(_MSC_VER) && (_MSC_VER >= 1700) // [[nodiscard]] is a C++17 feature, but VS 2017 supports it with a warning
	#define CHECK_RETURN _Check_return_ // Using MS specific annotation to avoid C4834 if compiled with -Wall
#elif defined(__clang__) && __has_attribute(warn_unused_result)
	#define CHECK_RETURN __attribute__((warn_unused_result))
#elif defined(__GNUC__) && (__GNUC__ >= 4)
	#define CHECK_RETURN __attribute__((warn_unused_result))
#else
	#define CHECK_RETURN
#endif


// Define an abstract type
#define ABSTRACT_TYPE( name ) \
	class name \
	{ \
	public: \
		name() {} \
		name( const name &v ) { *(char *)this = *(char *)&v; } \
		name &operator=( const name &v ) { *(char *)this = *(char *)&v; return *this; } \
	};

// Define a handle type, which is just a pointer to an unknown type
#define DECLARE_HANDLE( name ) struct name##__ { int unused; }; typedef struct name##__ *name

// Forward declare a handle type
#define FORWARD_DECLARE_HANDLE( name ) typedef struct name##__ *name

// Define a pointer handle type
#define DEFINE_POINTER_HANDLE( name ) typedef class name##__{ int unused; } *name;

// Marks the codepath from here until the next Push এন্ড Pop thread priority as critical,
// bumping the thread's priority up.
#define EnterCriticalThreadPri() \
	int oldPri = GetThreadPriority( GetCurrentThread() ); \
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST )

// Returns the thread's priority to what it was before EnterCriticalThreadPri()
#define ExitCriticalThreadPri() \
	SetThreadPriority( GetCurrentThread(), oldPri )


//-----------------------------------------------------------------------------
// String manipulation
//-----------------------------------------------------------------------------
#define COPY_ARRAY( dest, src ) \
	{ Assert( sizeof(src) == sizeof(dest) ); memcpy( (dest), (src), sizeof(src) ); }

// Define a an array of characters that has a specific size and is initialized to zeros.
// This should be used for character arrays that are used as buffers, since it will initialize the buffer to zeros.
// (This is important because if the buffer is not initialized, it could contain sensitive data from a previous use.)
#define ALIGNED_BUFFER( name, size, alignment ) \
	ALIGN( alignment ) char name[ size ] = { 0 }

// Define a an array of characters that has a specific size.
// This should be used for character arrays that are used as buffers, since it will initialize the buffer to zeros.
// (This is important because if the buffer is not initialized, it could contain sensitive data from a previous use.)
#define ZEROED_ARRAY( name, size ) \
	char name[ size ] = { 0 }

// Use this to quote a preprocessor token
#define QUOTE( x ) #x
#define QUOTE_JOIN( x, y ) QUOTE( x ## y )

// Use this to turn a preprocessor token into a string
#define V_STRINGIFY(x) #x
#define V_TOSTRING(x) V_STRINGIFY(x)

// Use this to concatenate two preprocessor tokens
#define V_JOIN(X, Y) V_JOIN_IMPL(X, Y)
#define V_JOIN_IMPL(X, Y) X##Y

// Marks a function as printf-style
#ifdef __GNUC__
#define FMTFUNCTION( string_index, first_to_check ) __attribute__ ( ( format( printf, string_index, first_to_check ) ) )
#else
#define FMTFUNCTION( string_index, first_to_check )
#endif

// Marks a function as scanf-style
#ifdef __GNUC__
#define SCANFFUNCTION( string_index, first_to_check ) __attribute__ ( ( format( scanf, string_index, first_to_check ) ) )
#else
#define SCANFFUNCTION( string_index, first_to_check )
#endif


//-----------------------------------------------------------------------------
// Unused variable macro
//-----------------------------------------------------------------------------
#if defined(_MSC_VER) && (_MSC_VER >= 1900) // VS 2015 supports [[maybe_unused]]
	#define UNREFERENCED_PARAMETER(P) (P) // Use C++17 [[maybe_unused]] when available. For now, just use the old macro.
	#define UNREFERENCED_LOCAL_VARIABLE(V) (V)
#elif defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 7))) // GCC 4.7 supports __attribute__((unused))
	#define UNREFERENCED_PARAMETER(P) (void)P
	#define UNREFERENCED_LOCAL_VARIABLE(V) /*V*/
#else
	#define UNREFERENCED_PARAMETER(P) (P)
	#define UNREFERENCED_LOCAL_VARIABLE(V) (V)
#endif


//-----------------------------------------------------------------------------
// Array size macro
//-----------------------------------------------------------------------------
// This is a helper macro that returns the number of elements in a statically-allocated array.
// (It will not work correctly for dynamically-allocated arrays.)
#define ARRAYSIZE(p) (sizeof(p)/sizeof(p[0]))

// This is a version of ARRAYSIZE that can be used in contexts where the array size is not yet known.
// (For example, in a template function that takes an array as a parameter.)
#define Q_ARRAYSIZE( Array ) ( sizeof( Array ) / sizeof( Array[0] ) )


//-----------------------------------------------------------------------------
// Path manipulation
//-----------------------------------------------------------------------------
#ifdef _WIN32
#define PATHSEPARATOR(c) ((c) == '\\' || (c) == '/')
#else	// _WIN32
#define PATHSEPARATOR(c) ((c) == '/')
#endif	// _WIN32

#define PATH_MAX_LENGTH 260 // Max path length


//-----------------------------------------------------------------------------
// Assertions
//-----------------------------------------------------------------------------
#if !defined( Assert )
	#if defined( DBGFLAG_ASSERT )
		#define Assert( _exp )	\
			( (void)0 )
	#else
		#define Assert( _exp )					\
			if ( !(_exp) )					\
			{								\
				/* TODO: Add assert code */	\
			}
	#endif
#endif

// Compile-time assert
#if defined(__cplusplus)
	template<bool> class CCompileTimeAssert;
	template<> class CCompileTimeAssert<true> {};
	#define COMPILE_TIME_ASSERT( pred ) static CCompileTimeAssert< (pred) > metamacro_LOCAL_NAME( CTA )
#else
	#define COMPILE_TIME_ASSERT( pred ) switch(0){case 0:case (pred):;}
#endif


//-----------------------------------------------------------------------------
// Bitfields
//-----------------------------------------------------------------------------
// Use this to declare a bitfield.
// (It's not actually necessary in C++, but it doesn't hurt and makes it more clear.)
#define BITFIELD( name, size ) unsigned name : size

// Use this to get the value of a bitfield.
#define GET_BITFIELD( var, name ) ( (var).name )

// Use this to set the value of a bitfield.
#define SET_BITFIELD( var, name, val ) ( (var).name = (val) )


//-----------------------------------------------------------------------------
// Miscellaneous
//-----------------------------------------------------------------------------
// Use this to create a unique name for a static variable.
#define metamacro_ vuos( a, b ) a##b
#define metamacro_LOCAL_NAME( name ) metamacro_paste( name, __LINE__ )

// Define a macro that can be used to make a class non-copyable.
// (This is useful for classes that manage resources, such as file handles or network sockets.)
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&) = delete; \
    void operator=(const TypeName&) = delete

// Define a macro that can be used to make a class a singleton.
// (This is useful for classes that should only have one instance, such as a global logger or configuration manager.)
#define DEFINE_SINGLETON(TypeName) \
	public: \
		static TypeName& GetInstance() \
		{ \
			static TypeName instance; \
			return instance; \
		} \
	private: \
		TypeName(); \
		~TypeName(); \
		TypeName(const TypeName&); \
		void operator=(const TypeName&);

// Define a macro that can be used to make a class abstract.
// (This is useful for classes that are not meant to be instantiated directly, but rather to be inherited from.)
#define DECLARE_ABSTRACT_CLASS(TypeName) \
	public: \
		virtual ~TypeName() {} \
	protected: \
		TypeName() {} \
	private: \
		TypeName(const TypeName&); \
		void operator=(const TypeName&);

// Define a macro that can be used to make a class final.
// (This is useful for classes that are not meant to be inherited from.)
#if defined(_MSC_VER) && (_MSC_VER >= 1900) // VS 2015 supports final
	#define DECLARE_FINAL_CLASS(TypeName) class TypeName final
#else
	#define DECLARE_FINAL_CLASS(TypeName) class TypeName
#endif


//-----------------------------------------------------------------------------
// Spew types
//-----------------------------------------------------------------------------
enum SpewType_t
{
	SPEW_MESSAGE = 0,
	SPEW_WARNING,
	SPEW_ASSERT,
	SPEW_ERROR,
	SPEW_LOG,			// log only

	SPEW_TYPE_COUNT
};

//-----------------------------------------------------------------------------
// This is a symbol that is not exported by the DLL, but is still visible to other files in the same DLL.
// (This is useful for helper functions that are only used by other functions in the same DLL.)
#if defined(_WIN32)
	#define PLATFORM_INTERFACE __declspec(dllexport)
	#define PLATFORM_OVERLOAD __declspec(dllexport)
	#define PLATFORM_HIDDEN
#elif defined(__GNUC__) && (__GNUC__ >= 4)
	#define PLATFORM_INTERFACE __attribute__ ((visibility("default")))
	#define PLATFORM_OVERLOAD __attribute__ ((visibility("default")))
	#define PLATFORM_HIDDEN __attribute__ ((visibility("hidden")))
#else
	#define PLATFORM_INTERFACE
	#define PLATFORM_OVERLOAD
	#define PLATFORM_HIDDEN
#endif


//-----------------------------------------------------------------------------
// Include common types
//-----------------------------------------------------------------------------
#include "tier0/basetypes.h"
#include "tier0/commonmacros.h"


#endif // PLATFORM_H
