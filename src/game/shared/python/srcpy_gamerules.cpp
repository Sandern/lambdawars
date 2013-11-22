//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy_gamerules.h"
#include "srcpy.h"
#include "hl2wars_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


namespace bp = boost::python;

bp::object g_pyGameRules;

#ifndef CLIENT_DLL
#include "networkstringtable_gamedll.h"
extern INetworkStringTable *g_StringTableGameRules;
#endif // CLIENT_DLL

static bool bLockGamerulesCreation = false;
void PyInstallGameRulesInternal( boost::python::object gamerules )
{
	// Gamerules == None
	// Destroy gamerules and install default
	if( gamerules.ptr() == Py_None )
	{
		if( g_pyGameRules.ptr() != Py_None )
		{
			ClearPyGameRules();
			CGameRulesRegister::FindByName("CHL2WarsGameRules")->CreateGameRules();
		}
		return;
	}

	// Verify the gamerules class is of the right type
	bp::object issubclass;
	bp::object cgamerules;
	try {
		issubclass = builtins.attr("issubclass");
		cgamerules = bp::import("_gamerules").attr("CHL2WarsGameRules");

		bool bIsSubclass = bp::extract<bool>( issubclass(gamerules, cgamerules) );
		if( bIsSubclass == false ) {
			Warning("InstallGamerules: gamerules class not a subclass of CGameRules\n");
			return;
		}
	} catch(bp::error_already_set &) {
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	// Clear old
	if( g_pyGameRules.ptr() != Py_None )
		ClearPyGameRules();
	else
		delete g_pGameRules;
	g_pGameRules = NULL;

	// Create the gamerules
	CHL2WarsGameRules *pRules = NULL;
	bp::object inst;
	try {
		inst = gamerules();
		g_pyGameRules = inst;		// This keeps the python gamerules instance alive
		pRules = bp::extract<CHL2WarsGameRules *>(g_pyGameRules);
		if( !pRules ) {
			Warning("Gamerules not valid\n");
			CGameRulesRegister::FindByName("CHL2WarsGameRules")->CreateGameRules();
			return;
		}
	} catch(bp::error_already_set &) {
		PyErr_Print();
		PyErr_Clear();
		CGameRulesRegister::FindByName("CHL2WarsGameRules")->CreateGameRules();
		return;
	}

	// Assign new
	g_pGameRules = pRules;
	IGameSystem::Add( g_pGameRules );

#ifndef CLIENT_DLL
	// Make sure the client gets a notification to make a new game rules object.
	Assert( g_StringTableGameRules );
	g_StringTableGameRules->AddString( true, "classname", strlen( "CHL2WarsGameRules" ) + 1, "CHL2WarsGameRules" );

	if ( g_pGameRules )
	{
		g_pGameRules->CreateCustomNetworkStringTables();
	}
#endif // CLIENT_DLL

	SrcPySystem()->Run( SrcPySystem()->Get("_PreInitGamerules", "core.gamerules.info") );

	// Tell new rules it may initialize
	if( g_pyGameRules.ptr() != Py_None )
		pRules->InitGamerules();
}

void PyInstallGameRules( boost::python::object gamerules )
{
	if( bLockGamerulesCreation )
	{
		Warning("Cannot install new gamerules while already installing gamerules\n");
		return;
	}

	// In case we are changed in a gamerules function, we should make sure we are not killed
	SrcPySystem()->AddToDeleteList(g_pyGameRules);

	bLockGamerulesCreation = true;
	PyInstallGameRulesInternal(gamerules);
	bLockGamerulesCreation = false;
}

boost::python::object PyGameRules()
{
	return g_pyGameRules;
}

void ClearPyGameRules()
{
	if( !g_pGameRules ) 
		return;

	if( g_pyGameRules.ptr() == Py_None ) 
	{
		Assert(0);
		return;
	}

	// If we have some rules installed already, tell it's going down
	static_cast<CHL2WarsGameRules *>(g_pGameRules)->ShutdownGamerules();
	g_pGameRules->Remove( g_pGameRules ); // Remove from gamesystem, do not update anymore.
	SrcPySystem()->AddToDeleteList(g_pyGameRules); // Might be deleted during a think call.
	g_pyGameRules = boost::python::object();
	g_pGameRules = NULL;
}