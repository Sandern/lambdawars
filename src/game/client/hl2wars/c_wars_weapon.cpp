//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_wars_weapon.h"
#include "prediction.h"
#include "unit_base_shared.h"
#include "c_hl2wars_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_NETWORK_TABLE_NOBASE( C_WarsWeapon, DT_WarsLocalWeaponData )
	RecvPropIntWithMinusOneFlag( RECVINFO(m_iClip2 )),
	RecvPropInt( RECVINFO(m_iSecondaryAmmoType )),
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE_NOBASE( C_WarsWeapon, DT_WarsActiveLocalWeaponData )
	//RecvPropTime( RECVINFO( m_flNextPrimaryAttack ) ),
	//RecvPropTime( RECVINFO( m_flNextSecondaryAttack ) ),
	//RecvPropInt( RECVINFO( m_nNextThinkTick ) ),
	//RecvPropTime( RECVINFO( m_flTimeWeaponIdle ) ),
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED( WarsWeapon, DT_WarsWeapon )

BEGIN_NETWORK_TABLE( CWarsWeapon, DT_WarsWeapon )
	RecvPropBool		( RECVINFO( m_bInReload ) ),
	RecvPropDataTable("WarsLocalWeaponData", 0, 0, &REFERENCE_RECV_TABLE(DT_WarsLocalWeaponData)),
	RecvPropDataTable("WarsActiveLocalWeaponData", 0, 0, &REFERENCE_RECV_TABLE(DT_WarsActiveLocalWeaponData)),
	RecvPropIntWithMinusOneFlag( RECVINFO(m_iClip1 )),
	RecvPropInt( RECVINFO(m_iPrimaryAmmoType )),
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( C_WarsWeapon )
	//DEFINE_PRED_FIELD( m_nNextThinkTick, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	//DEFINE_PRED_FIELD_TOL( m_flNextPrimaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
	//DEFINE_PRED_FIELD_TOL( m_flNextSecondaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
	DEFINE_PRED_FIELD( m_iPrimaryAmmoType, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iSecondaryAmmoType, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iClip1, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),			
	DEFINE_PRED_FIELD( m_iClip2, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),			

	DEFINE_PRED_FIELD( m_bInReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

	//DEFINE_PRED_FIELD( m_vecVelocity, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	//DEFINE_PRED_FIELD( m_flAnimTime, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),	
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),	
	DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nResetEventsParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
