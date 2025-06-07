//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef STRTOOLS_H
#define STRTOOLS_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h" // Defines MAX_PATH
#include "tier0/basetypes.h" // Defines int32, etc.
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// Placeholder for SAL annotations if not available
#ifndef OUT_CAP
#define OUT_CAP(x)
#endif
#ifndef INOUT_CAP
#define INOUT_CAP(x)
#endif

// MIN/MAX macros
#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif


//-----------------------------------------------------------------------------
// Portable string routines
//-----------------------------------------------------------------------------
#ifdef _WIN32
	#define V_strnchr strnchr // TODO: Make this an actual function
#elif LINUX
	char* V_strnchr( const char* pStr, char c, int n );
#else // OSX
	#define V_strnchr strnchr
#endif

// Convert a string to an integer
int32 V_atoi( const char *str );
int64 V_atoi64( const char *str );
// Convert a string to a float
float V_atof( const char *str );

// Convert an integer to a string
// NOTE: If you are holding onto the pointer, be sure to call V_FreeStr after you are done.
char *V_itoa( int val, char *buf, int buflen );
char *V_uitoa( unsigned int val, char *buf, int buflen );
char *V_i64toa( int64 val, char *buf, int buflen );
char *V_ui64toa( uint64 val, char *buf, int buflen );

// Convert a float to a string
// NOTE: If you are holding onto the pointer, be sure to call V_FreeStr after you are done.
char *V_ftoa( float val, char *buf, int buflen );

// Convert a string to all lowercase
char *V_strlower( char *str );
// Convert a string to all uppercase
char *V_strupr( char *str );

// Returns a pointer to the first occurrence of the substring in the string, or NULL if not found (case sensitive)
const char *V_strstr( const char *str, const char *search );
char *V_strstr( char *str, const char *search );
// Returns a pointer to the first occurrence of the substring in the string, or NULL if not found (case insensitive)
const char *V_stristr( const char *str, const char *search );
char *V_stristr( char *str, const char *search );

// string compare (case sensitive)
int V_strcmp( const char *s1, const char *s2 );
// string compare (case insensitive)
int V_stricmp( const char *s1, const char *s2 );
// string compare (case sensitive, max length)
int V_strncmp( const char *s1, const char *s2, int count );
// string compare (case insensitive, max length)
int V_strnicmp( const char *s1, const char *s2, int count );

// Get the length of a string
int V_strlen( const char *str );

// Copy a string (safe)
// Returns the number of characters copied (not including the null terminator)
// If the buffer is too small, the string is truncated and null terminated.
int V_strcpy_safe( OUT_CAP(maxLenInChars) char *dst, int maxLenInChars, const char *src );
// Copy a string (safe, max length)
// Returns the number of characters copied (not including the null terminator)
// If the buffer is too small, the string is truncated and null terminated.
int V_strncpy_safe( OUT_CAP(maxLenInChars) char *dst, int maxLenInChars, const char *src, int count );

// Concatenate strings (safe)
// Returns the number of characters copied (not including the null terminator)
// If the buffer is too small, the string is truncated and null terminated.
int V_strcat_safe( INOUT_CAP(maxLenInChars) char *dst, int maxLenInChars, const char *src );
// Concatenate strings (safe, max length)
// Returns the number of characters copied (not including the null terminator)
// If the buffer is too small, the string is truncated and null terminated.
int V_strncat_safe( INOUT_CAP(maxLenInChars) char *dst, int maxLenInChars, const char *src, int count );

// Printf to a string (safe)
// Returns the number of characters written (not including the null terminator)
// If the buffer is too small, the string is truncated and null terminated.
// If an error occurs, returns -1.
int V_snprintf( OUT_CAP(maxLenInChars) char *dst, int maxLenInChars, const char *fmt, ... ) FMTFUNCTION( 3, 4 );
// Printf to a string (safe) - va_list version
// Returns the number of characters written (not including the null terminator)
// If the buffer is too small, the string is truncated and null terminated.
// If an error occurs, returns -1.
int V_vsnprintf( OUT_CAP(maxLenInChars) char *dst, int maxLenInChars, const char *fmt, va_list args );

// Printf to a string (safe)
// Returns the number of characters that would have been written if the buffer was large enough (not including the null terminator)
// If an error occurs, returns -1.
// This is the C99 standard behavior.
int V_snprintfRet( OUT_CAP(maxLenInChars) char *dst, int maxLenInChars, const char *fmt, ... ) FMTFUNCTION( 3, 4 );
// Printf to a string (safe) - va_list version
// Returns the number of characters that would have been written if the buffer was large enough (not including the null terminator)
// If an error occurs, returns -1.
// This is the C99 standard behavior.
int V_vsnprintfRet( OUT_CAP(maxLenInChars) char *dst, int maxLenInChars, const char *fmt, va_list args );

// Returns true if the string is empty
bool V_isempty( const char *str );

// Returns true if the string is all whitespace
bool V_isspace( const char *str );

// Returns true if the string contains only printable characters
bool V_isprintable( const char *str );

// Returns true if the string contains only numeric characters
bool V_isnumeric( const char *str );

// Returns true if the string contains only alphanumeric characters
bool V_isalnum( const char *str );

// Returns true if the string contains only alphabetic characters
bool V_isalpha( const char *str );

// Returns true if the string contains only hexadecimal characters
bool V_isxdigit( const char *str );

// Returns true if the string contains only binary characters (0 or 1)
bool V_isbinary( const char *str );

