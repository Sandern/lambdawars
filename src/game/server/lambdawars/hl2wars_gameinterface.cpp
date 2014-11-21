//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "gameinterface.h"
#include "mapentities.h"

#include "networkstringtabledefs.h"
#include "filesystem.h"

#include "fmtstr.h"
#include "gameinterface.h"
#include "matchmaking/swarm/imatchext_swarm.h"

#ifdef ENABLE_PYTHON
	#include "srcpy.h"
	#include "srcpy_srcbuiltins.h"
#endif // ENABLE_PYTHON

#include "wars_gameserver.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_force_transmit_ents;

extern const char *COM_GetModDirectory( void );

extern INetworkStringTableContainer *networkstringtable;
bool AddToDownloadables( const char *file )
{
	INetworkStringTable *pDownloadablesTable = networkstringtable->FindTable( "downloadables" );
	// Actually, it would be desirable to add the last line to
	// ServerActivate and store the pointer, so each time you
	// add a download you dont have to find it again.
	if ( pDownloadablesTable )
	{
		bool save = engine->LockNetworkStringTables( false );
		int iRet = pDownloadablesTable->FindStringIndex( file );
		if ( iRet == INVALID_STRING_INDEX )
		{
			pDownloadablesTable->AddString( CBaseEntity::IsServer(), file, sizeof( file ) + 1 );
			engine->LockNetworkStringTables( save );
		}
		else
		{
			DevMsg( "AddDownload: Multiple download of \"%s\"", file );
		}
		return true;
	}
	DevMsg("Table downloadables not found!\n");
	return false;
}

/*
void AddMapDataToDownloadList( const char *pMapName )
{
	// Check if we have a map settings file
	boost::python::object mapdata = SrcPySystem()->Import(pMapName);
	if( mapdata.ptr() == Py_None )
		return;

	char path_to_info[MAX_PATH];

	// Check if the map data file is in the correct place
	Q_snprintf( path_to_info, sizeof( path_to_info ), "maps/%s.pyc\0", pMapName );
	if( filesystem->FileExists(path_to_info) == false ) {
		Warning("%s is missing\n", path_to_info);
		return;
	}
	AddDownload(path_to_info);

	const char *pMinimap = SrcPySystem()->Get<const char *>("minimap_material", mapdata, NULL);
	if( pMinimap ) {
		Q_snprintf( path_to_info, sizeof( path_to_info ), "materials/%s.vmt\0", pMinimap );
		if( filesystem->FileExists(path_to_info) == false ) {
			Warning("%s is missing\n", path_to_info);
			return;
		}
		AddDownload(path_to_info);
		Q_snprintf( path_to_info, sizeof( path_to_info ), "materials/%s.vtf\0", pMinimap );
		if( filesystem->FileExists(path_to_info) == false ) {
			Warning("%s is missing\n", path_to_info);
			return;
		}
		AddDownload(path_to_info);
	}
}
*/

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameClients implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameClients::GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers ) const
{
	minplayers = 1;  // Force multiplayer.
	maxplayers = MAX_PLAYERS;
	defaultMaxPlayers = 32;
}

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameDLL implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameDLL::LevelInit_ParseAllEntities( const char *pMapEntities )
{
}

bool g_bOfflineGame = false;

