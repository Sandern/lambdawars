//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Used to test the entity converters between python and c++
//-----------------------------------------------------------------------------
void SrcPyTest_EntityArg( CBaseEntity *pEntity )
{
	if( pEntity == NULL )
		return;
	if( dynamic_cast<CBaseEntity *>(pEntity) == NULL )
	{
		PyErr_SetString( PyExc_Exception, "Invalid entity pointer passed in as argument. Converter bug?" );
		throw boost::python::error_already_set(); 
		return;
	}
	// Call a function on the entity
	pEntity->entindex();
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CON_COMMAND_F( srctests_booloperator, "", FCVAR_CHEAT )
{
	// FIXME: There seems to be a problem with the bool operator
	//		  On the client, this can result in a crash.
	// See src\boost\boost\python\object_operators.hpp
	try
	{
		boost::python::object testvalue = boost::python::object( boost::python::exec( "def testmethod(): pass" ) );
		if( testvalue != boost::python::object() )
		{
			Msg("Tested value is not None\n");
		}
		else
		{
			Msg("Tested value is None\n");
		}
	}
	catch( ... )
	{
		PyErr_Print();
	}
}
#endif // CLIENT_DLL