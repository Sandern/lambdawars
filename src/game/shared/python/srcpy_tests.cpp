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
bool SrcPyTest_EntityArg( CBaseEntity *pEntity )
{
	if( pEntity == NULL )
	{
		Msg("Entity argument is null\n");
		return false;
	}
	if( dynamic_cast<CBaseEntity *>(pEntity) == NULL )
	{
		PyErr_SetString( PyExc_Exception, "Invalid entity pointer passed in as argument. Converter bug?" );
		throw boost::python::error_already_set(); 
		return false;
	}
	// Call a function on the entity
	Msg( "Entindex: %d\n", pEntity->entindex() );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Extracts a CBaseEntity from a python object
//-----------------------------------------------------------------------------
bool SrcPyTest_ExtractEntityArg( boost::python::object entity )
{
	CBaseEntity *pEntity = boost::python::extract< CBaseEntity * >( entity );

	if( pEntity != NULL )
	{
		if( dynamic_cast<CBaseEntity *>(pEntity) == NULL )
		{
			PyErr_SetString( PyExc_Exception, "Invalid entity pointer passed in as argument. Converter bug?" );
			throw boost::python::error_already_set(); 
			return false;
		}

		// Call a function on the entity
		Msg( "Entindex: %d\n", pEntity->entindex() );
	}
	else
	{
		Msg("Extracted entity is null\n");
	}
	return pEntity != NULL;
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