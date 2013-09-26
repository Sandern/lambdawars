
#include "cbase.h"
#include "deferred/deferred_shared_common.h"
#ifdef SHADER_EDITOR
#include "shadereditor/shadereditorsystem.h"
#include "shadereditor/ivshadereditor.h"
#endif // SHADER_EDITOR
#include "view_shared.h"
#include "view.h"

#include "collisionutils.h"
#include "beamdraw.h"

#include "filesystem.h"

extern void ScreenToWorld( int mousex, int mousey, float fov,
					const Vector& vecRenderOrigin,
					const QAngle& vecRenderAngles,
					Vector& vecPickingRay );

static CLightingEditor __g_lightingEditor;
CLightingEditor *GetLightingEditor()
{
	return &__g_lightingEditor;
}
#ifdef SHADER_EDITOR
extern IVShaderEditor *shaderEdit;
#endif // SHADER_EDITOR

def_light_editor_t::def_light_editor_t()
{
	iEditorId = -1;
}


CLightingEditor::CLightingEditor()
	: BaseClass( "LightingEditorSystem" )
{
	m_bActive = false;
	m_bLightsActive = false;
	m_bSelectionCenterLocked = false;

	m_iDebugMode = EDITOR_DBG_OFF;

	m_pKVVmf = NULL;
	m_pKVGlobalLight = NULL;
	*m_szCurrentVmf = 0;

	m_vecEditorView_Origin = vec3_origin;
	m_angEditorView_Angles = vec3_angle;

	m_vecEditorView_Velocity = vec3_origin;
	m_vecMoveDir = vec3_origin;
	m_vecSelectionCenterCache.Init();

	m_iInteractionMode = EDITORINTERACTION_SELECT;
	m_iCurrentSelectedAxis = EDITORAXIS_NONE;
}

CLightingEditor::~CLightingEditor()
{
	if ( m_pKVVmf )
		m_pKVVmf->deleteThis();
}

int DefLightSort( def_light_t *const *d0, def_light_t *const *d1 )
{
	Vector vec0 = ( *d0 )->pos;
	Vector vec1 = ( *d1 )->pos;
	Vector view = CurrentViewOrigin();

	float dist0 = (view-vec0).LengthSqr();
	float dist1 = (view-vec1).LengthSqr();

	return ( dist0 > dist1 ) ? -1 : 1;
}

bool CLightingEditor::Init()
{
	m_matSprite_Light.Init( "deferred/editor_light", TEXTURE_GROUP_OTHER );
	Assert( m_matSprite_Light.IsValid() );

	m_matSelect.Init( "deferred/editor_select", TEXTURE_GROUP_OTHER );
	Assert( m_matSelect.IsValid() );

	m_matHelper.Init( "deferred/editor_helper", TEXTURE_GROUP_OTHER );
	Assert( m_matHelper.IsValid() );

	return true;
}

void CLightingEditor::LevelInitPostEntity()
{
	CDeferredLightGlobal *pGlobal = GetGlobalLight();

	if ( pGlobal != NULL )
	{
		m_EditorGlobalState = pGlobal->GetState();
	}
	else
	{
		m_EditorGlobalState = lightData_Global_t();
	}
}

void CLightingEditor::LevelShutdownPreEntity()
{
	SetEditorActive( false );

	FlushEditorLights();

	m_iCurrentSelectedAxis = EDITORAXIS_NONE;
	SetDebugMode( EDITOR_DBG_OFF );
}

bool CLightingEditor::IsLightingEditorAllowed()
{
	//static ConVarRef cvarCheats( "sv_cheats" );
	//return cvarCheats.GetBool();

	return true;
}

