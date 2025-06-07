//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef UTLSTRING_H
#define UTLSTRING_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier0/basetypes.h"
#include "tier1/strtools.h"
#include "tier1/utlmemory.h"


//-----------------------------------------------------------------------------
// Purpose: A string class
//-----------------------------------------------------------------------------
class CUtlString
{
public:
	CUtlString();
	CUtlString( const char *pString );
	CUtlString( const CUtlString& string );
	~CUtlString();

	// operator=
	CUtlString& operator=( const CUtlString &src );
	CUtlString& operator=( const char *src );

	// operator==
	bool operator==( const CUtlString &src ) const;
	bool operator==( const char *src ) const;

	// operator!=
	bool operator!=( const CUtlString &src ) const;
	bool operator!=( const char *src ) const;

	// operator<
	bool operator<( const CUtlString &val ) const;
	bool operator<( const char *val ) const;

	// operator>
	bool operator>( const CUtlString &val ) const;
	bool operator>( const char *val ) const;

	// operator+=
	CUtlString& operator+=( const CUtlString &rhs );
	CUtlString& operator+=( const char *rhs );
	CUtlString& operator+=( char c );
	CUtlString& operator+=( int rhs );
	CUtlString& operator+=( float rhs );

	// Get a pointer to the string. Returns NULL if the string is empty.
	const char *Get() const;
	operator const char*() const;

	// Get the length of the string
	int Length() const;
	bool IsEmpty() const { return Length() == 0; }

	// Set the string.
	void Set( const char *pValue );

	// Set the string from a CUtlString.
	void Set( const CUtlString &value );

	// Set the string from a subset of pValue.
	void Set( const char *pValue, int nChars );

	// Set the string from a subset of a CUtlString.
	void Set( const CUtlString &value, int nChars );

	// Append a string to the string.
	void Append( const char *pAddition );

	// Append a CUtlString to the string.
	void Append( const CUtlString &addition );

	// Append a string subset to the string.
	void Append( const char *pAddition, int nChars );

	// Append a CUtlString subset to the string.
	void Append( const CUtlString &addition, int nChars );

	// Format the string.
	void Format( const char *pFormat, ... );

	// Format the string.
	void FormatV( const char *pFormat, va_list marker );

	// Convert to lowercase
	void ToLower();

	// Convert to uppercase
	void ToUpper();

	// Trim whitespace from the left side of the string
	void TrimLeft( char cTarget = ' ' );

	// Trim whitespace from the right side of the string
	void TrimRight( char cTarget = ' ' );

	// Trim whitespace from both sides of the string
	void Trim( char cTarget = ' ' );

	// Returns true if the string is equal to the other string, ignoring case
	bool IsEqual_CaseInsensitive( const char *pOther ) const;

	// Returns true if the string matches the pattern, with optional case sensitivity
	bool MatchesPattern( const CUtlString &Pattern, bool bCaseSensitive = true ) const;

	// Replace all occurrences of a substring with another substring
	void Replace( const char *pFind, const char *pReplace, bool bCaseSensitive = false );

	// Remove all occurrences of a character from the string
	void RemoveChar( char ch );

	// Remove all occurrences of a substring from the string
	void Remove( const char *pSubStr, bool bCaseSensitive = false );

	// Make a substring
	CUtlString Slice( int32 nStart = 0, int32 nEnd = INT_MAX ) const;

	// Return a CUtlString composed of the last nChars characters of this CUtlString.
	CUtlString Right( int32 nChars ) const;

	// Return a CUtlString composed of the first nChars characters of this CUtlString.
	CUtlString Left( int32 nChars ) const;

	// Returns the first index of a character, or -1 if not found
	int FindChar( char ch, int nStart = 0 ) const;

	// Returns the first index of a substring, or -1 if not found
	int Find( const char *pSubStr, int nStart = 0, bool bCaseSensitive = true ) const;

	// Clear the string
	void Clear();

	// Purge the string's memory
	void Purge();

	// Get the string as a path (ensures a trailing path separator)
	CUtlString As generalmentePath() const;

	// Get the filename (without path or extension)
	CUtlString GetFilename() const;

	// Get the extension (without the dot)
	CUtlString GetExtension() const;

	// Get the path (without the filename)
	CUtlString GetPath() const;

	// Set the filename (replaces the existing filename)
	void SetFilename( const char *pFilename );

	// Set the extension (replaces the existing extension)
	void SetExtension( const char *pExtension );

	// Set the path (replaces the existing path)
	void SetPath( const char *pPath );

	// Strip the filename (remove the filename, leaving only the path)
	void StripFilename();

	// Strip the extension (remove the extension, leaving only the filename and path)
	void StripExtension();

