//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose:
//
//===========================================================================//

#include "cbase.h"
#include "projected_texture_unlit.h"
#include "materialsystem/IMaterialProxy.h"
#include "materialsystem/IMaterialVar.h"
#include "materialsystem/IMaterial.h"

//-----------------------------------------------------------------------------
// Purpose: Constructors
//-----------------------------------------------------------------------------
ProjectedTexture::ProjectedTexture()
{
}
ProjectedTexture::ProjectedTexture( const char *pMaterial, Vector vMin, Vector vMax, 
								   Vector vOrigin, QAngle qAngle, int r, int g, int b, int a )
{
	Init( pMaterial, 0, vMin, vMax, vOrigin, qAngle, r, g, b, a );
}

ProjectedTexture::ProjectedTexture( const char *pMaterial, const char *pTextureGroupName, 
								   Vector vMin, Vector vMax, Vector vOrigin, QAngle qAngle, 
								   int r, int g, int b, int a )
{
	Init( pMaterial, pTextureGroupName, vMin, vMax, vOrigin, qAngle, r, g, b, a );
}

ProjectedTexture::ProjectedTexture( CMaterialReference &ref, Vector vMin, Vector vMax, Vector vOrigin, 
								   QAngle qAngle, int r, int g, int b, int a )
{
	Init( ref, vMin, vMax, vOrigin, qAngle, r, g, b, a );
}

ProjectedTexture::~ProjectedTexture()
{
	// Shutdown shadow if necessary
	if( m_shadowHandle != CLIENTSHADOW_INVALID_HANDLE )
		Shutdown();
}
#if 0 //def HL2WARS_ASW_DLL
//-----------------------------------------------------------------------------
// Purpose: Init
//-----------------------------------------------------------------------------
void ProjectedTexture::Init( const char *pMaterial, const char *pTextureGroupName, 
							Vector vMin, Vector vMax, Vector vOrigin, QAngle qAngle, 
							int r, int g, int b, int a)
{
	m_SpotlightTexture.Init(pMaterial, pTextureGroupName);
	Msg("ProjectedTexture::Init Getting texture for: %s %s. Valid: %d. r: %d, g: %d, b: %d\n", 
		pMaterial, pTextureGroupName, m_SpotlightTexture.IsValid(), r, g, b);

	m_Info.m_pSpotlightTexture = m_SpotlightTexture;
	m_Info.m_pProjectedMaterial = materials->FindMaterial(pMaterial, pTextureGroupName);

	SharedInit(vMin, vMax, vOrigin, qAngle, r, g, b, a );
}

void ProjectedTexture::Init( CMaterialReference &ref, Vector vMin, Vector vMax, 
							Vector vOrigin, QAngle qAngle, int r, int g, int b, int a )
{
	m_SpotlightTexture.Init( materials->FindTexture( ref->GetName(), ref->GetTextureGroupName() ) );
	Msg("ProjectedTexture::Init Getting texture ref for: %s %s, Valid: %d. r: %d, g: %d, b: %d\n", 
		ref->GetName(), ref->GetTextureGroupName(), m_SpotlightTexture.IsValid(), r, g, b);
	
	m_Info.m_pSpotlightTexture = m_SpotlightTexture;
	m_Info.m_pProjectedMaterial = ref;

	SharedInit(vMin, vMax, vOrigin, qAngle, r, g, b, a );
}

const bool m_bSimpleProjection = true;

ConVar pt_nearz("pt_nearz", "0.0");
ConVar pt_farz("pt_farz", "1000.0");
ConVar pt_linearatt("pt_linearatt", "100.0");
ConVar pt_constatt("pt_constatt", "0.0");
ConVar pt_fov("pt_fov", "4.0");