// Returns true if the string is a valid C/C++ identifier
bool V_isidentifier( const char *str );

// Returns true if the string is a valid path
bool V_ispath( const char *str );

// Returns true if the string is a valid filename
bool V_isfilename( const char *str );

// Returns true if the string is a valid URL
bool V_isurl( const char *str );

// Returns true if the string is a valid email address
bool V_isemail( const char *str );

// Returns true if the string is a valid IP address
bool V_isipaddress( const char *str );

// Returns true if the string is a valid MAC address
bool V_ismacaddress( const char *str );

// Returns true if the string is a valid GUID
bool V_isguid( const char *str );

// Returns true if the string is a valid MD5 hash
bool V_ismd5( const char *str );

// Returns true if the string is a valid SHA1 hash
bool V_issha1( const char *str );


//-----------------------------------------------------------------------------
// Path manipulation
//-----------------------------------------------------------------------------

// Returns a pointer to the filename part of a path string
// (everything after the last path separator)
const char *V_UnqualifiedFileName( const char *in );
char *V_UnqualifiedFileName( char *in );

// Returns a pointer to the extension part of a path string
// (everything after the last dot)
// Returns NULL if there is no extension
const char *V_GetFileExtension( const char *path );

// Returns true if the path is absolute (starts with a drive letter or path separator)
bool V_IsAbsolutePath( const char *path );

// Extracts the file base name (everything before the last dot) from a path string
// Returns true if successful, false otherwise
// If successful, the output buffer contains the file base name
// If the input path has no extension, the output buffer contains the entire input path
// If the input path is NULL or empty, the output buffer is empty
bool V_FileBase( const char *path, char *dest, int destlen );

// Extracts the file path (everything before the last path separator) from a path string
// Returns true if successful, false otherwise
// If successful, the output buffer contains the file path
// If the input path has no path separator, the output buffer is empty
// If the input path is NULL or empty, the output buffer is empty
bool V_ExtractFilePath( const char *path, char *dest, int destlen );

// Extracts the file name (everything after the last path separator) from a path string
// Returns true if successful, false otherwise
// If successful, the output buffer contains the file name
// If the input path has no path separator, the output buffer contains the entire input path
// If the input path is NULL or empty, the output buffer is empty
bool V_ExtractFileName( const char *path, char *dest, int destlen );

// Removes the extension from a path string
// If the path has no extension, the path is unchanged
// If the path is NULL or empty, the path is unchanged
void V_StripExtension( const char *in, char *out, int outLen );

// Removes the trailing path separator from a path string, if present
void V_StripTrailingSlash( char *path );

// Adds a path separator to the end of a path string, if not already present
void V_AppendSlash( char *path, int strSize );

// Composes a path from a base path and a relative path
// If the relative path is absolute, it is returned directly
// If the base path is NULL or empty, the relative path is returned directly
// If the relative path is NULL or empty, the base path is returned directly
// The output buffer should be at least MAX_PATH characters long
void V_ComposeFileName( const char *path, const char *filename, char *dest, int destlen );

// Sets the extension of a path string
// If the new extension is NULL or empty, the existing extension is removed
// If the path has no existing extension, the new extension is appended
// The output buffer should be at least MAX_PATH characters long
void V_SetExtension( const char *path, const char *ext, char *outname, int outnamesize );

// Normalizes a path string (removes redundant path separators, resolves "." and "..")
// The output buffer should be at least MAX_PATH characters long
void V_NormalizePath( char *path );

// Converts a path string to use the correct path separators for the current platform
void V_FixSlashes( char *pname, char separator = CORRECT_PATH_SEPARATOR );

// Converts a path string to use forward slashes
void V_FixSlashesToForward( char *pname );

// Converts a path string to use backslashes
void V_FixSlashesToBack( char *pname );

// Returns true if the given string matches the pattern (with optional case sensitivity)
// Supports wildcard characters * (matches any sequence of characters) and ? (matches any single character)
bool StringMatchesPattern( const char *pszSource, const char *pszPattern, bool bCaseSensitive = true );


//-----------------------------------------------------------------------------
// String parsing
//-----------------------------------------------------------------------------

// Parses a token from a string
// Returns a pointer to the next token, or NULL if no more tokens are found
// If the token buffer is too small, the token is truncated
// If the input string is NULL or empty, returns NULL
// If the token buffer is NULL or the buffer size is 0, returns NULL
const char *V_ParseToken( const char *data, char *token, int maxtoken, bool *hasQuotes = NULL );


//-----------------------------------------------------------------------------
// Miscellaneous
//-----------------------------------------------------------------------------

// Generate a unique string (e.g. for temp file names)
// The output buffer should be at least 32 characters long
void V_GenerateUniqueString( char *root, int rootlen, char *buf, int buflen );

// Free a string that was allocated by V_itoa, V_ftoa, etc.
void V_FreeStr( char *str );

// Correct path separator for the current platform
#ifdef _WIN32
#define CORRECT_PATH_SEPARATOR '\\'
#define INCORRECT_PATH_SEPARATOR '/'
#define CORRECT_PATH_SEPARATOR_CHAR '\\'
#define INCORRECT_PATH_SEPARATOR_CHAR '/'
#else
#define CORRECT_PATH_SEPARATOR '/'
#define INCORRECT_PATH_SEPARATOR '\\'
#define CORRECT_PATH_SEPARATOR_CHAR '/'
#define INCORRECT_PATH_SEPARATOR_CHAR '\\'
#endif


#endif // STRTOOLS_H