	// Strip the path (remove the path, leaving only the filename and extension)
	void StripPath();

private:
	// The string data
	CUtlMemory<char> m_Storage;
};


//-----------------------------------------------------------------------------
// CUtlString inline functions
//-----------------------------------------------------------------------------
inline CUtlString::CUtlString()
{
}

inline CUtlString::CUtlString( const char *pString )
{
	Set( pString );
}

inline CUtlString::CUtlString( const CUtlString& string )
{
	Set( string.Get() );
}

inline CUtlString::~CUtlString()
{
	Purge();
}

inline CUtlString& CUtlString::operator=( const CUtlString &src )
{
	Set( src.Get() );
	return *this;
}

inline CUtlString& CUtlString::operator=( const char *src )
{
	Set( src );
	return *this;
}

inline bool CUtlString::operator==( const CUtlString &src ) const
{
	return V_strcmp( Get(), src.Get() ) == 0;
}

inline bool CUtlString::operator==( const char *src ) const
{
	return V_strcmp( Get(), src ) == 0;
}

inline bool CUtlString::operator!=( const CUtlString &src ) const
{
	return V_strcmp( Get(), src.Get() ) != 0;
}

inline bool CUtlString::operator!=( const char *src ) const
{
	return V_strcmp( Get(), src ) != 0;
}

inline bool CUtlString::operator<( const CUtlString &val ) const
{
	return V_strcmp( Get(), val.Get() ) < 0;
}

inline bool CUtlString::operator<( const char *val ) const
{
	return V_strcmp( Get(), val ) < 0;
}

inline bool CUtlString::operator>( const CUtlString &val ) const
{
	return V_strcmp( Get(), val.Get() ) > 0;
}

inline bool CUtlString::operator>( const char *val ) const
{
	return V_strcmp( Get(), val ) > 0;
}

inline CUtlString& CUtlString::operator+=( const CUtlString &rhs )
{
	Append( rhs );
	return *this;
}

inline CUtlString& CUtlString::operator+=( const char *rhs )
{
	Append( rhs );
	return *this;
}

inline CUtlString& CUtlString::operator+=( char c )
{
	char str[2] = { c, 0 };
	Append( str );
	return *this;
}

inline CUtlString& CUtlString::operator+=( int rhs )
{
	char str[12];
	V_snprintf( str, sizeof(str), "%d", rhs );
	Append( str );
	return *this;
}

inline CUtlString& CUtlString::operator+=( float rhs )
{
	char str[32];
	V_snprintf( str, sizeof(str), "%f", rhs );
	Append( str );
	return *this;
}

inline const char *CUtlString::Get() const
{
	if ( m_Storage.Count() == 0 )
		return NULL; // Be nice when we're empty
	return m_Storage.Base();
}

inline CUtlString::operator const char*() const
{
	return Get();
}

inline int CUtlString::Length() const
{
	if ( m_Storage.Count() == 0 )
		return 0;
	return V_strlen( Get() );
}

inline void CUtlString::Set( const char *pValue )
{
	if ( !pValue )
	{
		pValue = "";
	}
	int len = V_strlen( pValue ) + 1;
	m_Storage.Purge(); // Clear out the old string
	m_Storage.Grow( len ); // Make sure we have enough space
	V_strcpy_safe( m_Storage.Base(), len, pValue ); // Copy the new string
}

inline void CUtlString::Set( const CUtlString &value )
{
	Set( value.Get() );
}

inline void CUtlString::Set( const char *pValue, int nChars )
{
	if ( !pValue )
	{
		pValue = "";
	}
	int len = MIN( nChars, V_strlen( pValue ) ) + 1;
	m_Storage.Purge(); // Clear out the old string
	m_Storage.Grow( len ); // Make sure we have enough space
	V_strncpy_safe( m_Storage.Base(), len, pValue, len - 1 ); // Copy the new string
	m_Storage[len-1] = '\0'; // Null terminate
}

inline void CUtlString::Set( const CUtlString &value, int nChars )
{
	Set( value.Get(), nChars );
}

inline void CUtlString::Append( const char *pAddition )
{
	if ( !pAddition || !pAddition[0] )
		return;

	int oldLen = Length();
	int addLen = V_strlen( pAddition );
	int newLen = oldLen + addLen + 1;

	m_Storage.Grow( newLen ); // Make sure we have enough space
	V_strcat_safe( m_Storage.Base(), newLen, pAddition ); // Append the new string
}

inline void CUtlString::Append( const CUtlString &addition )
{
	Append( addition.Get() );
}

