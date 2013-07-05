//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Sunlight shadow control entity.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "c_baseplayer.h"
#include "c_hl2wars_player.h"

//#include "deferred/cdeferred_manager_client.h"
//#include "deferred/deferred_shared_common.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern ConVar cl_sunlight_ortho_size;
extern ConVar cl_sunlight_depthbias;

ConVar cl_dklight_enable( "cl_dklight_enable", "1" );

#if 0
ConVar cl_globallight_ortho_size("cl_dklight_ortho_size", "300.0", FCVAR_CHEAT, "Set to values greater than 0 for ortho view render projections.");
ConVar cl_globallight_shadowquality( "cl_dklight_shadowquality", "1" );
ConVar cl_globallight_shadowhighres( "cl_dklight_shadowhighres", "1" );
ConVar cl_globalight_shadowfiltersize( "cl_dklight_shadowfiltersize", "1.0" );

ConVar cl_dklight_freeze( "cl_dklight_freeze", "0" );
ConVar cl_dklight_xoffset( "cl_dklight_xoffset", "0" );
ConVar cl_dklight_yoffset( "cl_dklight_yoffset", "0" );
ConVar cl_globallight_globallight( "cl_dklight_globallight", "0" );
ConVar cl_globallight_fov( "cl_dklight_fov", "0.2" );
ConVar cl_globallight_sundistance( "cl_dklight_sundistance", "10000" );
ConVar cl_globallight_brightnessscale("cl_dklight_brightnessscale", "0.1");
ConVar cl_globallight_drawfrustum( "cl_dklight_drawfrustum", "0" );

ConVar cl_dklight_quadraticatten( "cl_dklight_quadraticatten", "0" );
ConVar cl_dklight_linearatten( "cl_dklight_linearatten", "100" );
ConVar cl_dklight_constantatten( "cl_dklight_constantatten", "0" );
ConVar cl_dklight_farzatten( "cl_dklight_farzatten", "1000.0" );


// Uber light
ConVar cl_dklight_uberlight( "cl_dklight_uberlight", "0" );

ConVar cl_dklight_uberlight_nearedge( "cl_dklight_uberlight_nearedge", "2.0" );
ConVar cl_dklight_uberlight_faredge( "cl_dklight_uberlight_faredge", "1000.0f" );
ConVar cl_dklight_uberlight_cuton( "cl_dklight_uberlight_cuton", "10.0" );
ConVar cl_dklight_uberlight_cutoff( "cl_dklight_uberlight_cutoff", "650.0" );
ConVar cl_dklight_uberlight_shearx( "cl_dklight_uberlight_shearx", "0.0" );
ConVar cl_dklight_uberlight_sheary( "cl_dklight_uberlight_sheary", "0.0" );
ConVar cl_dklight_uberlight_width( "cl_dklight_uberlight_width", "0.3" );
ConVar cl_dklight_uberlight_wedge( "cl_dklight_uberlight_wedge", "0.05" );
ConVar cl_dklight_uberlight_height( "cl_dklight_uberlight_height", "0.3" );
ConVar cl_dklight_uberlight_hedge( "cl_dklight_uberlight_hedge", "0.05" );
ConVar cl_dklight_uberlight_roundness( "cl_dklight_uberlight_roundness", "0.8" );

#else // New
ConVar cl_globallight_ortho_size("cl_dklight_ortho_size", "0.0", FCVAR_CHEAT, "Set to values greater than 0 for ortho view render projections.");
ConVar cl_globallight_shadowquality( "cl_dklight_shadowquality", "1" );
ConVar cl_globallight_shadowhighres( "cl_dklight_shadowhighres", "1" );
ConVar cl_globalight_shadowfiltersize( "cl_dklight_shadowfiltersize", "0.5" );

