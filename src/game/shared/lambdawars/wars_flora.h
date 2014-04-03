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
	~CWarsFlora();
	
#ifndef CLIENT_DLL
	virtual void Precache( void );
#endif // CLIENT_DLL
	virtual void Spawn();

	virtual void FloraTouch( CBaseEntity *pOther );

	virtual void PlayDestructionAnimation();

#ifdef CLIENT_DLL
	void UpdateUnitAvoid();
	virtual void UpdateClientSideAnimation();

	bool KeyValue( const char *szKeyName, const char *szValue );
	bool Initialize();

	// Spawns all flora entities on the client side
	static const char *ParseEntity( const char *pEntData );
	static void SpawnMapFlora();
#endif // CLIENT_DLL 

	static void InitFloraGrid();
	static void DestroyFloraGrid();
	void		InsertInFloraGrid();
	void		RemoveFromFloraGrid();

	static void RemoveFloraInRadius( const Vector &vPosition, float fRadius );
	static void DestructFloraInRadius( const Vector &vPosition, float fRadius );
	static void IgniteFloraInRadius( const Vector &vPosition, float fRadius );

	bool			IsEditorManaged();

#ifndef CLIENT_DLL
	bool			FillKeyValues( KeyValues *pEntityKey );
#endif // CLIENT_DLL

private:
	static CWarsFlora *m_pFloraGrid;
	int				m_iKey;
	CWarsFlora		*m_pNext;

	string_t		m_iszIdleAnimationName;
	string_t		m_iszSqueezeDownAnimationName;
	string_t		m_iszSqueezeDownIdleAnimationName;
	string_t		m_iszSqueezeUpAnimationName;
	string_t		m_iszDestructionAnimationName;

	int				m_iIdleSequence;
	int				m_iSqueezeDownSequence;
	int				m_iSqueezeDownIdleSequence;
	int				m_iSqueezeUpSequence;
	int				m_iDestructSequence;

#ifdef CLIENT_DLL
	float			m_fAvoidTimeOut;
#endif // CLIENT_DLL

	bool			m_bEditorManaged;
};

inline bool CWarsFlora::IsEditorManaged()
{
	return m_bEditorManaged;
}

#endif // WARS_FLORA_H