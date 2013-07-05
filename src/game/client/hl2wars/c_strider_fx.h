//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_STRIDER_FX_H
#define C_STRIDER_FX_H
#ifdef _WIN32
#pragma once
#endif

#include "fx_envelope.h"

#define STRIDER_MSG_BIG_SHOT			1
#define STRIDER_MSG_STREAKS				2
#define STRIDER_MSG_DEAD				3

#define STOMP_IK_SLOT					11
const int NUM_STRIDER_IK_TARGETS = 6;

const float	STRIDERFX_BIG_SHOT_TIME = 1.25f;
const float STRIDERFX_END_ALL_TIME = 4.0f;

class C_StriderFX : public C_EnvelopeFX
{
public:
	typedef C_EnvelopeFX	BaseClass;

	C_StriderFX();
	~C_StriderFX()
	{
		EffectShutdown();
	}


	void			Update( C_BaseEntity *pOwner, const Vector &targetPos );

	// Returns the bounds relative to the origin (render bounds)
	virtual void	GetRenderBounds( Vector& mins, Vector& maxs )
	{
		ClearBounds( mins, maxs );
		AddPointToBounds( m_worldPosition, mins, maxs );
		AddPointToBounds( m_targetPosition, mins, maxs );
		mins -= GetRenderOrigin();
		maxs -= GetRenderOrigin();
	}

	virtual void EffectInit( int entityIndex, int attachment )
	{
		m_limitHitTime = 0;
		BaseClass::EffectInit( entityIndex, attachment );
	}
	virtual void EffectShutdown( void )
	{
		m_limitHitTime = 0;
		BaseClass::EffectShutdown();
	}

	virtual int	DrawModel( int flags, const RenderableInstance_t &instance );
	virtual void LimitTime( float tmax ) 
	{ 
		float dt = tmax - m_t;
		if ( dt < 0 )
		{
			dt = 0;
		}
		m_limitHitTime = gpGlobals->curtime + dt;
		BaseClass::LimitTime( tmax );
	}

	C_BaseEntity			*m_pOwner;
	Vector					m_targetPosition;
	Vector					m_beamEndPosition;
	pixelvis_handle_t		m_queryHandleGun;
	pixelvis_handle_t		m_queryHandleBeamEnd;
	float					m_limitHitTime;
};

#endif // C_STRIDER_FX_H