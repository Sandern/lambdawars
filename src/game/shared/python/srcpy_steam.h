//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_STEAM_H
#define SRCPY_STEAM_H

#ifdef _WIN32
#pragma once
#endif

#include "steam/steam_api.h"
#include "steam/isteammatchmaking.h"

// Generic macro for callbacks in Python
#define PY_STEAM_CALLBACK_WRAPPER( name, dataclass ) \
class name##Callback \
{ \
public: \
	name##Callback( SteamAPICall_t steamapicall ) \
	{ \
		m_CallResult.Set( steamapicall, this, &name##Callback::On##name##Internal ); \
	} \
	virtual void On##name( dataclass *data, bool iofailure ) \
	{ \
	} \
private: \
	void On##name##Internal( dataclass *data, bool bIOFailure ) \
	{ \
		On##name( data, bIOFailure ); \
	} \
	CCallResult<name##Callback, dataclass> m_CallResult; \
};

// Wrapper functions
boost::python::tuple PyGetLobbyDataByIndex( CSteamID steamIDLobby, int iLobbyData );

#endif // SRCPY_STEAM_H