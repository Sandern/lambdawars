//========= Copyright © 200x, Sandern Corporation, All rights reserved. ============//
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

#ifdef HL2WARS_ASW_DLL
#define STEAMAPP_ROOT "../../"
#else
#define STEAMAPP_ROOT "../../"
#endif // HL2WARS_ASW_DLL

#ifndef HL2WARS_ASW_DLL
// http://developer.valvesoftware.com/wiki/Steam_Application_IDs
// Let's hope the lists don't change too much
// For each game we want to mount we check if the gcf files are present and valid (since MountSteamContent doesn't do this)
static const char *pEpisode2Files[] = {
	"episode two content.gcf",
	"episode two materials.gcf",
#ifndef HL2WARS_ASW_DLL
	"episode two maps.gcf",
	"half-life 2 episode two english.gcf",
#endif // HL2WARS_ASW_DLL
};

static const char *pEpisode1Files[] = {
	"episode 1 shared.gcf",
	"episode one 2007 content.gcf",
#ifndef HL2WARS_ASW_DLL
	"half-life 2 episode one.gcf",
#endif // HL2WARS_ASW_DLL
};

static const char *pTeamFortress2Files[] = {
	"team fortress 2 content.gcf",
	"team fortress 2 materials.gcf",
	"team fortress 2 client content.gcf",
};

static const char *pPortalFiles[] = {
	"portal content.gcf",
};

static const char *pCounterStrikeSourceFiles[] = {
	"counter-strike source shared.gcf",
	"counter-strike source client.gcf",
};

static const char *pHalfLife1Files[] = {
	"half-life source.gcf",
};

static const char *pDODSFiles[] = {
	"day of defeat source.gcf",
};
#endif // HL2WARS_ASW_DLL

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
#ifndef HL2WARS_ASW_DLL
bool VerifyContentFiles( const char *pFileList[], int iArraySize, const char *pMountID, KeyValues *pMountList = NULL )
{
#ifdef GAME_DLL
	// Dedicated servers will always succeed for now
	if( engine->IsDedicatedServer() )
	{
		Msg("succeeded (dedicated server)\n");
		return true;
	}
#endif // GAME_DLL

	if( pMountList && pMountList->GetInt( pMountID, 0 ) == 0 )
	{
		MountMsg("failed (disabled in mount list)\n");
		return false;
	}

	unsigned int fileSize;
	int i;

	for( i=0; i < iArraySize; i++ )
	{
		// Get path
		char path[_MAX_PATH];
		V_ComposeFileName( STEAMAPP_ROOT, pFileList[i], path, _MAX_PATH );
		
		// Fixup path
		V_FixSlashes( path );
		V_RemoveDotSlashes( path );
		V_FixDoubleSlashes( path );
		V_strlower( path );

		// Try to load the file
		FileHandle_t f = filesystem->Open(path, "rb");
		if( f == FILESYSTEM_INVALID_HANDLE ) 
		{
			MountMsg("failed (missing cache file)\n");
			return false;
		}

		// Read header
		fileSize = filesystem->Size( f );
		if( fileSize < sizeof(CacheHeader) )
		{
			filesystem->Close(f);
			MountMsg("failed (invalid cache file, cannot read header)\n");
			return false;
		}

		struct CacheHeader header;
		filesystem->Read(&header, sizeof(CacheHeader), f);
		filesystem->Close(f);

		// TODO: Check if header is valid?

		// Check filesize
		if( header.FileSize != fileSize )
		{
			MountMsg("failed (invalid cache file, filesize does not match)\n");
			return false;
		}
	}
	MountMsg("succeeded\n");
	return true;
}
#endif // 0

#ifdef HL2WARS_ASW_DLL
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
#endif // HL2WARS_ASW_DLL

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

#ifndef HL2WARS_ASW_DLL
	// NOTE: Search paths are shared between client and server (or likely the filesystem is).
	//		 So we only add search paths on the server.
	//	     I do assume mounting is needed for now.
