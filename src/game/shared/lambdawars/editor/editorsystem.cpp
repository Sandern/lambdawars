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

// Selection helper defines
#define LIGHT_BOX_SIZE 8.0f
#define LIGHT_BOX_VEC Vector( LIGHT_BOX_SIZE, LIGHT_BOX_SIZE, LIGHT_BOX_SIZE )

#define HELPER_COLOR_MAX 0.8f
#define HELPER_COLOR_MIN 0.1f
#define HELPER_COLOR_LOW 0.4f
#define SELECTED_AXIS_COLOR Vector( HELPER_COLOR_MAX, HELPER_COLOR_MAX, HELPER_COLOR_MIN )

#define SELECTION_PICKER_SIZE 2.0f
#define SELECTION_PICKER_VEC Vector( SELECTION_PICKER_SIZE, SELECTION_PICKER_SIZE, SELECTION_PICKER_SIZE )

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
Vector CEditorSystem::GetSelectionCenter()
{
	if ( !IsAnythingSelected() )
	{
		Assert( 0 );
		return vec3_origin;
	}

	// TODO
	//if ( m_bSelectionCenterLocked )
	//	return m_vecSelectionCenterCache;

	CUtlVector< Vector > positions;

	FOR_EACH_VEC( m_hSelectedEntities, i )
	{
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
	if( !warseditorstorage )
		return;

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
void CEditorSystem::RenderSelection()
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_matSelect );

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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::RenderHelpers()
{
	if ( !m_hSelectedEntities.Count() )
		return;

	switch ( m_EditorMode )
	{
	case EDITORINTERACTION_TRANSLATE:
			RenderTranslate();
		break;
	case EDITORINTERACTION_ROTATE:
			RenderRotate();
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::RenderTranslate()
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_matHelper );

	const bool bMultiSelected = (GetNumSelected() > 1);

	const int iSubDiv = 16;
	const float flLengthBase = 16.0f + ( bMultiSelected ? LIGHT_BOX_SIZE : 0 );
	const float flLengthTip = 8.0f;
	const float flRadiusBase = 1.2f;
	const float flRadiusTip = 4.0f;
	const float flAngleStep = 360.0f / iSubDiv;
	const float flOffset = bMultiSelected ? 0 : LIGHT_BOX_SIZE;

	Vector directions[] = {
		Vector( 1, 0, 0 ),
		Vector( 0, 1, 0 ),
		Vector( 0, 0, 1 ),
	};

	Vector colors[] = {
		Vector( HELPER_COLOR_MAX, HELPER_COLOR_MIN, HELPER_COLOR_MIN ),
		Vector( HELPER_COLOR_MIN, HELPER_COLOR_MAX, HELPER_COLOR_MIN ),
		Vector( HELPER_COLOR_MIN, HELPER_COLOR_MIN, HELPER_COLOR_MAX ),
	};

	const float flColorLowScale = HELPER_COLOR_LOW;

	Vector center = GetSelectionCenter();

	for ( int iDir = 0; iDir < 3; iDir++ )
	{
		Vector fwd = directions[ iDir ];
		QAngle ang;

		VectorAngles( fwd, ang );

		IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_matHelper );

		CMeshBuilder mB;
		mB.Begin( pMesh, MATERIAL_TRIANGLES, 5 * iSubDiv );

		Vector cUp, nUp;
		Vector pos, tmp0, tmp1, tmp2;

		Vector colHigh = colors[ iDir ];

		if ( GetCurrentSelectedAxis() == EDITORAXIS_FIRST + iDir )
			colHigh = SELECTED_AXIS_COLOR;

		Vector colLow = colHigh * flColorLowScale;

		for ( int iStep = 0; iStep < iSubDiv; iStep++ )
		{
			QAngle angNext( ang );
			angNext.z += flAngleStep;

			AngleVectors( ang, NULL, NULL, &cUp );
			AngleVectors( angNext, NULL, NULL, &nUp );

			// disc end
			pos = center + fwd * flOffset;
			mB.Position3fv( pos.Base() );
			mB.TexCoord2f( 0, 0, 0 );
			mB.Color3fv( colLow.Base() );
			mB.AdvanceVertex();

			tmp0 = pos + cUp * flRadiusBase;
			mB.Position3fv( tmp0.Base() );
			mB.TexCoord2f( 0, 0, 0 );
			mB.Color3fv( colLow.Base() );
			mB.AdvanceVertex();

			tmp1 = pos + nUp * flRadiusBase;
			mB.Position3fv( tmp1.Base() );
			mB.TexCoord2f( 0, 0, 0 );
			mB.Color3fv( colLow.Base() );
			mB.AdvanceVertex();

			// base cylinder
			mB.Position3fv( tmp1.Base() );
			mB.TexCoord2f( 0, 0, 0 );
			mB.Color3fv( colHigh.Base() );
			mB.AdvanceVertex();

			mB.Position3fv( tmp0.Base() );
			mB.TexCoord2f( 0, 0, 0 );
			mB.Color3fv( colHigh.Base() );
			mB.AdvanceVertex();

			tmp2 = tmp0 + fwd * flLengthBase;
			mB.Position3fv( tmp2.Base() );
			mB.TexCoord2f( 0, 0, 0 );
			mB.Color3fv( colHigh.Base() );
			mB.AdvanceVertex();

			mB.Position3fv( tmp1.Base() );
			mB.TexCoord2f( 0, 0, 0 );
			mB.Color3fv( colHigh.Base() );
			mB.AdvanceVertex();

			mB.Position3fv( tmp2.Base() );
			mB.TexCoord2f( 0, 0, 0 );
			mB.Color3fv( colHigh.Base() );
			mB.AdvanceVertex();

			tmp1 += fwd * flLengthBase;
			mB.Position3fv( tmp1.Base() );
			mB.TexCoord2f( 0, 0, 0 );
			mB.Color3fv( colHigh.Base() );
			mB.AdvanceVertex();

			// disc mid
			pos += fwd * flLengthBase;
			mB.Position3fv( pos.Base() );
			mB.TexCoord2f( 0, 0, 0 );
			mB.Color3fv( colLow.Base() );
			mB.AdvanceVertex();

			tmp0 = pos + cUp * flRadiusTip;
			mB.Position3fv( tmp0.Base() );
			mB.TexCoord2f( 0, 0, 0 );
			mB.Color3fv( colLow.Base() );
			mB.AdvanceVertex();

			tmp1 = pos + nUp * flRadiusTip;
			mB.Position3fv( tmp1.Base() );
			mB.TexCoord2f( 0, 0, 0 );
			mB.Color3fv( colLow.Base() );
			mB.AdvanceVertex();

			// tip
			mB.Position3fv( tmp1.Base() );
			mB.TexCoord2f( 0, 0, 0 );
			mB.Color3fv( colHigh.Base() );
			mB.AdvanceVertex();

			mB.Position3fv( tmp0.Base() );
			mB.TexCoord2f( 0, 0, 0 );
			mB.Color3fv( colHigh.Base() );
			mB.AdvanceVertex();

			pos += fwd * flLengthTip;
			mB.Position3fv( pos.Base() );
			mB.TexCoord2f( 0, 0, 0 );
			mB.Color3fv( colHigh.Base() );
			mB.AdvanceVertex();

			ang = angNext;
		}

		mB.End();
		pMesh->Draw();

		if ( GetCurrentSelectedAxis() == EDITORAXIS_FIRST_PLANE + iDir )
		{
			const int index = GetCurrentSelectedAxis() - EDITORAXIS_FIRST_PLANE;
			const float flPlaneSize = LIGHT_BOX_SIZE + 12.0f;

			int orient[3][2] = {
				{ 0, 1 },
				{ 2, 0 },
				{ 1, 2 },
			};

			int axis_0 = orient[ index ][ 0 ];
			int axis_1 = orient[ index ][ 1 ];

			Vector points[4];
			points[0] = center;
			points[1] = center;
			points[2] = center;
			points[3] = center;

			points[1][axis_0] += flPlaneSize;
			points[2][axis_0] += flPlaneSize;
			points[2][axis_1] += flPlaneSize;
			points[3][axis_1] += flPlaneSize;

			Vector col = SELECTED_AXIS_COLOR;

			mB.Begin( pMesh, MATERIAL_QUADS, 2 );

			for ( int i = 3; i >= 0; i-- )
			{
				mB.Position3fv( points[i].Base() );
				mB.Color3fv( col.Base() );
				mB.TexCoord2f( 0, 0, 0 );
				mB.AdvanceVertex();
			}

			col *= HELPER_COLOR_LOW;

			for ( int i = 0; i < 4; i++ )
			{
				mB.Position3fv( points[i].Base() );
				mB.Color3fv( col.Base() );
				mB.TexCoord2f( 0, 0, 0 );
				mB.AdvanceVertex();
			}

			mB.End();
			pMesh->Draw();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::RenderRotate()
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_matHelper );

	const int iSubDiv = 64.0f;
	const float flRadius = 32.0f;
	const float flThickness = 4.0f;
	const float flRotationStep = 360.0f / iSubDiv;

	Vector center = GetSelectionCenter();
	QAngle orientation = GetSelectionOrientation();

	Vector colors[] = {
		Vector( HELPER_COLOR_MAX, HELPER_COLOR_MIN, HELPER_COLOR_MIN ),
		Vector( HELPER_COLOR_MIN, HELPER_COLOR_MAX, HELPER_COLOR_MIN ),
		Vector( HELPER_COLOR_MIN, HELPER_COLOR_MIN, HELPER_COLOR_MAX ),
	};
	const float flColorLowScale = HELPER_COLOR_LOW;

	Vector colHigh, colLow;
	Vector tmp[4];

	Vector reinterpretRoll[3];

	AngleVectors( orientation,
		&reinterpretRoll[0], &reinterpretRoll[1], &reinterpretRoll[2] );

	QAngle angCur, angNext;

	int iLookup[3][2] = {
		{ 1, 2 },
		{ 2, 1 },
		{ 0, 2 },
	};

	for ( int iDir = 0; iDir < 3; iDir++ )
	{
		colHigh = colors[ iDir ];

		if ( iDir == GetCurrentSelectedAxis() - EDITORAXIS_FIRST )
			colHigh = SELECTED_AXIS_COLOR;

		colLow = colHigh * flColorLowScale;

		IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_matHelper );

		CMeshBuilder mB;
		mB.Begin( pMesh, MATERIAL_QUADS, 2 * iSubDiv );

		VectorAngles( reinterpretRoll[ iLookup[iDir][0] ],
			reinterpretRoll[ iLookup[iDir][1] ],
			angCur );
		angNext = angCur;

		float flCurrentRadius = flRadius - iDir * 0.275f;

		for ( int iStep = 0; iStep < iSubDiv; iStep++ )
		{
			angNext.z += flRotationStep;

			Vector cUp, nUp, cFwd;

			AngleVectors( angCur, &cFwd, NULL, &cUp );
			AngleVectors( angNext, NULL, NULL, &nUp );

			tmp[0] = center
				+ cUp * flCurrentRadius
				+ cFwd * flThickness;

			tmp[1] = center
				+ nUp * flCurrentRadius
				+ cFwd * flThickness;

			tmp[2] = tmp[1] - cFwd * flThickness * 2;
			tmp[3] = tmp[0] - cFwd * flThickness * 2;

			for ( int iPoint = 0; iPoint < 4; iPoint++ )
			{
				mB.Position3fv( tmp[ iPoint ].Base() );
				mB.Color3fv( colHigh.Base() );
				mB.TexCoord2f( 0, 0, 0 );
				mB.AdvanceVertex();
			}

			for ( int iPoint = 3; iPoint >= 0; iPoint-- )
			{
				mB.Position3fv( tmp[ iPoint ].Base() );
				mB.Color3fv( colLow.Base() );
				mB.TexCoord2f( 0, 0, 0 );
				mB.AdvanceVertex();
			}

			angCur = angNext;
		}

		mB.End();
		pMesh->Draw();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::PostRender()
{
	RenderSelection();
	RenderHelpers();
}
#endif // CLIENT_DLL