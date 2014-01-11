//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
//=============================================================================//

#include "cbase.h"
#include "srcpy_steam.h"
#include "srcpy_boostpython.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

boost::python::tuple PyGetLobbyDataByIndex( CSteamID steamIDLobby, int iLobbyData )
{
	char key[1024];
	char value[1024];

	if( !steamapicontext->SteamMatchmaking() )
	{
		PyErr_SetString(PyExc_Exception, "No steam matchmaking API available!" );
		throw boost::python::error_already_set(); 
	}

	bool ret = steamapicontext->SteamMatchmaking()->GetLobbyDataByIndex( steamIDLobby, iLobbyData, key, sizeof(key), value, sizeof(value) );

	return boost::python::make_tuple( ret, key, value );
}