#ifndef CLIENT_DLL
	// Rebuild search path from scratch, since we want hl2 behind ep2
	char buf[2048];
	filesystem->GetSearchPath("GAME", true, buf, 2048);
	filesystem->RemoveSearchPaths("GAME");

	CUtlVector<char*> searchPaths;
	CUtlVector<int> searchPathsDelayAdd;
	V_SplitString( buf, ";", searchPaths );
	for ( int i = 0; i < searchPaths.Count(); i++ )
	{
		//Msg("SearchPath: %s %d\n", searchPaths[i], Q_strncmp(searchPaths[i]+Q_strlen(searchPaths[i])-4, "hl2\\", 3));
		if( Q_strncmp(searchPaths[i]+Q_strlen(searchPaths[i])-4, "hl2\\", 3) != 0  )
		{
			filesystem->AddSearchPath(searchPaths[i], "GAME");

		}
		else
		{
			searchPathsDelayAdd.AddToTail(i);
		}
	}

	if( searchPathsDelayAdd.Count() == 0 )
	{
		Warning("MountExtraContent: did not found hl2 in search path. Not detected? Search path: %s\n", buf);
	}
#endif // CLIENT_DLL

	// Try to mount Episode two content
	MountMsg("Mounting Episode 2 content...");

	if( VerifyContentFiles( pEpisode2Files, ARRAYSIZE(pEpisode2Files), "ep2", pMountList ) )
	{
		AddSearchPath("ep2", "GAME");
		AddSearchPath("episodic", "GAME");
		filesystem->MountSteamContent(-420);
		g_AppStatus[APP_EP2] = AS_AVAILABLE;
	}

	// Try to mount Episode one content
	MountMsg("Mounting Episode 1 content...");
	if( VerifyContentFiles( pEpisode1Files, ARRAYSIZE(pEpisode1Files), "episodic", pMountList ) )
	{
		if( !(g_AppStatus[APP_EP2] == AS_AVAILABLE) )
			AddSearchPath("episodic", "GAME");
		filesystem->MountSteamContent(-380);
		g_AppStatus[APP_EP1] = AS_AVAILABLE;
	}

#ifndef CLIENT_DLL
	// Must be behind ep2
	filesystem->AddSearchPath("sourcetest", "GAME");
	//filesystem->AddSearchPath("hl2", "GAME");
	for ( int i = 0; i < searchPathsDelayAdd.Count(); i++ )
		filesystem->AddSearchPath(searchPaths[searchPathsDelayAdd[i]], "GAME");
#endif // CLIENT_DLL

	// Try to mount team fortress 2 content
	MountMsg("Mounting Team Fortress 2 content...");
	if( VerifyContentFiles( pTeamFortress2Files, ARRAYSIZE(pTeamFortress2Files), "tf", pMountList ) )
	{
		AddSearchPath("tf", "GAME");
		filesystem->MountSteamContent(-440);
		g_AppStatus[APP_TF2] = AS_AVAILABLE;
	}

	// Try to mount portal content
	MountMsg("Mounting Portal content...");
	if( VerifyContentFiles( pPortalFiles, ARRAYSIZE(pPortalFiles), "portal", pMountList ) )
	{
		AddSearchPath("portal", "GAME");
		filesystem->MountSteamContent(-400);
		g_AppStatus[APP_PORTAL] = AS_AVAILABLE;
	}

	// Try to mount counter strike source content
	MountMsg("Mounting Counter Strike Source content...");
	if( VerifyContentFiles( pCounterStrikeSourceFiles, ARRAYSIZE(pCounterStrikeSourceFiles), "cstrike", pMountList ) )
	{
		AddSearchPath("cstrike", "GAME");
		filesystem->MountSteamContent(-240);
		g_AppStatus[APP_CSS] = AS_AVAILABLE;
	}

	// Try to mount Half-Life 1: Source content
	MountMsg("Mounting Half-Life 1: Source content...");
	if( VerifyContentFiles( pHalfLife1Files, ARRAYSIZE(pHalfLife1Files), "hl1", pMountList ) )
	{
		AddSearchPath("hl1", "GAME");
		filesystem->MountSteamContent(-280);
		g_AppStatus[APP_HL1] = AS_AVAILABLE;
	}

	// Try to mount Day of Defeat: Source content
	MountMsg("Mounting Day of Defeat: Source content...");
	if( VerifyContentFiles( pDODSFiles, ARRAYSIZE(pDODSFiles), "dod", pMountList ) )
	{
		AddSearchPath("dod", "GAME");
		filesystem->MountSteamContent(-300);
		g_AppStatus[APP_DODS] = AS_AVAILABLE;
	}

#else
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
#endif // HL2WARS_ASW_DLL

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

#ifdef HL2WARS_ASW_DLL
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
#endif // HL2WARS_ASW_DLL
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