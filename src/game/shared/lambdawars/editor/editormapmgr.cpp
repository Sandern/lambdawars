//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Responsible for loading the vmf files and writing changes back
//			to both VMF and BSP. The server is always responsible for writing
//			changes back to VMF, due the lack of certain info on the client,
//			like the entity Hammer ID.
//
//=============================================================================//

#include "cbase.h"
#include "editormapmgr.h"
#include <filesystem.h>
#include "wars_flora.h"
#include "checksum_md5.h"

#ifdef WIN32
#undef INVALID_HANDLE_VALUE
#include <windows.h>
#undef CopyFile
#endif // WIN32

#ifdef ENABLE_PYTHON
#include "srcpy.h"
#endif // ENABLE_PYTHON

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool ExtractKeyvalue( void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, char *szValue, int iMaxLen );

#ifndef CLIENT_DLL
#define VarArgs UTIL_VarArgs
#endif // CLIENT_DLL

#define VBSP_PATH "..\\..\\Alien Swarm\\bin\\vbsp.exe"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEditorMapMgr::CEditorMapMgr() : m_pKVVmf(NULL)
{
	m_szCurrentVmf[0] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorMapMgr::ClearLoadedMap()
{
	*m_szCurrentVmf = 0;
	m_iFloraVisGroupId = -1;

	if( m_pKVVmf ) 
	{
		m_pKVVmf->deleteThis();
		m_pKVVmf = NULL;

		SrcPySystem()->CallSignalNoArgs( SrcPySystem()->Get("editormapchanged", "core.signals", true) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorMapMgr::LoadVmf( const char *pszVmf, bool bDispatchSignal )
{
	V_snprintf( m_szCurrentVmf, sizeof( m_szCurrentVmf ), "%s", pszVmf );

	KeyValues *pKV = VmfToKeyValues( pszVmf );

	if ( !pKV )
	{
		ClearLoadedMap();
		return;
	}

	if( ParseVmfFile( pKV ) )
	{
		if( bDispatchSignal )
			SrcPySystem()->CallSignalNoArgs( SrcPySystem()->Get("editormapchanged", "core.signals", true) );
	}

	pKV->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorMapMgr::LoadCurrentVmf()
{
	ClearLoadedMap();

	char tmp[MAX_PATH*4];
	BuildCurrentVmfPath( tmp, sizeof(tmp) );

	if( !g_pFullFileSystem->FileExists( tmp ) )
	{
		LogError( VarArgs( "Expected file unavailable (moved, deleted): %s", tmp ) );
		return;
	}

	Msg( "Loading \"%s\"\n", tmp );
	LoadVmf( tmp );
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorMapMgr::SaveCurrentVmf()
{
	Assert( V_strlen( m_szCurrentVmf ) > 0 );

	if ( !g_pFullFileSystem->FileExists( m_szCurrentVmf ) )
	{
		LogError( "Expected file unavailable (moved, deleted)." );
		return;
	}

	// Load VMF file again to make sure we have the latest changes
	LoadVmf( m_szCurrentVmf, false );

	// Collect and update VMF Keyvalues
	if( !ApplyChangesToVmfFile() )
	{
		// Failed or nothing changed
		return;
	}

	// Store original file size
	unsigned int iOriFileSize = g_pFullFileSystem->Size( m_szCurrentVmf );

	// Write changes to file
	CUtlBuffer buffer;
	KeyValuesToVmf( m_pKVVmf, buffer );

	g_pFullFileSystem->WriteFile( m_szCurrentVmf, NULL, buffer );

	// Check if changed by filesize. TODO: Better check?
	if( iOriFileSize == g_pFullFileSystem->Size( m_szCurrentVmf ) )
	{
		Msg( "SaveCurrentVmf: VMF \"%s\" did not changed. Not updating BSP\n", m_szCurrentVmf );
		return;
	}

	// Save BSP
#ifdef WIN32
	char vbspPath[MAX_PATH];
	char gameDir[MAX_PATH];

	g_pFullFileSystem->RelativePathToFullPath( VBSP_PATH, NULL, vbspPath, sizeof(vbspPath) );
	g_pFullFileSystem->RelativePathToFullPath( ".", "MOD", gameDir, sizeof(gameDir) );

	DevMsg("VBSP: %s\n", vbspPath );
	DevMsg("GameDir: %s\n", gameDir );

	if ( !g_pFullFileSystem->FileExists( vbspPath ) )
	{
		LogError( VarArgs( "Could not find \"%s\". Unable to update BSP.", vbspPath ) );
		return;
	}

	char parameters[MAX_PATH];
	V_snprintf( parameters, MAX_PATH, "-onlyents -game \"%s\" \"%s\"", gameDir, m_szCurrentVmf );
	DevMsg("VBSP Parameters: %s\n", parameters );

    SHELLEXECUTEINFO shExecInfo;

    shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);

    shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NO_CONSOLE;
    shExecInfo.hwnd = NULL;
    shExecInfo.lpVerb = NULL;
    shExecInfo.lpFile = vbspPath;
    shExecInfo.lpParameters = parameters;
    shExecInfo.lpDirectory = NULL;
    shExecInfo.nShow = SW_FORCEMINIMIZE;
    shExecInfo.hInstApp = NULL;

    ShellExecuteEx(&shExecInfo);
	WaitForSingleObject(shExecInfo.hProcess,INFINITE);

	// Copy to Maps directory
	char srcPath[MAX_PATH];
	char destPath[MAX_PATH];
	const char *pszLevelname = STRING(gpGlobals->mapname);
	V_snprintf( srcPath, sizeof(srcPath), "mapsrc\\%s.bsp", pszLevelname );
	V_snprintf( destPath, sizeof(destPath), "maps\\%s.bsp", pszLevelname );
	DevMsg("Copy %s to %s\n", srcPath, destPath );
	engine->CopyFile( srcPath, destPath );
#endif // WIN32
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CEditorMapMgr::GetCurrentVmfPath()
{
	return m_szCurrentVmf;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorMapMgr::IsMapLoaded()
{
	return m_szCurrentVmf[0] != 0;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Helper to get map version on client
//-----------------------------------------------------------------------------
static int GetMapVersion( const char *pszLevelname )
{
	char destPath[MAX_PATH];
	V_snprintf( destPath, sizeof(destPath), "%s", pszLevelname );

	FileHandle_t f = filesystem->Open(destPath, "rb");
	if ( f == FILESYSTEM_INVALID_HANDLE ) {
		Warning( "GetMapVersion: could not open \"%s\"\n", destPath );
		return -1;
	}
	unsigned int fileSize = filesystem->Size( f );
	if( fileSize < sizeof(BSPHeader_t) )
	{
		Warning( "GetMapVersion: file at \"%s\" does not contain a valid bsp header\n", destPath );
		filesystem->Close(f);
		return -1;
	}
	BSPHeader_t header;
	filesystem->Read(&header, sizeof(BSPHeader_t), f);
	filesystem->Close(f);
	if( header.m_nVersion < MINBSPVERSION)
	{
		Warning( "GetMapVersion: file at \"%s\" does not fullfill minimum required bsp version %d (instead found %d)\n", destPath, MINBSPVERSION, header.m_nVersion );
		return -1;
	}

	return header.mapRevision;
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorMapMgr::ParseVmfFile( KeyValues *pKeyValues )
{
	Assert( pKeyValues );

	if ( m_pKVVmf )
		m_pKVVmf->deleteThis();

	m_pKVVmf = pKeyValues->MakeCopy();

	KeyValues *pVersionKey = m_pKVVmf->FindKey( "versioninfo" );
	if( !pVersionKey )
	{
		LogError( VarArgs( "%s: Could not parse VMF. versioninfo key missing!", m_szCurrentVmf ) );
		ClearLoadedMap();
		return false;
	}

#ifndef CLIENT_DLL
	int iBspMapVersion = gpGlobals->mapversion;
#else
	int iBspMapVersion = GetMapVersion( engine->GetLevelName() );
#endif // CLIENT_DLL
	const int iMapVersion = pVersionKey->GetInt( "mapversion", -1 );
	if( iMapVersion != iBspMapVersion )
	{
		// Just print a warning
		Warning( "%s: VMF Map version does not match BSP version! (vmf %d != bsp %d)\n", m_szCurrentVmf, iMapVersion, iBspMapVersion );
		//LogError(  VarArgs("%s: Could not parse VMF. Map version does not match! (vmf %d != bsp %d)", m_szCurrentVmf, iMapVersion, iBspMapVersion) );
		//ClearLoadedMap();
		//return false;
	}

	return true;
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorMapMgr::FillEntityEntry( KeyValues *pEntityKey, CBaseEntity *pEnt, int iTargetID, int iHammerID )
{
#if 0
	char szValue[256];

	// Try extract fields
	// TODO: a lot of fields are not extracted correctly
	for ( datamap_t *dmap = pEnt->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
	{
		int iNumFields = dmap->dataNumFields;
		typedescription_t 	*pField;
		typedescription_t *pFields = dmap->dataDesc;
		for ( int i = 0; i < iNumFields; i++ )
		{
			pField = &pFields[i];
			if( !(pField->flags & FTYPEDESC_KEY) )
				continue;

			if( pEntityKey->IsEmpty( pField->externalName ) )
			{
				szValue[0] = 0;
				if ( ::ExtractKeyvalue( pEnt, pFields, iNumFields, pField->externalName, szValue, 256 ) )
				{
					pEntityKey->SetString( pField->externalName, szValue );
				}
			}
		}
	}
#endif // 0

	// Write essential fields
	pEntityKey->SetName( "entity" );
	if( iTargetID != -1 )
		pEntityKey->SetInt( "id", iTargetID );
	if( iHammerID != -1 )
		pEntityKey->SetInt( "hammerid", iHammerID );
	pEntityKey->SetString( "classname", pEnt->GetClassname() );

	// Write Common fields
	pEntityKey->SetString( "model", STRING( pEnt->GetModelName() ) );
	pEntityKey->SetString( "origin", UTIL_VarArgs( "%.2f %.2f %.2f", XYZ( pEnt->GetAbsOrigin() ) ) );
	pEntityKey->SetString( "angles", UTIL_VarArgs( "%.2f %.2f %.2f", XYZ( pEnt->GetAbsAngles() ) ) );

	CWarsFlora *pFloraEnt = dynamic_cast<CWarsFlora *>( pEnt );
	if( pFloraEnt )
	{
		// Generate UUID if we don't have one yet. Hammer doesn't knows about this and this is specific
		// to our ingame flora editor.
		if( !pFloraEnt->HasFloraUUID() )
		{
			pFloraEnt->GenerateFloraUUID();
		}

		pFloraEnt->FillKeyValues( pEntityKey, m_iFloraVisGroupId );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorMapMgr::ApplyChangesToVmfFile()
{
	bool bVMFChanged = false;

	CUtlVector< KeyValues* > listToRemove;

	CUtlMap< int, CBaseEntity * > updatedEntities( 0, 0, DefLessFunc( int ) );
	CUtlVector< CBaseEntity * > newEntities;
	CollectNewAndUpdatedEntities( updatedEntities, newEntities );

	int iHighestId = 0;
	int iHighestHammerId = 0;

	// Flora is no longer stored in vmf, so no need to create a visgroup
#if 0
	// Find or create the Wars Flora visgroup
	m_iFloraVisGroupId = -1;
	int iHighestVisGroupId = 0;
	KeyValues *pVisGroupsKey = m_pKVVmf->FindKey( "visgroups" );
	if( pVisGroupsKey )
	{
		FOR_EACH_TRUE_SUBKEY( pVisGroupsKey, pVisGroup )
		{
			if( V_strcmp( pVisGroup->GetString( "name", "None"), "Wars Flora" ) == 0 )
			{
				m_iFloraVisGroupId = pVisGroup->GetInt( "visgroupid", -1 );
				break;
			}

			iHighestVisGroupId = Max( iHighestVisGroupId, pVisGroup->GetInt( "visgroupid", -1 ) );
		}

		if( m_iFloraVisGroupId == -1 )
		{
			m_iFloraVisGroupId = iHighestVisGroupId + 1;

			KeyValues *pKVNewVisGroup = new KeyValues("visgroup");
			pKVNewVisGroup->SetString( "name", "Wars Flora" );
			pKVNewVisGroup->SetInt( "visgroupid", m_iFloraVisGroupId );
			pKVNewVisGroup->SetString( "color", "0 255 0" );
			pVisGroupsKey->AddSubKey( pKVNewVisGroup );

			bVMFChanged = true;
		}
	}
#endif // 0

	// Update existing entities and determine removed entities
	for ( KeyValues *pKey = m_pKVVmf->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		if ( V_strcmp( pKey->GetName(), "entity" ) )
			continue;

		const int iTargetID = pKey->GetInt( "id", -1 );
		const int iHammerID = pKey->GetInt( "hammerid", -1 );

		if ( iTargetID < 0 || iHammerID < 0 )
			continue;

		iHighestId = Max( iHighestId, iTargetID );
		iHighestHammerId = Max( iHighestHammerId, iHammerID );

		int idx = updatedEntities.Find( iHammerID );
		if ( idx != updatedEntities.InvalidIndex() )
		{
			CBaseEntity *pEnt = updatedEntities[idx];

			const char *pClassname = pKey->GetString( "classname", NULL );
			if( !pClassname || V_strcmp( pEnt->GetClassname(), pClassname ) != 0 ) 
			{
				LogError( VarArgs( "%s: Failed to apply changes to VMF file. Class names do not match: %s != %s for updating entity with hammerid %d", 
					m_szCurrentVmf, pEnt->GetClassname(), pClassname, iHammerID ) );
				return false;
			}

			pKey->Clear();
			FillEntityEntry( pKey, pEnt, iTargetID, iHammerID );
			bVMFChanged = true;

			// Updated, so remove from list
			updatedEntities.RemoveAt( idx );
		}
		else if( m_DeletedHammerIDs.HasElement( iHammerID ) )
		{
			// Deleted Editor managed entity
			listToRemove.AddToTail( pKey );
		}
	}

	// Start new id range
	iHighestId++;
	iHighestHammerId++;

	// Remove deleted entities
	FOR_EACH_VEC( listToRemove, i )
	{
		if( m_pKVVmf->ContainsSubKey( listToRemove[i] ) )
		{
			m_pKVVmf->RemoveSubKey( listToRemove[i] );
			bVMFChanged = true;
		}
	}

	// Add new entities
	FOR_EACH_VEC( newEntities, entIdx )
	{
		CBaseEntity *pNewEnt = newEntities[entIdx];

		KeyValues *pKVNew = new KeyValues("data");
		FillEntityEntry( pKVNew, pNewEnt, iHighestId++, iHighestHammerId++ );

		m_pKVVmf->AddSubKey( pKVNew );

		bVMFChanged = true;
	}

	return bVMFChanged;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorMapMgr::CollectNewAndUpdatedEntities( CUtlMap< int, CBaseEntity * > &updatedEntities, CUtlVector< CBaseEntity * > &newEntities )
{
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		bool bIsFloraEnt = !V_stricmp( pEnt->GetClassname(), "wars_flora" );
		if( bIsFloraEnt )
		{
			// Editor managed flora moved to a separate "wars" map data file
			CWarsFlora *pFloraEnt = dynamic_cast<CWarsFlora *>( pEnt );
			if( pFloraEnt && pFloraEnt->IsEditorManaged() )
			{
				int iHammerID = pEnt->GetHammerID();
				if( iHammerID != 0 )
				{
					if( m_DeletedHammerIDs.Find( iHammerID ) == -1 )
						m_DeletedHammerIDs.AddToTail( iHammerID );
				}
			}
		}

		pEnt = gEntList.NextEnt( pEnt );
	}
}

#endif // CLIENT_DLL


extern const char *COM_GetModDirectory( void );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorMapMgr::BuildCurrentVmfPath( char *pszOut, int maxlen )
{
#ifdef CLIENT_DLL
	char pszLevelname[_MAX_PATH];
	V_FileBase(engine->GetLevelName(), pszLevelname, _MAX_PATH);
#else
	const char *pszLevelname = STRING(gpGlobals->mapname);
#endif
	if ( !pszLevelname || !*pszLevelname )
		return false;

	char instancePath[MAX_PATH];
	char vmfPath[MAX_PATH];
	char vmfFullPath[MAX_PATH];

	V_snprintf( instancePath, sizeof(instancePath), "mapsrc/%s", pszLevelname );
	if( filesystem->FileExists( instancePath ) && filesystem->IsDirectory( instancePath ) )
	{
		V_snprintf( vmfPath, sizeof(vmfPath), "mapsrc/%s/%s_instance.vmf", pszLevelname, pszLevelname );
	}
	else
	{
		V_snprintf( vmfPath, sizeof(vmfPath), "mapsrc/%s.vmf", pszLevelname );
		
	}
	V_FixSlashes( vmfPath );
	if ( g_pFullFileSystem->FullPathToRelativePath( vmfPath, vmfFullPath, sizeof(vmfFullPath) ) )
		V_strcpy( vmfPath, vmfFullPath );

	V_snprintf( pszOut, maxlen, "%s", vmfPath );
	return true;
}