inline void CUtlString::Append( const char *pAddition, int nChars )
{
	if ( !pAddition || !pAddition[0] || nChars <= 0 )
		return;

	int oldLen = Length();
	int addLen = MIN( nChars, V_strlen( pAddition ) );
	int newLen = oldLen + addLen + 1;

	m_Storage.Grow( newLen ); // Make sure we have enough space
	V_strncat_safe( m_Storage.Base(), newLen, pAddition, addLen ); // Append the new string
	m_Storage[newLen-1] = '\0'; // Null terminate
}

inline void CUtlString::Append( const CUtlString &addition, int nChars )
{
	Append( addition.Get(), nChars );
}

inline void CUtlString::Format( const char *pFormat, ... )
{
	va_list marker;
	va_start( marker, pFormat );
	FormatV( pFormat, marker );
	va_end( marker );
}

inline void CUtlString::FormatV( const char *pFormat, va_list marker )
{
	// Make a guess at the CUtlString length. Max an existing CUtlString, or 1024.
	int len = MAX( Length(), 1024 );
	for (;;)
	{
		m_Storage.Grow( len );
		int nChars = V_vsnprintfRet( m_Storage.Base(), len, pFormat, marker );
		if ( nChars >= 0 && nChars < len )
		{
			// It fit.
			// Fix the length to not include the terminating null.
			// This is only necessary if vsnprintf is a non-standard version that doesn't
			// return the number of chars that would have been written.
			if ( nChars == 0 && Get()[0] == '\0' )
			{
				m_Storage.Purge(); // Clear out the string if it's empty
			}
			else
			{
				m_Storage.Base()[nChars] = '\0'; // Null terminate
			}
			break;
		}
		else
		{
			// It didn't fit. If it gave us a size, use that. Otherwise, double the size and try again.
			if ( nChars > 0 )
				len = nChars + 1;
			else
				len *= 2;
		}
	}
}

inline void CUtlString::ToLower()
{
	if ( IsEmpty() )
		return;
	V_strlower( Get() );
}

inline void CUtlString::ToUpper()
{
	if ( IsEmpty() )
		return;
	V_strupr( Get() );
}

inline void CUtlString::TrimLeft( char cTarget )
{
	if ( IsEmpty() )
		return;

	const char *pTrim = Get();
	while ( *pTrim && *pTrim == cTarget )
	{
		++pTrim;
	}
	Set( pTrim );
}

inline void CUtlString::TrimRight( char cTarget )
{
	if ( IsEmpty() )
		return;

	char *pTrim = Get();
	int len = Length();
	while ( len > 0 && pTrim[len-1] == cTarget )
	{
		--len;
	}
	pTrim[len] = '\0';
}

inline void CUtlString::Trim( char cTarget )
{
	TrimLeft( cTarget );
	TrimRight( cTarget );
}

inline bool CUtlString::IsEqual_CaseInsensitive( const char *pOther ) const
{
	if ( IsEmpty() )
		return !pOther || !pOther[0];
	if ( !pOther || !pOther[0] )
		return false;
	return V_stricmp( Get(), pOther ) == 0;
}

inline bool CUtlString::MatchesPattern( const CUtlString &Pattern, bool bCaseSensitive ) const
{
	return StringMatchesPattern( Get(), Pattern.Get(), bCaseSensitive );
}

inline void CUtlString::Replace( const char *pFind, const char *pReplace, bool bCaseSensitive )
{
	if ( IsEmpty() || !pFind || !pFind[0] ) // if our string is empty, or the find string is empty, do nothing
		return;

	if ( !pReplace ) // if the replace string is null, treat it as an empty string
		pReplace = "";

	int findLen = V_strlen( pFind );
	int replaceLen = V_strlen( pReplace );

	CUtlString temp; // temp string to build the result in
	const char *pScan = Get();
	while ( *pScan )
	{
		const char *pFound = bCaseSensitive ? V_strstr( pScan, pFind ) : V_stristr( pScan, pFind );
		if ( pFound )
		{
			// copy the part before the found string
			if ( pFound > pScan )
			{
				temp.Append( pScan, pFound - pScan );
			}
			// copy the replacement string
			temp.Append( pReplace );
			// advance the scan pointer past the found string
			pScan = pFound + findLen;
		}
		else
		{
			// copy the rest of the string
			temp.Append( pScan );
			break;
		}
	}
	*this = temp; // copy the result back to this string
}

inline void CUtlString::RemoveChar( char ch )
{
	if ( IsEmpty() )
		return;

	char *pDest = Get();
	const char *pScan = Get();
	while ( *pScan )
	{
		if ( *pScan != ch )
		{
			*pDest++ = *pScan;
		}
		++pScan;
	}
	*pDest = '\0';
}