ConVar cl_dklight_freeze( "cl_dklight_freeze", "0" );
ConVar cl_dklight_xoffset( "cl_dklight_xoffset", "0" );
ConVar cl_dklight_yoffset( "cl_dklight_yoffset", "0" );
ConVar cl_globallight_globallight( "cl_dklight_globallight", "0" );
ConVar cl_globallight_fov( "cl_dklight_fov", "50.0" );
ConVar cl_globallight_sundistance( "cl_dklight_sundistance", "750" );
ConVar cl_globallight_brightnessscale("cl_dklight_brightnessscale", "0.1");
ConVar cl_globallight_drawfrustum( "cl_dklight_drawfrustum", "0" );

ConVar cl_dklight_quadraticatten( "cl_dklight_quadraticatten", "0" );
ConVar cl_dklight_linearatten( "cl_dklight_linearatten", "100" );
ConVar cl_dklight_constantatten( "cl_dklight_constantatten", "0" );
ConVar cl_dklight_farzatten( "cl_dklight_farzatten", "1000.0" );


// Uber light
ConVar cl_dklight_uberlight( "cl_dklight_uberlight", "1" );

ConVar cl_dklight_uberlight_nearedge( "cl_dklight_uberlight_nearedge", "2.0" );
ConVar cl_dklight_uberlight_faredge( "cl_dklight_uberlight_faredge", "1000.0f" );
ConVar cl_dklight_uberlight_cuton( "cl_dklight_uberlight_cuton", "10.0" );
ConVar cl_dklight_uberlight_cutoff( "cl_dklight_uberlight_cutoff", "650.0" );
ConVar cl_dklight_uberlight_shearx( "cl_dklight_uberlight_shearx", "0.0" );
ConVar cl_dklight_uberlight_sheary( "cl_dklight_uberlight_sheary", "0.0" );
ConVar cl_dklight_uberlight_width( "cl_dklight_uberlight_width", "0.3" );
ConVar cl_dklight_uberlight_wedge( "cl_dklight_uberlight_wedge", "0.05" );
ConVar cl_dklight_uberlight_height( "cl_dklight_uberlight_height", "0.3" );
ConVar cl_dklight_uberlight_hedge( "cl_dklight_uberlight_hedge", "0.05" );
ConVar cl_dklight_uberlight_roundness( "cl_dklight_uberlight_roundness", "0.8" );
#endif // 0

// Deferred light
ConVar cl_dklight_deferred_diffuse( "cl_dklight_deferred_diffuse", "171 67 36" );
ConVar cl_dklight_deferred_alpha( "cl_dklight_deferred_alpha", "255" );
ConVar cl_dklight_deferred_radius( "cl_dklight_deferred_radius", "1024.0" );
ConVar cl_dklight_deferred_falloff( "cl_dklight_deferred_falloff", "1.0" );
ConVar cl_dklight_deferred_dist( "cl_dklight_deferred_dist", "96.0" );
ConVar cl_dklight_deferred_visdist( "cl_dklight_deferred_visdist", "1024.0" );
ConVar cl_dklight_deferred_shadowdist( "cl_dklight_deferred_shadowdist", "512.0" );
ConVar cl_dklight_deferred_lightspeed( "cl_dklight_deferred_lightspeed", "512.0" );

//------------------------------------------------------------------------------
// Purpose : Sunlights shadow control entity
//------------------------------------------------------------------------------
class C_DKLight : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_DKLight, C_BaseEntity );

	DECLARE_CLIENTCLASS();

	virtual ~C_DKLight();

	void OnDataChanged( DataUpdateType_t updateType );
	void Spawn();
	bool ShouldDraw();
	void UpdateOnRemove( void );

	void ClientThink();

private:
	Vector m_shadowDirection;
	bool m_bEnabled;
	char m_TextureName[ MAX_PATH ];
	CTextureReference m_SpotlightTexture;
	color32	m_LightColor;
	Vector m_CurrentLinearFloatLightColor;
	float m_flCurrentLinearFloatLightAlpha;
	float m_flColorTransitionTime;
	float m_flSunDistance;
	float m_flFOV;
	float m_flNearZ;
	float m_flNorthOffset;
	bool m_bEnableShadows;
	bool m_bOldEnableShadows;

	static ClientShadowHandle_t m_LocalFlashlightHandle;

