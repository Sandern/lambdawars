//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "wars_network.h"
#include "steam/steam_api.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const char *WarsNet_TranslateP2PConnectErr( int errorCode )
{
	switch( errorCode )
	{
	case k_EP2PSessionErrorNotRunningApp:
		return "target is not running the same game";
	case k_EP2PSessionErrorNoRightsToApp:
		return "local user doesn't own the app that is running";
	case k_EP2PSessionErrorDestinationNotLoggedIn:
		return "target user isn't connected to Steam";
	case k_EP2PSessionErrorTimeout:
		return "target isn't responding/timed out";
	}

	return "Unknown P2P connect error";
}
