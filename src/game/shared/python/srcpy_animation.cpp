//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy_animation.h"
#include "bone_setup.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

boost::python::object Py_GetSeqdescActivityName( const mstudioseqdesc_t &pstudiohdr )
{
	const char *pActivityName = pstudiohdr.pszActivityName();
	return boost::python::object( pActivityName );
}

boost::python::object Py_GetSeqdescLabel( const mstudioseqdesc_t &pstudiohdr )
{
	const char *pLabel = pstudiohdr.pszLabel();
	return boost::python::object( pLabel );
}

float Py_Studio_GetMass( CStudioHdr *pstudiohdr )
{
	if( !pstudiohdr )
		return 0.0;
	return Studio_GetMass(pstudiohdr);
}