//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Tries to mount all available source engine games.
//
// Special Note: filesystem is shared and server dll is always loaded (for both client and server).
//				 This means you only need to add the search path once.
//				 We will still execute this on the client too, to verify which games
//				 are available.
//
//=============================================================================//

#include "cbase.h"
#include "wars_mount_system.h"
#include <filesystem.h>
#include "tier0/icommandline.h"

#ifdef CLIENT_DLL
	#include "steam/steam_api.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define VPKBIN "../../common/left 4 dead 2/bin/vpk.exe"

AppStatus g_AppStatus[NUM_APPS];

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool HasApp( MountApps app )
{
	if( app < 0 || app >= NUM_APPS )
		return false;
	return g_AppStatus[app] == AS_AVAILABLE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
AppStatus GetAppStatus( MountApps app )
{
	if( app < 0 || app >= NUM_APPS )
		return AS_NOT_AVAILABLE;
	return g_AppStatus[app];
}

#define STEAMAPP_ROOT "../../"

// Helpers
#define MountMsg(...) \
if( CBaseEntity::IsServer() ) \
	Msg( __VA_ARGS__  );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void AddSearchPath( const char *pPath, const char *pathID, SearchPathAdd_t addType = PATH_ADD_TO_TAIL )
{
#ifndef CLIENT_DLL
	filesystem->AddSearchPath( pPath, pathID, addType );
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool VerifyHaveAddonVPK( const char *pFileList[], int iArraySize, const char *pMountID, KeyValues *pMountList = NULL )
{
	char buf[_MAX_PATH];
	char path[_MAX_PATH];
	char addon_root[_MAX_PATH];
	int i;

	if( pMountList && pMountList->GetInt( pMountID, 0 ) == 0 )
	{
		MountMsg("failed (disabled in mount list)\n");
		return false;
	}
	
	filesystem->RelativePathToFullPath("addons", "MOD", addon_root, _MAX_PATH);

	for( i=0; i < iArraySize; i++ )
	{
		V_StripExtension(pFileList[i], buf, _MAX_PATH);
		V_SetExtension(buf, ".vpk", _MAX_PATH);
		Q_snprintf(path, _MAX_PATH, "%s/%s", addon_root, buf);
		if( !filesystem->FileExists(path) ) 
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Tries to add a VPK path to the mount list
//-----------------------------------------------------------------------------
bool TryMountVPKGame( const char *pName, const char *pPath, int id, const char *pMountID, KeyValues *pMountList = NULL )
{
	if( pMountList && pMountList->GetInt( pMountID, 0 ) == 0 )
	{
		return false;
	}

	MountMsg("Adding %s to search path...", pName);

	// Dedicated servers always succeed for now...
#ifdef GAME_DLL
	if( engine->IsDedicatedServer() )
	{
		Msg("succeeded (dedicated server)\n");
		g_AppStatus[id] = AS_AVAILABLE;
		return true;
	}
#endif // GAME_DLL

	if( filesystem->IsDirectory( pPath, "GAME" ) ) 
	{
		AddSearchPath( pPath, "GAME", PATH_ADD_TO_TAIL_ATINDEX );
		g_AppStatus[id] = AS_AVAILABLE;
		MountMsg("succeeded\n");
		return true;
	}
	else
	{
		MountMsg("failed (not installed?)\n");
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void MountExtraContent()
{
	memset(g_AppStatus, 0, sizeof(g_AppStatus));

	KeyValues *pMountList = new KeyValues( "MountList" );
	KeyValues::AutoDelete autodelete( pMountList );
	if( !pMountList->LoadFromFile( filesystem, "mountlist.txt" ) )
	{
		// Create default
		pMountList->SetString( "dota", "0" );
		pMountList->SetString( "left4dead", "0" );
		pMountList->SetString( "left4dead2", "0" );
		pMountList->SetString( "portal2", "0" );
		pMountList->SetString( "csgo", "0" );
		pMountList->SetString( "dearesther", "0" );
		
		pMountList->SaveToFile( filesystem, "mountlist.txt", "MOD" );
	}

	TryMountVPKGame( "DOTA", "../../common/dota 2 beta/dota", APP_DOTA, "dota", pMountList );
	TryMountVPKGame( "Portal 2", "../../common/portal 2/portal2", APP_PORTAL2, "portal2", pMountList );
	TryMountVPKGame( "Left 4 Dead 2", "../../common/left 4 dead 2/left4dead2", APP_L4D2, "left4dead2", pMountList );
	TryMountVPKGame( "Left 4 Dead", "../../common/left 4 dead/left4dead", APP_L4D1, "left4dead", pMountList );
	TryMountVPKGame( "Counter-Strike Global Offensive", "../../common/Counter-Strike Global Offensive/csgo", APP_CSGO, "csgo", pMountList );
	TryMountVPKGame( "Dear Esther", "../../common/dear esther/dearesther", APP_DEARESTHER, "dearesther", pMountList );
}
