//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Selection/axis rendering and drag/rotate operations
//
//=============================================================================//

#include "cbase.h"
#include "editorsystem.h"
#include <filesystem.h>
#include "wars_flora.h"
#include "collisionutils.h"

#ifdef CLIENT_DLL
#include "view_shared.h"
#include "view.h"
#include "iviewrender.h"
#include "inputsystem/iinputsystem.h"
#include <vgui/IInput.h>
#include <vgui_controls/Controls.h>
#include <vgui/cursor.h>
#endif // CLIENT_DLL

#ifdef ENABLE_PYTHON
#include "srcpy.h"
#endif // ENABLE_PYTHON

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Selection helper defines
#define LIGHT_BOX_SIZE 8.0f
#define LIGHT_BOX_VEC Vector( LIGHT_BOX_SIZE, LIGHT_BOX_SIZE, LIGHT_BOX_SIZE )

#define HELPER_COLOR_MAX 0.8f
#define HELPER_COLOR_MIN 0.1f
#define HELPER_COLOR_LOW 0.4f
#define SELECTED_AXIS_COLOR Vector( HELPER_COLOR_MAX, HELPER_COLOR_MAX, HELPER_COLOR_MIN )

#define SELECTION_PICKER_SIZE 2.0f
#define SELECTION_PICKER_VEC Vector( SELECTION_PICKER_SIZE, SELECTION_PICKER_SIZE, SELECTION_PICKER_SIZE )

#ifdef CLIENT_DLL
ConVar cl_wars_editor_interactiondebug("cl_wars_editor_interactiondebug", "0", FCVAR_CHEAT);

