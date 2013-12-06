//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "srcpy_srcbuiltins.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void SrcPyStdOut::write( boost::python::object msg )
{
	const char *pMsg = boost::python::extract< const char * >( msg );
	Msg( "%s", pMsg );
}

void SrcPyStdErr::write( boost::python::object msg ) 
{ 
	const char *pMsg = boost::python::extract< const char * >( msg );
	Warning( "%s", pMsg ); 
}