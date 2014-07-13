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
			min[ x ] = Min( min[ x ], list[ i ][ x ] );
			max[ x ] = Max( max[ x ], list[ i ][ x ] );
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
		// GetFloraUUID returns a pooled game string, so valid while the level exists
		m_WarsMapManager.AddDeletedUUID( pFloraEnt->GetFloraUUID() );
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
#ifndef CLIENT_DLL
	// Sync from client
	return;
#endif // CLIENT_DLL

	Vector vPos = pPlayer->Weapon_ShootPosition();
	const Vector &vMouseAim = pPlayer->GetMouseAim();
	const Vector &vCamOffset = pPlayer->GetCameraOffset();

	Vector vStartPos, vEndPos;
	vStartPos = vPos + vCamOffset;
	vEndPos = vStartPos + (vMouseAim *  MAX_TRACE_LENGTH);

	//NDebugOverlay::Line( vStartPos, vEndPos, 0, 255, 0, true, 4.0f );

	Ray_t pickerRay;
	pickerRay.Init( vStartPos, vEndPos );

	CWarsFlora *pBest = NULL;
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
			// Prefer testing against hitboxes
			// If no hitboxes available, use the bounding box
			trace_t trace;
			bool bTestedHitBoxes = pFlora->TestHitboxes( pickerRay, MASK_SOLID, trace );
			if( bTestedHitBoxes )
			{
				if( trace.DidHit() )
				{
					float fDistance = trace.endpos.DistTo( vStartPos );
					if( !pBest || fDistance < fBestDistance )
					{
						pBest = pFlora;
						fBestDistance = fDistance;
					}
				}
			}
			else
			{
				CBaseTrace trace2;
				if ( IntersectRayWithBox( pickerRay,
					pFlora->GetAbsOrigin() + pFlora->CollisionProp()->OBBMins(), pFlora->GetAbsOrigin() + pFlora->CollisionProp()->OBBMaxs(),
					0.5f, &trace2 ) )
				{
					float fDistance = trace2.endpos.DistTo( vStartPos );
					if( !pBest || fDistance < fBestDistance )
					{
						pBest = pFlora;
						fBestDistance = fDistance;
					}
				}
			}
		}
	}

	if( pBest )
	{
		if( !IsSelected( pBest ) )
		{
			Select( pBest );

			KeyValues *pOperation = new KeyValues( "data" );
			pOperation->SetString("operation", "select");
			KeyValues *pFloraKey = new KeyValues( "flora", "uuid", pBest->GetFloraUUID() );
			pFloraKey->SetBool(  "select", true );
			pOperation->AddSubKey( pFloraKey );
			warseditorstorage->QueueServerCommand( pOperation );
		}
		else
		{
			Deselect( pBest );

			KeyValues *pOperation = new KeyValues( "data" );
			pOperation->SetString("operation", "select");
			KeyValues *pFloraKey = new KeyValues( "flora", "uuid", pBest->GetFloraUUID() );
			pFloraKey->SetBool(  "select", false );
			pOperation->AddSubKey( pFloraKey );
			warseditorstorage->QueueServerCommand( pOperation );
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
	warseditorstorage->QueueClientCommand( pOperation );
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEditorSystem::ClearCopyCommands()
{
	for( int i = 0; i < m_SelectionCopyCommands.Count(); i++ )
	{
		m_SelectionCopyCommands[i]->deleteThis();
	}
	m_SelectionCopyCommands.Purge();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEditorSystem::CopySelection()
{
	ClearCopyCommands();

	if( !IsAnythingSelected() )
		return;

	Vector vCopyOffset(64.0f, 64.0f, 0.0f );
	for( int i = 0; i < m_hSelectedEntities.Count(); i++ )
	{
		if( !m_hSelectedEntities[i] )
			continue;

		CWarsFlora *pFlora = dynamic_cast<CWarsFlora *>( m_hSelectedEntities[i].Get() );
		if( pFlora )
		{
			KeyValues *pOperation = CreateFloraCreateCommand( pFlora, &vCopyOffset );
			pOperation->SetBool( "select", true );
			m_SelectionCopyCommands.AddToTail( pOperation );
		}
		else
		{
			Warning( "CEditorSystem::CopySelection: Trying to copy an unsupported entity\n" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEditorSystem::PasteSelection()
{
	if( m_SelectionCopyCommands.Count() == 0 )
		return;

	// Clear selection, will select copy!
	KeyValues *pClearSelection = CreateClearSelectionCommand();
	QueueCommand( pClearSelection );

	for( int i = 0; i < m_SelectionCopyCommands.Count(); i++ )
	{
		QueueCommand( m_SelectionCopyCommands[i] );
	}

	m_SelectionCopyCommands.Purge();
}
#endif // CLIENT_DLL

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
CBaseEntity *CEditorSystem::GetSelected( int idx )
{
	return m_hSelectedEntities.IsValidIndex( idx ) ? m_hSelectedEntities[idx] : NULL;
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

	if( positions.Count() == 0 )
	{
		return m_vecSelectionCenterCache;
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
	KeyValues *pCommand = NULL;
	while( (pCommand = warseditorstorage->PopClientCommandQueue()) != NULL )
	{
		if( !ProcessCommand( pCommand ) )
			break; // Assume the command was readded, so no need for cleanup. TODO: refine this.

		// Clean up keyvalues after processing
		pCommand->deleteThis();
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
#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::FrameUpdatePostEntityThink()
{
	if( !IsActive() )
		return;

	if( !warseditorstorage )
		return;

	KeyValues *pCommand = NULL;
	while( (pCommand = warseditorstorage->PopServerCommandQueue()) != NULL )
	{
		if( !ProcessCommand( pCommand ) )
			break; // Assume the command was readded, so no need for cleanup. TODO: refine this.

		// Clean up keyvalues after processing
		pCommand->deleteThis();
	}
}
#endif // CLIENT_DLL