void ProjectedTexture::SharedInit( Vector vMin, Vector vMax, 
	Vector vPos, QAngle qAngle,
	int r, int g, int b, int a )
{
	// Calculate direction
	// Setup light
	m_Info.m_vecLightOrigin = vPos;
	m_Info.m_NearZ = pt_nearz.GetFloat();
	m_Info.m_FarZ = pt_farz.GetFloat();

	const float m_flCurrentLinearFloatLightAlpha = a;

	const Vector vDirection = Vector(0, 0, -1);

	float flAlpha = m_flCurrentLinearFloatLightAlpha * ( 1.0f / 255.0f );

	float m_flProjectionSize = (vMax.x - vMin.x) / 2.0f;

	QAngle angAngles;
	VectorAngles( vDirection, angAngles );

	Vector vForward, vRight, vUp;
	AngleVectors( angAngles, &vForward, &vRight, &vUp );

	m_Info.m_fHorizontalFOVDegrees = pt_fov.GetFloat();
	m_Info.m_fVerticalFOVDegrees = pt_fov.GetFloat();

	m_Info.m_vecLightOrigin = vPos;
	BasisToQuaternion( vForward, vRight, vUp, m_Info.m_quatOrientation );

	m_Info.m_fQuadraticAtten = 0.0f;
	m_Info.m_fLinearAtten = pt_linearatt.GetFloat(); //100.0f;
	m_Info.m_fConstantAtten =pt_constatt.GetFloat();
	m_Info.m_FarZAtten = m_Info.m_FarZ;
	m_Info.m_Color[0] = r * ( 1.0f / 255.0f ) * a;
	m_Info.m_Color[1] = g * ( 1.0f / 255.0f ) * a;
	m_Info.m_Color[2] = b * ( 1.0f / 255.0f ) * a;
	m_Info.m_Color[3] = a * ( 1.0f / 255.0f ); // fixme: need to make ambient work m_flAmbient;
	m_Info.m_fBrightnessScale = 1.0f;
	m_Info.m_bGlobalLight = false;

	float flOrthoSize = m_flProjectionSize; //4000;

	if ( flOrthoSize > 0 )
	{
		m_Info.m_bOrtho = true;
		m_Info.m_fOrthoLeft = -flOrthoSize;
		m_Info.m_fOrthoTop = -flOrthoSize;
		m_Info.m_fOrthoRight = flOrthoSize;
		m_Info.m_fOrthoBottom = flOrthoSize;
	}
	else
	{
		m_Info.m_bOrtho = false;
	}

	m_Info.m_bDrawShadowFrustum = true;
	m_Info.m_flShadowSlopeScaleDepthBias = g_pMaterialSystemHardwareConfig->GetShadowSlopeScaleDepthBias();;
	m_Info.m_flShadowDepthBias = g_pMaterialSystemHardwareConfig->GetShadowDepthBias();
	m_Info.m_bEnableShadows = true;
	m_Info.m_pSpotlightTexture = m_SpotlightTexture;
	//m_Info.m_pProjectedMaterial = NULL; // don't complain cause we aren't using simple projection in this class
	m_Info.m_nSpotlightTextureFrame = 0;
	m_Info.m_flProjectionRotation = qAngle[YAW];

	m_Info.m_nShadowQuality = 1; // Allow entity to affect shadow quality
	m_Info.m_bShadowHighRes = true;

	if( m_bSimpleProjection )
	{
		m_shadowHandle = g_pClientShadowMgr->CreateProjection( m_Info );
	}
	else
	{
		m_shadowHandle = g_pClientShadowMgr->CreateFlashlight( m_Info );
	}
	

	// Updating
	if( m_bSimpleProjection )
		g_pClientShadowMgr->UpdateProjectionState( m_shadowHandle, m_Info );
	else
		g_pClientShadowMgr->UpdateFlashlightState( m_shadowHandle, m_Info );

	g_pClientShadowMgr->SetFlashlightTarget( m_shadowHandle, NULL );
	g_pClientShadowMgr->SetFlashlightLightWorld( m_shadowHandle, true );

	g_pClientShadowMgr->UpdateProjectedTexture( m_shadowHandle, true );

	//g_pClientShadowMgr->SetShadowFromWorldLightsEnabled( true );
}


