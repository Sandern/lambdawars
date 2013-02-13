//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
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

typedef unsigned long DWORD2;

AppStatus g_AppStatus[NUM_APPS];
bool HasApp( MountApps app )
{
	if( app < 0 || app >= NUM_APPS )
		return false;
	return g_AppStatus[app] == AS_AVAILABLE;
}

AppStatus GetAppStatus( MountApps app )
{
	if( app < 0 || app >= NUM_APPS )
		return AS_NOT_AVAILABLE;
	return g_AppStatus[app];
}

// GCF File Header
// http://www.wunderboy.org/docs/gcfformat.php
// http://wiki.steamwhore.com/GCF_Format_Documentation
struct CacheHeader
{
    DWORD2 HeaderVersion;
    DWORD2 CacheType;
    DWORD2 FormatVersion;
    DWORD2 ApplicationID;
    DWORD2 ApplicationVersion;
    DWORD2 IsMounted;
    DWORD2 Dummy0;
    DWORD2 FileSize;
    DWORD2 SectorSize;
    DWORD2 SectorCount;
    DWORD2 Checksum;
};

#define STEAMAPP_ROOT "../../"

// Helpers
#define MountMsg(...) \
if( CBaseEntity::IsServer() ) \
	Msg( __VA_ARGS__  );

static void AddSearchPath( const char *pPath, const char *pathID, SearchPathAdd_t addType = PATH_ADD_TO_TAIL )
{
#ifndef CLIENT_DLL
	filesystem->AddSearchPath( pPath, pathID, addType );
#endif // CLIENT_DLL
}

// Check functions
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

void MountExtraContent()
{
	memset(g_AppStatus, 0, sizeof(g_AppStatus));

	KeyValues *pMountList = new KeyValues( "MountList" );
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
#if 0
	// Build list of games that require extraction
	// Mark as "need extraction" in case not done.
	// Otherwise available
	// Try to mount Episode two content
	MountMsg("Mounting Episode 2 content...");
	if( VerifyContentFiles( pEpisode2Files, ARRAYSIZE(pEpisode2Files), "ep2", pMountList ) )
	{
		if( VerifyHaveAddonVPK( pEpisode2Files, ARRAYSIZE(pEpisode2Files), "ep2", pMountList ) )
		{
			g_AppStatus[APP_EP2] = AS_AVAILABLE;
			MountMsg("succeeded\n");
		}
		else
		{
			g_AppStatus[APP_EP2] = AS_AVAILABLE_REQUIRES_EXTRACTION;
			MountMsg("failed, requires extraction\n");
		}
	}

	MountMsg("Mounting Episode 1 content...");
	if( VerifyContentFiles( pEpisode1Files, ARRAYSIZE(pEpisode1Files), "episodic", pMountList ) )
	{
		if( VerifyHaveAddonVPK( pEpisode1Files, ARRAYSIZE(pEpisode1Files), "episodic", pMountList ) )
		{
			g_AppStatus[APP_EP1] = AS_AVAILABLE;
			MountMsg("succeeded\n");
		}
		else
		{
			g_AppStatus[APP_EP1] = AS_AVAILABLE_REQUIRES_EXTRACTION;
			MountMsg("failed, requires extraction\n");
		}
	}
#endif // 0

	if( pMountList )
	{
		pMountList->deleteThis();
		pMountList = NULL;
	}
}

void TryMountVPKGame( const char *pName, const char *pPath, int id, const char *pMountID, KeyValues *pMountList = NULL )
{
	MountMsg("Adding %s to search path...", pName);

	if( pMountList && pMountList->GetInt( pMountID, 0 ) == 0 )
	{
		MountMsg("failed (disabled in mount list)\n");
		return;
	}

	// Dedicated servers always succeed for now...
#ifdef GAME_DLL
	if( engine->IsDedicatedServer() )
	{
		Msg("succeeded (dedicated server)\n");
		g_AppStatus[id] = AS_AVAILABLE;
		return;
	}
#endif // GAME_DLL

	if( filesystem->IsDirectory( pPath, "GAME" ) ) 
	{
		AddSearchPath( pPath, "GAME" );
		g_AppStatus[id] = AS_AVAILABLE;
		MountMsg("succeeded\n");
	}
	else
	{
		MountMsg("failed\n");
	}
}

