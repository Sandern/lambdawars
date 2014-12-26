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

#include "wars_matchmaking.h"

enum WarsNetType_e {
	WARSNET_ERROR = 0,
	WARSNET_ENTVAR,
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
void WarsNet_StartEntityUpdate( edict_t *pClientEdict, EHANDLE ent );
void WarsNet_EndEntityUpdate();
void WarsNet_WriteEntityData( const char *name, boost::python::object data );
#else
void WarsNet_ReceiveEntityUpdate( CUtlBuffer &data );
#endif // CLIENT_DLL

#endif // WARS_NETWORK_H