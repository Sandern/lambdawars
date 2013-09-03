#include "cbase.h"
#include "wars_model_panel.h"
#include "renderparm.h"
#include "animation.h"

#include "view_shared.h"
#include "view.h"
#include "ivrenderview.h"
#include "vgui_int.h"
#include "viewpostprocess.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

ConVar asw_key_falloff( "asw_key_falloff" , ".1" , FCVAR_NONE );
ConVar asw_rim_falloff( "asw_rim_falloff" , ".1" , FCVAR_NONE );
ConVar asw_key_atten( "asw_key_atten" , "1.0" , FCVAR_NONE );
ConVar asw_rim_atten( "asw_rim_atten" , ".25" , FCVAR_NONE );
ConVar asw_key_innerCone( "asw_key_innerCone" , "20.0" , FCVAR_NONE );
ConVar asw_key_outerCone( "asw_key_outerCone" , "30.0" , FCVAR_NONE );
ConVar asw_rim_innerCone( "asw_rim_innerCone" , "20.0" , FCVAR_NONE );
ConVar asw_rim_outerCone( "asw_rim_outerCone" , "30.0" , FCVAR_NONE );
ConVar asw_key_range( "asw_key_range" , "0" , FCVAR_NONE );
ConVar asw_rim_range( "asw_rim_range" , "0" , FCVAR_NONE );

CWars_Model_Panel::CWars_Model_Panel( vgui::Panel *parent, const char *name ) : CBaseModelPanel( parent, name )
{
	m_bShouldPaint = true;
	m_pStudioHdr = NULL;
}

CWars_Model_Panel::~CWars_Model_Panel()
{

}


void CWars_Model_Panel::Paint()
{
	CMatRenderContextPtr pRenderContext( materials );

	// Turn off depth-write to dest alpha so that we get white there instead.  The code that uses
	// the render target needs a mask of where stuff was rendered.
	pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_WRITE_DEPTH_TO_DESTALPHA, false );

	// Disable flashlights when drawing our model
	pRenderContext->SetFlashlightMode(false);

	if ( m_bShouldPaint )
	{
		BaseClass::Paint();
	}

	pRenderContext.SafeRelease();
}

void CWars_Model_Panel::SetupCustomLights( Color cAmbient, Color cKey, float fKeyBoost, Color cRim, float fRimBoost )
{
	memset( &m_LightingState, 0, sizeof(MaterialLightingState_t) );
	
	for ( int i = 0; i < 6; ++i )
	{
		m_LightingState.m_vecAmbientCube[0].Init( cAmbient[0]/255.0f  , cAmbient[1]/255.0f , cAmbient[2]/255.0f );
	}
	
	// set up the lighting
	//QAngle angDir = vec3_angle;
	//Vector vecPos = vec3_origin;
	matrix3x4_t lightMatrix;
	matrix3x4_t rimLightMatrix;

	m_LightingState.m_nLocalLightCount = 2;
	// key light settings
	GetAttachment( "attach_light", lightMatrix );
	m_LightingState.m_pLocalLightDesc[0].m_Type = MATERIAL_LIGHT_SPOT;
	m_LightingState.m_pLocalLightDesc[0].m_Position = Vector( 0, 0, 100 );
	m_LightingState.m_pLocalLightDesc[0].m_Color.Init( ( cKey[0]/255.0f )* fKeyBoost  , ( cKey[1]/255.0f )* fKeyBoost  , ( cKey[2]/255.0f ) * fKeyBoost );
	m_LightToWorld[0] = lightMatrix;
	m_LightingState.m_pLocalLightDesc[0].m_Falloff = asw_key_falloff.GetFloat();
	m_LightingState.m_pLocalLightDesc[0].m_Attenuation0 = asw_key_atten.GetFloat();
	m_LightingState.m_pLocalLightDesc[0].m_Attenuation1 = 0;
	m_LightingState.m_pLocalLightDesc[0].m_Attenuation2 = 0;
	m_LightingState.m_pLocalLightDesc[0].m_Theta = asw_key_innerCone.GetFloat();
	m_LightingState.m_pLocalLightDesc[0].m_Phi = asw_key_outerCone.GetFloat();
	m_LightingState.m_pLocalLightDesc[0].m_Range = asw_key_range.GetFloat();
	m_LightingState.m_pLocalLightDesc[0].RecalculateDerivedValues();


	// rim light settings
	GetAttachment( "attach_light_rim", rimLightMatrix );
	m_LightingState.m_pLocalLightDesc[1].m_Type = MATERIAL_LIGHT_SPOT;
	m_LightingState.m_pLocalLightDesc[1].m_Position = Vector( 0, 0, 100 );
	m_LightingState.m_pLocalLightDesc[1].m_Color.Init( ( cRim[0]/255.0f ) * fRimBoost , ( cRim[1]/255.0f ) * fRimBoost, ( cRim[2]/255.0f )* fRimBoost);
	m_LightToWorld[1] = rimLightMatrix;
	m_LightingState.m_pLocalLightDesc[1].m_Falloff = asw_rim_falloff.GetFloat();
	m_LightingState.m_pLocalLightDesc[1].m_Attenuation0 = asw_rim_atten.GetFloat();
	m_LightingState.m_pLocalLightDesc[1].m_Attenuation1 = 0;
	m_LightingState.m_pLocalLightDesc[1].m_Attenuation2 = 0;
	m_LightingState.m_pLocalLightDesc[1].m_Theta = asw_rim_innerCone.GetFloat();
	m_LightingState.m_pLocalLightDesc[1].m_Phi = asw_rim_outerCone.GetFloat();
	m_LightingState.m_pLocalLightDesc[1].m_Range = asw_rim_range.GetFloat();
	m_LightingState.m_pLocalLightDesc[1].RecalculateDerivedValues();
}

