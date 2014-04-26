//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_CLASS_SHARED_H
#define SRCPY_CLASS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "srcpy_boostpython.h"

//-----------------------------------------------------------------------------
// Shared between server and client class
// When you add a new type:
// 1. Add DECLARE_PYSERVERCLASS and DECLARE_PYCLIENTCLASS to the headers of the class
// 2. Add IMPLEMENT_PYSERVERCLASS and IMPLEMENT_PYCLIENTCLASS to the cpp files of the class
// 3. Link recv and send tables in SetupClientClassRecv and SetupServerClass to the type
// 4. Add a fall back factory in ClientClassFactory.
// 5. Regenerate bindings
//-----------------------------------------------------------------------------
enum PyNetworkTypes
{
	PN_NONE = 0,
	PN_BASEENTITY,
	PN_BASEANIMATING,
	PN_BASEANIMATINGOVERLAY,
	PN_BASEFLEX,
	PN_BASECOMBATCHARACTER,
	PN_BASEPLAYER,
	PN_BASEPROJECTILE,
	PN_BASEGRENADE,
	PN_BASECOMBATWEAPON,
	PN_PLAYERRESOURCE,
	PN_BREAKABLEPROP,
	PN_BASETOGGLE,
	PN_BASETRIGGER,
	PN_SPRITE,
	PN_SMOKETRAIL,
	PN_BEAM,

#ifdef HL2WARS_DLL
	PN_HL2WARSPLAYER,
	PN_UNITBASE,
	PN_FUNCUNIT,
	PN_WARSWEAPON,
	PN_BASEFUNCMAPBOUNDARY,
#endif // HL2WARS_DLL
};

boost::python::object CreatePyHandleHelper( const CBaseEntity *pEnt, const char *handlename );

//-----------------------------------------------------------------------------
// Purpose: Implement a python class. For python/c++ handle conversion
//-----------------------------------------------------------------------------
#define DECLARE_PYCLASS_HELPER( name )																\
	public:																							\
	virtual boost::python::object CreatePyHandle( void ) const										\
{																									\
	return CreatePyHandleHelper(this, #name "HANDLE");												\
}

#define DECLARE_PYCLASS( name ) DECLARE_PYCLASS_HELPER( name )

#endif // SRCPY_CLASS_SHARED_H