extern void ScreenToWorld( int mousex, int mousey, float fov,
					const Vector& vecRenderOrigin,
					const QAngle& vecRenderAngles,
					Vector& vecPickingRay );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CEditorSystem::GetPickingRay( int x, int y )
{
	Vector ray_out;

	const CViewSetup *setup = view->GetViewSetup();
	float ratio = (4.0f/3.0f/(setup->width/(float)setup->height));
	float flFov = ScaleFOVByWidthRatio( setup->fov, ratio );

	ScreenToWorld( x, y, flFov, setup->origin, setup->angles, ray_out );

	return ray_out;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CEditorSystem::GetViewOrigin()
{
	const CViewSetup *setup = view->GetViewSetup();
	return setup->origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
QAngle CEditorSystem::GetViewAngles()
{
	const CViewSetup *setup = view->GetViewSetup();
	return setup->angles;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::UpdateCurrentSelectedAxis( int x, int y )
{
	if ( !IsAnythingSelected() ||
		GetEditorMode() != EDITORINTERACTION_TRANSLATE &&
		GetEditorMode() != EDITORINTERACTION_ROTATE )
	{
		SetCurrentSelectedAxis( EDITORAXIS_NONE );
		return;
	}

	Vector picker = GetPickingRay( x, y );
	Vector origin = GetViewOrigin();

	Vector center = GetSelectionCenter();
	QAngle orientation = GetSelectionOrientation();

	Ray_t pickerRay;
	pickerRay.Init( origin, origin + picker * MAX_TRACE_LENGTH );

	EditiorSelectedAxis_t idealAxis = EDITORAXIS_SCREEN;

	switch ( GetEditorMode() )
	{
	case EDITORINTERACTION_TRANSLATE:
		{
			const float flBoxLength = 24.0f + LIGHT_BOX_SIZE;
			const float flBoxThick = 4.0f;

			Vector boxes[3][2] = {
				{ Vector( LIGHT_BOX_SIZE, -flBoxThick, -flBoxThick ),
						Vector( flBoxLength, flBoxThick, flBoxThick ) },
				{ Vector( -flBoxThick, LIGHT_BOX_SIZE, -flBoxThick ),
						Vector( flBoxThick, flBoxLength, flBoxThick ) },
				{ Vector( -flBoxThick, -flBoxThick, LIGHT_BOX_SIZE ),
						Vector( flBoxThick, flBoxThick, flBoxLength ) },
			};

			float flLastFrac = MAX_TRACE_LENGTH;

			for ( int i = 0; i < 3; i++ )
			{
				CBaseTrace trace;
				if ( IntersectRayWithBox( pickerRay,
					center + boxes[i][0], center + boxes[i][1],
					0.5f, &trace ) )
				{
					if ( trace.fraction > flLastFrac )
						continue;

					flLastFrac = trace.fraction;
					idealAxis = (EditiorSelectedAxis_t)(EDITORAXIS_FIRST + i);
				}
			}

			if ( idealAxis == EDITORAXIS_SCREEN )
			{
				const float flPlaneSize = LIGHT_BOX_SIZE + 12.0f;

				Vector planes[3][2] = {
					{ Vector( 0, 0, -1 ), Vector( flPlaneSize, flPlaneSize, 1 ) },
					{ Vector( 0, -1, 0 ), Vector( flPlaneSize, 1, flPlaneSize ) },
					{ Vector( -1, 0, 0 ), Vector( 1, flPlaneSize, flPlaneSize ) },
				};

				for ( int i = 0; i < 3; i++ )
				{
					CBaseTrace trace;
					if ( IntersectRayWithBox( pickerRay,
						center + planes[i][0], center + planes[i][1],
						0.5f, &trace ) )
					{
						if ( trace.fraction > flLastFrac )
							continue;

						flLastFrac = trace.fraction;
						idealAxis = (EditiorSelectedAxis_t)(EDITORAXIS_FIRST_PLANE + i);
					}
				}
			}
		}
		break;
	case EDITORINTERACTION_ROTATE:
		{
			const int iSubDiv = 32.0f;
			const float flRadius = 32.0f;
			const float flMaxDist = 4.0f * 4.0f;
			const float flRotationStep = 360.0f / iSubDiv;

			float flBestViewDist = MAX_TRACE_LENGTH;

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
				float flBestDist = MAX_TRACE_LENGTH;

				VectorAngles( reinterpretRoll[ iLookup[iDir][0] ],
					reinterpretRoll[ iLookup[iDir][1] ],
					angCur );
				angNext = angCur;

				for ( int iStep = 0; iStep < iSubDiv; iStep++ )
				{
					angNext.z += flRotationStep;

					Vector cUp, nUp;

					AngleVectors( angCur, NULL, NULL, &cUp );
					AngleVectors( angNext, NULL, NULL, &nUp );

					Ray_t ringRay;
					ringRay.Init( center + cUp * flRadius,
						center + nUp * flRadius );

					float t = 0, s = 0;
					IntersectRayWithRay( pickerRay, ringRay, t, s );

					Vector a = pickerRay.m_Start + pickerRay.m_Delta.Normalized() * t;
					Vector b = ringRay.m_Start + ringRay.m_Delta.Normalized() * s;

					float flDist = ( a - b ).LengthSqr();
					float flStepLengthSqr = ringRay.m_Delta.LengthSqr();

					if ( flDist < flBestDist &&
						flDist < flMaxDist && 
						s * s < flStepLengthSqr &&
						t > 0 && t < (flBestViewDist-7) )
					{
						flBestDist = flDist;
						flBestViewDist = t;

						idealAxis = (EditiorSelectedAxis_t)(EDITORAXIS_FIRST + iDir);
					}

					angCur = angNext;
				}
			}
		}
		break;
	}

	SetCurrentSelectedAxis( idealAxis );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::UpdateEditorInteraction()
{
	if( cl_wars_editor_interactiondebug.GetBool() )
	{
		engine->Con_NPrintf( 1, "Interaction mode: %d", m_EditorMode );
		engine->Con_NPrintf( 2, "Selected Axis: %d", m_iCurrentSelectedAxis );
		engine->Con_NPrintf( 3, "Drag Mode: %d", m_iDragMode );
	}

	int x, y;
	g_pInputSystem->GetCursorPosition( &x, &y );

	int delta_x = ( x - m_iLastMouse_x );
	int delta_y = ( y - m_iLastMouse_y );

	if( delta_x == 0 && delta_y == 0 )
		return;

	m_iLastMouse_x = x;
	m_iLastMouse_y = y;

	// Drag
	bool bShouldReset = false;

	if ( m_bDragSelectArmed )
	{
		m_bDragSelectArmed = false;

		StartDrag( EDITORDRAG_SELECT );
	}

	switch ( m_iDragMode )
	{
	default:
		{
			UpdateCurrentSelectedAxis( x, y );
		}
		break;
	/*case EDITORDRAG_VIEW:
		{
			QAngle angles;
			m_pEditorSystem->GetEditorView( NULL, &angles );

			normalizeAngles( angles );

			const float flTurnSpeed = 0.5f;
			angles.x += delta_y * flTurnSpeed;
			angles.y -= delta_x * flTurnSpeed;

			angles.x = clamp( angles.x, -89, 89 );
			angles.y = AngleNormalize( angles.y );

			m_pEditorSystem->SetEditorView( NULL, &angles );

			bShouldReset = true;
		}
		break;*/
	case EDITORDRAG_TRANSLATE:
	case EDITORDRAG_ROTATE:
		{
			Vector vecCurrentDragPos = GetDragWorldPos( x, y );

			if ( m_iDragMode == EDITORDRAG_TRANSLATE )
			{
				Vector vecMove = vecCurrentDragPos - m_vecLastDragWorldPos;
				MoveSelection( vecMove );
			}
			else
			{
				Vector center = GetSelectionCenter();
				QAngle orientation = GetSelectionOrientation();

				EditiorSelectedAxis_t axis = GetCurrentSelectedAxis();
				Assert( axis >= CEditorSystem::EDITORAXIS_FIRST && axis < CEditorSystem::EDITORAXIS_FIRST_PLANE );

				int iAxis = axis - CEditorSystem::EDITORAXIS_FIRST;
				Assert( iAxis >= 0 && iAxis < 4 );

				Vector vDirections[6];
				AngleVectors( orientation,
					&vDirections[0], &vDirections[1], &vDirections[2] );

				Vector vecViewCenter = center - GetViewOrigin();
				vecViewCenter.NormalizeInPlace();

				VectorVectors( vecViewCenter, vDirections[3], vDirections[4] );
				vDirections[5] = vecViewCenter;

				int iDirIndices[4][3] = {
					{ 2, 0, 1 },
					{ 0, 1, 2 },
					{ 1, 2, 0 },
					{ 3, 4, 5 },
				};

				Vector vecDelta[2] = {
					m_vecLastDragWorldPos - center,
					vecCurrentDragPos - center,
				};

				vecDelta[0].NormalizeInPlace();
				vecDelta[1].NormalizeInPlace();

				float flAngle[2] = {
					atan2( DotProduct( vDirections[ iDirIndices[iAxis][0] ], vecDelta[0] ),
						DotProduct( vDirections[ iDirIndices[iAxis][1] ], vecDelta[0] ) ),
					atan2( DotProduct( vDirections[ iDirIndices[iAxis][0] ], vecDelta[1] ),
						DotProduct( vDirections[ iDirIndices[iAxis][1] ], vecDelta[1] ) ),
				};

				float flRotate = flAngle[1] - flAngle[0];

				VMatrix matRotate;
				matRotate.Identity();

				MatrixBuildRotationAboutAxis( matRotate,
					vDirections[ iDirIndices[iAxis][2] ],
					RAD2DEG( flRotate ) );

				RotateSelection( matRotate );
			}

			m_vecLastDragWorldPos = vecCurrentDragPos;
		}
		break;
	}

	if ( bShouldReset )
	{
		int whalf = ScreenWidth() / 2;
		int thalf = ScreenHeight() / 2;

		m_iLastMouse_x = whalf;
		m_iLastMouse_y = thalf;

		// HACK: reset tracker
		x = m_iLastMouse_x;
		y = m_iLastMouse_y;

		g_pInputSystem->SetCursorPosition( whalf, thalf );
	}
}

void CEditorSystem::OnMousePressed( vgui::MouseCode code )
{
	StopDrag();

	if ( code == MOUSE_RIGHT )
	{
		// HACK: can't REMOVE FOCUS from anywhere. WHY?!
		// ALL of these stupid problems would be gone if the root panel could accept input
		// autonomically without being either a popup or a frame.
		//Panel *pFocused = vgui::ipanel()->GetPanel( vgui::input()->GetFocus(), GetModuleName() );

		//if ( pFocused && pFocused->GetParent() )
		//	pFocused->GetParent()->RequestFocus();

		//input()->SetMouseCapture( GetVPanel() );

		StartDrag( EDITORDRAG_VIEW );

		m_iLastMouse_x = ScreenWidth() / 2;
		m_iLastMouse_y = ScreenHeight() / 2;
		vgui::input()->SetCursorPos( m_iLastMouse_x, m_iLastMouse_y );

		vgui::input()->SetCursorOveride( vgui::dc_none );
	}
	else if ( code == MOUSE_LEFT )
	{
		int mx, my;
		vgui::input()->GetCursorPos( mx, my );

		switch ( GetEditorMode() )
		{
		case EDITORINTERACTION_SELECT:
			{
				m_bDragSelectArmed = true;
			}
			break;
		/*case EDITORINTERACTION_ADD:
			{
				if ( m_pCurrentProperties != NULL )
					m_pEditorSystem->AddLightFromScreen( mx, my, m_pCurrentProperties );
			}
			break;*/
		case EDITORINTERACTION_TRANSLATE:
			{
				if ( GetCurrentSelectedAxis() != EDITORAXIS_NONE )
				{
					StartDrag( EDITORDRAG_TRANSLATE );
					m_vecLastDragWorldPos = GetDragWorldPos( mx, my );
				}
			}
			break;
		case EDITORINTERACTION_ROTATE:
			{
				if ( GetCurrentSelectedAxis() != EDITORAXIS_NONE )
				{
					Assert( GetCurrentSelectedAxis() < EDITORAXIS_FIRST_PLANE );
					StartDrag( EDITORDRAG_ROTATE );
					m_vecLastDragWorldPos = GetDragWorldPos( mx, my );
					SetSelectionCenterLocked( true );
				}
			}
			break;
		}
	}
}

void CEditorSystem::OnMouseReleased( vgui::MouseCode code )
{
	StopDrag();
}

Vector CEditorSystem::GetDragWorldPos( int x, int y )
{
	Assert( IsAnythingSelected() );

	Vector picker = GetPickingRay( x, y );
	Vector viewOrigin = GetViewOrigin();

	Vector center = GetSelectionCenter();
	QAngle orientation = GetSelectionOrientation();

	Vector viewFwd;
	AngleVectors( GetViewAngles(), &viewFwd );

	Vector ret = center;

	Ray_t pickingRay;
	pickingRay.Init( viewOrigin, viewOrigin + picker * MAX_TRACE_LENGTH );

	EditiorSelectedAxis_t axis = GetCurrentSelectedAxis();

	Assert( axis >= EDITORAXIS_FIRST && axis < EDITORAXIS_COUNT );

	switch( m_iDragMode )
	{
	default:
		Assert( 0 );
	case EDITORDRAG_TRANSLATE:
		{
			const bool bMoveScreen = axis == EDITORAXIS_SCREEN;
			const bool bMovePlane = axis >= EDITORAXIS_FIRST_PLANE;

			if ( bMovePlane || bMoveScreen )
			{
				Vector normals[3] = {
					Vector( 0, 0, 1 ),
					Vector( 0, 1, 0 ),
					Vector( 1, 0, 0 ),
				};

				cplane_t plane;
				if ( bMoveScreen )
					plane.normal = viewFwd;
				else
					plane.normal = normals[ axis - EDITORAXIS_FIRST_PLANE ];
				plane.dist = DotProduct( plane.normal, center );

				float flDist = IntersectRayWithPlane( pickingRay, plane );

				ret = pickingRay.m_Start + pickingRay.m_Delta * flDist;
			}
			else
			{
				Vector direction( vec3_origin );
				direction[ axis - EDITORAXIS_FIRST ] = 1;

				Ray_t axisRay;
				axisRay.Init( center, center + direction );

				float t, s = 0;
				IntersectRayWithRay( pickingRay, axisRay, t, s );

				ret = center + direction * s;
			}
		}
		break;
	case EDITORDRAG_ROTATE:
		{
			Vector normals[4] = {
				Vector( 0, 1, 0 ),
				Vector( 0, 0, 1 ),
				Vector( 1, 0, 0 ),
			};

			normals[3] = viewOrigin - center;
			normals[3].NormalizeInPlace();

			AngleVectors( orientation,
				&normals[2], &normals[0], &normals[1] );

			int iAxis = axis - EDITORAXIS_FIRST;

			Assert( iAxis >= 0 && iAxis < 4 );

			Vector planeNormal = normals[ iAxis ];

			cplane_t plane;
			plane.normal = planeNormal;
			plane.dist = DotProduct( planeNormal, center );

			float t = IntersectRayWithPlane( pickingRay, plane );

			ret = pickingRay.m_Start + pickingRay.m_Delta * t;
		}
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::GetDragBounds( int *pi2_min, int *pi2_max )
{
	int iCurPos[2];
	vgui::input()->GetCursorPos( iCurPos[0], iCurPos[1] );

	pi2_min[ 0 ] = MIN( iCurPos[0], m_iDragStartPos[0] );
	pi2_min[ 1 ] = MIN( iCurPos[1], m_iDragStartPos[1] );

	pi2_max[ 0 ] = MAX( iCurPos[0], m_iDragStartPos[0] );
	pi2_max[ 1 ] = MAX( iCurPos[1], m_iDragStartPos[1] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::StartDrag( int mode )
{
	m_iDragMode = mode;
	//vgui::input()->SetMouseCapture( GetVPanel() );
	vgui::input()->GetCursorPos( m_iDragStartPos[0], m_iDragStartPos[1] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::StopDrag()
{
	m_bDragSelectArmed = false;

	if ( m_iDragMode == EDITORDRAG_NONE )
		return;

	bool bView = m_iDragMode == EDITORDRAG_VIEW;

	//if ( vgui::input()->GetMouseCapture() == GetVPanel() )
	//	vgui::input()->SetMouseCapture( 0 );

	// Update flora on server
	switch( m_iDragMode )
	{
	case EDITORDRAG_TRANSLATE:
		{
			FOR_EACH_VEC( m_hSelectedEntities, i )
			{
				CBaseEntity *pEnt = m_hSelectedEntities[ i ];
				if( !pEnt )
					continue;

				CWarsFlora *pFlora = dynamic_cast<CWarsFlora *>( pEnt );
				if( pFlora )
				{
					engine->ServerCmd( VarArgs("wars_editor_translateflora %s %f %f %f\n", 
						pFlora->GetFloraUUID(), pFlora->GetAbsOrigin().x, pFlora->GetAbsOrigin().y, pFlora->GetAbsOrigin().z ) );
				}
			}
		}
		break;
	case EDITORDRAG_ROTATE:
		{
			FOR_EACH_VEC( m_hSelectedEntities, i )
			{
				CBaseEntity *pEnt = m_hSelectedEntities[ i ];
				if( !pEnt )
					continue;

				CWarsFlora *pFlora = dynamic_cast<CWarsFlora *>( pEnt );
				if( pFlora )
				{
					engine->ServerCmd( VarArgs("wars_editor_rotateflora %s %f %f %f\n", 
						pFlora->GetFloraUUID(), pFlora->GetAbsAngles().x, pFlora->GetAbsAngles().y, pFlora->GetAbsAngles().z ) );
				}
			}
		}
		break;
	default:
		break;
	}

	m_iDragMode = EDITORDRAG_NONE;

	if ( bView )
		vgui::input()->SetCursorPos( m_iDragStartPos[0], m_iDragStartPos[1] );

	vgui::input()->SetCursorOveride( vgui::dc_user );

	SetSelectionCenterLocked( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::MoveSelection( const Vector &delta )
{
	FOR_EACH_VEC( m_hSelectedEntities, i )
	{
		CBaseEntity *pEnt = m_hSelectedEntities[ i ];
		if( !pEnt )
			continue;

		pEnt->SetAbsOrigin( pEnt->GetAbsOrigin() + delta );
	}

	// TODO: Sync server
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::RotateSelection( const VMatrix &matRotate )
{
	FOR_EACH_VEC( m_hSelectedEntities, i )
	{
		CBaseEntity *pEnt = m_hSelectedEntities[ i ];
		if( !pEnt )
			continue;

		VMatrix matCurrent, matDst;
		matCurrent.SetupMatrixOrgAngles( vec3_origin, pEnt->GetAbsAngles() );
		MatrixMultiply( matRotate, matCurrent, matDst );
		QAngle newAngle;
		MatrixToAngles( matDst, newAngle );
		pEnt->SetAbsAngles( newAngle );
	}

	if ( GetNumSelected() < 2 )
		return;

	Vector center = GetSelectionCenter();

	FOR_EACH_VEC( m_hSelectedEntities, i )
	{
		CBaseEntity *pEnt = m_hSelectedEntities[ i ];
		if( !pEnt )
			continue;

		Vector delta = pEnt->GetAbsOrigin() - center;

		VMatrix matCurrent, matDst;
		matCurrent.SetupMatrixOrgAngles( delta, vec3_angle );
		MatrixMultiply( matRotate, matCurrent, matDst );
		
		Vector rotatedDelta = matDst.GetTranslation();

		Vector move = rotatedDelta - delta;
		pEnt->SetAbsOrigin( pEnt->GetAbsOrigin() + move );
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

#else
CON_COMMAND_F( wars_editor_translateflora, "", FCVAR_CHEAT|FCVAR_HIDDEN )
{
	if( args.ArgC() < 5 )
		return;

	CWarsFlora *pFlora = CWarsFlora::FindFloraByUUID( args[1] );
	if( !pFlora )
		return;

	pFlora->SetAbsOrigin( Vector( atof(args[2] ), atof(args[3] ), atof(args[4] ) ) );
}

CON_COMMAND_F( wars_editor_rotateflora, "", FCVAR_CHEAT|FCVAR_HIDDEN )
{
	if( args.ArgC() < 5 )
		return;

	CWarsFlora *pFlora = CWarsFlora::FindFloraByUUID( args[1] );
	if( !pFlora )
		return;

	pFlora->SetAbsAngles( QAngle( atof(args[2] ), atof(args[3] ), atof(args[4] ) ) );
}

#endif // CLIENT_DLL