void CLightingEditor::Update( float ft )
{
	if ( !IsEditorActive() )
		return;

	if ( !IsLightingEditorAllowed() )
	{
		SetEditorActive( false );
	}

	Vector fwd, right;
	AngleVectors( m_angEditorView_Angles, &fwd, &right, NULL );

	float flIdealSpeed = ( 300.0f + m_vecMoveDir.z * 400.0f );
	Vector vecIdealMove = fwd * m_vecMoveDir.x + right * m_vecMoveDir.y;
	vecIdealMove *= flIdealSpeed;

	Vector vecDelta = vecIdealMove - m_vecEditorView_Velocity;
	if ( !vecDelta.IsZero() )
		m_vecEditorView_Velocity += vecDelta * ft * 10.0f;

	if ( m_vecEditorView_Velocity.LengthSqr() > 1 )
		m_vecEditorView_Origin += m_vecEditorView_Velocity * ft;
}

void CLightingEditor::SetSelectionCenterLocked( bool locked )
{
	if ( locked )
		m_vecSelectionCenterCache = GetSelectionCenter();

	m_bSelectionCenterLocked = locked;
}

CLightingEditor::EDITOR_DBG_MODES CLightingEditor::GetDebugMode()
{
	return m_iDebugMode;
}

void CLightingEditor::SetDebugMode( EDITOR_DBG_MODES mode )
{
	m_iDebugMode = mode;
}

CLightingEditor::EDITOR_SELECTEDAXIS CLightingEditor::GetCurrentSelectedAxis()
{
	return m_iCurrentSelectedAxis;
}

void CLightingEditor::SetCurrentSelectedAxis( EDITOR_SELECTEDAXIS axis )
{
	m_iCurrentSelectedAxis = axis;
}

void CLightingEditor::OnRender()
{
	if ( !IsEditorActive() )
		return;

#ifdef SHADER_EDITOR
	const EDITOR_DBG_MODES dbg = GetDebugMode();
	if ( dbg != EDITOR_DBG_OFF && g_ShaderEditorSystem->IsReady() )
	{
		const char *pszDebugList[] = {
			"dbg_editor_ppe_lighting",
			"dbg_editor_ppe_depth",
			"dbg_editor_ppe_normals",
		};
		Assert( dbg >= 1 && dbg <= 3 );
		const char *pszDebugName = pszDebugList[ dbg - 1 ];

		int iDbgIndex = shaderEdit->GetPPEIndex( pszDebugName );
		if ( iDbgIndex != -1 )
			shaderEdit->DrawPPEOnDemand( iDbgIndex );
	}
#endif // SHADER_EDITOR

	RenderSprites();

	RenderSelection();

	RenderHelpers();
}

void CLightingEditor::RenderSprites()
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_matSprite_Light );

	CUtlVector< def_light_t* > hSortedLights;
	hSortedLights.AddVectorToTail( m_hEditorLights );
	hSortedLights.Sort( DefLightSort );

	FOR_EACH_VEC( hSortedLights, i )
	{
		def_light_t *pLight = hSortedLights[ i ];

		Vector origin = pLight->pos;

		float flC3[3] = { 0 };
		float flMax = MAX( 1, MAX( pLight->col_diffuse[0],
			MAX( pLight->col_diffuse[1], pLight->col_diffuse[2] ) ) );

		for ( int i = 0; i < 3; i++ )
			flC3[i] = clamp( pLight->col_diffuse[i] / flMax, 0, 1 );

		DrawHalo( m_matSprite_Light, origin, LIGHT_BOX_SIZE, flC3 );
	}
}

