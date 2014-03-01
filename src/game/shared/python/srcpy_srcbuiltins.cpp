//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "srcpy_srcbuiltins.h"
#include "srcpy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void RegisterTickMethod( boost::python::object method, float ticksignal, bool looped, bool userealtime ) 
{ 
	SrcPySystem()->RegisterTickMethod(method, ticksignal, looped, userealtime); 
}

void UnregisterTickMethod( boost::python::object method ) 
{ 
	SrcPySystem()->UnregisterTickMethod(method); 
}

boost::python::list GetRegisteredTickMethods() 
{ 
	return SrcPySystem()->GetRegisteredTickMethods(); 
}

bool IsTickMethodRegistered( boost::python::object method ) 
{ 
	return SrcPySystem()->IsTickMethodRegistered( method ); 
}

void RegisterPerFrameMethod( boost::python::object method ) 
{ 
	SrcPySystem()->RegisterPerFrameMethod(method); 
}

void UnregisterPerFrameMethod( boost::python::object method ) 
{ 
	SrcPySystem()->UnregisterPerFrameMethod(method); 
}

boost::python::list GetRegisteredPerFrameMethods() 
{ 
	return SrcPySystem()->GetRegisteredPerFrameMethods(); 
}

bool IsPerFrameMethodRegistered( boost::python::object method ) 
{ 
	return SrcPySystem()->IsPerFrameMethodRegistered( method ); 
}