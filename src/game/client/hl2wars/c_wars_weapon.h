#ifndef _INCLUDED_C_WARSWEAPON_H
#define _INCLUDED_C_WARSWEAPON_H
#ifdef _WIN32
#pragma once
#endif

#include "wars_weapon_shared.h"
#include "basecombatweapon_shared.h"

class C_HL2WarsPlayer;

class C_WarsWeapon : public C_BaseCombatWeapon
{
	DECLARE_CLASS( C_WarsWeapon, C_BaseCombatWeapon );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_PYCLIENTCLASS( C_WarsWeapon );

	C_WarsWeapon();

	virtual bool IsPredicted( void ) const;
	virtual void OnDataChanged( DataUpdateType_t type );
	virtual bool ShouldPredict( void );
	virtual C_BasePlayer* GetPredictionOwner( void );

	C_HL2WarsPlayer* GetCommander();

	virtual void			PrimaryAttack( void );
	void					GetShootOriginAndDirection( Vector &vShootOrigin, Vector &vShootDirection);
	virtual void			WeaponSound( WeaponSound_t sound_type, float soundtime = 0.0f );
	virtual bool			GetShootPosition( Vector &vOrigin, QAngle &vAngles );
	CBaseEntity *			GetMuzzleAttachEntity();

	virtual void			MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );

	virtual void			Equip( CBaseCombatCharacter *pOwner );
	virtual bool			DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt );

	virtual void			SetDormant( bool bDormant );
	virtual bool			ShouldDraw( void );

	virtual bool			IsCarriedByLocalPlayer( void );
	virtual bool			IsActiveByLocalPlayer( void );

	virtual void			SetWeaponVisible( bool visible );
	virtual bool			IsWeaponVisible( void );

	virtual void			SendViewModelAnim( int nSequence );
	virtual float			GetViewModelSequenceDuration();	// Return how long the current view model sequence is.
	virtual bool			IsViewModelSequenceFinished( void ); // Returns if the viewmodel's current animation is finished

	virtual void			EnsureCorrectRenderingModel();

	virtual void			Operator_FrameUpdate( CBaseCombatCharacter  *pOperator );

	virtual Activity		GetPrimaryAttackActivity( void );
	virtual Activity		GetSecondaryAttackActivity( void );
	virtual void			SetPrimaryAttackActivity( Activity act );
	virtual void			SetSecondaryAttackActivity( Activity act );

	virtual void SetViewModel();

public:
	//void GetShootOriginAndDirection( Vector &vecShootOrigin, Vector &vecShootDirection );

public:
	float m_fFireRate;
	Vector m_vBulletSpread;

private:
	float m_fFireTimeOut;
	Activity m_PrimaryAttackActivity;
	Activity m_SecondaryAttackActivity;

private:	
	C_WarsWeapon( const C_WarsWeapon & ); // not defined, not accessible
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

#endif // _INCLUDED_C_WARSWEAPON_H