void CLightingEditor::RenderSelection()
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_matSelect );

	CUtlVector< def_light_t* > hSelectedLights;
	hSelectedLights.AddVectorToTail( m_hSelectedLights );
	hSelectedLights.Sort( DefLightSort );

	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_matSelect );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 6 * hSelectedLights.Count() );

	FOR_EACH_VEC( hSelectedLights, i )
	{
		def_light_t *l = hSelectedLights[ i ];

		Vector position = l->pos;
		float flSize = LIGHT_BOX_SIZE;
		Vector color( 0.5f, 1.0f, 0.25f );

		Vector points[8] = {
			Vector( flSize, flSize, flSize ),
			Vector( -flSize, flSize, flSize ),
			Vector( flSize, -flSize, flSize ),
			Vector( flSize, flSize, -flSize ),
			Vector( -flSize, -flSize, flSize ),
			Vector( flSize, -flSize, -flSize ),
			Vector( -flSize, flSize, -flSize ),
			Vector( -flSize, -flSize, -flSize ),
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

		if ( !GetLightingManager()->IsLightRendered( l ) )
			color.Init( 1.0f, 0.5f, 0.25f );

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

void CLightingEditor::RenderHelpers()
{
	if ( !IsAnythingSelected() )
		return;

	switch ( GetEditorInteractionMode() )
	{
	case EDITORINTERACTION_TRANSLATE:
			RenderTranslate();
		break;
	case EDITORINTERACTION_ROTATE:
			RenderRotate();
		break;
	}
}

void CLightingEditor::RenderTranslate()
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

void CLightingEditor::RenderRotate()
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

Vector CLightingEditor::GetViewOrigin()
{
	const CViewSetup *setup = view->GetViewSetup();
	return setup->origin;
}

QAngle CLightingEditor::GetViewAngles()
{
	const CViewSetup *setup = view->GetViewSetup();
	return setup->angles;
}

void CLightingEditor::UpdateCurrentSelectedAxis( int x, int y )
{
	if ( !IsAnythingSelected() ||
		GetEditorInteractionMode() != EDITORINTERACTION_TRANSLATE &&
		GetEditorInteractionMode() != EDITORINTERACTION_ROTATE )
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

	EDITOR_SELECTEDAXIS idealAxis = EDITORAXIS_SCREEN;

	switch ( GetEditorInteractionMode() )
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
					idealAxis = (EDITOR_SELECTEDAXIS)(EDITORAXIS_FIRST + i);
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
						idealAxis = (EDITOR_SELECTEDAXIS)(EDITORAXIS_FIRST_PLANE + i);
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

						idealAxis = (EDITOR_SELECTEDAXIS)(EDITORAXIS_FIRST + iDir);
					}

					angCur = angNext;
				}
			}
		}
		break;
	}

	SetCurrentSelectedAxis( idealAxis );
}

void CLightingEditor::SetEditorActive( bool bActive, bool bView, bool bLights )
{
	if ( bActive && !IsLightingEditorAllowed() )
		return;

	if ( bView )
		m_bActive = bActive;

	if ( bLights )
	{
		m_bLightsActive = bActive;

		GetLightingManager()->SetRenderWorldLights( !bActive );

		ApplyEditorLightsToWorld( bActive );
	}
}

bool CLightingEditor::IsEditorActive()
{
	return m_bActive;
}

bool CLightingEditor::IsEditorLightingActive()
{
	return m_bLightsActive;
}

void CLightingEditor::SetEditorInteractionMode( EDITORINTERACTION_MODE mode )
{
	m_iInteractionMode = mode;
}

CLightingEditor::EDITORINTERACTION_MODE CLightingEditor::GetEditorInteractionMode()
{
	return m_iInteractionMode;
}

void CLightingEditor::FlushEditorLights()
{
	ApplyEditorLightsToWorld( false );
	m_hSelectedLights.Purge();
	m_hEditorLights.PurgeAndDeleteElements();
}

void CLightingEditor::ApplyEditorLightsToWorld( bool bVisible )
{
	FOR_EACH_VEC_FAST( def_light_t*, m_hEditorLights, l )
	{
		GetLightingManager()->RemoveLight( l );

		if ( bVisible )
			GetLightingManager()->AddLight( l );
	}
	FOR_EACH_VEC_FAST_END
}

KeyValues *CLightingEditor::VmfToKeyValues( const char *pszVmf )
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

void CLightingEditor::KeyValuesToVmf( KeyValues *pKV, CUtlBuffer &vmf )
{
	for ( KeyValues *pKey = m_pKVVmf->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		pKey->RecursiveSaveToFile( vmf, 0 );
	}
}

void CLightingEditor::LoadVmf( const char *pszVmf )
{
	Q_snprintf( m_szCurrentVmf, sizeof( m_szCurrentVmf ), "%s", pszVmf );

	KeyValues *pKV = VmfToKeyValues( pszVmf );

	if ( !pKV )
	{
		*m_szCurrentVmf = 0;
		return;
	}

	ParseVmfFile( pKV );
	pKV->deleteThis();
}

