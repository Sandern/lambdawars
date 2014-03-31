//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:		
//
//=============================================================================//

#ifndef WARS_FLORA_H
#define WARS_FLORA_H

#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "c_baseanimating.h"
#else
#include "baseanimating.h"
#endif // CLIENT_DLL

class CWarsFlora : public CBaseAnimating
{
	DECLARE_CLASS( CWarsFlora, CBaseAnimating );
	DECLARE_DATADESC();

public:
	CWarsFlora();
	
#ifndef CLIENT_DLL
	virtual void Precache( void );
#endif // CLIENT_DLL
	virtual void Spawn();

	virtual void FloraTouch( CBaseEntity *pOther );

#ifdef CLIENT_DLL
	bool KeyValue( const char *szKeyName, const char *szValue );
	bool Initialize();

	// Spawns all flora entities on the client side
	static const char *ParseEntity( const char *pEntData );
	static void SpawnMapFlora();
#endif // CLIENT_DLL 

	static void RemoveFloraInRadius( const Vector &vPosition, float fRadius );

	bool			IsEditorManaged();

#ifndef CLIENT_DLL
	bool			FillKeyValues( KeyValues *pEntityKey );
#endif // CLIENT_DLL

private:
	string_t		m_iszIdleAnimationName;
	string_t		m_iszSqueezeDownAnimationName;
	string_t		m_iszSqueezeUpAnimationName;
	string_t		m_iszDestructionAnimationName;

	int				m_iIdleSequence;
	int				m_iSqueezeDownSequence;
	int				m_iSqueezeUpSequence;
	int				m_iDestructSequence;

	bool			m_bEditorManaged;
};

inline bool CWarsFlora::IsEditorManaged()
{
	return m_bEditorManaged;
}

#endif // WARS_FLORA_H