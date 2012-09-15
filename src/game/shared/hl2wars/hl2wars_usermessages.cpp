//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "usermessages.h"
#include "shake.h"
#include "voice_gamemgr.h"

void RegisterUserMessages()
{
	usermessages->Register( "Geiger", 1 );		// geiger info data
	usermessages->Register( "Train", 1 );		// train control data
	usermessages->Register( "HudText", -1 );	
	usermessages->Register( "SayText", -1 );	
	usermessages->Register( "SayText2", -1 );	
	usermessages->Register( "TextMsg", -1 );
	usermessages->Register( "HudMsg", -1 );
	usermessages->Register( "ResetHUD", 1 );	// called every respawn
	usermessages->Register( "GameTitle", 0 );	// show game title
	usermessages->Register( "ItemPickup", -1 );	// for item history on screen
	usermessages->Register( "ShowMenu", -1 );	// show hud menu
	usermessages->Register( "Shake", 13 );		// shake view
	usermessages->Register( "Fade", 10 );	// fade HUD in/out
	usermessages->Register( "VGUIMenu", -1 );	// Show VGUI menu
	usermessages->Register( "Rumble", 3 );	// Send a rumble to a controller
	usermessages->Register( "CloseCaption", -1 ); // Show a caption (by string id number)(duration in 10th of a second)
#ifdef HL2WARS_ASW_DLL
	usermessages->Register( "CloseCaptionDirect", -1 ); // Show a forced caption (by string id number)(duration in 10th of a second)
#endif // HL2WARS_ASW_DLL

	usermessages->Register( "SendAudio", -1 );	// play radion command

	usermessages->Register( "VoiceMask", VOICE_MAX_PLAYERS_DW*4 * 2 + 1 );
	usermessages->Register( "RequestState", 0 );

	usermessages->Register( "BarTime", -1 );	// For the C4 progress bar.
	usermessages->Register( "Damage", -1 );		// for HUD damage indicators
	usermessages->Register( "RadioText", -1 );		// for HUD damage indicators
	usermessages->Register( "HintText", -1 );	// Displays hint text display
	usermessages->Register( "KeyHintText", -1 );	// Displays hint text display
	
	usermessages->Register( "ReloadEffect", 2 );			// a player reloading..
	usermessages->Register( "PlayerAnimEvent", -1 );	// jumping, firing, reload, etc.

	usermessages->Register( "AmmoDenied", 2 );
	usermessages->Register( "AchievementEvent", -1 );

	usermessages->Register( "UpdateRadar", -1 );

#ifdef HL2WARS_ASW_DLL
	usermessages->Register( "CurrentTimescale", 4 );	// Send one float for the new timescale
	usermessages->Register( "DesiredTimescale", 13 );	// Send timescale and some blending vars
#endif // HL2WARS_ASW_DLL

	// Used to send a sample HUD message
	usermessages->Register( "GameMessage", -1 );

	// Python messages
	usermessages->Register( "PyMessage", -1 );
	usermessages->Register( "PyNetworkCls", -1 );	// Import module with class def. Add fallbacks if client class is missing
	usermessages->Register( "PyNetworkVar", -1 );
	usermessages->Register( "PyNetworkVarCC", -1 );
	usermessages->Register( "PyNetworkArrayFull", -1 );
	usermessages->Register( "PyNetworkDictElement", -1 );
	usermessages->Register( "PyNetworkDictFull", -1 );
}