void ProjectedTexture::Shutdown()
{
	// Destroy shadow 
	if( m_shadowHandle == CLIENTSHADOW_INVALID_HANDLE ) {
		Warning("ProjectedTexture::Shutdown: Invalid shadow handle on shutdown!\n");
		return;
	}

	if( m_bSimpleProjection )
		g_pClientShadowMgr->DestroyProjection( m_shadowHandle );
	else
		g_pClientShadowMgr->DestroyFlashlight( m_shadowHandle );
	m_shadowHandle = CLIENTSHADOW_INVALID_HANDLE;

	// Shutdown old material

}

void ProjectedTexture::UpdateShadow()
{
	// Update shadow
	if( m_shadowHandle == CLIENTSHADOW_INVALID_HANDLE ) {
		Warning("ProjectedTexture::UpdateShadow: Invalid shadow handle on update!\n");
		return;
	}
	//g_pClientShadowMgr->SetFlashlightLightWorld( m_shadowHandle, true );
	//g_pClientShadowMgr->UpdateProjectedTexture( m_shadowHandle, true );

	if( m_bSimpleProjection )
		g_pClientShadowMgr->UpdateProjectionState( m_shadowHandle, m_Info );
	else
		g_pClientShadowMgr->UpdateFlashlightState( m_shadowHandle, m_Info );

	g_pClientShadowMgr->SetFlashlightTarget( m_shadowHandle, NULL );
	g_pClientShadowMgr->SetFlashlightLightWorld( m_shadowHandle, true );

	g_pClientShadowMgr->UpdateProjectedTexture( m_shadowHandle, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ProjectedTexture::Update( Vector vOrigin, QAngle qAngle )
{
	// Update position
	m_Info.m_vecLightOrigin = vOrigin;
	m_Info.m_flProjectionRotation = qAngle.y;

	UpdateShadow();
}

void ProjectedTexture::SetMinMax( Vector &vMin, Vector vMax )
{


	UpdateShadow();
}

void ProjectedTexture::SetMaterial( const char *pMaterial, const char *pTextureGroupName )
{
	
	UpdateShadow();
}

void ProjectedTexture::SetMaterial( CMaterialReference &ref )
{
	

	UpdateShadow();
}

void ProjectedTexture::SetProjectionDistance( float dist )
{
	

	UpdateShadow();
}
#else
//-----------------------------------------------------------------------------
// Purpose: Init
//-----------------------------------------------------------------------------
void ProjectedTexture::Init( const char *pMaterial, const char *pTextureGroupName, 
							Vector vMin, Vector vMax, Vector vOrigin, QAngle qAngle, 
							int r, int g, int b, int a)
{
	// Shutdown old material if necessary 
	if( m_Info.m_UnlitShadow.IsValid() )
		m_Info.m_UnlitShadow.Shutdown();

	// Initialize material and fill in info
	m_Info.m_UnlitShadow.Init( pMaterial, pTextureGroupName, true );
	m_Info.m_vUnlitMins = vMin;
	m_Info.m_vUnlitMaxs = vMax;
	m_Info.m_vOrigin = vOrigin;
	m_Info.m_Angle = qAngle;
	m_Info.m_fR = r;
	m_Info.m_fG = g;
	m_Info.m_fB = b;
	m_Info.m_fA = a;

	if( m_Info.m_UnlitShadow.IsValid() == false )
	{
		Warning("ProjectedTexture::Init: Invalid material\n");
		return;
	}

	// Create the shadow
	m_shadowHandle = g_pClientShadowMgr->CreatePTUnlit( m_Info, INVALID_CLIENTENTITY_HANDLE, SHADOW_FLAGS_SHADOW|SHADOW_FLAGS_PROJECTTEXTUREUNLIT);
	if( m_shadowHandle == CLIENTSHADOW_INVALID_HANDLE ) {
		Warning("ProjectedTexture::Init: Invalid shadow handle on creation!\n");
	}

	g_pClientShadowMgr->SetFlashlightTarget( m_shadowHandle, NULL );
}

void ProjectedTexture::Init( CMaterialReference &ref, Vector vMin, Vector vMax, 
							Vector vOrigin, QAngle qAngle, int r, int g, int b, int a )
{
	// Shutdown old material if necessary 
	if( m_Info.m_UnlitShadow.IsValid() )
		m_Info.m_UnlitShadow.Shutdown();

	// Initialize material and fill in info
	m_Info.m_UnlitShadow = ref;
	m_Info.m_vUnlitMins = vMin;
	m_Info.m_vUnlitMaxs = vMax;
	m_Info.m_vOrigin = vOrigin;
	m_Info.m_Angle = qAngle;
	m_Info.m_fR = r;
	m_Info.m_fG = g;
	m_Info.m_fB = b;
	m_Info.m_fA = a;

	if( m_Info.m_UnlitShadow.IsValid() == false )
	{
		Warning("ProjectedTexture::Init: Invalid material\n");
		return;
	}

	// Create the shadow
	m_shadowHandle = g_pClientShadowMgr->CreatePTUnlit( m_Info, INVALID_CLIENTENTITY_HANDLE, SHADOW_FLAGS_SHADOW|SHADOW_FLAGS_PROJECTTEXTUREUNLIT);
	if( m_shadowHandle == CLIENTSHADOW_INVALID_HANDLE ) {
		Warning("ProjectedTexture::Init: Invalid shadow handle on creation!\n");
	}

	g_pClientShadowMgr->SetFlashlightTarget( m_shadowHandle, NULL );
}


void ProjectedTexture::Shutdown()
{
	// Destroy shadow 
	if( m_shadowHandle == CLIENTSHADOW_INVALID_HANDLE ) {
		Warning("ProjectedTexture::Shutdown: Invalid shadow handle on shutdown!\n");
		return;
	}
	g_pClientShadowMgr->DestroyPTUnlit(m_shadowHandle);
	m_shadowHandle = CLIENTSHADOW_INVALID_HANDLE;

	// Shutdown old material
	if( m_Info.m_UnlitShadow.IsValid() )
		m_Info.m_UnlitShadow.Shutdown();
}

void ProjectedTexture::UpdateShadow()
{
	// Update shadow
	if( m_shadowHandle == CLIENTSHADOW_INVALID_HANDLE ) {
		Warning("ProjectedTexture::UpdateShadow: Invalid shadow handle on update!\n");
		return;
	}
	g_pClientShadowMgr->UpdatePTUnlitState(m_shadowHandle, m_Info, true);
	g_pClientShadowMgr->SetFlashlightTarget( m_shadowHandle, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ProjectedTexture::Update( Vector vOrigin, QAngle qAngle )
{
	// Update position
	m_Info.m_vOrigin = vOrigin;
	m_Info.m_Angle = qAngle;

	UpdateShadow();
}

void ProjectedTexture::SetMinMax( Vector &vMin, Vector vMax )
{
	m_Info.m_vUnlitMins = vMin;
	m_Info.m_vUnlitMaxs = vMax;

	UpdateShadow();
}

void ProjectedTexture::SetMaterial( const char *pMaterial, const char *pTextureGroupName )
{
	// Shutdown old material if necessary 
	if( m_Info.m_UnlitShadow.IsValid() )
		m_Info.m_UnlitShadow.Shutdown();

	m_Info.m_UnlitShadow.Init( pMaterial, pTextureGroupName, true );

	UpdateShadow();
}

void ProjectedTexture::SetMaterial( CMaterialReference &ref )
{
	// Shutdown old material if necessary 
	if( m_Info.m_UnlitShadow.IsValid() )
		m_Info.m_UnlitShadow.Shutdown();

	m_Info.m_UnlitShadow = ref;

	UpdateShadow();
}

void ProjectedTexture::SetProjectionDistance( float dist )
{
	m_Info.m_fShadowDistance = dist;

	UpdateShadow();
}

void ProjectedTexture::SetColor( int r, int g, int b, int a )
{
	m_Info.m_fR = r;
	m_Info.m_fG = g;
	m_Info.m_fB = b;
	m_Info.m_fA = a;

	UpdateShadow();
}
#endif // HL2WARS_ASW_DLL