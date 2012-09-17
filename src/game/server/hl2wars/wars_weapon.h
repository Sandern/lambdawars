//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef _INCLUDED_WARSWEAPON_H
#define _INCLUDED_WARSWEAPON_H
#ifdef _WIN32
#pragma once
#endif

#include "wars_weapon_shared.h"
#include "basecombatweapon_shared.h"

class CHL2WarsPlayer;

class CWarsWeapon : public CBaseCombatWeapon
{
public:
	DECLARE_CLASS( CWarsWeapon, CBaseCombatWeapon );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();
	DECLARE_PYSERVERCLASS( CWarsWeapon );

	CWarsWeapon();

	// Temp
	//int	UpdateTransmitState()
	//{	
	//	return SetTransmitState( FL_EDICT_ALWAYS );
	//}

	virtual bool			IsPredicted( void ) const;

	CHL2WarsPlayer*			GetCommander();

	virtual void			PrimaryAttack( void );
	void					GetShootOriginAndDirection( Vector &vShootOrigin, Vector &vShootDirection);
	virtual void			WeaponSound( WeaponSound_t sound_type, float soundtime = 0.0f );
	virtual bool			WeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );	

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

	virtual void SetViewModel();

public:
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iClip1 );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iClip2 );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iPrimaryAmmoType );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iSecondaryAmmoType );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_flNextPrimaryAttack );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_flNextSecondaryAttack );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_nNextThinkTick );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_flTimeWeaponIdle );

	float m_fFireRate;
	Vector m_vBulletSpread;
	Activity m_PrimaryAttackActivity;
	Activity m_SecondaryAttackActivity;

private:
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


#endif // _INCLUDED_WARSWEAPON_H