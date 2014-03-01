//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_SRCBUILTINS_H
#define SRCPY_SRCBUILTINS_H
#ifdef _WIN32
#pragma once
#endif

#include <tier0/dbg.h>
#include "srcpy_boostpython.h"

// These classes redirect input to Msg and Warning respectively
class SrcPyStdOut 
{
public:
	void write( boost::python::object msg )
	{
		if( PyUnicode_Check( msg.ptr() ) )
		{
			const wchar_t *pUniMsg = PyUnicode_AS_UNICODE( msg.ptr() );
			if( pUniMsg )
			{
				Msg( "%ls", pUniMsg );
			}
		}
		else
		{
			char* pMsg = PyString_AsString( msg.ptr() );
			if( pMsg == NULL )
				return;
			Msg( "%s", pMsg );
		}
	}

	void flush() {}
};

class SrcPyStdErr 
{
public:
	void write( boost::python::object msg )
	{
		if( PyUnicode_Check( msg.ptr() ) )
		{
			const wchar_t *pUniMsg = PyUnicode_AS_UNICODE( msg.ptr() );
			if( pUniMsg )
			{
				Warning( "%ls", pUniMsg );
			}
		}
		else
		{
			char* pMsg = PyString_AsString( msg.ptr() );
			if( pMsg )
			{
				Warning( "%s", pMsg );
			}
		}
	}

	void flush() {}
};

// Wrappers for Msg, Warning and DevMsg (Python does not use VarArgs)
inline void SrcPyMsg( boost::python::object msg ) 
{ 
	if( PyUnicode_Check( msg.ptr() ) )
	{
		const wchar_t *pUniMsg = PyUnicode_AS_UNICODE( msg.ptr() );
		if( pUniMsg )
		{
			Msg( "%ls", pUniMsg );
		}
	}
	else
	{
		char* pMsg = PyString_AsString( msg.ptr() );
		if( pMsg )
		{
			Msg( "%s", pMsg );
		}
	}
}

inline void SrcPyWarning( boost::python::object msg ) 
{ 
	if( PyUnicode_Check( msg.ptr() ) )
	{
		const wchar_t *pUniMsg = PyUnicode_AS_UNICODE( msg.ptr() );
		if( pUniMsg )
		{
			Warning( "%ls", pUniMsg );
		}
	}
	else
	{
		char* pMsg = PyString_AsString( msg.ptr() );
		if( pMsg )
		{
			Warning( "%s", pMsg );
		}
	}
}

inline void SrcPyDevMsg( int level, boost::python::object msg ) 
{ 
	if( PyUnicode_Check( msg.ptr() ) )
	{
		const wchar_t *pUniMsg = PyUnicode_AS_UNICODE( msg.ptr() );
		if( pUniMsg )
		{
			DevMsg( level, "%ls", pUniMsg );
		}
	}
	else
	{
		char* pMsg = PyString_AsString( msg.ptr() );
		if( pMsg )
		{
			DevMsg( level, "%s", pMsg );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline void PyCOM_TimestampedLog( boost::python::object msg )
{
	if( PyUnicode_Check( msg.ptr() ) )
	{
		const wchar_t *pUniMsg = PyUnicode_AS_UNICODE( msg.ptr() );
		if( pUniMsg )
		{
			COM_TimestampedLog( "%ls", pUniMsg );
		}
	}
	else
	{
		char* pMsg = PyString_AsString( msg.ptr() );
		if( pMsg )
		{
			COM_TimestampedLog( "%s", pMsg );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RegisterTickMethod( boost::python::object method, float ticksignal, bool looped = true, bool userealtime = false );
void UnregisterTickMethod( boost::python::object method );
boost::python::list GetRegisteredTickMethods();
bool IsTickMethodRegistered( boost::python::object method );

void RegisterPerFrameMethod( boost::python::object method );
void UnregisterPerFrameMethod( boost::python::object method );
boost::python::list GetRegisteredPerFrameMethods();
bool IsPerFrameMethodRegistered( boost::python::object method );

#endif // SRCPY_SRCBUILTINS_H