void CWars_Model_Panel::SetBodygroup( int iGroup, int iValue )
{
	studiohdr_t *pStudioHdr = m_RootMDL.m_MDL.GetStudioHdr();
	if ( !pStudioHdr )
		return;

	CStudioHdr studioHdr( pStudioHdr, g_pMDLCache );

	::SetBodygroup( &studioHdr, m_RootMDL.m_MDL.m_nBody, iGroup, iValue );
}


int CWars_Model_Panel::FindBodygroupByName( const char *name )
{
	studiohdr_t *pStudioHdr = m_RootMDL.m_MDL.GetStudioHdr();
	if ( !pStudioHdr )
		return -1;

	CStudioHdr studioHdr( pStudioHdr, g_pMDLCache );

	return ::FindBodygroupByName( &studioHdr, name );
}

int CWars_Model_Panel::GetNumBodyGroups( void )
{
	studiohdr_t *pStudioHdr = m_RootMDL.m_MDL.GetStudioHdr();
	if ( !pStudioHdr )
		return 0;

	CStudioHdr studioHdr( pStudioHdr, g_pMDLCache );

	return ::GetNumBodyGroups( &studioHdr );
}

/*
void SetBodygroup( CStudioHdr *pstudiohdr, int& body, int iGroup, int iValue )

// Build list of submodels
BodyPartInfo_t *pBodyPartInfo = (BodyPartInfo_t*)stackalloc( m_pStudioHdr->numbodyparts * sizeof(BodyPartInfo_t) );
for ( int i=0 ; i < m_pStudioHdr->numbodyparts; ++i ) 
{
	pBodyPartInfo[i].m_nSubModelIndex = R_StudioSetupModel( i, body, &pBodyPartInfo[i].m_pSubModel, m_pStudioHdr );
}
R_StudioRenderFinal( pRenderContext, skin, m_pStudioHdr->numbodyparts, pBodyPartInfo, 
					pEntity, ppMaterials, pMaterialFlags, boneMask, lod, pColorMeshes )

					m_VertexCache.SetBodyPart( i );
m_VertexCache.SetModel( pBodyPartInfo[i].m_nSubModelIndex );

numFacesRendered += R_StudioDrawPoints( pRenderContext, skin, pClientEntity, 
									   ppMaterials, pMaterialFlags, boneMask, lod, pColorMeshes );

							
			*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWars_Model_Panel::SetAmbientCube( Color &cAmbient )
{
	for ( int i = 0; i < 6; ++i )
	{
		m_LightingState.m_vecAmbientCube[0].Init( cAmbient[0]/255.0f  , cAmbient[1]/255.0f , cAmbient[2]/255.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWars_Model_Panel::AddLight( LightDesc_t &newlight, const char *attachlight )
{
#if 0
	DECLARE_DMX_CONTEXT();
	CDmxElement* DMX = CreateDmxElement("LightData");

	//CDmxAttribute *locallights = DMX->AddAttribute("localLights");

	DMX->AddAttribute("cubemap")->SetValue("VGUI/combine/hud_portrait_bg");
	DMX->AddAttribute("cubemapHdr")->SetValue("VGUI/combine/hud_portrait_bg");

	DMX->AddAttribute("SPWP_LightProbeBackground")->SetValue("VGUI/combine/hud_portrait_bg");

	CDmxElement* lightpoint = CreateDmxElement("localLights");
	lightpoint->AddAttribute("name")->SetValue("point");
	lightpoint->AddAttribute("color")->SetValue<Vector>(Vector(1,1,1));
	lightpoint->AddAttribute("origin")->SetValue<Vector>(Vector(0,0,200));
	lightpoint->AddAttribute("attenuation")->SetValue<Vector>(Vector(1,0,0));
	lightpoint->AddAttribute("maxDistance")->SetValue<float>(20000.0f);
	
	//locallights->SetValue<CDmxElement*>( lightpoint );
	DMX->AddAttribute("localLights")->GetArrayForEdit<CDmxElement*>().AddToTail( lightpoint );
	

	this->SetLightProbe( DMX );
#else
	if( m_LightingState.m_nLocalLightCount == MATERIAL_MAX_LIGHT_COUNT )
	{
		Warning("CBaseModelPanel::AddLight: Too many lights\n");
		return;
	}

	matrix3x4_t lightMatrix;
	GetAttachment( attachlight, lightMatrix );
	m_LightingState.m_pLocalLightDesc[m_LightingState.m_nLocalLightCount] = newlight;
	m_LightToWorld[m_LightingState.m_nLocalLightCount] = lightMatrix;
	m_LightingState.m_nLocalLightCount++;
#endif // 0
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWars_Model_Panel::AddLight( LightDesc_t &newlight, matrix3x4_t &lightToWorld )
{
	if( m_LightingState.m_nLocalLightCount == MATERIAL_MAX_LIGHT_COUNT )
	{
		Warning("CBaseModelPanel::AddLight: Too many lights\n");
		return;
	}

	m_LightingState.m_pLocalLightDesc[m_LightingState.m_nLocalLightCount] = newlight;
	m_LightToWorld[m_LightingState.m_nLocalLightCount] = lightToWorld;
	m_LightingState.m_nLocalLightCount++;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWars_Model_Panel::ClearLights( )
{
	memset( &m_LightingState, 0, sizeof(MaterialLightingState_t) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
LightDesc_t CWars_Model_Panel::GetLight( int i )
{
	if( i < 0 || i >= m_LightingState.m_nLocalLightCount )
	{
		Warning("CWars_Model_Panel::GetLight: Invalid light index\n");
		return LightDesc_t();
	}
	return m_LightingState.m_pLocalLightDesc[i];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
matrix3x4_t CWars_Model_Panel::GetLightToWorld( int i )
{
	if( i < 0 || i >= m_LightingState.m_nLocalLightCount )
	{
		Warning("CWars_Model_Panel::GetLightToWorld: Invalid light index\n");
		return matrix3x4_t();
	}
	return m_LightToWorld[i];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWars_Model_Panel::SetModelAnglesAndPosition( const QAngle &angRot, const Vector &vecPos )
{
	m_vecModelPos = vecPos;
	m_angModelRot = angRot;

	BaseClass::SetModelAnglesAndPosition( angRot, vecPos );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWars_Model_Panel::LookAtBounds( const Vector &vecBoundsMin, const Vector &vecBoundsMax )
{
	// Get the model space render bounds.
	Vector vecMin = vecBoundsMin;
	Vector vecMax = vecBoundsMax;
	Vector vecCenter = ( vecMax + vecMin ) * 0.5f;
	vecMin -= vecCenter;
	vecMax -= vecCenter;

	// Get the bounds points and transform them by the desired model panel rotation.
	Vector aBoundsPoints[8];
	aBoundsPoints[0].Init( vecMax.x, vecMax.y, vecMax.z ); 
	aBoundsPoints[1].Init( vecMin.x, vecMax.y, vecMax.z ); 
	aBoundsPoints[2].Init( vecMax.x, vecMin.y, vecMax.z ); 
	aBoundsPoints[3].Init( vecMin.x, vecMin.y, vecMax.z ); 
	aBoundsPoints[4].Init( vecMax.x, vecMax.y, vecMin.z ); 
	aBoundsPoints[5].Init( vecMin.x, vecMax.y, vecMin.z ); 
	aBoundsPoints[6].Init( vecMax.x, vecMin.y, vecMin.z ); 
	aBoundsPoints[7].Init( vecMin.x, vecMin.y, vecMin.z ); 

	// Translated center point (offset from camera center).
	Vector vecTranslateCenter = -vecCenter;

	// Build the rotation matrix.
	matrix3x4_t matRotation;
	AngleMatrix( m_angModelRot, matRotation );

	Vector aXFormPoints[8];
	for ( int iPoint = 0; iPoint < 8; ++iPoint )
	{
		VectorTransform( aBoundsPoints[iPoint], matRotation, aXFormPoints[iPoint] );
	}

	Vector vecXFormCenter;
	VectorTransform( -vecTranslateCenter, matRotation, vecXFormCenter );

	int w, h;
	GetSize( w, h );
	float flW = (float)w;
	float flH = (float)h;

	float flFOVx = DEG2RAD( GetCameraFOV() * 0.5f );
	float flFOVy = CalcFovY( ( GetCameraFOV() * 0.5f ), flW/flH );
	flFOVy = DEG2RAD( flFOVy );

	float flTanFOVx = tan( flFOVx );
	float flTanFOVy = tan( flFOVy );

	// Find the max value of x, y, or z
	Vector2D dist[8];
	float flDist = 0.0f;
	for ( int iPoint = 0; iPoint < 8; ++iPoint )
	{
		float flDistY = fabs( aXFormPoints[iPoint].y / flTanFOVx ) - aXFormPoints[iPoint].x;
		float flDistZ = fabs( aXFormPoints[iPoint].z / flTanFOVy ) - aXFormPoints[iPoint].x;
		dist[iPoint].x = flDistY;
		dist[iPoint].y = flDistZ;
		float flTestDist = MAX( flDistZ, flDistY );
		flDist = MAX( flDist, flTestDist );
	}

	// Screen space points.
	Vector2D aScreenPoints[8];
	Vector aCameraPoints[8];
	for ( int iPoint = 0; iPoint < 8; ++iPoint )
	{
		aCameraPoints[iPoint] = aXFormPoints[iPoint];
		aCameraPoints[iPoint].x += flDist;

		aScreenPoints[iPoint].x = aCameraPoints[iPoint].y / ( flTanFOVx * aCameraPoints[iPoint].x );
		aScreenPoints[iPoint].y = aCameraPoints[iPoint].z / ( flTanFOVy * aCameraPoints[iPoint].x );

		aScreenPoints[iPoint].x = ( aScreenPoints[iPoint].x * 0.5f + 0.5f ) * flW;
		aScreenPoints[iPoint].y = ( aScreenPoints[iPoint].y * 0.5f + 0.5f ) * flH;
	}

	// Find the min/max and center of the 2D bounding box of the object.
	Vector2D vecScreenMin( 99999.0f, 99999.0f ), vecScreenMax( -99999.0f, -99999.0f );
	for ( int iPoint = 0; iPoint < 8; ++iPoint )
	{
		vecScreenMin.x = MIN( vecScreenMin.x, aScreenPoints[iPoint].x );
		vecScreenMin.y = MIN( vecScreenMin.y, aScreenPoints[iPoint].y );
		vecScreenMax.x = MAX( vecScreenMax.x, aScreenPoints[iPoint].x );
		vecScreenMax.y = MAX( vecScreenMax.y, aScreenPoints[iPoint].y );
	}

	// Offset the model to the be the correct distance away from the camera.
	Vector vecModelPos;
	vecModelPos.x = flDist - vecXFormCenter.x;
	vecModelPos.y = -vecXFormCenter.y;
	vecModelPos.z = -vecXFormCenter.z;
	SetModelAnglesAndPosition( m_angModelRot, vecModelPos );

	// Back project to figure out the camera offset to center the model.
	Vector2D vecPanelCenter( ( flW * 0.5f ), ( flH * 0.5f ) );
	Vector2D vecScreenCenter = ( vecScreenMax + vecScreenMin ) * 0.5f;

	Vector2D vecPanelCenterCamera, vecScreenCenterCamera;
	vecPanelCenterCamera.x = ( ( vecPanelCenter.x / flW ) * 2.0f ) - 0.5f;
	vecPanelCenterCamera.y = ( ( vecPanelCenter.y / flH ) * 2.0f ) - 0.5f;
	vecPanelCenterCamera.x *= ( flTanFOVx * flDist );
	vecPanelCenterCamera.y *= ( flTanFOVy * flDist );
	vecScreenCenterCamera.x = ( ( vecScreenCenter.x / flW ) * 2.0f ) - 0.5f;
	vecScreenCenterCamera.y = ( ( vecScreenCenter.y / flH ) * 2.0f ) - 0.5f;
	vecScreenCenterCamera.x *= ( flTanFOVx * flDist );
	vecScreenCenterCamera.y *= ( flTanFOVy * flDist );

	Vector2D vecCameraOffset( 0.0f, 0.0f );
	vecCameraOffset.x = vecPanelCenterCamera.x - vecScreenCenterCamera.x;
	vecCameraOffset.y = vecPanelCenterCamera.y - vecScreenCenterCamera.y;

	// Clear the camera pivot and set position matrix.
	ResetCameraPivot();
	SetCameraOffset( Vector( 0.0f, -vecCameraOffset.x, -vecCameraOffset.y ) );
	UpdateCameraTransform();
}