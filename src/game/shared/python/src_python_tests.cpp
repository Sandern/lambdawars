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
	//Msg("SrcPyTest_EntityArg: %d\n", pEntity);
	if( pEntity == NULL )
		return;
	//Msg("Entity pointer is not NULL\n");
	if( dynamic_cast<CBaseEntity *>(pEntity) == NULL )
	{
		PyErr_SetString(PyExc_Exception, "Invalid entity pointer passed in as argument. Converter bug?" );
		throw boost::python::error_already_set(); 
		return;
	}
}

void SrcPyTest_NCrossProducts( int n, Vector &a, Vector &b )
{
	int i;
	for( i=0; i<n; i++ )
		Vector c = a.Cross(b);
}