void CLightingEditor::SaveCurrentVmf()
{
	Assert( Q_strlen( m_szCurrentVmf ) > 0 );

	if ( !g_pFullFileSystem->FileExists( m_szCurrentVmf ) )
	{
		AssertMsg( 0, "Expected file unavailable (moved, deleted)." );
		return;
	}

	ApplyLightsToCurrentVmfFile();

	CUtlBuffer buffer;
	KeyValuesToVmf( m_pKVVmf, buffer );

	g_pFullFileSystem->WriteFile( m_szCurrentVmf, NULL, buffer );
}

const char *CLightingEditor::GetCurrentVmfPath()
{
	return m_szCurrentVmf;
}

void CLightingEditor::ParseVmfFile( KeyValues *pKeyValues )
{
	Assert( pKeyValues );

	FlushEditorLights();

	if ( m_pKVVmf )
		m_pKVVmf->deleteThis();

	m_pKVGlobalLight = NULL;
	m_pKVVmf = pKeyValues->MakeCopy();

	for ( KeyValues *pKey = m_pKVVmf->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		if ( Q_strcmp( pKey->GetName(), "entity" ) )
			continue;

		const bool bLightEntity = !Q_stricmp( pKey->GetString( "classname" ), "light_deferred" ) || !Q_stricmp( pKey->GetString( "classname" ), "env_deferred_light" );
		const bool bGlobalLightEntity = !Q_stricmp( pKey->GetString( "classname" ), "light_deferred_global" );

		if ( bGlobalLightEntity )
		{
			m_pKVGlobalLight = pKey;

			ApplyKVToGlobalLight( m_pKVGlobalLight );
			continue;
		}

		if ( !bLightEntity )
			continue;

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
	}
}