void CServerGameDLL::ApplyGameSettings( KeyValues *pKV )
{
	if ( !pKV )
		return;

	// Fix the game settings request when a generic request for
	// map launch comes in (it might be nested under reservation
	// processing)
	bool bAlreadyLoadingMap = false;
	char const *szBspName = NULL;
	const char *pGameDir = COM_GetModDirectory();
	if ( !Q_stricmp( pKV->GetName(), "::ExecGameTypeCfg" ) )
	{
		if ( !engine )
			return;

		char const *szNewMap = pKV->GetString( "map/mapname", "" );
		if ( !szNewMap || !*szNewMap )
			return;

		KeyValues *pLaunchOptions = engine->GetLaunchOptions();
		if ( !pLaunchOptions )
			return;

		if ( FindLaunchOptionByValue( pLaunchOptions, "changelevel" ) ||
			FindLaunchOptionByValue( pLaunchOptions, "changelevel2" ) )
			return;

		if ( FindLaunchOptionByValue( pLaunchOptions, "map_background" ) )
		{

			return;
		}

		bAlreadyLoadingMap = true;

		if ( FindLaunchOptionByValue( pLaunchOptions, "reserved" ) )
		{
			// We are already reserved, but we still need to let the engine
			// baseserver know how many human slots to allocate
			pKV->SetInt( "members/numSlots", g_bOfflineGame ? 1 : 8 );
			return;
		}

		pKV->SetName( pGameDir );
	}

	if ( Q_stricmp( pKV->GetName(), pGameDir ) || bAlreadyLoadingMap )
		return;

	if( WarsGameServer() )
	{
		// Make sure messages are processed, needed for offline mode
		WarsGameServer()->RunFrame();

		if( WarsGameServer()->GetLocalPlayerGameData() )
		{
			pKV = WarsGameServer()->GetLocalPlayerGameData();
			DevMsg("Using local player game data\n");
		}
	}

	//g_bOfflineGame = pKV->GetString( "map/offline", NULL ) != NULL;
	g_bOfflineGame = !Q_stricmp( pKV->GetString( "system/network", "LIVE" ), "offline" );

	Msg( "GameInterface reservation payload:\n" );
	KeyValuesDumpAsDevMsg( pKV );

	// Vitaliy: Disable cheats as part of reservation in case they were enabled (unless we are on Steam Beta)
	if ( sv_force_transmit_ents.IsFlagSet( FCVAR_DEVELOPMENTONLY ) &&	// any convar with FCVAR_DEVELOPMENTONLY flag will be sufficient here
		sv_cheats && sv_cheats->GetBool() )
	{
		sv_cheats->SetValue( 1 );
	}

	// Between here and the last part: apply settings...
	char const *szMapCommand = pKV->GetString( "map/mapcommand", "map" );

	const char *szMode = pKV->GetString( "game/mode", "sdk" );

#if 0
	char const *szGameMode = pKV->GetString( "game/mode", "" );
	if ( szGameMode && *szGameMode )
	{
		extern ConVar mp_gamemode;
		mp_gamemode.SetValue( szGameMode );
	}
#endif // 0

	// LAUNCH GAME
	if ( !Q_stricmp( szMode, "sdk" ) )
	{
		szBspName = pKV->GetString( "game/mission", NULL );
		if ( !szBspName )
			return;

		engine->ServerCommand( CFmtStr( "%s %s sdk reserved\n",
			szMapCommand,
			szBspName ) );
	}
	else
	{
		bool bSuccess = false;
#ifdef ENABLE_PYTHON
		try 
		{
			bSuccess = boost::python::extract<bool>(gamemgr.attr("_ApplyGameSettings")( PyKeyValuesToDict( pKV ) ));
		} 
		catch( boost::python::error_already_set & ) {
			Warning( "ApplyGameSettings: Error occurred while letting Python apply game settings for game mode \"%s\"\n", szMode );
			PyErr_Print();
		}
#endif // ENABLE_PYTHON

		if( !bSuccess )
		{
			Warning( "ApplyGameSettings: Unknown game mode \"%s\"! Falling back to gamelobby.\n", szMode );
			engine->ServerCommand( CFmtStr( "%s gamelobby reserved\n",
				szMapCommand ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: callback hook for debug text emitted from the Steam API
//-----------------------------------------------------------------------------
extern "C" void __cdecl SteamAPIDebugTextHook( int nSeverity, const char *pchDebugText )
{
	// if you're running in the debugger, only warnings (nSeverity >= 1) will be sent
	// if you add -debug_steamapi to the command-line, a lot of extra informational messages will also be sent
	Warning( pchDebugText );
	Warning("\n");

	if ( nSeverity >= 1 )
	{
		// place to set a breakpoint for catching API errors
		int x = 3;
		x = x;
	}
}

void WarsUpdateGameServer()
{
	//if( !engine->IsDedicatedServer() )
	//	return;

	//if( !steamgameserverapicontext || !g_pSteamClientGameServer )
	//	return;

	if( g_pSteamClientGameServer && !steamgameserverapicontext->SteamGameServerNetworking() )
	{
		steamgameserverapicontext->Init();

		// set our debug handler
		g_pSteamClientGameServer->SetWarningMessageHook( &SteamAPIDebugTextHook );
	}

	/*if( !steamgameserverapicontext->SteamGameServerNetworking() )
	{
		steamgameserverapicontext->Init();

		// set our debug handler
		g_pSteamClientGameServer->SetWarningMessageHook( &SteamAPIDebugTextHook );
	}

	if( !steamgameserverapicontext->SteamGameServerNetworking() )
		return;*/

	if( !WarsGameServer() )
	{
		WarsInitGameServer();
	}
	else 
	{
		WarsGameServer()->RunFrame();
	}
}
