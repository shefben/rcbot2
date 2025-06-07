//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef BITBUF_H
#define BITBUF_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier0/basetypes.h" // For uintp


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class Vector;
class QAngle;


//-----------------------------------------------------------------------------
// Usually, we only allow reliable messages to be > 4k. Otherwise, it's an error.
//-----------------------------------------------------------------------------
#define NET_MAX_PAYLOAD			( 262144 - 4 )		// largest message was 262140 for sv_consistency
#define NET_MAX_PAYLOAD_BITS	( 18 )				// ( 1 << 18 ) = 262144 (bytes)

// Max # of datagrams to send before we error out.
#define MAX_DATAGRAMS_RETURNED 4096

// Max # of bits for a user message
#define MAX_USER_MSG_BITS 12
#define MAX_USER_MSG_DATA ( ( 1 << ( MAX_USER_MSG_BITS - 3 ) ) - 1 )


//-----------------------------------------------------------------------------
// Bit-Packing functions
//-----------------------------------------------------------------------------

// This is a helper function to align a value to the next byte boundary.
inline int BitByteAlign( int bits )
{
	return ( bits + 7 ) >> 3;
}

// This is a helper function to get the number of bits for a given number of bytes.
inline int BytesToBits( int bytes )
{
	return bytes << 3;
}

// This is a helper function to get the number of bytes for a given number of bits.
inline int BitsToBytes( int bits )
{
	return ( bits + 7 ) >> 3;
}


//-----------------------------------------------------------------------------
// namespaced helpers
//-----------------------------------------------------------------------------
namespace bitbuf
{
	// ZigZag Transform functions, used for VarInt encoding.
	// These functions map signed integers to unsigned integers in a way that small
	// signed values (around zero) are mapped to small unsigned values.
	// This is useful because VarInt encoding is more efficient for small unsigned values.
	inline uint32 ZigZagEncode32( int32 n )
	{
		return ( n << 1 ) ^ ( n >> 31 );
	}

	inline int32 ZigZagDecode32( uint32 n )
	{
		return ( n >> 1 ) ^ ( -(int32)( n & 1 ) );
	}

	inline uint64 ZigZagEncode64( int64 n )
	{
		return ( n << 1 ) ^ ( n >> 63 );
	}

	inline int64 ZigZagDecode64( uint64 n )
	{
		return ( n >> 1 ) ^ ( -(int64)( n & 1 ) );
	}
}


//-----------------------------------------------------------------------------
// Base interface for bit buffer writing
//-----------------------------------------------------------------------------
class bf_write
{
public:
	bf_write();

	// nMaxBits can be used as the number of bits in the buffer.
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	bf_write( void *pData, int nBytes, int nMaxBits = -1 );
	bf_write( const char *pDebugName, void *pData, int nBytes, int nMaxBits = -1 );

	// Start writing to the buffer.
	// Will also go to the start of the buffer.
	void			StartWriting( void *pData, int nBytes, int iStartBit = 0, int nMaxBits = -1 );

	// Restart buffer writing.
	void			Reset();

	// Get the base structure.
	unsigned char*	GetBasePointer() { return m_pData; }

	// Get the data that's been written in bits.
	int				GetNumBitsWritten() const;
	// Get the data that's been written in bytes.
	int				GetNumBytesWritten() const;
	// Get the max number of bits.
	int				GetMaxNumBits();
	// Get the number of bits left.
	int				GetNumBitsLeft();
	// Get the number of bytes left.
	int				GetNumBytesLeft();
	// Is the buffer overflowed?
	bool			IsOverflowed() const;
	// Does the buffer have an error?
	bool			HasError() const;
	// Sets the assert on overflow.
	void			SetAssertOnOverflow( bool bAssert );
	// Get the name of the buffer.
	const char		*GetDebugName();

	// Debugging. This will print out the contents of the buffer to the console.
	void			PrintState();

	// Seek to a specific position.
	void			SeekToBit( int bit );

	// Write a single bit.
	void			WriteOneBit(int nValue);
	// Write a 0.
	void			WriteOneBitNoCheck(int nValue);
	// Write a 1.
	void			WriteOneBitNoCheck();

