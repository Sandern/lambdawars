//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Responsible for loading the wars files and writing changes back
//		    The server is always responsible for writing
//			changes back to Wars files, due the lack of certain info on the client.
//
//=============================================================================//

#include "cbase.h"
#include "editorwarsmapmgr.h"
#include <filesystem.h>
#include "wars_flora.h"
#include "checksum_md5.h"
#include "wars_grid.h"

#ifdef ENABLE_PYTHON
#include "srcpy.h"
#endif // ENABLE_PYTHON

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
	extern bool ExtractKeyvalue( void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, char *szValue, int iMaxLen );
#endif

#ifndef CLIENT_DLL
#define VarArgs UTIL_VarArgs
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEditorWarsMapMgr::CEditorWarsMapMgr() : m_pKVWars(NULL)
{
	m_szCurrentMap[0] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorWarsMapMgr::IsMapLoaded()
{
	return m_szCurrentMap[0] != 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorWarsMapMgr::LoadCurrentMap()
{
	ClearLoadedMap();
	
	BuildCurrentWarsPath( m_szCurrentMap, sizeof( m_szCurrentMap ) );

	m_pKVWars = new KeyValues( "WarsMap" ); // VmfToKeyValues( m_szCurrentMap );
	if( !m_pKVWars->LoadFromFile( filesystem, m_szCurrentMap, NULL ) )
	{
		// Doesn't matter if it doesn't exists yet
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorWarsMapMgr::ClearLoadedMap()
{
	*m_szCurrentMap = 0;

	if( m_pKVWars )
	{
		m_pKVWars->deleteThis();
		m_pKVWars = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorWarsMapMgr::SaveCurrentMap()
{
#ifndef CLIENT_DLL
	if( !WriteChangesToWarsFile() )
	{
		return;
	}
	
	// Write changes to file
	Msg( "Saved %s...\n", m_szCurrentMap );
	m_pKVWars->SaveToFile( filesystem, m_szCurrentMap, NULL );
#endif // CLIENT_DLL
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorWarsMapMgr::FillEntityEntry( KeyValues *pEntityKey, CBaseEntity *pEnt, int iTargetID, int iHammerID )
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

		pFloraEnt->FillKeyValues( pEntityKey, -1 );
	}
}

static const char *GetUUID( KeyValues *pKey )
{
	const char *pUUID = pKey->GetString( "florauuid", NULL );
	if( pUUID )
		return pUUID;

	return pKey->GetString( "uuid", NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorWarsMapMgr::CollectNewAndUpdatedEntities( CUtlMap< const char*, CBaseEntity * > &updatedEntities, CUtlVector< CBaseEntity * > &newEntities )
{
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		bool bIsFloraEnt = !V_stricmp( pEnt->GetClassname(), "wars_flora" );
		if( bIsFloraEnt )
		{
			CWarsFlora *pFloraEnt = dynamic_cast<CWarsFlora *>( pEnt );
			if( pFloraEnt && pFloraEnt->IsEditorManaged() && m_DeletedUUIDS.Find( pFloraEnt->GetFloraUUID() ) == -1 )
			{
				// GetFloraUUID returns a pooled game string, so valid while the level exists
				updatedEntities.Insert( pFloraEnt->GetFloraUUID(), pEnt );
			}
		}

		pEnt = gEntList.NextEnt( pEnt );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorWarsMapMgr::WriteChangesToWarsFile()
{
	// Grid
	WarsGrid().SaveGridData( m_pKVWars );

	// Flora
	KeyValues *pFloraKey = m_pKVWars->FindKey( "flora", true );

	CUtlVector< KeyValues* > listToRemove;

	CUtlMap< const char*, CBaseEntity * > updatedEntities;
	SetDefLessFunc( updatedEntities );
	CUtlVector< CBaseEntity * > newEntities;
	CollectNewAndUpdatedEntities( updatedEntities, newEntities );

	// Update existing entities and determine removed entities
	for ( KeyValues *pKey = pFloraKey->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		if ( V_strcmp( pKey->GetName(), "entity" ) )
			continue;

		const char *pUUID = GetUUID( pKey );
		if( !pUUID )
			continue;

		int idx = updatedEntities.Find( pUUID );
		if ( idx != updatedEntities.InvalidIndex() )
		{
			CBaseEntity *pEnt = updatedEntities[idx];

			const char *pClassname = pKey->GetString( "classname", NULL );
			if( !pClassname || V_strcmp( pEnt->GetClassname(), pClassname ) != 0 ) 
			{
				LogError( VarArgs( "%s: Failed to apply changes to Wars file. Class names do not match: %s != %s for updating entity with uuid %s", 
					m_szCurrentMap, pEnt->GetClassname(), pClassname, pUUID ) );
				return false;
			}

			pKey->Clear();
			FillEntityEntry( pKey, pEnt, -1, -1 );

			// Updated, so remove from list
			updatedEntities.RemoveAt( idx );
		}
		else
		{
			// Deleted Editor managed entity
			listToRemove.AddToTail( pKey );
		}
	}

	// Remove deleted entities
	FOR_EACH_VEC( listToRemove, i )
		pFloraKey->RemoveSubKey( listToRemove[i] );

	// Add new entities
	FOR_EACH_MAP( updatedEntities, iter )
	{
		CBaseEntity *pNewEnt = updatedEntities[iter];

		KeyValues *pKVNew = new KeyValues("data");
		FillEntityEntry( pKVNew, pNewEnt, -1, -1 );

		pFloraKey->AddSubKey( pKVNew );
	}

	return true;
}
#endif // CLIENT_DLL


extern const char *COM_GetModDirectory( void );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorWarsMapMgr::BuildCurrentWarsPath( char *pszOut, int maxlen )
{
#ifdef CLIENT_DLL
	char pszLevelname[_MAX_PATH];
	V_FileBase(engine->GetLevelName(), pszLevelname, _MAX_PATH);
#else
	const char *pszLevelname = STRING(gpGlobals->mapname);
#endif
	if ( !pszLevelname || !*pszLevelname )
		return false;

	char vmfPath[MAX_PATH];
	char vmfFullPath[MAX_PATH];

	V_snprintf( vmfPath, sizeof(vmfPath), "maps/%s.wars", pszLevelname );
	V_FixSlashes( vmfPath );
	if ( g_pFullFileSystem->FullPathToRelativePath( vmfPath, vmfFullPath, sizeof(vmfFullPath) ) )
		V_strcpy( vmfPath, vmfFullPath );

	V_snprintf( pszOut, maxlen, "%s", vmfPath );
	return true;
}
