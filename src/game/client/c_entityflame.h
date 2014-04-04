//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#ifndef C_ENTITY_FLAME_H
#define C_ENTITY_FLAME_H

#include "c_baseentity.h"

//
// Entity flame, client-side implementation
//

class C_EntityFlame : public C_BaseEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_EntityFlame, C_BaseEntity );

	C_EntityFlame( void );
	virtual ~C_EntityFlame( void );

	static C_EntityFlame	*Create( CBaseEntity *pTarget, float flLifetime, float flSize = 0.0f );

	virtual bool	Simulate( void );
	virtual void	UpdateOnRemove( void );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	ClientThink( void );

	// Pure client side flame entities only functions
	void	AttachToEntity( CBaseEntity *pTarget );
	void	SetLifetime( float lifetime );

	EHANDLE				m_hEntAttached;		// The entity that we are burning (attached to).

private:
	void	CreateEffect( void );
	void	StopEffect( void );

	CUtlReference<CNewParticleEffect> m_hEffect;
	EHANDLE				m_hOldAttached;
	bool				m_bCheapEffect;

	// Pure client side flame entities only variables
	int m_flSize;
	float m_flLifetime;
};

inline void C_EntityFlame::AttachToEntity( CBaseEntity *pTarget )
{
	m_hEntAttached = pTarget;
}

inline void C_EntityFlame::SetLifetime( float lifetime )
{
	SetNextClientThink( gpGlobals->curtime + lifetime );
}

#endif // C_ENTITY_FLAME_H