void PostInitExtraContent()
{
	KeyValues *pMountList = new KeyValues("MountList");
	if( !pMountList->LoadFromFile(filesystem, "mountlist.txt") )
	{
		pMountList->deleteThis();
		pMountList = NULL;
	}

	// Add Left 4 Dead 1 + 2 to the search paths
	// Note: VPK files override base mod files
	//		 So we don't add it earlier, otherwise it loads up some l4d files like modevents, scripts, etc.
	TryMountVPKGame( "DOTA", "../../common/dota 2 beta/dota", APP_DOTA, "dota", pMountList );
	TryMountVPKGame( "Portal 2", "../../common/portal 2/portal2", APP_PORTAL2, "portal2", pMountList );
	TryMountVPKGame( "Left 4 Dead 2", "../../common/left 4 dead 2/left4dead2", APP_L4D2, "left4dead2", pMountList );
	TryMountVPKGame( "Left 4 Dead", "../../common/left 4 dead/left4dead", APP_L4D1, "left4dead", pMountList );
	TryMountVPKGame( "Counter-Strike Global Offensive", "../../common/Counter-Strike Global Offensive/csgo", APP_CSGO, "csgo", pMountList );
	TryMountVPKGame( "Dear Esther", "../../common/dear esther/dearesther", APP_DEARESTHER, "dearesther", pMountList );

	if( pMountList )
	{
		pMountList->deleteThis();
		pMountList = NULL;
	}
}

#if 0
#ifdef CLIENT_DLL
CON_COMMAND_F( cl_test_mount_system, "", FCVAR_CHEAT)
#else
CON_COMMAND_F( test_mount_system, "", FCVAR_CHEAT)
#endif // CLIENT_DLL
{
	MountExtraContent();

	/*
	FileHandle_t f = filesystem->Open("../../team fortress 2 content.gcf", "rb");
	if( f == FILESYSTEM_INVALID_HANDLE ) 
	{
		Warning("No such file\n");
		return;
	}

	unsigned int fileSize = filesystem->Size( f );
	if( fileSize < sizeof(CacheHeader) )
	{
		filesystem->Close(f);
		Warning("Invalid gcf file (no header)\n");
		return;
	}

	struct CacheHeader header;
	filesystem->Read(&header, sizeof(CacheHeader), f);
	filesystem->Close(f);

	Msg("../team fortress 2 content.gcf info (filesize: %u): \n", fileSize);
	Msg("HeaderVersion: %lu\n", header.HeaderVersion);
	Msg("CacheType: %lu\n", header.CacheType);
	Msg("FormatVersion: %lu\n", header.FormatVersion);
	Msg("ApplicationID: %lu\n", header.ApplicationID);
	Msg("ApplicationVersion: %lu\n", header.ApplicationVersion);
	Msg("IsMounted: %lu\n", header.IsMounted);
	Msg("Dummy0: %lu\n", header.Dummy0);
	Msg("FileSize: %lu\n", header.FileSize);
	Msg("SectorSize: %lu\n", header.SectorSize);
	Msg("SectorCount: %lu\n", header.SectorCount);
	Msg("Checksum: %lu\n", header.Checksum);
	*/
}
#endif // 0

#if 0
CON_COMMAND_F( test_open_file, "", FCVAR_CHEAT)
{
	char path[_MAX_PATH];
	V_FixupPathName(path, _MAX_PATH, args.ArgS());
	FileHandle_t f = filesystem->Open(path, "rb");
	if( f == FILESYSTEM_INVALID_HANDLE ) 
	{
		Warning("No such file %s\n", path);
		return;
	}

	Msg("FIle opened %s\n", path);
	filesystem->Close(f);
}
#endif // 0

#if 0
#ifdef CLIENT_DLL
CON_COMMAND_F( test_subscribed, "", FCVAR_CHEAT)
{
	bool bSubscribed = steamapicontext->SteamApps()->BIsSubscribedApp(420);
	bool bSubscribed2 = steamapicontext->SteamApps()->BIsSubscribedApp(1300);
	bool bSubscribed3 = steamapicontext->SteamApps()->BIsSubscribedApp(320);
	bool bSubscribed4 = steamapicontext->SteamApps()->BIsSubscribedApp(400);

	Msg("Subscribed to 420: %d, subscribed to 1300: %d, subscribed to 320: %d, subscribed to 400: %d, logged on: %d, cur app id: %d\n", 
		bSubscribed, bSubscribed2, bSubscribed3, bSubscribed4, steamapicontext->SteamUser()->BLoggedOn(), steamapicontext->SteamUtils()->GetAppID() );
}
#endif // CLIENT_DLL
#endif // 0