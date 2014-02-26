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

	if( TryMountVPKGame( "DOTA", "../../common/dota 2 beta/dota", APP_DOTA, "dota", pMountList ) )
		PostProcessDota2( "models" );
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
static bool CanFindFile( const char *pPath, const char *pathID = 0, char **ppszResolvedFilename = NULL )
{
	FileHandle_t f = filesystem->OpenEx(pPath, "rb", 0, pathID, ppszResolvedFilename );
	if( f == FILESYSTEM_INVALID_HANDLE ) 
	{
		return false;
	}
	filesystem->Close(f);
	return true;
}

#include <windows.h>

typedef struct extractvpkfile_t
{
	extractvpkfile_t( const char *pPath ) { Q_strncpy( path, pPath, MAX_PATH ); }
	char path[MAX_PATH];
} extractvpkfile_t;

static void RecursiveParsePath( const char *pPath, CUtlVector<extractvpkfile_t> &worklist, const char *pathID = 0 )
{
	char wildcard[MAX_PATH];
	FileFindHandle_t findHandle;
		
	Q_snprintf( wildcard, sizeof( wildcard ), "%s/*.dx90.vtx", pPath );
	const char *pFilename = filesystem->FindFirstEx( wildcard, "DOTA", &findHandle );
	while ( pFilename != NULL )
	{

		if( Q_strncmp(pFilename, ".", 1) != 0 &&
				Q_strncmp(pFilename, "..", 2) != 0 ) 
		{
			char fullpath[MAX_PATH];
			Q_snprintf( fullpath, MAX_PATH, "%s/%s", pPath, pFilename );
			if( filesystem->FindIsDirectory( findHandle ) )
			{
				RecursiveParsePath( fullpath, worklist, pathID );
			}
			else
			{
				char resolvedFilename[MAX_PATH];
				resolvedFilename[0] = '\0';
				char *ppszResolvedFilename = resolvedFilename;
				bool bFound = CanFindFile( fullpath, pathID, &(ppszResolvedFilename) );
				if( !bFound )
				{
					// Make sure the directory exists
					if( filesystem->FileExists(pPath, "MOD") == false )
					{
						filesystem->CreateDirHierarchy(pPath, "MOD");
					}

					char buf[MAX_PATH];
					Q_snprintf( buf, MAX_PATH, "\"%s\"", fullpath );
					worklist.AddToTail( extractvpkfile_t( buf ) );
				}
			}
		}
		pFilename = filesystem->FindNext( findHandle );
	}
	filesystem->FindClose( findHandle );
}

void PostProcessDota2( const char *pPath )
{
#ifdef CLIENT_DLL
	return;
#endif // CLIENT_DLL

	float start = Plat_FloatTime();

	if( !filesystem->FileExists( VPKBIN ) )
	{
		Warning("PostProcessDota2: %s does not exists. Unable to parse models. Please install the Left 4 Dead 2 SDK\n", VPKBIN);
		return;
	}
	filesystem->AddSearchPath( "../../common/dota 2 beta/dota", "DOTA", PATH_ADD_TO_TAIL );

	Msg("PostProcessDota2...\n");

	CUtlVector<extractvpkfile_t> worklist;
	RecursiveParsePath( pPath, worklist, "DOTA" );

	Msg( "worklist: %d\n", worklist.Count() );

	#define MAX_COMMAND_SIZE 32768
	char args[MAX_COMMAND_SIZE];
	args[0] = '\0';

	Q_strcat( args, "x \"../../common/dota 2 beta/dota/pak01_dir.vpk\"", MAX_COMMAND_SIZE );

	Msg("Extracting files...\n");

	for( int i = 0; i < worklist.Count(); i++ )
	{
		Q_strcat( args, " ", MAX_COMMAND_SIZE );
		Q_strcat( args, worklist[i].path, MAX_COMMAND_SIZE );

		if( i % 20 == 0 )
		{
			SHELLEXECUTEINFO ShExecInfo = {0};
			ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
			ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
			ShExecInfo.hwnd = NULL;
			ShExecInfo.lpVerb = NULL;
			ShExecInfo.lpFile = VPKBIN;		
			ShExecInfo.lpParameters = args;	
			ShExecInfo.lpDirectory = NULL;
			ShExecInfo.nShow = SW_SHOW;
			ShExecInfo.hInstApp = NULL;	
			ShellExecuteEx(&ShExecInfo);
			ShowWindow(ShExecInfo.hwnd, SW_HIDE);
			WaitForSingleObject(ShExecInfo.hProcess,INFINITE);

			args[0] = '\0';
			Q_strcat( args, "x \"../../common/dota 2 beta/dota/pak01_dir.vpk\"", MAX_COMMAND_SIZE );
		}
	}

	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = VPKBIN;		
	ShExecInfo.lpParameters = args;	
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_SHOW;
	ShExecInfo.hInstApp = NULL;	
	ShellExecuteEx(&ShExecInfo);
	WaitForSingleObject(ShExecInfo.hProcess,INFINITE);

	filesystem->RemoveSearchPaths( "DOTA" );

	Msg("PostProcessDota2: %f seconds\n", Plat_FloatTime() - start );
}
#else
void PostProcessDota2( const char *pPath )
{
}
#endif // 0
