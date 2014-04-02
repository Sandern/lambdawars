//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "editorsystem.h"
#include "wars_flora.h"
#include "collisionutils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

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
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::RemoveFloraInRadius( const Vector &vPosition, float fRadius )
{
	CWarsFlora::RemoveFloraInRadius( vPosition, fRadius );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
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
		Vector color( 0.5f, 1.0f, 0.25f );

		const Vector &mins = l->CollisionProp()->OBBMins();
		const Vector &maxs = l->CollisionProp()->OBBMaxs();

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