	// Write a number of bits.
	void			WriteUBitLong(unsigned int data, int numbits, bool bCheckRange=true);
	// Write a number of bits. (doesn't check range)
	void			WriteUBitLongNoRange(unsigned int data, int numbits);
	// Write a number of bits. (signed)
	void			WriteSBitLong(int data, int numbits);

	// Write a number of bits. (varint)
	void			WriteVarInt32( uint32 data );
	void			WriteVarInt64( uint64 data );
	void			WriteSignedVarInt32( int32 data );
	void			WriteSignedVarInt64( int64 data );
	int				ByteSizeVarInt32( uint32 data );
	int				ByteSizeVarInt64( uint64 data );
	int				ByteSizeSignedVarInt32( int32 data );
	int				ByteSizeSignedVarInt64( int64 data );

	// Write a boolean.
	void			WriteBool(bool bValue) { WriteUBitLong(bValue, 1); }
	// Write a character.
	void			WriteChar(int val);
	// Write a byte.
	void			WriteByte(int val);
	// Write a short.
	void			WriteShort(int val);
	// Write a word.
	void			WriteWord(int val);
	// Write a long.
	void			WriteLong(long val);
	// Write a long long.
	void			WriteLongLong(int64 val);
	// Write a float.
	void			WriteFloat(float val);
	// Write a an angle.
	void			WriteAngle(float fAngle, int numbits);
	// Write a an angle (degrees).
	void			WriteAngle(const QAngle& fa, int numbits);
	// Write a normal.
	void			WriteNormal(float f);
	// Write a vector (x,y,z).
	void			WriteVector(const Vector& v);
	// Write a vector (x,y).
	void			WriteVector2D(const Vector2D& v);
	// Write a an QAngle (x,y,z).
	void			WriteQAngle(const QAngle& fa);
	// Write a string.
	bool			WriteString(const char *pStr, int maxLen=-1, bool bWriteLen = true);

	// Write a bit string. (chars)
	void			WriteBits(const void *pIn, int nBits);
	// Write a bit string. (bytes)
	void			WriteBytes(const void *pIn, int nBytes);

	// Append the contents of another bit buffer.
	// pIn can be a bf_read or a bf_write.
	// If the buffer is too small to hold the new data, it will be truncated.
	// Returns false if the buffer is overflowed.
	bool			WriteBitsFromBuffer( class bf_read *pIn, int nBits );

protected:
	// The current buffer.
	unsigned char*	m_pData;
	// The number of bytes in the buffer.
	int				m_nDataBytes;
	// The number of bits in the buffer. If -1, then it's nBytes * 8.
	int				m_nDataBits;
	// The current writing position in bits.
	int				m_iCurBit;
	// The overflow flag.
	bool			m_bOverflow;
	// The assert on overflow flag.
	bool			m_bAssertOnOverflow;
	// The debug name of the buffer.
	const char		*m_pDebugName;
};


//-----------------------------------------------------------------------------
// Base interface for bit buffer reading
//-----------------------------------------------------------------------------
class bf_read
{
public:
	bf_read();

	// nMaxBits can be used as the number of bits in the buffer.
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	bf_read( const void *pData, int nBytes, int nBits = -1 );
	bf_read( const char *pDebugName, const void *pData, int nBytes, int nBits = -1 );

	// Start reading from the buffer.
	// Will also go to the start of the buffer.
	void			StartReading( const void *pData, int nBytes, int iStartBit = 0, int nBits = -1 );

	// Restart buffer reading.
	void			Reset();

	// Get the base structure.
	const unsigned char*	GetBasePointer() { return m_pData; }

	// Get the data that's been read in bits.
	int				GetNumBitsRead() const;
	// Get the data that's been read in bytes.
	int				GetNumBytesRead() const;
	// Get the max number of bits.
	int				GetMaxNumBits();
	// Get the number of bits left.
	int				GetNumBitsLeft();
	// Get the number of bytes left.
	int				GetNumBytesLeft();
	// Is the buffer overflowed?
	bool			IsOverflowed() const;
	// Does the buffer have an error?
	bool			HasError() const;
	// Sets the assert on overflow.
	void			SetAssertOnOverflow( bool bAssert );
	// Get the name of the buffer.
	const char		*GetDebugName();

	// Debugging. This will print out the contents of the buffer to the console.
	void			PrintState();