void CLightingEditor::ApplyLightsToCurrentVmfFile()
{
	CUtlVector< KeyValues* > listKeyValues;
	CUtlVector< KeyValues* > listToRemove;
	GetKVFromAll( listKeyValues );

	int nextId = 0;

	for ( KeyValues *pKey = m_pKVVmf->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		if ( Q_strcmp( pKey->GetName(), "entity" ) )
			continue;

		const int iTargetID = pKey->GetInt( "id", -1 );

		if ( iTargetID < 0 )
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

		bool bIsDeferredLight = !Q_stricmp( pKey->GetString( "classname" ), "light_deferred" ) || !Q_stricmp( pKey->GetString( "classname" ), "env_deferred_light" );

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

	FOR_EACH_VEC( listToRemove, i )
		m_pKVVmf->RemoveSubKey( listToRemove[i] );

	if ( GetKVGlobalLight() != NULL )
		ApplyLightStateToKV( m_EditorGlobalState );

	FOR_EACH_VEC( listKeyValues, i )
		listKeyValues[ i ]->deleteThis();
}

void CLightingEditor::GetEditorView( Vector *origin, QAngle *angles )
{
	if ( origin != NULL )
		*origin = m_vecEditorView_Origin;

	if ( angles != NULL )
		*angles = m_angEditorView_Angles;
}

void CLightingEditor::SetEditorView( const Vector *origin, const QAngle *angles )
{
	if ( origin != NULL )
		m_vecEditorView_Origin = *origin;

	if ( angles != NULL )
		m_angEditorView_Angles = *angles;
}

Vector &CLightingEditor::GetMoveDirForModify()
{
	return m_vecMoveDir;
}

void CLightingEditor::AbortEditorMovement( bool bStompVelocity )
{
	m_vecMoveDir.Init();

	if ( bStompVelocity )
		m_vecEditorView_Velocity.Init();
}

void CLightingEditor::AddLightFromScreen( int x, int y, KeyValues *pData )
{
	Vector picker = GetPickingRay( x, y );

	Vector origin = GetViewOrigin();
	trace_t tr;
	Ray_t ray;
	ray.Init( origin, origin + picker * MAX_TRACE_LENGTH, -LIGHT_BOX_VEC, LIGHT_BOX_VEC );

	UTIL_TraceRay( ray, MASK_SOLID, C_BasePlayer::GetLocalPlayer(), COLLISION_GROUP_NONE, &tr );

	if ( !tr.DidHit() )
		return;

	Vector lightPos = tr.endpos;

	def_light_t *pLight = new def_light_editor_t();

	pLight->ApplyKeyValueProperties( pData );
	pLight->pos = lightPos;

	AddEditorLight( pLight );
}

bool CLightingEditor::SelectLight( int x, int y, bool bGroup, bool bToggle )
{
	Vector picker = GetPickingRay( x, y );

	Vector origin = GetViewOrigin();

	trace_t tr;
	CTraceFilterWorldOnly filter;

	UTIL_TraceLine( origin, origin + picker * MAX_TRACE_LENGTH,
		MASK_SOLID, &filter, &tr );

	bool bSelectionChanged = false;

	if ( tr.DidHit() )
	{
		bool bFound = false;
		BoxTraceInfo_t boxtr;
		FOR_EACH_VEC( m_hEditorLights, i )
		{
			def_light_t *l = m_hEditorLights[ i ];

			if ( IntersectRayWithBox( tr.startpos, tr.endpos - tr.startpos,
				-LIGHT_BOX_VEC + l->pos, LIGHT_BOX_VEC + l->pos, 0.1f, &boxtr ) )
			{
				if ( !bGroup && !bToggle )
					ClearSelection();

				if ( bToggle && IsLightSelected( l ) )
					RemoveLightFromSelection( l );
				else
					AddLightToSelection( l );

				bFound = true;
				bSelectionChanged = true;
				break;
			}
		}

		if ( !bFound && !bGroup )
		{
			ClearSelection();
			bSelectionChanged = true;
		}
	}

	return bSelectionChanged;
}

bool CLightingEditor::SelectLights( int *pi2_BoundsMin, int *pi2_BoundsMax,
	bool bGroup, bool bToggle )
{
	if ( !bGroup && !bToggle )
		ClearSelection();

	bool bSelectedAny = false;

	trace_t tr;
	CTraceFilterWorldOnly filter;

	FOR_EACH_VEC( m_hEditorLights, i )
	{
		def_light_t *pLight = m_hEditorLights[ i ];

		const bool bSelected = IsLightSelected( pLight );

		if ( !bToggle && bSelected )
			continue;

		if ( !render->AreAnyLeavesVisible( pLight->GetLeaves(), pLight->GetNumLeaves() ) )
			continue;

		Vector pos = pLight->pos;
		int iscreenPos[2];

		if ( !GetVectorInScreenSpace( pos, iscreenPos[0], iscreenPos[1] ) )
			continue;

		if ( iscreenPos[ 0 ] < pi2_BoundsMin[ 0 ] ||
			iscreenPos[ 1 ] < pi2_BoundsMin[ 1 ] )
			continue;

		if ( iscreenPos[ 0 ] > pi2_BoundsMax[ 0 ] ||
			iscreenPos[ 1 ] > pi2_BoundsMax[ 1 ] )
			continue;

		UTIL_TraceLine( m_vecEditorView_Origin, pos, MASK_SOLID, &filter, &tr );
		if ( tr.fraction != 1.0f )
			continue;

		if ( bSelected )
			RemoveLightFromSelection( pLight );
		else
			AddLightToSelection( pLight );

		bSelectedAny = true;
	}

	return bSelectedAny;
}

KeyValues *CLightingEditor::GetKVFromLight( def_light_t *pLight )
{
	def_light_editor_t *pEditorLight = assert_cast<def_light_editor_t*>(pLight);

	KeyValues *pKV = pEditorLight->AllocateAsKeyValues();
	Assert( pKV != NULL );

	pKV->SetInt( "id", pEditorLight->iEditorId );

	return pKV;
}

void CLightingEditor::GetKVFromAll( CUtlVector< KeyValues* > &listOut )
{
	FOR_EACH_VEC( m_hEditorLights, i )
	{
		listOut.AddToTail( GetKVFromLight( m_hEditorLights[i] ) );
	}
}

void CLightingEditor::GetKVFromSelection( CUtlVector< KeyValues* > &listOut )
{
	FOR_EACH_VEC( m_hSelectedLights, i )
	{
		listOut.AddToTail( GetKVFromLight( m_hSelectedLights[ i ] ) );
	}
}

void CLightingEditor::ApplyKVToSelection( KeyValues *pKVChanges )
{
	FOR_EACH_VEC( m_hSelectedLights, i )
	{
		m_hSelectedLights[ i ]->ApplyKeyValueProperties( pKVChanges );
	}
}

Vector CLightingEditor::GetPickingRay( int x, int y )
{
	Vector ray_out;

	const CViewSetup *setup = view->GetViewSetup();
	float ratio = (4.0f/3.0f/(setup->width/(float)setup->height));
	float flFov = ScaleFOVByWidthRatio( setup->fov, ratio );

	ScreenToWorld( x, y, flFov, setup->origin, setup->angles, ray_out );

	return ray_out;
}

void CLightingEditor::AddEditorLight( def_light_t *pDef )
{
	m_hEditorLights.AddToTail( pDef );

	ApplyEditorLightsToWorld( m_bLightsActive );
}

void CLightingEditor::ClearSelection()
{
	m_hSelectedLights.Purge();
}

void CLightingEditor::AddLightToSelection( def_light_t *l )
{
	if ( !IsLightSelected( l ) )
		m_hSelectedLights.AddToTail( l );
}

void CLightingEditor::RemoveLightFromSelection( def_light_t *l )
{
	m_hSelectedLights.FindAndRemove( l );
}

bool CLightingEditor::IsLightSelected( def_light_t *l )
{
	return m_hSelectedLights.HasElement( l );
}

void CLightingEditor::DeleteSelectedLights()
{
	FOR_EACH_VEC( m_hSelectedLights, i )
	{
		m_hEditorLights.FindAndRemove( m_hSelectedLights[i] );

		GetLightingManager()->RemoveLight( m_hSelectedLights[i] );
	}

	m_hSelectedLights.PurgeAndDeleteElements();
}

int CLightingEditor::GetNumSelected()
{
	return m_hSelectedLights.Count();
}

bool CLightingEditor::IsAnythingSelected()
{
	return GetNumSelected() > 0;
}

Vector CLightingEditor::GetSelectionCenter()
{
	if ( !IsAnythingSelected() )
	{
		Assert( 0 );
		return vec3_origin;
	}

	if ( m_bSelectionCenterLocked )
		return m_vecSelectionCenterCache;

	CUtlVector< Vector > positions;

	FOR_EACH_VEC( m_hSelectedLights, i )
	{
		positions.AddToTail( m_hSelectedLights[ i ]->pos );
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

QAngle CLightingEditor::GetSelectionOrientation()
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
		Assert( m_hSelectedLights.Count() == 1 );

		return m_hSelectedLights[ 0 ]->ang;
	}
}

void CLightingEditor::MoveSelectedLights( Vector delta )
{
	FOR_EACH_VEC( m_hSelectedLights, i )
	{
		def_light_t *l = m_hSelectedLights[ i ];

		l->pos += delta;

		l->MakeDirtyXForms();
	}
}

void CLightingEditor::RotateSelectedLights( VMatrix matRotate )
{
	FOR_EACH_VEC( m_hSelectedLights, i )
	{
		def_light_t *l = m_hSelectedLights[i];

		VMatrix matCurrent, matDst;
		matCurrent.SetupMatrixOrgAngles( vec3_origin, l->ang );
		MatrixMultiply( matRotate, matCurrent, matDst );
		MatrixToAngles( matDst, l->ang );

		l->MakeDirtyXForms();
	}

	if ( GetNumSelected() < 2 )
		return;

	Vector center = GetSelectionCenter();

	FOR_EACH_VEC( m_hSelectedLights, i )
	{
		def_light_t *l = m_hSelectedLights[i];

		Vector delta = l->pos - center;

		VMatrix matCurrent, matDst;
		matCurrent.SetupMatrixOrgAngles( delta, vec3_angle );
		MatrixMultiply( matRotate, matCurrent, matDst );
		
		Vector rotatedDelta = matDst.GetTranslation();

		Vector move = rotatedDelta - delta;
		l->pos += move;
	}
}

void CLightingEditor::ApplyLightStateToKV( lightData_Global_t state )
{
	if ( m_pKVGlobalLight == NULL )
		return;

	char tmp[256] = {0};

	vecToStringCol( state.diff.AsVector3D(), tmp, sizeof( tmp ) );
	m_pKVGlobalLight->SetString( GetLightParamName( LPARAM_DIFFUSE ), tmp );

	vecToStringCol( state.ambh.AsVector3D(), tmp, sizeof( tmp ) );
	m_pKVGlobalLight->SetString( GetLightParamName( LPARAM_AMBIENT_HIGH ), tmp );

	vecToStringCol( state.ambl.AsVector3D(), tmp, sizeof( tmp ) );
	m_pKVGlobalLight->SetString( GetLightParamName( LPARAM_AMBIENT_LOW ), tmp );

	QAngle ang;
	VectorAngles( -state.vecLight.AsVector3D(), ang );
	normalizeAngles( ang );
	m_pKVGlobalLight->SetString( GetLightParamName( LPARAM_ANGLES ), VarArgs( "%.2f %.2f %.2f", XYZ( ang ) ) );

	int flags = 0;
	if ( state.bEnabled )
		flags |= DEFLIGHTGLOBAL_ENABLED;
	if ( state.bShadow )
		flags |= DEFLIGHTGLOBAL_SHADOW_ENABLED;

	m_pKVGlobalLight->SetInt( GetLightParamName( LPARAM_SPAWNFLAGS ), flags );
}

void CLightingEditor::ApplyKVToGlobalLight( KeyValues *pKVChanges )
{
	if ( m_pKVGlobalLight == NULL )
		return;

	if ( pKVChanges != m_pKVGlobalLight )
	{
		for ( KeyValues *pValue = pKVChanges->GetFirstValue();
			pValue;
			pValue = pValue->GetNextValue() )
		{
			const char *pszName = pValue->GetName();
			const char *pszValue = pValue->GetString();

			m_pKVGlobalLight->SetString( pszName, pszValue );
		}
	}

	CDeferredViewRender *pDefView = assert_cast< CDeferredViewRender* >( view );
	pDefView->ResetCascadeDelay();

	m_EditorGlobalState.bEnabled = ( m_pKVGlobalLight->GetInt( GetLightParamName( LPARAM_SPAWNFLAGS ), 1 ) & DEFLIGHTGLOBAL_ENABLED ) != 0;
	m_EditorGlobalState.bShadow = ( m_pKVGlobalLight->GetInt( GetLightParamName( LPARAM_SPAWNFLAGS ), 1 ) & DEFLIGHTGLOBAL_SHADOW_ENABLED ) != 0;

	QAngle lightAng;
	UTIL_StringToVector( lightAng.Base(), m_pKVGlobalLight->GetString( GetLightParamName( LPARAM_ANGLES ), "45 0 0" ) );
	AngleVectors( lightAng, &m_EditorGlobalState.vecLight.AsVector3D() );
	m_EditorGlobalState.vecLight.AsVector3D() *= -1;

	m_EditorGlobalState.diff.AsVector3D() = stringColToVec( m_pKVGlobalLight->GetString( GetLightParamName( LPARAM_DIFFUSE ), "255 255 255 255" ) );
	m_EditorGlobalState.ambl.AsVector3D() = stringColToVec( m_pKVGlobalLight->GetString( GetLightParamName( LPARAM_AMBIENT_LOW ), "0 0 0 0" ) );
	m_EditorGlobalState.ambh.AsVector3D() = stringColToVec( m_pKVGlobalLight->GetString( GetLightParamName( LPARAM_AMBIENT_HIGH ), "0 0 0 0" ) );
}

KeyValues *CLightingEditor::GetKVGlobalLight()
{
	return m_pKVGlobalLight;
}

const lightData_Global_t &CLightingEditor::GetGlobalState()
{
	return m_EditorGlobalState;
}