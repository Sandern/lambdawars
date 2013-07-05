//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WARS_WEAPON_SHARED_H
#define WARS_WEAPON_SHARED_H

#ifdef _WIN32
#pragma once
#endif

// Shared header file for weapons
#if defined( CLIENT_DLL )
	#define CWarsWeapon C_WarsWeapon
	#define CHL2WarsPlayer C_HL2WarsPlayer
#endif

class CHL2WarsPlayer;

class CWarsWeapon : public CBaseCombatWeapon
{
public:
	DECLARE_CLASS( CWarsWeapon, CBaseCombatWeapon );

	CWarsWeapon();

	virtual bool			IsPredicted( void ) const;

#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_PYCLIENTCLASS( C_WarsWeapon );

	virtual void			OnDataChanged( DataUpdateType_t type );
	virtual bool			ShouldPredict( void );
	virtual C_BasePlayer*	GetPredictionOwner( void );

	virtual void			SetDormant( bool bDormant );
	virtual bool			ShouldDraw( void );

	virtual bool			IsCarriedByLocalPlayer( void );
	virtual bool			IsActiveByLocalPlayer( void );

	virtual bool			GetShootPosition( Vector &vOrigin, QAngle &vAngles );
	CBaseEntity *			GetMuzzleAttachEntity();

	virtual void			EnsureCorrectRenderingModel();
#else
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();
	DECLARE_PYSERVERCLASS( CWarsWeapon );

	virtual bool			WeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );

	virtual bool			WeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, CBaseEntity *pTarget = NULL );	
#endif // CLIENT_DLL

	CHL2WarsPlayer*			GetCommander();

	virtual void			PrimaryAttack( void );
	void					GetShootOriginAndDirection( Vector &vShootOrigin, Vector &vShootDirection);
	virtual void			WeaponSound( WeaponSound_t sound_type, float soundtime = 0.0f );

	virtual void			MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );

	virtual void			Equip( CBaseCombatCharacter *pOwner );
	virtual bool			DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt );

	virtual void			SetWeaponVisible( bool visible );
	virtual bool			IsWeaponVisible( void );

	virtual void			SendViewModelAnim( int nSequence );
	virtual float			GetViewModelSequenceDuration();	// Return how long the current view model sequence is.
	virtual bool			IsViewModelSequenceFinished( void ); // Returns if the viewmodel's current animation is finished

	virtual void			Operator_FrameUpdate( CBaseCombatCharacter  *pOperator );

	virtual Activity		GetPrimaryAttackActivity( void );
	virtual Activity		GetSecondaryAttackActivity( void );
	virtual void			SetPrimaryAttackActivity( Activity act );
	virtual void			SetSecondaryAttackActivity( Activity act );

	virtual int				GetMinBurst();
	virtual int				GetMaxBurst();
	virtual float			GetMinRestTime();
	virtual float			GetMaxRestTime();

	virtual void			SetViewModel();

public:
#ifndef CLIENT_DLL
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iClip1 );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iClip2 );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iPrimaryAmmoType );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iSecondaryAmmoType );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_flNextPrimaryAttack );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_flNextSecondaryAttack );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_nNextThinkTick );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_flTimeWeaponIdle );
#else
	Vector m_vTracerColor;
#endif // CLIENT_DLL

	float m_fFireRate;
	float m_fOverrideAmmoDamage;
	float m_fMaxBulletRange;
	Vector m_vBulletSpread;

	int m_iMinBurst;
	int m_iMaxBurst;
	float m_fMinRestTime;
	float m_fMaxRestTime;
	bool m_bEnableBurst;
	int m_nBurstShotsRemaining;

private:
	Activity m_PrimaryAttackActivity;
	Activity m_SecondaryAttackActivity;

	float m_fFireTimeOut;
};

inline Activity CWarsWeapon::GetPrimaryAttackActivity( void )
{
	return m_PrimaryAttackActivity;
}

inline Activity CWarsWeapon::GetSecondaryAttackActivity( void )
{
	return m_SecondaryAttackActivity;
}

inline void CWarsWeapon::SetPrimaryAttackActivity( Activity act )
{
	m_PrimaryAttackActivity = act;
}

inline void CWarsWeapon::SetSecondaryAttackActivity( Activity act )
{
	m_SecondaryAttackActivity = act;
}

inline int CWarsWeapon::GetMinBurst()
{
	return m_iMinBurst;
}

inline int CWarsWeapon::GetMaxBurst()
{
	return m_iMaxBurst;
}

inline float CWarsWeapon::GetMinRestTime()
{
	return m_fMinRestTime;
}

inline float CWarsWeapon::GetMaxRestTime()
{
	return m_fMaxRestTime;
}

#endif // WARS_WEAPON_SHARED_H