	// Seek to a specific position.
	// This is only safe if you are seeking forwards.
	// If you are seeking backwards, you may read data that has already been read.
	// Returns false if the seek is out of bounds.
	bool			Seek(int iBit);
	// Seek to a specific position relative to the current position.
	// This is only safe if you are seeking forwards.
	// If you are seeking backwards, you may read data that has already been read.
	// Returns false if the seek is out of bounds.
	bool			SeekRelative(int iBitDelta);

	// Read a single bit.
	int				ReadOneBit();
	// Read a number of bits.
	unsigned int	ReadUBitLong( int numbits );
	// Read a number of bits (signed)
	int				ReadSBitLong( int numbits );

	// Read a number of bits (varint)
	uint32			ReadVarInt32();
	uint64			ReadVarInt64();
	int32			ReadSignedVarInt32();
	int64			ReadSignedVarInt64();

	// Read a boolean.
	bool			ReadBool() { return ReadUBitLong(1) != 0; }
	// Read a character.
	int				ReadChar();
	// Read a byte.
	int				ReadByte();
	// Read a short.
	int				ReadShort();
	// Read a word.
	int				ReadWord();
	// Read a long.
	long			ReadLong();
	// Read a long long.
	int64			ReadLongLong();
	// Read a float.
	float			ReadFloat();
	// Read a an angle.
	float			ReadAngle(int numbits);
	// Read a an angle (degrees).
	void			ReadAngle(QAngle& fa, int numbits);
	// Read a normal.
	float			ReadNormal();
	// Read a vector (x,y,z).
	void			ReadVector(Vector& v);
	// Read a vector (x,y).
	void			ReadVector2D(Vector2D& v);
	// Read a an QAngle (x,y,z).
	void			ReadQAngle(QAngle& fa);
	// Read a string.
	// Returns false if the buffer is overflowed.
	// If the buffer is overflowed, the string will be truncated.
	// If bWithBom is true, it will skip the UTF-8 BOM if present.
	bool			ReadString( char *pStr, int maxLen, bool bLine=false, int *pOutNumChars=NULL );

	// Read a bit string. (chars)
	// Returns false if the buffer is overflowed.
	// If the buffer is overflowed, the string will be truncated.
	bool			ReadBits(void *pOut, int nBits);
	// Read a bit string. (bytes)
	// Returns false if the buffer is overflowed.
	// If the buffer is overflowed, the string will be truncated.
	bool			ReadBytes(void *pOut, int nBytes);

	// Returns the current position in the buffer, in bits.
	// This is useful for saving the current position and then restoring it later.
	int				Tell() const { return m_iCurBit; }

	// Returns the total number of bits in the buffer.
	int				TotalBytesAvailable( void ) const { return m_nDataBytes; }

protected:
	// The current buffer.
	const unsigned char*	m_pData;
	// The number of bytes in the buffer.
	int				m_nDataBytes;
	// The number of bits in the buffer. If -1, then it's nBytes * 8.
	int				m_nDataBits;
	// The current reading position in bits.
	int				m_iCurBit;
	// The overflow flag.
	bool			m_bOverflow;
	// The assert on overflow flag.
	bool			m_bAssertOnOverflow;
	// The debug name of the buffer.
	const char		*m_pDebugName;
};


//-----------------------------------------------------------------------------
// bf_write inline functions
//-----------------------------------------------------------------------------
inline bf_write::bf_write()
{
	m_pData = NULL;
	m_nDataBytes = 0;
	m_nDataBits = -1; // set to -1 so we calculate it later
	m_iCurBit = 0;
	m_bOverflow = false;
	m_bAssertOnOverflow = true;
	m_pDebugName = NULL;
}

inline bf_write::bf_write( void *pData, int nBytes, int nMaxBits )
{
	m_bAssertOnOverflow = true;
	m_pDebugName = NULL;
	StartWriting( pData, nBytes, 0, nMaxBits );
}

inline bf_write::bf_write( const char *pDebugName, void *pData, int nBytes, int nMaxBits )
{
	m_bAssertOnOverflow = true;
	m_pDebugName = pDebugName;
	StartWriting( pData, nBytes, 0, nMaxBits );
}

