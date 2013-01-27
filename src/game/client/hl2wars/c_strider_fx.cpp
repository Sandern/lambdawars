//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_strider_fx.h"
#include "c_ai_basenpc.h"
#include "c_te_particlesystem.h"
#include "fx.h"
#include "fx_sparks.h"
#include "c_tracer.h"
#include "clientsideeffects.h"
#include "iefx.h"
#include "dlight.h"
#include "bone_setup.h"
#include "c_rope.h"
#include "fx_line.h"
#include "c_sprite.h"
#include "view.h"
#include "view_scene.h"
#include "materialsystem/imaterialvar.h"
#include "simple_keys.h"
#include "iclientvehicle.h"
#include "engine/ivdebugoverlay.h"
#include "particles_localspace.h"
#include "dlight.h"
#include "iefx.h"
#include "c_te_effect_dispatch.h"
#include "tier0/vprof.h"
//#include "ClientEffectPrecacheSystem.h"
#include <bitbuf.h>
#include "fx_water.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

C_StriderFX::C_StriderFX()
{
	m_pOwner = NULL;
	m_active = false;
}

void C_StriderFX::Update( C_BaseEntity *pOwner, const Vector &targetPos )
{
	BaseClass::Update();

	m_pOwner = pOwner;
	
	if ( m_active )
	{
		m_targetPosition = targetPos;
	}
}

// --on gun
// warpy sprite bit
// darkening sprite
// glowy blue flare sprite
// bubble warpy sprite
// after glow sprite

// --on line of sight
// narrow beam
// wide beam

// --on impact point
// sparkly white bits
// sparkly white streaks
// pale blue particle steam

enum 
{
	STRIDERFX_WARP_SCALE = 0,
	STRIDERFX_DARKNESS,
	STRIDERFX_FLARE_COLOR,
	STRIDERFX_FLARE_SIZE,
	STRIDERFX_BUBBLE_SIZE,
	STRIDERFX_BUBBLE_REFRACT,

	STRIDERFX_NARROW_BEAM_COLOR,
	STRIDERFX_NARROW_BEAM_SIZE,
	
	STRIDERFX_WIDE_BEAM_COLOR,
	STRIDERFX_WIDE_BEAM_SIZE,

	STRIDERFX_AFTERGLOW_COLOR,

	STRIDERFX_WIDE_BEAM_LENGTH,

	STRIDERFX_SPARK_COUNT,
	STRIDERFX_STREAK_COUNT,
	STRIDERFX_STEAM_COUNT,


	// must be last
	STRIDERFX_PARAMETERS,
};

class CStriderFXEnvelope
{
public:
	CStriderFXEnvelope();

	void AddKey( int parameterIndex, const CSimpleKeyInterp &key )
	{
		Assert( parameterIndex >= 0 && parameterIndex < STRIDERFX_PARAMETERS );

		if ( parameterIndex >= 0 && parameterIndex < STRIDERFX_PARAMETERS )
		{
			m_parameters[parameterIndex].Insert( key );
		}

	}

	CSimpleKeyList		m_parameters[STRIDERFX_PARAMETERS];
};

// NOTE: Beam widths are half-widths or radii, so this is a beam that represents a cylinder with 2" radius
const float NARROW_BEAM_WIDTH = 2;
const float WIDE_BEAM_WIDTH = 16;
const float FLARE_SIZE = 128;
const float	DARK_SIZE = 64;
const float AFTERGLOW_SIZE = 64;

const float WARP_SIZE = 512;
const float WARP_REFRACT = 0.075f;
const float WARP_BUBBLE_SIZE = 256;
const float WARP_BUBBLE_REFRACT = 1.0f;