END_PREDICTION_DATA()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_WarsWeapon::C_WarsWeapon()
{
	SetPredictionEligible( true );

	m_fMaxBulletRange = MAX_TRACE_LENGTH;
	m_PrimaryAttackActivity = ACT_VM_PRIMARYATTACK;
	m_SecondaryAttackActivity = ACT_VM_SECONDARYATTACK;

	// Default is white (exact output depends on the used particle effect and material)
	m_vTracerColor = Vector( 1, 1, 1 ); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_WarsWeapon::OnDataChanged( DataUpdateType_t type )
{	
	bool bPredict = ShouldPredict();
	if (bPredict)
	{
		SetPredictionEligible( true );
	}
	else
	{
		SetPredictionEligible( false );
	}

	BaseClass::OnDataChanged( type );

	if ( GetPredictable() && !bPredict )
		ShutdownPredictable();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_WarsWeapon::ShouldPredict( void )
{
	C_BasePlayer *pOwner= ToBasePlayer( GetCommander() );
	if ( C_BasePlayer::GetLocalPlayer() == pOwner )
		return true;

	return BaseClass::ShouldPredict();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BasePlayer *C_WarsWeapon::GetPredictionOwner( void )
{
	return GetCommander();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_WarsWeapon::SetDormant( bool bDormant )
{
	// C_BaseCombatWeapon holsters, which is dumb shit.
	BASECOMBATWEAPON_DERIVED_FROM::SetDormant( bDormant );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWarsWeapon::ShouldDraw( void )
{
	if ( IsEffectActive( EF_NODRAW ) )
		return false;

	// If it's a player, then only show active weapons
	CUnitBase* pUnit = dynamic_cast<CUnitBase*>(GetOwner());
	if (pUnit)
	{
		if ( pUnit->IsEffectActive( EF_NODRAW ) )
			return false;
		if(  pUnit->GetCommander() && C_BasePlayer::GetLocalPlayer() == pUnit->GetCommander() )
		{
			if( !pUnit->GetCommander()->ShouldDrawLocalPlayer() )
				return false; // Viewmodel should take care of this
		}

		// NOTE: Active stuff is updated in UpdateClientData, could consider adding it for units.
		//bool bIsActive = ( m_iState == WEAPON_IS_ACTIVE );
		//if( !bIsActive )
		//	return false;

		return (pUnit->GetActiveWeapon() == this);
	}
	if (!BaseClass::ShouldDraw())
	{
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Check for commander
//-----------------------------------------------------------------------------
void CWarsWeapon::EnsureCorrectRenderingModel()
{
	C_BasePlayer *localplayer = C_BasePlayer::GetLocalPlayer();
	if ( localplayer && 
		localplayer == GetCommander() &&
		!localplayer->ShouldDrawLocalPlayer() )
	{
		return;
	}

	BaseClass::EnsureCorrectRenderingModel();
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this client's carrying this weapon
//-----------------------------------------------------------------------------
bool CWarsWeapon::IsCarriedByLocalPlayer( void )
{
	if ( !GetOwner() )
		return false;

	CUnitBase* pUnit = GetOwner()->MyUnitPointer();
	if ( !pUnit || !pUnit->GetCommander() )
		return false;

	return ( C_BasePlayer::GetLocalPlayer() == pUnit->GetCommander() );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this weapon is the local client's currently wielded weapon
//-----------------------------------------------------------------------------
bool CWarsWeapon::IsActiveByLocalPlayer( void )
{
	if ( IsCarriedByLocalPlayer() )
	{
		return true;
		//return (m_iState == WEAPON_IS_ACTIVE);
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWarsWeapon::GetShootPosition( Vector &vOrigin, QAngle &vAngles )
{
	// Get the entity because the weapon doesn't have the right angles.
	C_BasePlayer *player = NULL;
	C_BaseCombatCharacter *pEnt = ToBaseCombatCharacter( GetOwner() );
	CUnitBase* pUnit = GetOwner()->MyUnitPointer();
	if ( pEnt )
	{
		if ( pEnt == C_BasePlayer::GetLocalPlayer() )
		{
			vAngles = pEnt->EyeAngles();
		}
		else
		{
			vAngles = pEnt->GetRenderAngles();	
		}

		if( pEnt->IsPlayer() )
			player = ToBasePlayer( pEnt );
		else if( pUnit && pUnit->GetCommander() )
			player = pUnit->GetCommander();
	}
	else
	{
		vAngles.Init();
	}

	QAngle vDummy;
	if ( IsActiveByLocalPlayer() && player && !player->ShouldDrawLocalPlayer() )
	{
		C_BaseViewModel *vm = player ? player->GetViewModel( 0 ) : NULL;
		if ( vm )
		{
			int iAttachment = vm->LookupAttachment( "muzzle" );
			if ( vm->GetAttachment( iAttachment, vOrigin, vDummy ) )
			{
				return true;
			}
		}
	}
	else
	{
		// Thirdperson
		int iAttachment = LookupAttachment( "muzzle" );
		if ( GetAttachment( iAttachment, vOrigin, vDummy ) )
		{
			return true;
		}
	}

	vOrigin = GetRenderOrigin();
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity * CWarsWeapon::GetMuzzleAttachEntity()
{
	if( !GetOwner() )
		return this;

	CUnitBase* pUnit = GetOwner()->MyUnitPointer();
	C_BasePlayer *player = NULL;

	if( GetOwner()->IsPlayer() )
		player = ToBasePlayer( GetOwner() );
	else if( pUnit && pUnit->GetCommander() )
		player = pUnit->GetCommander();

	if ( player && IsActiveByLocalPlayer() && player && !player->ShouldDrawLocalPlayer() )
	{
		C_BaseViewModel *vm = player ? player->GetViewModel( 0 ) : NULL;
		if ( vm )
		{
			return vm;
		}
	} 

	return this;
}