inline void bf_write::StartWriting( void *pData, int nBytes, int iStartBit, int nMaxBits )
{
	// Make sure we have a valid buffer
	Assert( pData );
	Assert( nBytes >= 0 );
	Assert( iStartBit >= 0 );
	Assert( nMaxBits == -1 || nMaxBits <= nBytes * 8 );

	m_pData = (unsigned char*)pData;
	m_nDataBytes = nBytes;

	if (nMaxBits == -1)
	{
		m_nDataBits = nBytes << 3;
	}
	else
	{
		m_nDataBits = nMaxBits;
	}

	m_iCurBit = iStartBit;
	m_bOverflow = false;
}

inline void bf_write::Reset()
{
	m_iCurBit = 0;
	m_bOverflow = false;
}

inline int bf_write::GetNumBitsWritten() const
{
	return m_iCurBit;
}

inline int bf_write::GetNumBytesWritten() const
{
	return BitsToBytes(m_iCurBit);
}

inline int bf_write::GetMaxNumBits()
{
	return m_nDataBits;
}

inline int bf_write::GetNumBitsLeft()
{
	return m_nDataBits - m_iCurBit;
}

inline int bf_write::GetNumBytesLeft()
{
	return GetNumBitsLeft() >> 3;
}

inline bool bf_write::IsOverflowed() const
{
	return m_bOverflow;
}

inline bool bf_write::HasError() const
{
	return m_bOverflow;
}

inline void bf_write::SetAssertOnOverflow( bool bAssert )
{
	m_bAssertOnOverflow = bAssert;
}

inline const char *bf_write::GetDebugName()
{
	return m_pDebugName;
}

inline void bf_write::SeekToBit( int bit )
{
	m_iCurBit = bit;
}


//-----------------------------------------------------------------------------
// bf_read inline functions
//-----------------------------------------------------------------------------
inline bf_read::bf_read()
{
	m_pData = NULL;
	m_nDataBytes = 0;
	m_nDataBits = -1; // set to -1 so we calculate it later
	m_iCurBit = 0;
	m_bOverflow = false;
	m_bAssertOnOverflow = true;
	m_pDebugName = NULL;
}

inline bf_read::bf_read( const void *pData, int nBytes, int nBits )
{
	m_bAssertOnOverflow = true;
	m_pDebugName = NULL;
	StartReading( pData, nBytes, 0, nBits );
}

inline bf_read::bf_read( const char *pDebugName, const void *pData, int nBytes, int nBits )
{
	m_bAssertOnOverflow = true;
	m_pDebugName = pDebugName;
	StartReading( pData, nBytes, 0, nBits );
}

inline void bf_read::StartReading( const void *pData, int nBytes, int iStartBit, int nBits )
{
	// Make sure we have a valid buffer
	Assert( pData );
	Assert( nBytes >= 0 );
	Assert( iStartBit >= 0 );
	Assert( nBits == -1 || nBits <= nBytes * 8 );

	m_pData = (const unsigned char*)pData;
	m_nDataBytes = nBytes;

	if (nBits == -1)
	{
		m_nDataBits = nBytes << 3;
	}
	else
	{
		m_nDataBits = nBits;
	}

	m_iCurBit = iStartBit;
	m_bOverflow = false;
}

inline void bf_read::Reset()
{
	m_iCurBit = 0;
	m_bOverflow = false;
}

inline int bf_read::GetNumBitsRead() const
{
	return m_iCurBit;
}

inline int bf_read::GetNumBytesRead() const
{
	return BitsToBytes(m_iCurBit);
}

inline int bf_read::GetMaxNumBits()
{
	return m_nDataBits;
}

inline int bf_read::GetNumBitsLeft()
{
	return m_nDataBits - m_iCurBit;
}

inline int bf_read::GetNumBytesLeft()
{
	return GetNumBitsLeft() >> 3;
}

inline bool bf_read::IsOverflowed() const
{
	return m_bOverflow;
}

inline bool bf_read::HasError() const
{
	return m_bOverflow;
}

inline void bf_read::SetAssertOnOverflow( bool bAssert )
{
	m_bAssertOnOverflow = bAssert;
}

inline const char *bf_read::GetDebugName()
{
	return m_pDebugName;
}

inline bool bf_read::Seek(int iBit)
{
	if ( iBit < 0 || iBit > m_nDataBits )
	{
		m_bOverflow = true;
		return false;
	}
	m_iCurBit = iBit;
	return true;
}

inline bool bf_read::SeekRelative(int iBitDelta)
{
	return Seek( m_iCurBit + iBitDelta );
}


#endif // BITBUF_H
