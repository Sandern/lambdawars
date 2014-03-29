//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "editorsystem.h"

#include <filesystem.h>

#include "wars_flora.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
	extern bool ExtractKeyvalue( void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, char *szValue, int iMaxLen );
#endif

static CEditorSystem g_EditorSystem;

CEditorSystem *EditorSystem()
{
	return &g_EditorSystem;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *CEditorSystem::VmfToKeyValues( const char *pszVmf )
{
	CUtlBuffer vmfBuffer;
	vmfBuffer.SetBufferType( true, true );

	vmfBuffer.SeekPut( CUtlBuffer::SEEK_HEAD, 0 );
	vmfBuffer.PutString( "\"VMFfile\"\r\n{" );

	KeyValues *pszKV = NULL;

	if ( g_pFullFileSystem->ReadFile( pszVmf, NULL, vmfBuffer ) )
	{
		vmfBuffer.SeekPut( CUtlBuffer::SEEK_TAIL, 0 );
		vmfBuffer.PutString( "\r\n}" );
		vmfBuffer.PutChar( '\0' );

		pszKV = new KeyValues("");

		if ( !pszKV->LoadFromBuffer( "", vmfBuffer ) )
		{
			pszKV->deleteThis();
			pszKV = NULL;
		}
	}

	return pszKV;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::KeyValuesToVmf( KeyValues *pKV, CUtlBuffer &vmf )
{
	for ( KeyValues *pKey = m_pKVVmf->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		pKey->RecursiveSaveToFile( vmf, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::LoadVmf( const char *pszVmf )
{
	V_snprintf( m_szCurrentVmf, sizeof( m_szCurrentVmf ), "%s", pszVmf );

	KeyValues *pKV = VmfToKeyValues( pszVmf );

	if ( !pKV )
	{
		*m_szCurrentVmf = 0;
		return;
	}

	ParseVmfFile( pKV );
	pKV->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::LoadCurrentVmf()
{
	char tmp[MAX_PATH*4];
	BuildCurrentVmfPath( tmp, sizeof(tmp) );

	if( !g_pFullFileSystem->FileExists( tmp ) )
	{
		Warning( "Expected file unavailable (moved, deleted).\n" );
		return;
	}

	LoadVmf( tmp );
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::SaveCurrentVmf()
{
	Assert( V_strlen( m_szCurrentVmf ) > 0 );

	if ( !g_pFullFileSystem->FileExists( m_szCurrentVmf ) )
	{
		Warning( "Expected file unavailable (moved, deleted).\n" );
		return;
	}

	ApplyChangesToVmfFile();

	CUtlBuffer buffer;
	KeyValuesToVmf( m_pKVVmf, buffer );

	g_pFullFileSystem->WriteFile( m_szCurrentVmf, NULL, buffer );
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CEditorSystem::GetCurrentVmfPath()
{
	return m_szCurrentVmf;
}

bool CEditorSystem::IsMapLoaded()
{
	return m_szCurrentVmf[0] != 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::ParseVmfFile( KeyValues *pKeyValues )
{
	Assert( pKeyValues );

	if ( m_pKVVmf )
		m_pKVVmf->deleteThis();

	m_pKVVmf = pKeyValues->MakeCopy();

	KeyValues *pVersionKey = m_pKVVmf->FindKey( "versioninfo" );
	if( !pVersionKey )
	{
		Warning( "Could not parse VMF. versioninfo key missing!\n" );
		return;
	}

#ifndef CLIENT_DLL
	const int iMapVersion = pVersionKey->GetInt( "mapversion", -1 );
	if( iMapVersion != gpGlobals->mapversion )
	{
		Warning( "Could not parse VMF. Map version does not match! (%d != %d)\n", iMapVersion, gpGlobals->mapversion );
		return;
	}
#endif // CLIENT_DLL

	for ( KeyValues *pKey = m_pKVVmf->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		if ( V_strcmp( pKey->GetName(), "entity" ) )
			continue;

		const bool bLightEntity = !V_stricmp( pKey->GetString( "classname" ), "light_deferred" ) || !V_stricmp( pKey->GetString( "classname" ), "env_deferred_light" );
		//const bool bGlobalLightEntity = !V_stricmp( pKey->GetString( "classname" ), "light_deferred_global" );

		if ( !bLightEntity )
			continue;

#if 0
		const char *pszDefault = "";
		const char *pszTarget = pKey->GetString( "targetname", pszDefault );
		const char *pszParent = pKey->GetString( "parentname", pszDefault );

		if ( pszTarget != pszDefault && *pszTarget ||
			pszParent != pszDefault && *pszParent )
			continue;

		def_light_editor_t *pLight = new def_light_editor_t();

		if ( !pLight )
			continue;

		pLight->ApplyKeyValueProperties( pKey );
		pLight->iEditorId = pKey->GetInt( "id" );

		AddEditorLight( pLight );
#endif // 0
	}
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::ApplyChangesToVmfFile()
{
	CUtlVector< KeyValues* > listToRemove;

	CUtlMap< int, CBaseEntity * > updatedEntities( 0, 0, DefLessFunc( int ) );
	CUtlVector< CBaseEntity * > newEntities;
	CollectNewAndUpdatedEntities( updatedEntities, newEntities );

	int iHighestId = 0;
	int iHighestHammerId = 0;

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

		}
	}

	// Add new entities
	char szValue[256];
	FOR_EACH_VEC( newEntities, entIdx )
	{
		CBaseEntity *pNewEnt = newEntities[entIdx];

		KeyValues *pKVNew = new KeyValues("data");

		// Try extract fields
		for ( datamap_t *dmap = pNewEnt->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
		{
			int iNumFields = dmap->dataNumFields;
			typedescription_t 	*pField;
			typedescription_t *pFields = dmap->dataDesc;
			for ( int i = 0; i < iNumFields; i++ )
			{
				pField = &pFields[i];
				if( !(pField->flags & FTYPEDESC_KEY) )
					continue;

				if( pKVNew->IsEmpty( pField->externalName ) )
				{
					szValue[0] = 0;
					if ( ::ExtractKeyvalue( pNewEnt, pFields, iNumFields, pField->externalName, szValue, 256 ) )
					{
						pKVNew->SetString( pField->externalName, szValue );
					}
				}
			}
		}

		// Write essential fields
		pKVNew->SetName( "entity" );
		pKVNew->SetInt( "id", iHighestId );
		pKVNew->SetInt( "hammerid", iHighestHammerId );
		pKVNew->SetString( "classname", pNewEnt->GetClassname() );

		// Write Common fields
		pKVNew->SetString( "model", STRING( pNewEnt->GetModelName() ) );
		pKVNew->SetString( "origin", UTIL_VarArgs( "%.2f %.2f %.2f", XYZ( pNewEnt->GetAbsOrigin() ) ) );
		pKVNew->SetString( "angles", UTIL_VarArgs( "%.2f %.2f %.2f", XYZ( pNewEnt->GetAbsAngles() ) ) );

		m_pKVVmf->AddSubKey( pKVNew );

		iHighestId++;
		iHighestHammerId++;
	}

	// Delete
	FOR_EACH_VEC( listToRemove, i )
		m_pKVVmf->RemoveSubKey( listToRemove[i] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::CollectNewAndUpdatedEntities( CUtlMap< int, CBaseEntity * > &updatedEntities, CUtlVector< CBaseEntity * > &newEntities )
{
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		bool bIsFloraEnt = !V_stricmp( pEnt->GetClassname(), "wars_flora" );
		if( bIsFloraEnt )
		{
			CWarsFlora *pFloraEnt = dynamic_cast<CWarsFlora *>( pEnt );
			if( pFloraEnt && pFloraEnt->IsEditorManaged() )
			{
				int iHammerID = pEnt->GetHammerID();
				if( iHammerID != 0 )
				{
					updatedEntities.Insert( iHammerID, pEnt );
				}
				else
				{
					newEntities.AddToTail( pEnt );
				}
			}
		}

		pEnt = gEntList.NextEnt( pEnt );
	}
}
#endif // CLIENT_DLL

#if 0
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::ApplyLightsToCurrentVmfFile()
{
	CUtlVector< KeyValues* > listKeyValues;
	CUtlVector< KeyValues* > listToRemove;
	//GetKVFromAll( listKeyValues );

	int nextId = 0;

	for ( KeyValues *pKey = m_pKVVmf->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		if ( V_strcmp( pKey->GetName(), "entity" ) )
			continue;

		const int iTargetID = pKey->GetInt( "id", -1 );
		const int iHammerID = pKey->GetInt( "hammerid", -1 );

		if ( iTargetID < 0 || iHammerID < 0 )
			continue;

		nextId = MAX( nextId, iTargetID );

		KeyValues *pSrcKey = NULL;

		FOR_EACH_VEC( listKeyValues, iKeys )
		{
			const int iSourceID = listKeyValues[iKeys]->GetInt( "id", -1 );
			if ( iSourceID < 0 )
				continue;

			if ( iSourceID != iTargetID )
				continue;

			pSrcKey = listKeyValues[iKeys];
			break;
		}

		bool bIsDeferredLight = !V_stricmp( pKey->GetString( "classname" ), "light_deferred" ) || !V_stricmp( pKey->GetString( "classname" ), "env_deferred_light" );

		const char *pszDefault = "";
		const char *pszTarget = pKey->GetString( "targetname", pszDefault );
		const char *pszParent = pKey->GetString( "parentname", pszDefault );

		if ( pSrcKey == NULL )
		{
			if ( bIsDeferredLight &&
				(pszTarget == pszDefault || !*pszTarget) &&
				(pszParent == pszDefault || !*pszParent) )
				listToRemove.AddToTail( pKey );
			continue;
		}

		Assert( bIsDeferredLight );
		Assert( (pszTarget == NULL || !*pszTarget) && (pszParent == NULL || !*pszParent) );

		for ( KeyValues *pValue = pSrcKey->GetFirstValue(); pValue;
			pValue = pValue->GetNextValue() )
		{
			const char *pszKeyName = pValue->GetName();
			const char *pszKeyValue = pValue->GetString();

			pKey->SetString( pszKeyName, pszKeyValue );
		}
	}

	nextId++;

#if 0
	FOR_EACH_VEC( m_hEditorLights, iLight )
	{
		def_light_editor_t *pLight = assert_cast< def_light_editor_t* >( m_hEditorLights[ iLight ] );

		if ( pLight->iEditorId != -1 )
			continue;

		pLight->iEditorId = nextId;

		KeyValues *pKVNew = GetKVFromLight( pLight );
		pKVNew->SetName( "entity" );
		pKVNew->SetString( "classname", "light_deferred" );
		m_pKVVmf->AddSubKey( pKVNew );

		nextId++;
	}
#endif // 0
	//FOR_EACH_VEC( listToRemove, i )
	//	m_pKVVmf->RemoveSubKey( listToRemove[i] );

	//if ( GetKVGlobalLight() != NULL )
	//	ApplyLightStateToKV( m_EditorGlobalState );

	//FOR_EACH_VEC( listKeyValues, i )
	//	listKeyValues[ i ]->deleteThis();
}
#endif // 0

extern const char *COM_GetModDirectory( void );

void CEditorSystem::BuildVmfPath( char *pszOut, int maxlen, bool bMakeRelative )
{
	//char tmp[MAX_PATH*4];
	char tmp2[MAX_PATH*4];

	/*if( Q_strcmp( deferred_lighteditor_defaultvmfpath.GetString(), "" ) != 0 &&
		g_pFullFileSystem->IsDirectory( deferred_lighteditor_defaultvmfpath.GetString() ) )
	{
		Q_strcpy( tmp, deferred_lighteditor_defaultvmfpath.GetString() );
	}
	else
	{
		V_snprintf( tmp, sizeof(tmp), "%s/mapsrc", COM_GetModDirectory() );
		V_FixSlashes( tmp );
	}*/
	char tmp[MAX_PATH*4];
	V_strncpy( tmp, "mapsrc", MAX_PATH*4 );

	if ( bMakeRelative && g_pFullFileSystem->FullPathToRelativePath( tmp, tmp2, sizeof(tmp2) ) )
		V_strcpy( tmp, tmp2 );
	else if ( !bMakeRelative && g_pFullFileSystem->RelativePathToFullPath( tmp, "MOD", tmp2, sizeof(tmp2) ) )
		V_strcpy( tmp, tmp2 );

	V_snprintf( pszOut, maxlen, "%s", tmp );
}

bool CEditorSystem::BuildCurrentVmfPath( char *pszOut, int maxlen )
{
#ifdef CLIENT_DLL
	char pszLevelname[_MAX_PATH];
	V_FileBase(engine->GetLevelName(), pszLevelname, _MAX_PATH);
#else
	const char *pszLevelname = STRING(gpGlobals->mapname);
#endif
	if ( !pszLevelname || !*pszLevelname )
		return false;

	char tmp[MAX_PATH*4];
	char tmp2[MAX_PATH*4];

	BuildVmfPath( tmp2, sizeof(tmp2) );
	V_snprintf( tmp, sizeof(tmp), "%s/%s.vmf", tmp2, pszLevelname );
	V_FixSlashes( tmp );

	if ( g_pFullFileSystem->FullPathToRelativePath( tmp, tmp2, sizeof(tmp2) ) )
		V_strcpy( tmp, tmp2 );

	V_snprintf( pszOut, maxlen, "%s", tmp );
	return true;
}