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
	Msg( boost::python::extract<const tchar *>( boost::python::str( msg ) ) );
}

void SrcPyStdErr::write( boost::python::object msg ) 
{ 
	Warning( boost::python::extract<const tchar *>( boost::python::str( msg ) ) ); 
}