//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "wars_weapon_shared.h"
#include "unit_base_shared.h"

#ifdef CLIENT_DLL
	#include "c_wars_weapon.h"
	#include "c_hl2wars_player.h"
	#include "iclientmode.h"
	#include "c_te_effect_dispatch.h"
	#include "c_wars_fx.h"
#else
	#include "wars_weapon.h"
	#include "hl2wars_player.h"
	#include "te_effect_dispatch.h"
#endif // CLIENT_DLL

#include "vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool CWarsWeapon::IsPredicted(void) const
{
	return true;
}

// get the player owner of this weapon (either the unit's commander if the weapon is
//  being held by unit, or the player directly if a player is holding this weapon)
CHL2WarsPlayer* CWarsWeapon::GetCommander()
{
	CHL2WarsPlayer *pOwner = NULL;
	CUnitBase *pUnit = NULL;

	pUnit = dynamic_cast<CUnitBase*>( GetOwner() );
	if ( pUnit )
	{
		pOwner = pUnit->GetCommander();
	}
	else
	{
		pOwner = ToHL2WarsPlayer( dynamic_cast<CBasePlayer*>( GetOwner() ) );
	}

	return pOwner;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsWeapon::PrimaryAttack( void )
{
	VPROF_BUDGET( "CWarsWeapon::PrimaryAttack", VPROF_BUDGETGROUP_UNITS );

	CUnitBase *pOwner = GetOwner()  ? GetOwner()->MyUnitPointer() : NULL;
	if( !pOwner )
		return;

#ifndef CLIENT_DLL
	if( m_bEnableBurst )
	{
		// Auto reset burst shots next time we fire
		if( m_nBurstShotsRemaining <= 0 )
		{
			m_nBurstShotsRemaining = GetRandomBurst();
		}
	}
#endif // CLIENT_DLL

	pOwner->DoMuzzleFlash();
	
	SendWeaponAnim( GetPrimaryAttackActivity() );

	Vector vecShootOrigin, vecShootDir;
	GetShootOriginAndDirection(vecShootOrigin, vecShootDir);

    int shots = 0;

	// Assume still firing if gpGlobals.curtime-nextprimaryattack falss within this range
	// In the other case reset nextprimaryattack, so we only fire one shot
	if( (gpGlobals->curtime - m_flNextPrimaryAttack) > m_fFireTimeOut )
		m_flNextPrimaryAttack = gpGlobals->curtime;

	//WeaponSound(SINGLE, m_flNextPrimaryAttack);
	while( m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
#ifdef CLIENT_DLL
		// MUST call sound before removing a round from the clip of a CMachineGun
		WeaponSound(SINGLE_NPC, m_flNextPrimaryAttack);
#endif // CLIENT_DLL
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + m_fFireRate;
		shots += 1;
		if( !m_fFireRate )
			break;
	}
    
	// Fill in bullets info
	FireBulletsInfo_t info;
	info.m_vecSrc = vecShootOrigin;
	info.m_vecDirShooting = vecShootDir;
	info.m_iShots = shots;
	info.m_flDistance = m_fMaxBulletRange;
	info.m_iAmmoType = GetPrimaryAmmoType();
	info.m_iTracerFreq = 2;
	info.m_vecSpread = m_vBulletSpread;
	info.m_flDamage = m_fOverrideAmmoDamage;

	pOwner->FireBullets( info );

#ifndef CLIENT_DLL
	if( m_bEnableBurst )
	{
		m_nBurstShotsRemaining -= shots;
		// Dispatch burst finished event if we have no shots left
		// It's up to the AI to rest
		if( m_nBurstShotsRemaining <= 0 )
		{
			pOwner->DispatchBurstFinished();
		}
	}
#endif // CLIENT_DLL

	// Add our view kick in
	AddViewKick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsWeapon::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
#if 0
	const char* tracer = "ASWUTracer";
	//if (GetActiveASWWeapon())
	//	tracer = GetActiveASWWeapon()->GetUTracerType();
#ifdef CLIENT_DLL
	CEffectData data;
	data.m_vOrigin = tr.endpos;
	data.m_hEntity = this;
	//data.m_nMaterial = m_iDamageAttributeEffects;

	DispatchEffect( tracer, data );
#else
	CRecipientFilter filter;
	filter.AddAllPlayers();
	if (gpGlobals->maxClients > 1 && GetCommander())
	{ 
		filter.RemoveRecipient(GetCommander());
	}

	UserMessageBegin( filter, tracer );
		WRITE_SHORT( entindex() );
		WRITE_FLOAT( tr.endpos.x );
		WRITE_FLOAT( tr.endpos.y );
		WRITE_FLOAT( tr.endpos.z );
		//WRITE_SHORT( m_iDamageAttributeEffects );
	MessageEnd();
#endif
#else
#ifdef CLIENT_DLL
	WarsUTracer( GetOwner(), tr.endpos, m_vTracerColor );
#endif
#endif // 0
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsWeapon::GetShootOriginAndDirection( Vector &vShootOrigin, Vector &vShootDirection)
{
	CBaseEntity *pOwner = GetOwner();
	if( !pOwner )
		return;

	CUnitBase *pUnit = pOwner->MyUnitPointer();
	if( !pUnit )
		return;

#if 0
#ifdef CLIENT_DLL
	if( pUnit->GetCommander() && !ShouldDrawLocalPlayer() )
	{
		QAngle vDummy;
		C_BasePlayer *player = ToBasePlayer( pUnit->GetCommander() );
		C_BaseViewModel *vm = player ? player->GetViewModel( 0 ) : NULL;
		if ( vm )
		{
			int iAttachment = vm->LookupAttachment( "muzzle" );
			if ( vm->GetAttachment( iAttachment, vShootOrigin, vDummy ) )
			{
				AngleVectors( QAngle(pUnit->m_fEyePitch, pUnit->m_fEyeYaw, 0.0), &vShootDirection );
				return;
			}
		}
	}
#endif // CLIENT_DLL
#endif // 0
	vShootOrigin = pUnit->Weapon_ShootPosition();
	AngleVectors( QAngle(pUnit->m_fEyePitch, pUnit->m_fEyeYaw, 0.0), &vShootDirection );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsWeapon::WeaponSound( WeaponSound_t sound_type, float soundtime /* = 0.0f */ )
{
#if 0
	// Limit amount of sounds
	// Make better fix?
	// Units spam too many sounds...
	static float fNextResetTime = -1;
	static int nSounds = 0;
	if( fNextResetTime < gpGlobals->curtime )
	{
		fNextResetTime = gpGlobals->curtime + 0.1f;
		nSounds = 0;
	}

	if( nSounds >= 50 ) {
		return;
	}

	nSounds += 1;
#endif // 0
	BaseClass::WeaponSound( sound_type, soundtime );
}

void CWarsWeapon::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );

	if ( GetCommander() )
	{
		SetModel( GetViewModel() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWarsWeapon::DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt )
{
	bool ret = BaseClass::DefaultDeploy( szViewModel, szWeaponModel, iActivity, szAnimExt );

	CBasePlayer *pOwner = GetCommander();
	if ( pOwner )
	{
		// Dead men deploy no weapons
		if ( pOwner->IsAlive() == false )
			return false;

		pOwner->SetAnimationExtension( szAnimExt );

		SetViewModel();
		SendWeaponAnim( iActivity );

		pOwner->SetNextAttack( gpGlobals->curtime + SequenceDuration() );
	}
	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iActivity - 
//-----------------------------------------------------------------------------
void CWarsWeapon::SendViewModelAnim( int nSequence )
{
#if defined( CLIENT_DLL )
	if ( !IsPredicted() )
	{
		return;
	}
#endif
	
	if ( nSequence < 0 )
		return;

	CBasePlayer *pOwner = ToBasePlayer( GetCommander() );
	if ( pOwner == NULL ) {
		BaseClass::SendViewModelAnim( nSequence );
		return;
	}
	
	CBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex );
	if ( vm == NULL )
	{
		return;
	}

	SetViewModel();
	Assert( vm->ViewModelIndex() == m_nViewModelIndex );
	vm->SendViewModelMatchingSequence( nSequence );
}

float CWarsWeapon::GetViewModelSequenceDuration()
{
	CBasePlayer *pOwner = ToBasePlayer( GetCommander() );
	if ( pOwner == NULL )
	{
		return BaseClass::GetViewModelSequenceDuration( );
	}
	
	CBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex );
	if ( vm == NULL )
	{
		Assert( false );
		return 0;
	}

	SetViewModel();
	Assert( vm->ViewModelIndex() == m_nViewModelIndex );
	return vm->SequenceDuration();
}

bool CWarsWeapon::IsViewModelSequenceFinished( void )
{
	// These are not valid activities and always complete immediately
	if ( GetActivity() == ACT_RESET || GetActivity() == ACT_INVALID )
		return true;

	CBasePlayer *pOwner = ToBasePlayer( GetCommander() );
	if ( pOwner == NULL )
	{
		return BaseClass::IsViewModelSequenceFinished( );
	}
	
	CBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex );
	if ( vm == NULL )
	{
		Assert( false );
		return false;
	}

	return vm->IsSequenceFinished();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsWeapon::SetViewModel()
{
	CUnitBase *pUnit;
	CHL2WarsPlayer *pOwner;

	pUnit = GetOwner()->MyUnitPointer();
	if ( pUnit )
	{
		pOwner = pUnit->GetCommander();
		if( pOwner )
		{
			CBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex );
			if ( vm == NULL )
				return;
			Assert( vm->ViewModelIndex() == m_nViewModelIndex );
			vm->SetWeaponModel( GetViewModel( m_nViewModelIndex ), this );
		}
	}
	BaseClass::SetViewModel();
}

//-----------------------------------------------------------------------------
// Purpose: Show/hide weapon and corresponding view model if any
// Input  : visible - 
//-----------------------------------------------------------------------------
void CWarsWeapon::SetWeaponVisible( bool visible )
{
	CBaseViewModel *vm = NULL;
	CUnitBase *pUnit;

	if( GetOwner() )
	{
		pUnit = GetOwner()->MyUnitPointer();
		if( pUnit && pUnit->GetCommander() )
		{
			CBasePlayer *pOwner = ToBasePlayer( pUnit->GetCommander() );
			if ( pOwner )
			{
				vm = pOwner->GetViewModel( m_nViewModelIndex );
			}
		}
	}

	if ( vm )
	{
		if ( visible )
		{
			vm->RemoveEffects( EF_NODRAW );
		}
		else
		{
			vm->AddEffects( EF_NODRAW );
		}
	}
	BaseClass::SetWeaponVisible( visible );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWarsWeapon::IsWeaponVisible( void )
{
	CBaseViewModel *vm = NULL;

	CUnitBase *pUnit;

	CBasePlayer *pOwner = NULL;
	if( GetOwner() )
	{
		pUnit = GetOwner()->MyUnitPointer();
		if( pUnit && pUnit->GetCommander() )
		{
			pOwner = ToBasePlayer( pUnit->GetCommander() );
		}
	}

	if ( pOwner )
	{
		vm = pOwner->GetViewModel( m_nViewModelIndex );
		if ( vm )
			return ( !vm->IsEffectActive(EF_NODRAW) );
	}

	return BaseClass::IsWeaponVisible();
}


void CWarsWeapon::Operator_FrameUpdate( CBaseCombatCharacter  *pOperator )
{
	BaseClass::Operator_FrameUpdate( pOperator );


	if( GetCommander() )
	{
		CBaseViewModel *vm =  GetCommander()->GetViewModel( m_nViewModelIndex );
		
		if ( vm != NULL )
		{
#ifndef CLIENT_DLL
			vm->StudioFrameAdvance();
			vm->DispatchAnimEvents( this );
#else
			//if( vm->ShouldPredict() )
			//	vm->StudioFrameAdvance();
#endif // CLIENT_DLL
		}
	}

}