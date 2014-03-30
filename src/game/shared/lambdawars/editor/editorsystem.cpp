//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "editorsystem.h"
#include <filesystem.h>
#include "wars_flora.h"
#include "collisionutils.h"

#ifdef ENABLE_PYTHON
#include "srcpy.h"
#endif // ENABLE_PYTHON

#ifdef WIN32
#undef INVALID_HANDLE_VALUE
#include <windows.h>
#undef CopyFile
#endif // WIN32

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
	extern bool ExtractKeyvalue( void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, char *szValue, int iMaxLen );
#endif

#define VBSP_PATH "..\\..\\Alien Swarm\\bin\\vbsp.exe"

static CEditorSystem g_EditorSystem;

CEditorSystem *EditorSystem()
{
	return &g_EditorSystem;
}

bool CEditorSystem::Init()
{
#ifndef CLIENT_DLL
	gEntList.AddListenerEntity( this );
#else
	m_matSelect.Init( "deferred/editor_select", TEXTURE_GROUP_OTHER );
	Assert( m_matSelect.IsValid() );
#endif // CLIENT_DLL

	return true;
}

void CEditorSystem::Shutdown()
{
#ifndef CLIENT_DLL
	gEntList.RemoveListenerEntity( this );
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::LevelShutdownPreEntity() 
{
	ClearLoadedMap();
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
void CEditorSystem::ClearLoadedMap()
{
	if( m_pKVVmf == NULL ) 
		return;

	*m_szCurrentVmf = 0;
	m_pKVVmf->deleteThis();
	m_pKVVmf = NULL;

	SrcPySystem()->CallSignalNoArgs( SrcPySystem()->Get("editormapchanged", "core.signals", true) );
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
		ClearLoadedMap();
		return;
	}

	if( ParseVmfFile( pKV ) )
	{
		SrcPySystem()->CallSignalNoArgs( SrcPySystem()->Get("editormapchanged", "core.signals", true) );
	}

	pKV->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::LoadCurrentVmf()
{
	ClearLoadedMap();

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

	// Save VMF
	ApplyChangesToVmfFile();

	CUtlBuffer buffer;
	KeyValuesToVmf( m_pKVVmf, buffer );

	g_pFullFileSystem->WriteFile( m_szCurrentVmf, NULL, buffer );

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
		Warning( "Could not find \"%s\". Unable to update BSP.\n", vbspPath );
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
bool CEditorSystem::ParseVmfFile( KeyValues *pKeyValues )
{
	Assert( pKeyValues );

	if ( m_pKVVmf )
		m_pKVVmf->deleteThis();

	m_pKVVmf = pKeyValues->MakeCopy();

	KeyValues *pVersionKey = m_pKVVmf->FindKey( "versioninfo" );
	if( !pVersionKey )
	{
		ClearLoadedMap();
		Warning( "Could not parse VMF. versioninfo key missing!\n" );
		return false;
	}

#ifndef CLIENT_DLL
	const int iMapVersion = pVersionKey->GetInt( "mapversion", -1 );
	if( iMapVersion != gpGlobals->mapversion )
	{
		ClearLoadedMap();
		Warning( "Could not parse VMF. Map version does not match! (%d != %d)\n", iMapVersion, gpGlobals->mapversion );
		return false;
	}
#endif // CLIENT_DLL

	return true;
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::FillEntityEntry( KeyValues *pEntityKey, CBaseEntity *pEnt, int iTargetID, int iHammerID )
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
	pEntityKey->SetInt( "id", iTargetID );
	pEntityKey->SetInt( "hammerid", iHammerID );
	pEntityKey->SetString( "classname", pEnt->GetClassname() );

	// Write Common fields
	pEntityKey->SetString( "model", STRING( pEnt->GetModelName() ) );
	pEntityKey->SetString( "origin", UTIL_VarArgs( "%.2f %.2f %.2f", XYZ( pEnt->GetAbsOrigin() ) ) );
	pEntityKey->SetString( "angles", UTIL_VarArgs( "%.2f %.2f %.2f", XYZ( pEnt->GetAbsAngles() ) ) );

	CWarsFlora *pFloraEnt = dynamic_cast<CWarsFlora *>( pEnt );
	if( pFloraEnt )
	{
		pFloraEnt->FillKeyValues( pEntityKey );
	}
}

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
			pKey->Clear();
			CBaseEntity *pEnt = updatedEntities[idx];
			FillEntityEntry( pKey, pEnt, iTargetID, iHammerID );

			// Updated, so remove from list
			updatedEntities.RemoveAt( idx );
		}
		else if( m_DeletedHammerIDs.HasElement( iHammerID ) )
		{
			// Deleted Editor managed entity
			listToRemove.AddToTail( pKey );
		}
	}

	// Remove deleted entities
	FOR_EACH_VEC( listToRemove, i )
		m_pKVVmf->RemoveSubKey( listToRemove[i] );

	// Add new entities
	FOR_EACH_VEC( newEntities, entIdx )
	{
		CBaseEntity *pNewEnt = newEntities[entIdx];

		KeyValues *pKVNew = new KeyValues("data");
		FillEntityEntry( pKVNew, pNewEnt, iHighestId++, iHighestHammerId++ );

		m_pKVVmf->AddSubKey( pKVNew );
	}
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::OnEntityDeleted( CBaseEntity *pEntity )
{
	// Remember which editor managed entities got deleted
	//Msg("Entity %s deleted\n", pEntity->GetClassname());
	if( !m_pKVVmf )
		return;

	CWarsFlora *pFloraEnt = dynamic_cast<CWarsFlora *>( pEntity );
	if( pFloraEnt && pFloraEnt->IsEditorManaged() )
	{
		int iHammerId = pEntity->GetHammerID();
		if( iHammerId != 0 )
		{
			m_DeletedHammerIDs.AddToTail( iHammerId );
		}
	}
}
#endif // CLIENT_DLL

extern const char *COM_GetModDirectory( void );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
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

#define EDITOR_MOUSE_TRACE_BOX_MINS Vector(-8.0, -8.0, 0.0)
#define EDITOR_MOUSE_TRACE_BOX_MAXS Vector(8.0, 8.0, 8.0)

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::DoSelect( CHL2WarsPlayer *pPlayer )
{
	Vector vPos = pPlayer->Weapon_ShootPosition();
	const Vector &vMouseAim = pPlayer->GetMouseAim();
	const Vector &vCamOffset = pPlayer->GetCameraOffset();

	trace_t tr;
	Vector vStartPos, vEndPos;

	// Note: Filter player "controlled unit" rather than the player. 
	//		 The player is already not traceable. When activating mouse in direct control we need to ignore the unit.
	vStartPos = vPos + vCamOffset;
	vEndPos = vStartPos + (vMouseAim *  MAX_TRACE_LENGTH);

	NDebugOverlay::Line( vStartPos, vEndPos, 0, 255, 0, true, 4.0f );

	Ray_t pickerRay;
	pickerRay.Init( vStartPos, vEndPos, EDITOR_MOUSE_TRACE_BOX_MINS, EDITOR_MOUSE_TRACE_BOX_MAXS );

	CBaseEntity *pBest = NULL;
	float fBestDistance = 0;

#ifdef CLIENT_DLL
	for( CBaseEntity *pEntity = ClientEntityList().FirstBaseEntity(); pEntity; pEntity = ClientEntityList().NextBaseEntity( pEntity ) )
#else
	for( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity != NULL; pEntity = gEntList.NextEnt( pEntity ) )
#endif // CLIENT_DLL
	{
		CWarsFlora *pFlora = dynamic_cast<CWarsFlora *>( pEntity );
		if( pFlora )
		{
			CBaseTrace trace;
			if ( IntersectRayWithBox( pickerRay,
				pFlora->GetAbsOrigin() + pFlora->CollisionProp()->OBBMins(), pFlora->GetAbsOrigin() + pFlora->CollisionProp()->OBBMaxs(),
				0.5f, &trace ) )
			{
				float fDistance = tr.endpos.DistTo( vStartPos );
				if( !pBest || fDistance < fBestDistance )
				{
					pBest = pEntity;
					fBestDistance = fDistance;
				}
				
				break;
			}
		}
	}

	if( pBest )
	{
		if( !IsSelected( pBest ) )
		{
			Select( pBest );
		}
		else
		{
			Deselect( pBest );
		}
		//NDebugOverlay::Box( pBest->GetAbsOrigin(), -Vector(8, 8, 8), Vector(8, 8, 8), 0, 255, 0, 255, 5.0f);
	}
}

#ifdef CLIENT_DLL
void CEditorSystem::RenderSelection()
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_matSelect );

	//CUtlVector< def_light_t* > hSelectedLights;
	//hSelectedLights.AddVectorToTail( m_hSelectedLights );
	//hSelectedLights.Sort( DefLightSort );

	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_matSelect );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 6 * m_hSelectedEntities.Count() );

	FOR_EACH_VEC( m_hSelectedEntities, i )
	{
		CBaseEntity *l = m_hSelectedEntities[ i ];
		if( !l )
			continue;

		Vector position = l->GetAbsOrigin();
		float flSize = l->CollisionProp()->BoundingRadius();
		Vector color( 0.5f, 1.0f, 0.25f );

		const Vector &mins = l->CollisionProp()->OBBMins();
		const Vector &maxs = l->CollisionProp()->OBBMaxs();

		/*Vector points[8] = {
			Vector( flSize, flSize, flSize ),
			Vector( -flSize, flSize, flSize ),
			Vector( flSize, -flSize, flSize ),
			Vector( flSize, flSize, -flSize ),
			Vector( -flSize, -flSize, flSize ),
			Vector( flSize, -flSize, -flSize ),
			Vector( -flSize, flSize, -flSize ),
			Vector( -flSize, -flSize, -flSize ),
		};*/

		Vector points[8] = {
			Vector( maxs.x, maxs.y, maxs.z ),
			Vector( mins.x, maxs.y, maxs.z ),
			Vector( maxs.x, mins.y, maxs.z ),
			Vector( maxs.x, maxs.y, mins.z ),
			Vector( mins.x, mins.y, maxs.z ),
			Vector( maxs.x, mins.y, mins.z ),
			Vector( mins.x, maxs.y, mins.z ),
			Vector( mins.x, mins.y, mins.z ),
		};

		for ( int p = 0; p < 8; p++ )
			points[p] += position;

		int iPlaneSetup[6][4] = {
			{7,5,3,6},
			{0,3,5,2},
			{1,4,7,6},
			{0,1,6,3},
			{2,5,7,4},
			{0,2,4,1},
		};

		Vector2D uvs[4] = {
			Vector2D( 0, 0 ),
			Vector2D( 1, 0 ),
			Vector2D( 1, 1 ),
			Vector2D( 0, 1 ),
		};

		//if ( !GetLightingManager()->IsLightRendered( l ) )
		//	color.Init( 1.0f, 0.5f, 0.25f );

		for ( int p = 0; p < 6; p++ )
		{
			for ( int v = 0; v < 4; v++ )
			{
				meshBuilder.Position3fv( points[iPlaneSetup[p][v]].Base() );
				meshBuilder.Color3fv( color.Base() );
				meshBuilder.TexCoord2fv( 0, uvs[v].Base() );
				meshBuilder.AdvanceVertex();
			}
		}
	}

	meshBuilder.End();
	pMesh->Draw();
}

void CEditorSystem::PostRender()
{
	RenderSelection();
}
#endif // CLIENT_DLL