#ifdef DEFERRED_ENABLED
	// deferred version
	Vector m_vSmootherLightOrigin;
	def_light_t *m_pLight;
#endif // DEFERRED_ENABLED
};


ClientShadowHandle_t C_DKLight::m_LocalFlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;


IMPLEMENT_CLIENTCLASS_DT(C_DKLight, DT_DKLight, CDKLight)
	RecvPropVector(RECVINFO(m_shadowDirection)),
	RecvPropBool(RECVINFO(m_bEnabled)),
	RecvPropString(RECVINFO(m_TextureName)),
	RecvPropInt(RECVINFO(m_LightColor), 0, RecvProxy_Int32ToColor32),
	RecvPropFloat(RECVINFO(m_flColorTransitionTime)),
	RecvPropFloat(RECVINFO(m_flSunDistance)),
	RecvPropFloat(RECVINFO(m_flFOV)),
	RecvPropFloat(RECVINFO(m_flNearZ)),
	RecvPropFloat(RECVINFO(m_flNorthOffset)),
	RecvPropBool(RECVINFO(m_bEnableShadows)),
END_RECV_TABLE()


C_DKLight::~C_DKLight()
{
	if ( m_LocalFlashlightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->DestroyFlashlight( m_LocalFlashlightHandle );
		m_LocalFlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}

void C_DKLight::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_SpotlightTexture.Init( m_TextureName, TEXTURE_GROUP_OTHER, true );
	}

	BaseClass::OnDataChanged( updateType );
}