inline void CUtlString::Remove( const char *pSubStr, bool bCaseSensitive )
{
	Replace( pSubStr, "", bCaseSensitive );
}

inline CUtlString CUtlString::Slice( int32 nStart, int32 nEnd ) const
{
	int len = Length();
	if ( nStart < 0 )
		nStart = len + nStart; // negative start is from end
	if ( nEnd == INT_MAX || nEnd > len )
		nEnd = len;
	if ( nStart >= len || nEnd <= nStart )
		return CUtlString(); // empty string if invalid range

	CUtlString result;
	result.Set( Get() + nStart, nEnd - nStart );
	return result;
}

inline CUtlString CUtlString::Right( int32 nChars ) const
{
	return Slice( Length() - nChars );
}

inline CUtlString CUtlString::Left( int32 nChars ) const
{
	return Slice( 0, nChars );
}

inline int CUtlString::FindChar( char ch, int nStart ) const
{
	if ( nStart < 0 || nStart >= Length() )
		return -1;
	const char *pFound = V_strnchr( Get() + nStart, ch, Length() - nStart );
	return pFound ? (pFound - Get()) : -1;
}

inline int CUtlString::Find( const char *pSubStr, int nStart, bool bCaseSensitive ) const
{
	if ( !pSubStr || !pSubStr[0] || nStart < 0 || nStart >= Length() )
		return -1;
	const char *pFound = bCaseSensitive ? V_strstr( Get() + nStart, pSubStr ) : V_stristr( Get() + nStart, pSubStr );
	return pFound ? (pFound - Get()) : -1;
}

inline void CUtlString::Clear()
{
	Set( "" );
}

inline void CUtlString::Purge()
{
	m_Storage.Purge();
}

inline CUtlString CUtlString::As generalmentePath() const
{
	CUtlString path = *this;
	if ( !path.IsEmpty() )
	{
		char lastChar = path[path.Length()-1];
		if ( lastChar != '\\' && lastChar != '/' )
		{
			path += CORRECT_PATH_SEPARATOR_CHAR;
		}
	}
	return path;
}

inline CUtlString CUtlString::GetFilename() const
{
	if ( IsEmpty() )
		return CUtlString();
	const char *pFilename = V_UnqualifiedFileName( Get() );
	CUtlString filename( pFilename );
	filename.StripExtension(); // remove extension if present
	return filename;
}

inline CUtlString CUtlString::GetExtension() const
{
	if ( IsEmpty() )
		return CUtlString();
	const char *pExt = V_GetFileExtension( Get() );
	return pExt ? CUtlString( pExt ) : CUtlString();
}

inline CUtlString CUtlString::GetPath() const
{
	if ( IsEmpty() )
		return CUtlString();
	char path[MAX_PATH];
	V_ExtractFilePath( Get(), path, sizeof(path) );
	return CUtlString( path );
}

inline void CUtlString::SetFilename( const char *pFilename )
{
	if ( IsEmpty() )
	{
		Set( pFilename );
		return;
	}
	char path[MAX_PATH];
	V_ExtractFilePath( Get(), path, sizeof(path) );
	char newPath[MAX_PATH];
	V_ComposeFileName( path, pFilename, newPath, sizeof(newPath) );
	Set( newPath );
}

inline void CUtlString::SetExtension( const char *pExtension )
{
	if ( IsEmpty() )
	{
		if ( pExtension && pExtension[0] )
		{
			CUtlString temp( "." );
			temp += pExtension;
			Set( temp );
		}
		return;
	}
	char newPath[MAX_PATH];
	V_SetExtension( Get(), pExtension, newPath, sizeof(newPath) );
	Set( newPath );
}

inline void CUtlString::SetPath( const char *pPath )
{
	if ( IsEmpty() )
	{
		Set( pPath );
		return;
	}
	char filename[MAX_PATH];
	V_FileBase( Get(), filename, sizeof(filename) ); // get filename with extension
	char newPath[MAX_PATH];
	V_ComposeFileName( pPath, filename, newPath, sizeof(newPath) );
	Set( newPath );
}

inline void CUtlString::StripFilename()
{
	if ( IsEmpty() )
		return;
	char path[MAX_PATH];
	V_ExtractFilePath( Get(), path, sizeof(path) );
	Set( path );
}

inline void CUtlString::StripExtension()
{
	if ( IsEmpty() )
		return;
	char path[MAX_PATH];
	V_StripExtension( Get(), path, sizeof(path) );
	Set( path );
}

inline void CUtlString::StripPath()
{
	if ( IsEmpty() )
		return;
	const char *pFilename = V_UnqualifiedFileName( Get() );
	Set( pFilename );
}

#endif // UTLSTRING_H