CStriderFXEnvelope::CStriderFXEnvelope()
{
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 1.25, KEY_ACCELERATE, 1 ) );
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 1.3, KEY_LINEAR, 0 ) );

	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 0.5, KEY_SPLINE, 1 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 1.0, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 1.25, KEY_SPLINE, 0 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 2.0, KEY_SPLINE, 0 ) );

	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 0.5, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 1.25, KEY_ACCELERATE, 1 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 1.5, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 2.0, KEY_SPLINE, 0 ) );

	AddKey( STRIDERFX_FLARE_SIZE, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_FLARE_SIZE, CSimpleKeyInterp( 1.0, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_FLARE_SIZE, CSimpleKeyInterp( 2.0, KEY_LINEAR, 1 ) );

	AddKey( STRIDERFX_BUBBLE_SIZE, CSimpleKeyInterp( 1.3, KEY_LINEAR, 0.5 ) );
	AddKey( STRIDERFX_BUBBLE_SIZE, CSimpleKeyInterp( 2.0, KEY_DECELERATE, 2 ) );

	AddKey( STRIDERFX_BUBBLE_REFRACT, CSimpleKeyInterp( 1.3, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_BUBBLE_REFRACT, CSimpleKeyInterp( 2.0, KEY_LINEAR, 0 ) );

	AddKey( STRIDERFX_NARROW_BEAM_COLOR, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_NARROW_BEAM_COLOR, CSimpleKeyInterp( 1.25, KEY_ACCELERATE, 1.0 ) );
	AddKey( STRIDERFX_NARROW_BEAM_COLOR, CSimpleKeyInterp( 1.5, KEY_SPLINE, 0 ) );

	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 0.5, KEY_ACCELERATE, 1 ) );
	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 1.5, KEY_DECELERATE, 2 ) );
	
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 1.25, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 1.5, KEY_SPLINE, 1 ) );
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 1.75, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 2.1, KEY_SPLINE, 0 ) );
	
	AddKey( STRIDERFX_WIDE_BEAM_SIZE, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_WIDE_BEAM_SIZE, CSimpleKeyInterp( 2.1, KEY_LINEAR, 1 ) );

	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 1.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 1.25, KEY_SPLINE, 1 ) );
	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 3.0, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 3.5, KEY_ACCELERATE, 0 ) );

	AddKey( STRIDERFX_WIDE_BEAM_LENGTH, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1.0 ) );
	AddKey( STRIDERFX_WIDE_BEAM_LENGTH, CSimpleKeyInterp( 1.5, KEY_ACCELERATE, 0.0 ) );
	AddKey( STRIDERFX_WIDE_BEAM_LENGTH, CSimpleKeyInterp( 2.1, KEY_LINEAR, 0 ) );

	//AddKey( STRIDERFX_SPARK_COUNT,
	//AddKey( STRIDERFX_STREAK_COUNT,
	//AddKey( STRIDERFX_STEAM_COUNT,
}

CStriderFXEnvelope g_StriderCannonEnvelope;

void ScaleColor( color32 &out, const color32 &in, float scale )
{
	out.r = (byte)(int)((float)in.r * scale);
	out.g = (byte)(int)((float)in.g * scale);
	out.b = (byte)(int)((float)in.b * scale);
	out.a = (byte)(int)((float)in.a * scale);
}

void DrawSpriteTangentSpace( const Vector &vecOrigin, float flWidth, float flHeight, color32 color )
{
	unsigned char pColor[4] = { color.r, color.g, color.b, color.a };

	// Generate half-widths
	flWidth *= 0.5f;
	flHeight *= 0.5f;

	// Compute direction vectors for the sprite
	Vector fwd, right( 1, 0, 0 ), up( 0, 1, 0 );
	VectorSubtract( CurrentViewOrigin(), vecOrigin, fwd );
	float flDist = VectorNormalize( fwd );
	if (flDist >= 1e-3)
	{
		CrossProduct( CurrentViewUp(), fwd, right );
		flDist = VectorNormalize( right );
		if (flDist >= 1e-3)
		{
			CrossProduct( fwd, right, up );
		}
		else
		{
			// In this case, fwd == g_vecVUp, it's right above or 
			// below us in screen space
			CrossProduct( fwd, CurrentViewRight(), up );
			VectorNormalize( up );
			CrossProduct( up, fwd, right );
		}
	}

	Vector left = -right;
	Vector down = -up;
	Vector back = -fwd;

	CMeshBuilder meshBuilder;
	Vector point;
	CMatRenderContextPtr pRenderContext( materials );
	IMesh* pMesh = pRenderContext->GetDynamicMesh( );

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 0, 1);
	VectorMA (vecOrigin, -flHeight, up, point);
	VectorMA (point, -flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 0, 0);
	VectorMA (vecOrigin, flHeight, up, point);
	VectorMA (point, -flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 1, 0);
	VectorMA (vecOrigin, flHeight, up, point);
	VectorMA (point, flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 1, 1);
	VectorMA (vecOrigin, -flHeight, up, point);
	VectorMA (point, flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();
	
	meshBuilder.End();
	pMesh->Draw();
}


