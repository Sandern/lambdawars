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
	DECLARE_PYCLASS( CWarsFlora ); 

public:
	CWarsFlora();
	~CWarsFlora();
	
#ifndef CLIENT_DLL
	virtual void Precache( void );
#endif // CLIENT_DLL
	virtual void Spawn();
	virtual void InitFloraData();

	// For managing flora in ingame prop editor
	bool HasFloraUUID();
	void GenerateFloraUUID();
	const char *GetFloraUUID();

	virtual void PlayDestructionAnimation();

#ifdef CLIENT_DLL
	void UpdateUnitAvoid();
	virtual void UpdateClientSideAnimation();

	void Ignite( float flFlameLifetime, float flSize );
	void IgniteLifetime( float flFlameLifetime );

	bool KeyValue( const char *szKeyName, const char *szValue );
	bool Initialize();

	// Spawns all flora entities on the client side
	static const char *ParseEntity( const char *pEntData );
#endif // CLIENT_DLL 

	static void		SpawnMapFlora();

	static void		InitFloraGrid();
	static void		DestroyFloraGrid();
	void			InsertInFloraGrid();
	void			RemoveFromFloraGrid();
	static void		InitFloraDataKeyValues();

	static bool		SpawnFlora( const char *modelname, const Vector &position, const QAngle &angle, KeyValues *pExtraKV = NULL, 
		boost::python::object fnpostspawn = boost::python::object() );

	static void		RemoveFloraByUUID( const char *pUUID );
	static void		RemoveFloraInRadius( const Vector &position, float radius, int max = -1 );
	static CWarsFlora *FindFloraByUUID( const char *pUUID );
	static int		CountFloraInRadius( const Vector &position, float radius );
	static int		CountFloraInRadius( const Vector &position, float radius, CUtlVector<int> &restrictmodels );
	static int		PyCountFloraInRadius( const Vector &position, float radius, boost::python::list restrictmodels = boost::python::list() );
	static void		DestructFloraInRadius( const Vector &position, float radius );
	static void		IgniteFloraInRadius( const Vector &position, float radius, float lifetime = 30.0f );

	bool			IsEditorManaged();

	bool			FillKeyValues( KeyValues *pEntityKey, int iVisGroupId = -1 );

private:
	int				m_iKey;

	static KeyValues *m_pKVFloraData;

	bool			m_bCanBeIgnited;

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

	int				m_iPoseX;
	int				m_iPoseY;
	int				m_iPoseZ;

#ifdef CLIENT_DLL
	Vector			m_vCurrentSway;
	float			m_fAvoidTimeOut;
#endif // CLIENT_DLL

	bool			m_bEditorManaged;
	string_t		m_iszFloraUUID;
};

inline bool CWarsFlora::IsEditorManaged()
{
	return m_bEditorManaged;
}

#endif // WARS_FLORA_H