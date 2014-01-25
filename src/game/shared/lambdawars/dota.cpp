//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Fixes for DOTA
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "deferred/CDefLight.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
class CEnvDeferredLight : public CDeferredLight
{
public:
	DECLARE_CLASS( CEnvDeferredLight, CDeferredLight );
	DECLARE_DATADESC();

	virtual void Spawn();

private:
	float m_fIntensity;
	float m_fStartFalloff;
	Vector m_vLightColor;
	float m_fRadius;
};

LINK_ENTITY_TO_CLASS(env_deferred_light, CEnvDeferredLight);

BEGIN_DATADESC( CEnvDeferredLight )
	DEFINE_KEYFIELD( m_fIntensity,	FIELD_FLOAT,	"intensity" ),
	DEFINE_KEYFIELD( m_vLightColor,		FIELD_VECTOR,	"lightcolor" ),
	DEFINE_KEYFIELD( m_fStartFalloff,	FIELD_FLOAT,	"start_falloff" ),
END_DATADESC()

void CEnvDeferredLight::Spawn()
{
	BaseClass::Spawn();

	// Always on!
	AddSpawnFlags( DEFLIGHT_ENABLED|DEFLIGHT_SHADOW_ENABLED );

	KeyValue( "light_type", "0" );
	KeyValue( "diffuse", UTIL_VarArgs( "%f %f %f %f", m_vLightColor.x, m_vLightColor.y, m_vLightColor.z, 255.0f * m_fIntensity ) );
	KeyValue( "ambient", "10 10 10 255" );
	
	KeyValue( "power", UTIL_VarArgs( "%f", m_fStartFalloff ) );

	KeyValue( "vis_dist", UTIL_VarArgs( "%d", 2048 ) );
	KeyValue( "vis_range", UTIL_VarArgs( "%d", 512 ) );
	KeyValue( "shadow_dist", UTIL_VarArgs( "%d", 1536 ) );
	KeyValue( "shadow_range", UTIL_VarArgs( "%d", 512 ) );

	KeyValue( "spot_cone_inner", UTIL_VarArgs( "%f", 35.0 ) );
	KeyValue( "spot_cone_outer", UTIL_VarArgs( "%f", 45.0 ) );
}

class CEnvDeferredSpotLight : public CEnvDeferredLight
{
public:
	DECLARE_CLASS( CEnvDeferredSpotLight, CEnvDeferredLight );
	DECLARE_DATADESC();

	virtual void Spawn();

private:
	float m_fSpotFOV;
	float m_fSpotLightDistance;
};

BEGIN_DATADESC( CEnvDeferredSpotLight  )
	DEFINE_KEYFIELD( m_fSpotFOV,	FIELD_FLOAT,	"spot_fov" ),
	DEFINE_KEYFIELD( m_fSpotLightDistance,	FIELD_FLOAT,	"spot_light_distance" ),
END_DATADESC()


void CEnvDeferredSpotLight::Spawn()
{
	BaseClass::Spawn();

	KeyValue( "light_type", "1" );
	KeyValue( "spot_cone_inner", m_fSpotFOV );
	KeyValue( "spot_cone_outer", m_fSpotFOV );
	KeyValue( "radius", m_fSpotLightDistance );

}

LINK_ENTITY_TO_CLASS(env_deferred_spot_light, CEnvDeferredSpotLight);


#endif // CLIENT_DLL