void C_DKLight::Spawn()
{
	BaseClass::Spawn();

#ifdef DEFERRED_ENABLED
	m_vSmootherLightOrigin = vec3_origin;
#endif // DEFERRED_ENABLED
	m_bOldEnableShadows = m_bEnableShadows;

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

//------------------------------------------------------------------------------
// We don't draw...
//------------------------------------------------------------------------------
bool C_DKLight::ShouldDraw()
{
	return false;
}

void C_DKLight::UpdateOnRemove( void )
{
	if ( m_LocalFlashlightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->DestroyFlashlight( m_LocalFlashlightHandle );
		m_LocalFlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}

#ifdef DEFERRED_ENABLED
	if( m_pLight )
	{
		GetLightingManager()->RemoveLight( m_pLight );
		delete m_pLight;
		m_pLight = NULL;
	}
#endif // DEFERRED_ENABLED

	BaseClass::UpdateOnRemove();
}

void C_DKLight::ClientThink()
{
	VPROF("C_DKLight::ClientThink");

	bool bSupressWorldLights = false;

	if ( cl_dklight_freeze.GetBool() == true )
	{
		return;
	}

	C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
	if( !pPlayer )
		return;

#ifdef DEFERRED_ENABLED
	bool bDeferredRendering = GetDeferredManager()->IsDeferredRenderingEnabled();
	bool bEnabled = m_bEnabled && !pPlayer->GetControlledUnit() && !bDeferredRendering && cl_dklight_enable.GetBool();
	bool bDeferredEnabled = m_bEnabled && !pPlayer->GetControlledUnit() && bDeferredRendering && cl_dklight_enable.GetBool();
#else
	bool bEnabled = m_bEnabled && !pPlayer->GetControlledUnit() && cl_dklight_enable.GetBool();
#endif // DEFERRED_ENABLED

	if ( bEnabled )
	{
		Vector vLinearFloatLightColor( m_LightColor.r, m_LightColor.g, m_LightColor.b );
		float flLinearFloatLightAlpha = m_LightColor.a;

		if ( m_CurrentLinearFloatLightColor != vLinearFloatLightColor || m_flCurrentLinearFloatLightAlpha != flLinearFloatLightAlpha )
		{
			float flColorTransitionSpeed = gpGlobals->frametime * m_flColorTransitionTime * 255.0f;

			m_CurrentLinearFloatLightColor.x = Approach( vLinearFloatLightColor.x, m_CurrentLinearFloatLightColor.x, flColorTransitionSpeed );
			m_CurrentLinearFloatLightColor.y = Approach( vLinearFloatLightColor.y, m_CurrentLinearFloatLightColor.y, flColorTransitionSpeed );
			m_CurrentLinearFloatLightColor.z = Approach( vLinearFloatLightColor.z, m_CurrentLinearFloatLightColor.z, flColorTransitionSpeed );
			m_flCurrentLinearFloatLightAlpha = Approach( flLinearFloatLightAlpha, m_flCurrentLinearFloatLightAlpha, flColorTransitionSpeed );
		}

		FlashlightState_t state;

		Vector vDirection = m_shadowDirection;
		VectorNormalize( vDirection );

		Vector vForward, vRight, vUp;
		//C_BasePlayer::GetLocalPlayer()->GetVectors(&vForward, &vRight, &vUp);
		//vDirection = vForward;
		vDirection = pPlayer->GetMouseAim();

		//Vector vViewUp = Vector( 0.0f, 1.0f, 0.0f );
		Vector vSunDirection2D = vDirection;
		vSunDirection2D.z = 0.0f;

		HACK_GETLOCALPLAYER_GUARD( "C_DKLight::ClientThink" );

		if ( !C_BasePlayer::GetLocalPlayer() )
			return;

		Vector vPos;
		QAngle EyeAngles;
		//float flZNear, flZFar, flFov; 

		m_flFOV = cl_globallight_fov.GetFloat();
		m_flSunDistance = cl_globallight_sundistance.GetFloat();

//		C_BasePlayer::GetLocalPlayer()->CalcView( vPos, EyeAngles, flZNear, flZFar, flFov );
//		Vector vPos = C_BasePlayer::GetLocalPlayer()->GetAbsOrigin();
		vPos = pPlayer->GetMouseData().m_vWorldOnlyEndPos;
		
//		vPos = Vector( 0.0f, 0.0f, 500.0f );
		vPos = ( vPos + vSunDirection2D * m_flNorthOffset ) - vDirection * m_flSunDistance;
		vPos += Vector( cl_dklight_xoffset.GetFloat(), cl_dklight_yoffset.GetFloat(), 0.0f );

		QAngle angAngles;
		VectorAngles( vDirection, angAngles );

		//Vector vForward, vRight, vUp;
		AngleVectors( angAngles, &vForward, &vRight, &vUp );

		state.m_fHorizontalFOVDegrees = m_flFOV;
		state.m_fVerticalFOVDegrees = m_flFOV;

		state.m_vecLightOrigin = vPos;
		BasisToQuaternion( vForward, vRight, vUp, state.m_quatOrientation );

		
		state.m_fQuadraticAtten = 0.0f;
		state.m_fLinearAtten = m_flSunDistance * 2.0f;
		state.m_fConstantAtten = 0.0f;
		state.m_FarZAtten = m_flSunDistance * 2.0f;
		/*
		state.m_fQuadraticAtten = cl_dklight_quadraticatten.GetFloat();
		state.m_fLinearAtten = cl_dklight_linearatten.GetFloat();
		state.m_fConstantAtten = cl_dklight_constantatten.GetFloat();
		state.m_FarZAtten = cl_dklight_farzatten.GetFloat();
		*/
		state.m_Color[0] = m_CurrentLinearFloatLightColor.x * ( 1.0f / 255.0f ) * m_flCurrentLinearFloatLightAlpha;
		state.m_Color[1] = m_CurrentLinearFloatLightColor.y * ( 1.0f / 255.0f ) * m_flCurrentLinearFloatLightAlpha;
		state.m_Color[2] = m_CurrentLinearFloatLightColor.z * ( 1.0f / 255.0f ) * m_flCurrentLinearFloatLightAlpha;
		state.m_Color[3] = 0.0f; // fixme: need to make ambient work m_flAmbient;
		state.m_NearZ = 4.0f;
		state.m_FarZ = m_flSunDistance * 2.0f;
		state.m_fBrightnessScale = cl_globallight_brightnessscale.GetFloat();
		state.m_bGlobalLight = cl_globallight_globallight.GetBool();

		float flOrthoSize = cl_globallight_ortho_size.GetFloat();

		if ( flOrthoSize > 0 )
		{
			state.m_bOrtho = true;
			state.m_fOrthoLeft = -flOrthoSize;
			state.m_fOrthoTop = -flOrthoSize;
			state.m_fOrthoRight = flOrthoSize;
			state.m_fOrthoBottom = flOrthoSize;
		}
		else
		{
			state.m_bOrtho = false;
		}

		state.m_bDrawShadowFrustum = cl_globallight_drawfrustum.GetBool();
		state.m_flShadowSlopeScaleDepthBias = g_pMaterialSystemHardwareConfig->GetShadowSlopeScaleDepthBias();;
		state.m_flShadowDepthBias = g_pMaterialSystemHardwareConfig->GetShadowDepthBias();
		state.m_bEnableShadows = m_bEnableShadows;
		state.m_pSpotlightTexture = m_SpotlightTexture;
		state.m_pProjectedMaterial = NULL; // don't complain cause we aren't using simple projection in this class
		state.m_nSpotlightTextureFrame = 0;

		state.m_nShadowQuality = cl_globallight_shadowquality.GetInt(); // Allow entity to affect shadow quality
		state.m_bShadowHighRes = cl_globallight_shadowhighres.GetBool();
		state.m_flShadowFilterSize = cl_globalight_shadowfiltersize.GetFloat();

		// Uberlight test
		state.m_bUberlight = cl_dklight_uberlight.GetBool();

		state.m_uberlightState.m_fNearEdge 	= cl_dklight_uberlight_nearedge.GetFloat();
		state.m_uberlightState.m_fFarEdge  	= cl_dklight_uberlight_faredge.GetFloat();
		state.m_uberlightState.m_fCutOn    	= cl_dklight_uberlight_cuton.GetFloat();
		state.m_uberlightState.m_fCutOff   	= cl_dklight_uberlight_cutoff.GetFloat();
		state.m_uberlightState.m_fShearx   	= cl_dklight_uberlight_shearx.GetFloat();
		state.m_uberlightState.m_fSheary   	= cl_dklight_uberlight_sheary.GetFloat();
		state.m_uberlightState.m_fWidth    	= cl_dklight_uberlight_width.GetFloat();
		state.m_uberlightState.m_fWedge    	= cl_dklight_uberlight_wedge.GetFloat();
		state.m_uberlightState.m_fHeight	= cl_dklight_uberlight_height.GetFloat();
		state.m_uberlightState.m_fHedge		= cl_dklight_uberlight_hedge.GetFloat();
		state.m_uberlightState.m_fRoundness	= cl_dklight_uberlight_roundness.GetFloat();

		if ( m_bOldEnableShadows != m_bEnableShadows )
		{
			// If they change the shadow enable/disable, we need to make a new handle
			if ( m_LocalFlashlightHandle != CLIENTSHADOW_INVALID_HANDLE )
			{
				g_pClientShadowMgr->DestroyFlashlight( m_LocalFlashlightHandle );
				m_LocalFlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
			}

			m_bOldEnableShadows = m_bEnableShadows;
		}

		if( m_LocalFlashlightHandle == CLIENTSHADOW_INVALID_HANDLE )
		{
			m_LocalFlashlightHandle = g_pClientShadowMgr->CreateFlashlight( state );
		}
		else
		{
			g_pClientShadowMgr->UpdateFlashlightState( m_LocalFlashlightHandle, state );
			g_pClientShadowMgr->UpdateProjectedTexture( m_LocalFlashlightHandle, true );
		}

		bSupressWorldLights = m_bEnableShadows;
	}
	else if ( m_LocalFlashlightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->DestroyFlashlight( m_LocalFlashlightHandle );
		m_LocalFlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}

	g_pClientShadowMgr->SetShadowFromWorldLightsEnabled( !bSupressWorldLights );

#ifdef DEFERRED_ENABLED
	if( bDeferredEnabled )
	{
		// Create if needed
		if( !m_pLight )
		{
			m_pLight = new def_light_t();

			GetLightingManager()->AddLight( m_pLight );
		}

		Vector diff;
		UTIL_StringToVector( diff.Base(), cl_dklight_deferred_diffuse.GetString() );

		color32 color;
		color.r = diff.x;
		color.g = diff.y;
		color.b = diff.z;
		color.a = cl_dklight_deferred_alpha.GetInt();

		// Update data
		Assert( m_pLight != NULL );
		ABS_QUERY_GUARD( true );

		Vector vTargetLightOrigin = pPlayer->GetMouseData().m_vEndPos + - (pPlayer->GetMouseAim() * cl_dklight_deferred_dist.GetFloat());
		Vector vDir = vTargetLightOrigin - m_vSmootherLightOrigin;
		float fDist = VectorNormalize(vDir);
		if( !m_vSmootherLightOrigin.IsValid() || fDist > 128.0f )
		{
			m_vSmootherLightOrigin = vTargetLightOrigin;
		}
		else
		{
			m_vSmootherLightOrigin = m_vSmootherLightOrigin + vDir * MIN(fDist, gpGlobals->frametime * cl_dklight_deferred_lightspeed.GetFloat());
		}

		//Msg("Smoothed: %f %f %f, Target: %f %f %f, dist: %f\n", m_vSmootherLightOrigin.x, m_vSmootherLightOrigin.y, m_vSmootherLightOrigin.z,
		//	vTargetLightOrigin.x, vTargetLightOrigin.y, vTargetLightOrigin.z, fDist );

		m_pLight->ang = GetRenderAngles();
		m_pLight->pos = m_vSmootherLightOrigin; 

		m_pLight->col_diffuse = c32ToVec(color);
		//m_pLight->col_ambient = Vector(20, 20, 20);

		m_pLight->flRadius = cl_dklight_deferred_radius.GetFloat(); 
		m_pLight->flFalloffPower = cl_dklight_deferred_falloff.GetFloat();

		m_pLight->flSpotCone_Inner = SPOT_DEGREE_TO_RAD( 15.0f );
		m_pLight->flSpotCone_Outer = SPOT_DEGREE_TO_RAD( 45.0f );

		m_pLight->iVisible_Dist = cl_dklight_deferred_visdist.GetFloat();
		m_pLight->iVisible_Range = cl_dklight_deferred_visdist.GetFloat();
		m_pLight->iShadow_Dist = cl_dklight_deferred_shadowdist.GetFloat();
		m_pLight->iShadow_Range = cl_dklight_deferred_shadowdist.GetFloat();

		m_pLight->iStyleSeed = (unsigned short)-1;
		m_pLight->flStyle_Amount = 0.5;
		m_pLight->flStyle_Random = 0.5; 
		m_pLight->flStyle_Smooth = 0.5;
		m_pLight->flStyle_Speed = 10;

		//m_pLight->iLighttype = GetLight_Type();
		m_pLight->iFlags >>= DEFLIGHTGLOBAL_FLAGS_MAX_SHARED_BITS;
		m_pLight->iFlags <<= DEFLIGHTGLOBAL_FLAGS_MAX_SHARED_BITS;
		m_pLight->iFlags |= DEFLIGHT_SHADOW_ENABLED|DEFLIGHT_LIGHTSTYLE_ENABLED;
		//m_pLight->iCookieIndex = GetCookieIndex();

		m_pLight->MakeDirtyXForms();
		m_pLight->MakeDirtyRenderMesh();
	}
	else if( m_pLight )
	{
		GetLightingManager()->RemoveLight( m_pLight );
		delete m_pLight;
		m_pLight = NULL;
	}
#endif // DEFERRED_ENABLED

	BaseClass::ClientThink();
}
