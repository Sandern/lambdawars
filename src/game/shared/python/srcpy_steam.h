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

// Generic macros for callbacks in Python
#define PY_STEAM_CALLBACK_WRAPPER( name, dataclass ) \
class name##Callback \
{ \
public: \
	name##Callback() : m_Callback( this, &name##Callback::On##name##Internal ) \
	{ \
	} \
	virtual void On##name( dataclass *data ) \
	{ \
	} \
private: \
	STEAM_CALLBACK( name##Callback, On##name##Internal, dataclass, m_Callback ); \
}; \
void name##Callback::On##name##Internal( dataclass *data ) \
{ \
	On##name( data ); \
}

#define PY_STEAM_CALLRESULT_WRAPPER( name, dataclass ) \
class name##CallResult \
{ \
public: \
	name##CallResult( SteamAPICall_t steamapicall ) \
	{ \
		m_CallResult.Set( steamapicall, this, &name##CallResult::On##name##Internal ); \
	} \
	virtual void On##name( dataclass *data, bool iofailure ) \
	{ \
	} \
private: \
	void On##name##Internal( dataclass *data, bool bIOFailure ) \
	{ \
		On##name( data, bIOFailure ); \
	} \
	CCallResult<name##CallResult, dataclass> m_CallResult; \
};

// Wrapper functions
boost::python::tuple PyGetLobbyDataByIndex( CSteamID steamIDLobby, int iLobbyData );
bool PySendLobbyChatMsg( CSteamID steamIDLobby, const char *pvMsgBody, int cubMsgBody );

#endif // SRCPY_STEAM_H