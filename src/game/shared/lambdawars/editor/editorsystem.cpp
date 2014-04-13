//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "editorsystem.h"
#include "wars_flora.h"
#include "collisionutils.h"

#include "warseditor/iwars_editor_storage.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Deferred helper
static void CalcBoundaries( Vector *list, const int &num, Vector &min, Vector &max )
{
	Assert( num > 0 );

	min = *list;
	max = *list;

	for ( int i = 1; i < num; i++ )
	{
		for ( int x = 0; x < 3; x++ )
		{
			min[ x ] = MIN( min[ x ], list[ i ][ x ] );
			max[ x ] = MAX( max[ x ], list[ i ][ x ] );
		}
	}
}

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

	m_matHelper.Init( "deferred/editor_helper", TEXTURE_GROUP_OTHER );
	Assert( m_matHelper.IsValid() );

	m_iDragMode = EDITORDRAG_NONE;
	m_bDragSelectArmed = false;
	m_iLastMouse_x = 0;
	m_iLastMouse_y = 0;
	m_vecLastDragWorldPos.Init();

	m_bSelectionCenterLocked = false;
	m_EditorMode = EDITORINTERACTION_SELECT;
	m_iCurrentSelectedAxis = EDITORAXIS_NONE;
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
	m_hSelectedEntities.Purge();
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::OnEntityDeleted( CBaseEntity *pEntity )
{
	// Remember which editor managed entities got deleted
	//Msg("Entity %s deleted\n", pEntity->GetClassname());
	if( !m_MapManager.IsMapLoaded() )
		return;

	CWarsFlora *pFloraEnt = dynamic_cast<CWarsFlora *>( pEntity );
	if( pFloraEnt && pFloraEnt->IsEditorManaged() )
	{
		int iHammerId = pEntity->GetHammerID();
		if( iHammerId != 0 )
		{
			m_MapManager.AddDeletedHammerID( iHammerId );
		}
	}
}
#endif // CLIENT_DLL

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
	vStartPos = vPos + vCamOffset;
	vEndPos = vStartPos + (vMouseAim *  MAX_TRACE_LENGTH);

	//NDebugOverlay::Line( vStartPos, vEndPos, 0, 255, 0, true, 4.0f );

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

//-----------------------------------------------------------------------------
// Purpose: Delete selected entities/props. 
//-----------------------------------------------------------------------------
void CEditorSystem::DeleteSelection()
{
	if( !IsAnythingSelected() )
		return;

	KeyValues *pOperation = new KeyValues( "data" );
	pOperation->SetString("operation", "deleteflora");

	for( int i = 0; i < m_hSelectedEntities.Count(); i++ )
	{
		if( !m_hSelectedEntities[i] )
			continue;

		CWarsFlora *pFlora = dynamic_cast<CWarsFlora *>( m_hSelectedEntities[i].Get() );
		if( pFlora )
		{
			pOperation->AddSubKey( new KeyValues( "flora", "uuid", pFlora->GetFloraUUID() ) );
			pFlora->Remove();
		}
		else
		{
			m_hSelectedEntities[i]->Remove();
		}
	}

	m_hSelectedEntities.Purge();
	warseditorstorage->AddEntityToQueue( pOperation );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEditorSystem::GetNumSelected()
{
	return m_hSelectedEntities.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorSystem::IsAnythingSelected()
{
	return GetNumSelected() > 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::SetSelectionCenterLocked( bool locked )
{
	if ( locked )
		m_vecSelectionCenterCache = GetSelectionCenter();

	m_bSelectionCenterLocked = locked;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CEditorSystem::GetSelectionCenter()
{
	if ( !IsAnythingSelected() )
	{
		Assert( 0 );
		return vec3_origin;
	}

	if ( m_bSelectionCenterLocked )
		return m_vecSelectionCenterCache;

	CUtlVector< Vector > positions;

	FOR_EACH_VEC( m_hSelectedEntities, i )
	{
		if( !m_hSelectedEntities[ i ] )
			continue;
		positions.AddToTail( m_hSelectedEntities[ i ]->GetAbsOrigin() );
	}

#if 1 // median
	Vector min, max;

	CalcBoundaries( positions.Base(), positions.Count(), min, max );

	return min + ( max - min ) * 0.5f;
#else // arth mean
	Vector center = vec3_origin;

	FOR_EACH_VEC( positions, i )
		center += positions[ i ];

	return center / positions.Count();
#endif
}

QAngle CEditorSystem::GetSelectionOrientation()
{
	if ( !IsAnythingSelected() )
	{
		Assert( 0 );
		return vec3_angle;
	}

	if ( GetNumSelected() > 1 )
	{
		return vec3_angle;
	}
	else
	{
		Assert( m_hSelectedEntities.Count() == 1 );

		return m_hSelectedEntities[ 0 ]->GetAbsAngles();
	}
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::Update( float frametime )
{
	if( !IsActive() )
		return;

	UpdateEditorInteraction();

	if( !warseditorstorage )
		return;

	// Process commands from server
	KeyValues *pEntity = NULL;
	while( (pEntity = warseditorstorage->PopEntityFromQueue()) != NULL )
	{
		const char *pOperation = pEntity->GetString("operation", "");

		if( V_strcmp( pOperation, "create" ) == 0 )
		{
			const char *pClassname = pEntity->GetString("classname", "");
			KeyValues *pEntityValues = pEntity->FindKey("keyvalues");
			if( pClassname && pEntityValues )
			{
				const char *pModelname = pEntityValues->GetString("model", NULL);
				if( pModelname && modelinfo->GetModelIndex( pModelname ) < 0 )
				{
					//modelinfo->FindOrLoadModel( pModelname );
					warseditorstorage->AddEntityToQueue( pEntity );
					break; // Not loaded yet
				}

				CBaseEntity *pEnt = CreateEntityByName( pClassname );
				if( pEnt )
				{
					FOR_EACH_VALUE( pEntityValues, pValue )
					{
						pEnt->KeyValue( pValue->GetName(), pValue->GetString() );
					}

					//KeyValuesDumpAsDevMsg( pEntityValues );

					CWarsFlora *pFlora = dynamic_cast<CWarsFlora *>( pEnt );
					if( pFlora )
					{
						if( !pFlora->Initialize() )
						{
							pFlora->Release();
							Warning( "CEditorSystem: Failed to initialize flora entity on client with model %s\n", pEntityValues->GetString("model", "models/error.mdl") );
						}
					}
					else if( !pEnt->InitializeAsClientEntity( pEntityValues->GetString("model", "models/error.mdl"), false ) )
					{
						if( pEnt->GetBaseAnimating() )
							pEnt->GetBaseAnimating()->Release();
						Warning( "CEditorSystem: Failed to initialize entity on client with model %s\n", pEntityValues->GetString("model", "models/error.mdl") );
					}
				}
			}
		}
		else if( V_strcmp( pOperation, "deleteflora" ) == 0 )
		{
			for ( KeyValues *pKey = pEntity->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
			{
				if ( V_strcmp( pKey->GetName(), "flora" ) )
					continue;

				const char *uuid = pKey->GetString("uuid", NULL);
				if( !uuid )
					continue;

				CWarsFlora::RemoveFloraByUUID( uuid );
			}
		}

		pEntity->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::PostRender()
{
	if( !IsActive() )
		return;

	RenderSelection();
	RenderHelpers();
}
#endif // CLIENT_DLL