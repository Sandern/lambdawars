//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: A simple class for performing safe and in-expression sprintf-style
//			string formatting
//
// $NoKeywords: $
//=============================================================================//

#ifndef FMTSTR_H
#define FMTSTR_H

#include <stdarg.h>
#include <stdio.h>
#include "tier0/platform.h"
#include "tier1/strtools.h"

#if defined( _WIN32 )
#pragma once
#endif

//=============================================================================

// using macro to be compatable with GCC
#define FmtStrVSNPrintf( szBuf, nBufSize, ppszFormat ) \
	do \
	{ \
		int     result; \
		va_list arg_ptr; \
	\
		va_start(arg_ptr, (*(ppszFormat))); \
		result = Q_vsnprintf((szBuf), (nBufSize)-1, (*(ppszFormat)), arg_ptr); \
		va_end(arg_ptr); \
	\
		(szBuf)[(nBufSize)-1] = 0; \
	} \
	while (0)


//-----------------------------------------------------------------------------
//
// Purpose: String formatter with specified size
//

template <int SIZE_BUF>
class CFmtStrN
{
public:
	CFmtStrN()									{ m_szBuf[0] = 0; }
	
	// Standard C formatting
	CFmtStrN(const char *pszFormat, ...) FMTFUNCTION( 2, 3 )
	{
		FmtStrVSNPrintf(m_szBuf, SIZE_BUF, &pszFormat);
	}

	// Use this for pass-through formatting
	CFmtStrN(const char ** ppszFormat, ...)
	{
		FmtStrVSNPrintf(m_szBuf, SIZE_BUF, ppszFormat);
	}

	// Explicit reformat
	const char *sprintf(const char *pszFormat, ...)	FMTFUNCTION( 2, 3 )
	{
		FmtStrVSNPrintf(m_szBuf, SIZE_BUF, &pszFormat); 
		return m_szBuf;
	}

	// Use this for pass-through formatting
	void VSprintf(const char **ppszFormat, ...)
	{
		FmtStrVSNPrintf(m_szBuf, SIZE_BUF, ppszFormat);
	}

	// Use for access
	operator const char *() const				{ return m_szBuf; }
	char *Access()								{ return m_szBuf; }
	CFmtStrN<SIZE_BUF> & operator=( const char *pchValue ) { sprintf( pchValue ); return *this; }
	CFmtStrN<SIZE_BUF> & operator+=( const char *pchValue ) { Append( pchValue ); return *this; }
	int Length() const { return V_strlen( m_szBuf ); }

	void Clear()								{ m_szBuf[0] = 0; }

	void AppendFormat( const char *pchFormat, ... ) { int nLength = Length(); char *pchEnd = m_szBuf + nLength; FmtStrVSNPrintf( pchEnd, SIZE_BUF - nLength, &pchFormat ); }
	void AppendFormatV( const char *pchFormat, va_list args );
	void Append( const char *pchValue ) { AppendFormat( pchValue ); }

	void AppendIndent( uint32 unCount, char chIndent = '\t' );
private:
	char m_szBuf[SIZE_BUF];
};


template< int SIZE_BUF >
void CFmtStrN<SIZE_BUF>::AppendIndent( uint32 unCount, char chIndent )
{
	int nLength = Length();
	if( nLength + unCount >= SIZE_BUF )
		unCount = SIZE_BUF - (1+nLength);
	for ( uint32 x = 0; x < unCount; x++ )
	{
		m_szBuf[ nLength++ ] = chIndent;
	}
	m_szBuf[ nLength ] = '\0';
}

template< int SIZE_BUF >
void CFmtStrN<SIZE_BUF>::AppendFormatV( const char *pchFormat, va_list args )
{
	int nLength = Length();
	V_vsnprintf( m_szBuf+nLength, SIZE_BUF - nLength, pchFormat, args );
}


//-----------------------------------------------------------------------------
//
// Purpose: Default-sized string formatter
//

#define FMTSTR_STD_LEN 256

typedef CFmtStrN<FMTSTR_STD_LEN> CFmtStr;
typedef CFmtStrN<1024> CFmtStr1024;
typedef CFmtStrN<8192> CFmtStrMax;