void Strider_DrawSprite( const Vector &vecOrigin, float size, const color32 &color )
{
	DrawSpriteTangentSpace( vecOrigin, size, size, color );
}


void Strider_DrawLine( const Vector &start, const Vector &end, float width, IMaterial *pMaterial, const color32 &color )
{
	FX_DrawLineFade( start, end, width, pMaterial, color, 8.0f );
}

int	C_StriderFX::DrawModel( int flags, const RenderableInstance_t &instance )
{
	static color32 white = {255,255,255,255};
	Vector params[STRIDERFX_PARAMETERS];
	bool hasParam[STRIDERFX_PARAMETERS];

	if ( !m_active )
		return 1;

	C_BaseEntity *ent = cl_entitylist->GetEnt( m_entityIndex );
	if ( ent )
	{
		QAngle angles;
		ent->GetAttachment( m_attachment, m_worldPosition, angles );
	}

	// This forces time to drive from the main clock instead of being integrated per-draw below
	// that way the effect moves on even when culled for visibility
	if ( m_limitHitTime > 0 && m_tMax > 0 )
	{
		float dt = m_limitHitTime - gpGlobals->curtime;
		if ( dt < 0 )
		{
			dt = 0;
		}
		// if the clock needs to move, update it.
		if ( m_tMax - dt > m_t )
		{
			m_t = m_tMax - dt;
			m_beamEndPosition = m_worldPosition;
		}
	}
	else
	{
		// don't have enough info to derive the time, integrate current frame time
		m_t += gpGlobals->frametime;
		if ( m_tMax > 0 )
		{
			m_t = clamp( m_t, 0, m_tMax );
			m_beamEndPosition = m_worldPosition;
		}
	}
	float t = m_t;

	bool hasAny = false;
	memset( hasParam, 0, sizeof(hasParam) );
	for ( int i = 0; i < STRIDERFX_PARAMETERS; i++ )
	{
		hasParam[i] = g_StriderCannonEnvelope.m_parameters[i].Interp( params[i], t );
		hasAny = hasAny || hasParam[i];
	}

	pixelvis_queryparams_t gunParams;
	gunParams.Init(m_worldPosition, 4.0f);
	float gunFractionVisible = PixelVisibility_FractionVisible( gunParams, &m_queryHandleGun );
	bool gunVisible = gunFractionVisible > 0.0f ? true : false;

	// draw the narrow beam
	if ( hasParam[STRIDERFX_NARROW_BEAM_COLOR] && hasParam[STRIDERFX_NARROW_BEAM_SIZE] )
	{
		IMaterial *pMat = materials->FindMaterial( "sprites/bluelaser1", TEXTURE_GROUP_CLIENT_EFFECTS );
		float width = NARROW_BEAM_WIDTH * params[STRIDERFX_NARROW_BEAM_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_NARROW_BEAM_COLOR].x;
		ScaleColor( color, white, bright );

		Strider_DrawLine( m_beamEndPosition, m_targetPosition, width, pMat, color );
	}

	// draw the wide beam
	if ( hasParam[STRIDERFX_WIDE_BEAM_COLOR] && hasParam[STRIDERFX_WIDE_BEAM_SIZE] )
	{
		IMaterial *pMat = materials->FindMaterial( "effects/blueblacklargebeam", TEXTURE_GROUP_CLIENT_EFFECTS );
		float width = WIDE_BEAM_WIDTH * params[STRIDERFX_WIDE_BEAM_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_WIDE_BEAM_COLOR].x;
		ScaleColor( color, white, bright );
		Vector wideBeamEnd = m_beamEndPosition;
		if ( hasParam[STRIDERFX_WIDE_BEAM_LENGTH] )
		{
			float amt = params[STRIDERFX_WIDE_BEAM_LENGTH].x;
			wideBeamEnd = m_beamEndPosition * amt + m_targetPosition * (1-amt);
		}

		Strider_DrawLine( wideBeamEnd, m_targetPosition, width, pMat, color );
	}

// after glow sprite
	bool updated = false;
	CMatRenderContextPtr pRenderContext( materials );
// warpy sprite bit
	if ( hasParam[STRIDERFX_WARP_SCALE] && !hasParam[STRIDERFX_BUBBLE_SIZE] && gunVisible )
	{
		if ( !updated )
		{
			updated = true;
			pRenderContext->Flush();
			UpdateRefractTexture();
		}

		IMaterial *pMat = materials->FindMaterial( "effects/strider_pinch_dudv", TEXTURE_GROUP_CLIENT_EFFECTS );
		float size = WARP_SIZE;
		float refract = params[STRIDERFX_WARP_SCALE].x * WARP_REFRACT * gunFractionVisible;

		pRenderContext->Bind( pMat, (IClientRenderable*)this );
		IMaterialVar *pVar = pMat->FindVar( "$refractamount", NULL );
		pVar->SetFloatValue( refract );
		Strider_DrawSprite( m_worldPosition, size, white );
	}
// darkening sprite
// glowy blue flare sprite
	if ( hasParam[STRIDERFX_FLARE_COLOR] && hasParam[STRIDERFX_FLARE_SIZE] && hasParam[STRIDERFX_DARKNESS] && gunVisible )
	{
		IMaterial *pMat = materials->FindMaterial( "effects/blueblackflash", TEXTURE_GROUP_CLIENT_EFFECTS );
		float size = FLARE_SIZE * params[STRIDERFX_FLARE_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_FLARE_COLOR].x * gunFractionVisible;
		ScaleColor( color, white, bright );
		color.a = (int)(255 * params[STRIDERFX_DARKNESS].x);
		pRenderContext->Bind( pMat, (IClientRenderable*)this );
		Strider_DrawSprite( m_worldPosition, size, color );
	}
// bubble warpy sprite
	if ( hasParam[STRIDERFX_BUBBLE_SIZE] )
	{
		Vector wideBeamEnd = m_beamEndPosition;
		if ( hasParam[STRIDERFX_WIDE_BEAM_LENGTH] )
		{
			float amt = params[STRIDERFX_WIDE_BEAM_LENGTH].x;
			wideBeamEnd = m_beamEndPosition * amt + m_targetPosition * (1-amt);
		}
		pixelvis_queryparams_t endParams;
		endParams.Init(wideBeamEnd, 4.0f, 0.001f);
		float endFractionVisible = PixelVisibility_FractionVisible( endParams, &m_queryHandleBeamEnd );
		bool endVisible = endFractionVisible > 0.0f ? true : false;

		if ( endVisible )
		{
			if ( !updated )
			{
				updated = true;
				pRenderContext->Flush();
				UpdateRefractTexture();
			}
			IMaterial *pMat = materials->FindMaterial( "effects/strider_bulge_dudv", TEXTURE_GROUP_CLIENT_EFFECTS );
			float refract = endFractionVisible * WARP_BUBBLE_REFRACT * params[STRIDERFX_BUBBLE_REFRACT].x;
			float size = WARP_BUBBLE_SIZE * params[STRIDERFX_BUBBLE_SIZE].x;
			IMaterialVar *pVar = pMat->FindVar( "$refractamount", NULL );
			pVar->SetFloatValue( refract );

			pRenderContext->Bind( pMat, (IClientRenderable*)this );
			Strider_DrawSprite( wideBeamEnd, size, white );
		}
	}
	else
	{
		// call this to have the check ready on the first frame
		pixelvis_queryparams_t endParams;
		endParams.Init(m_beamEndPosition, 4.0f, 0.001f);
		PixelVisibility_FractionVisible( endParams, &m_queryHandleBeamEnd );
	}
	if ( hasParam[STRIDERFX_AFTERGLOW_COLOR] && gunVisible )
	{
		IMaterial *pMat = materials->FindMaterial( "effects/blueblackflash", TEXTURE_GROUP_CLIENT_EFFECTS );
		float size = AFTERGLOW_SIZE;// * params[STRIDERFX_FLARE_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_AFTERGLOW_COLOR].x * gunFractionVisible;
		ScaleColor( color, white, bright );

		pRenderContext->Bind( pMat, (IClientRenderable*)this );
		Strider_DrawSprite( m_worldPosition, size, color );

		dlight_t *dl = effects->CL_AllocDlight( m_entityIndex );
		dl->origin = m_worldPosition;
		dl->color.r = 40;
		dl->color.g = 60;
		dl->color.b = 255;
		dl->color.exponent = 5;
		dl->radius = bright * 128;
		dl->die = gpGlobals->curtime + 0.001;
	}

	if ( m_t >= STRIDERFX_END_ALL_TIME && !hasAny )
	{
		EffectShutdown();
	}
	return 1;
}

