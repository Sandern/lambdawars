//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
//===========================================================================//

#include "cbase.h"
#include "projected_texture_unlit.h"
#include "materialsystem/IMaterialProxy.h"
#include "materialsystem/IMaterialVar.h"
#include "materialsystem/IMaterial.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

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

//-----------------------------------------------------------------------------
// Purpose: Init
//-----------------------------------------------------------------------------
void ProjectedTexture::Init( const char *pMaterial, const char *pTextureGroupName, 
							Vector vMin, Vector vMax, Vector vOrigin, QAngle qAngle, 
							int r, int g, int b, int a)
{
	m_shadowHandle = CLIENTSHADOW_INVALID_HANDLE;

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
	m_shadowHandle = CLIENTSHADOW_INVALID_HANDLE;

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
	m_shadowHandle = g_pClientShadowMgr->CreatePTUnlit( m_Info, INVALID_CLIENTENTITY_HANDLE, SHADOW_FLAGS_SHADOW|SHADOW_FLAGS_PROJECTTEXTUREUNLIT );
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
	g_pClientShadowMgr->UpdatePTUnlitState( m_shadowHandle, m_Info, true );
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