//-----------------------------------------------------------------------------
// Purpose: Fast-path number-to-string helper (with optional quoting)
//			Derived off of the Steam CNumStr but with a few tweaks, such as
//			trimming off the in-our-cases-unnecessary strlen calls (by not
//			storing the length in the class).
//-----------------------------------------------------------------------------

class CNumStr
{
public:
	CNumStr() { m_szBuf[0] = 0; }

	explicit CNumStr( bool b )		{ SetBool( b ); } 

	explicit CNumStr( int8 n8 )		{ SetInt8( n8 ); }
	explicit CNumStr( uint8 un8 )	{ SetUint8( un8 );  }
	explicit CNumStr( int16 n16 )	{ SetInt16( n16 ); }
	explicit CNumStr( uint16 un16 )	{ SetUint16( un16 );  }
	explicit CNumStr( int32 n32 )	{ SetInt32( n32 ); }
	explicit CNumStr( uint32 un32 )	{ SetUint32( un32 ); }
	explicit CNumStr( int64 n64 )	{ SetInt64( n64 ); }
	explicit CNumStr( uint64 un64 )	{ SetUint64( un64 ); }

#if defined(COMPILER_GCC) && defined(PLATFORM_64BITS)
	explicit CNumStr( lint64 n64 )		{ SetInt64( (int64)n64 ); }
	explicit CNumStr( ulint64 un64 )	{ SetUint64( (uint64)un64 ); }
#endif

	explicit CNumStr( double f )	{ SetDouble( f ); }
	explicit CNumStr( float f )		{ SetFloat( f ); }

	inline void SetBool( bool b )			{ Q_memcpy( m_szBuf, b ? "1" : "0", 2 ); } 

#ifdef _WIN32
	inline void SetInt8( int8 n8 )			{ _itoa( (int32)n8, m_szBuf, 10 ); }
	inline void SetUint8( uint8 un8 )		{ _itoa( (int32)un8, m_szBuf, 10 ); }
	inline void SetInt16( int16 n16 )		{ _itoa( (int32)n16, m_szBuf, 10 ); }
	inline void SetUint16( uint16 un16 )	{ _itoa( (int32)un16, m_szBuf, 10 ); }
	inline void SetInt32( int32 n32 )		{ _itoa( n32, m_szBuf, 10 ); }
	inline void SetUint32( uint32 un32 )	{ _i64toa( (int64)un32, m_szBuf, 10 ); }
	inline void SetInt64( int64 n64 )		{ _i64toa( n64, m_szBuf, 10 ); }
	inline void SetUint64( uint64 un64 )	{ _ui64toa( un64, m_szBuf, 10 ); }
#else
	inline void SetInt8( int8 n8 )			{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%d", (int32)n8 ); }
	inline void SetUint8( uint8 un8 )		{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%d", (int32)un8 ); }
	inline void SetInt16( int16 n16 )		{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%d", (int32)n16 ); }
	inline void SetUint16( uint16 un16 )	{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%d", (int32)un16 ); }
	inline void SetInt32( int32 n32 )		{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%d", n32 ); }
	inline void SetUint32( uint32 un32 )	{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%u", un32 ); }
	inline void SetInt64( int64 n64 )		{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%lld", n64 ); }
	inline void SetUint64( uint64 un64 )	{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%llu", un64 ); }
#endif

	inline void SetDouble( double f )		{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%.18g", f ); }
	inline void SetFloat( float f )			{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%.18g", f ); }

	inline void SetHexUint64( uint64 un64 )	{ Q_binarytohex( (byte *)&un64, sizeof( un64 ), m_szBuf, sizeof( m_szBuf ) ); }

	operator const char *() const { return m_szBuf; }
	const char* String() const { return m_szBuf; }
	
	void AddQuotes()
	{
		Assert( m_szBuf[0] != '"' );
		const int nLength = Q_strlen( m_szBuf );
		Q_memmove( m_szBuf + 1, m_szBuf, nLength );
		m_szBuf[0] = '"';
		m_szBuf[nLength + 1] = '"';
		m_szBuf[nLength + 2] = 0;
	}

protected:
	char m_szBuf[28]; // long enough to hold 18 digits of precision, a decimal, a - sign, e+### suffix, and quotes

};


//=============================================================================

#endif // FMTSTR_H
