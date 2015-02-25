//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WARS_NETWORK_H
#define WARS_NETWORK_H

#ifdef _WIN32
#pragma once
#endif

const char *WarsNet_TranslateP2PConnectErr( int errorCode );

#ifdef ENABLE_PYTHON

#include "wars_matchmaking.h"

enum WarsNetType_e {
	WARSNET_ERROR = 0,
	WARSNET_ENTVAR,
	WARSNET_ENTVAR_CC, // Change callback
	WARSNET_INT,
	WARSNET_FLOAT,
	WARSNET_STRING,
	WARSNET_BOOL,
	WARSNET_NONE,
	WARSNET_VECTOR,
	WARSNET_QANGLE,
	WARSNET_HANDLE,
	WARSNET_LIST,
	WARSNET_DICT,
	WARSNET_TUPLE,
	WARSNET_SET,
	WARSNET_COLOR,
	WARSNET_STEAMID,
};

typedef struct WarsEntityUpdateMessage_t : public WarsMessage_t
{
	WarsEntityUpdateMessage_t() {}

	long iEncodedEHandle;
} WarsEntityUpdateMessage_t;

#ifndef CLIENT_DLL
// Entity data updates
void WarsNet_StartEntityUpdate( edict_t *pClientEdict, EHANDLE ent );
bool WarsNet_EndEntityUpdate();
void WarsNet_WriteEntityData( const char *name, boost::python::object data, bool changecallback );

// Generic messages
void WarsNet_WriteMessageData( IRecipientFilter& filter, const char *name, boost::python::object msg );
#else
void WarsNet_Init();
void WarsNet_Shutdown();

// Receiving entity data updates
void WarsNet_ReceiveEntityUpdate( CUtlBuffer &data );

// Receiving generic messages
void WarsNet_ReceiveMessageUpdate( CUtlBuffer &data );
#endif // CLIENT_DLL

#endif // ENABLE_PYTHON

#endif // WARS_NETWORK_H