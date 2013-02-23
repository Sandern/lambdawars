//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "wars_weapon.h"
#include "hl2wars_player.h"
#include "unit_base_shared.h"
#include "hl2wars_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar g_debug_wars_weapon("g_debug_wars_weapon", "0", FCVAR_CHEAT);

void* SendProxy_SendWarsLocalWeaponDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the weapon entity
	CWarsWeapon *pWeapon = (CWarsWeapon*)pVarData;
	if ( pWeapon )
	{
		// Only send this chunk of data to the commander of the marine carrying this weapon
		CHL2WarsPlayer *pPlayer = pWeapon->GetCommander();
		if ( pPlayer )
		{
			pRecipients->SetOnly( pPlayer->GetClientIndex() );
		}
		else
		{
			pRecipients->ClearAllRecipients();
		}
	}
	
	return (void*)pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendWarsLocalWeaponDataTable );


//-----------------------------------------------------------------------------
// Purpose: Only send to local player if this weapon is the active weapon and this marine is the active marine
// Input  : *pStruct - 
//			*pVarData - 
//			*pRecipients - 
//			objectID - 
// Output : void*
//-----------------------------------------------------------------------------
void* SendProxy_SendWarsActiveLocalWeaponDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the weapon entity
	CWarsWeapon *pWeapon = (CWarsWeapon*)pVarData;
	if ( pWeapon )
	{
		// Only send this chunk of data to the commander of the marine carrying this weapon
		CHL2WarsPlayer *pPlayer = pWeapon->GetCommander();
		if ( pPlayer && pPlayer->GetControlledUnit() == pWeapon->GetOwner() )
		{
			pRecipients->SetOnly( pPlayer->GetClientIndex() );
		}
		else
		{
			pRecipients->ClearAllRecipients();
		}
	}

	return (void*)pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendWarsActiveLocalWeaponDataTable );

LINK_ENTITY_TO_CLASS( wars_weapon, CWarsWeapon );

BEGIN_NETWORK_TABLE_NOBASE( CWarsWeapon, DT_WarsLocalWeaponData )
	SendPropIntWithMinusOneFlag( SENDINFO(m_iClip2 ), 8 ),
	SendPropInt( SENDINFO(m_iSecondaryAmmoType ), 8 ),
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE_NOBASE( CWarsWeapon, DT_WarsActiveLocalWeaponData )
	SendPropTime( SENDINFO( m_flNextPrimaryAttack ) ),
	SendPropTime( SENDINFO( m_flNextSecondaryAttack ) ),
	SendPropInt( SENDINFO( m_nNextThinkTick ) ),
	SendPropTime( SENDINFO( m_flTimeWeaponIdle ) ),
END_NETWORK_TABLE()

IMPLEMENT_SERVERCLASS_ST(CWarsWeapon, DT_WarsWeapon)
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),	
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	SendPropExclude( "DT_BaseCombatWeapon", "LocalWeaponData" ),
	SendPropExclude( "DT_BaseCombatWeapon", "LocalActiveWeaponData" ),

	SendPropDataTable("WarsLocalWeaponData", 0, &REFERENCE_SEND_TABLE(DT_WarsLocalWeaponData), SendProxy_SendWarsLocalWeaponDataTable ),
	SendPropDataTable("WarsActiveLocalWeaponData", 0, &REFERENCE_SEND_TABLE(DT_WarsActiveLocalWeaponData), SendProxy_SendWarsActiveLocalWeaponDataTable ),

	SendPropIntWithMinusOneFlag( SENDINFO(m_iClip1 ), 8 ),
	SendPropInt( SENDINFO(m_iPrimaryAmmoType ), 8 ),
END_SEND_TABLE()

// This class is exposed in python and networkable
IMPLEMENT_PYSERVERCLASS( CWarsWeapon, PN_WARSWEAPON );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CWarsWeapon )
END_DATADESC()

CWarsWeapon::CWarsWeapon()
{
	SetPredictionEligible(true);
	m_fFireTimeOut = 0.25f;
	m_fMaxBulletRange = MAX_TRACE_LENGTH;
	AddFOWFlags(FOWFLAG_HIDDEN|FOWFLAG_NOTRANSMIT);

	m_PrimaryAttackActivity = ACT_VM_PRIMARYATTACK;
	m_SecondaryAttackActivity = ACT_VM_SECONDARYATTACK;
}

//-----------------------------------------------------------------------------
// Purpose: Weapons ignore other weapons when LOS tracing
//-----------------------------------------------------------------------------
class CWarsWeaponLOSFilter : public CTraceFilterSkipTwoEntities
{
	DECLARE_CLASS( CWarsWeaponLOSFilter, CTraceFilterSkipTwoEntities );
public:
	CWarsWeaponLOSFilter( IHandleEntity *pHandleEntity, IHandleEntity *pHandleEntity2, int collisionGroup ) :
	  CTraceFilterSkipTwoEntities( pHandleEntity, pHandleEntity2, collisionGroup ), m_pVehicle( NULL )
	{
		// If the tracing entity is in a vehicle, then ignore it
		if ( pHandleEntity != NULL )
		{
			CBaseCombatCharacter *pBCC = ((CBaseEntity *)pHandleEntity)->MyCombatCharacterPointer();
			if ( pBCC != NULL )
			{
				m_pVehicle = pBCC->GetVehicleEntity();
			}
		}
	}
	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
		if ( !pEntity )
			return false;

		if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_WEAPON )
			return false;

		// Don't collide with the tracing entity's vehicle (if it exists)
		if ( pServerEntity == m_pVehicle )
			return false;

		/*if ( pEntity->GetHealth() > 0 )
		{
			CBreakable *pBreakable = dynamic_cast<CBreakable *>(pEntity);
			if ( pBreakable  && pBreakable->IsBreakable() && pBreakable->GetMaterialType() == matGlass)
			{
				return false;
			}
		}*/

		if( !BaseClass::ShouldHitEntity( pServerEntity, contentsMask ) )
			return false;

		if( !pEntity->BlocksLOS() )
			return false;

		return true;
	}

private:
	CBaseEntity *m_pVehicle;
};

bool CWarsWeapon::WeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions )
{
	// --------------------
	// Check for occlusion
	// --------------------
	CUnitBase *npcOwner = GetOwner()->MyUnitPointer();
	if( !npcOwner )
		return true;
	
	// Find its relative shoot position
	Vector vecRelativeShootPosition;
	VectorSubtract( npcOwner->Weapon_ShootPosition(), npcOwner->GetAbsOrigin(), vecRelativeShootPosition );
	Vector barrelPos = ownerPos + vecRelativeShootPosition;

	// FIXME: If we're in a vehicle, we need some sort of way to handle shooting out of them

	// Use the custom LOS trace filter
	CWarsWeaponLOSFilter traceFilter( npcOwner, npcOwner->GetEnemy(), COLLISION_GROUP_BREAKABLE_GLASS );
	trace_t tr;
	UTIL_TraceLine( barrelPos, targetPos, MASK_SHOT, &traceFilter, &tr );

	// See if we completed the trace without interruption
	if ( tr.fraction == 1.0 )
	{
		if ( g_debug_wars_weapon.GetBool() )
		{
			NDebugOverlay::Line( barrelPos, targetPos, 0, 255, 0, false, 1.0 );
		}

		return true;
	}

	CBaseEntity	*pHitEnt = tr.m_pEnt;

	/*
	CBasePlayer *pEnemyPlayer = ToBasePlayer( npcOwner->GetEnemy() );

	// is player in a vehicle? if so, verify vehicle is target and return if so (so npc shoots at vehicle)
	if ( pEnemyPlayer && pEnemyPlayer->IsInAVehicle() )
	{
		// Ok, player in vehicle, check if vehicle is target we're looking at, fire if it is
		// Also, check to see if the owner of the entity is the vehicle, in which case it's valid too.
		// This catches vehicles that use bone followers.
		CBaseEntity	*pVehicle  = pEnemyPlayer->GetVehicle()->GetVehicleEnt();
		if ( pHitEnt == pVehicle || pHitEnt->GetOwnerEntity() == pVehicle )
			return true;
	}*/

	// Hitting our enemy is a success case
	if ( pHitEnt == npcOwner->GetEnemy() )
	{
		if ( g_debug_wars_weapon.GetBool() )
		{
			NDebugOverlay::Line( barrelPos, targetPos, 0, 255, 0, false, 1.0 );
		}

		return true;
	}

	// If a vehicle is blocking the view, grab its driver and use that as the combat character
	
#if 0
	CBaseCombatCharacter *pBCC;
	/*IServerVehicle *pVehicle = pHitEnt->GetServerVehicle();
	if ( pVehicle )
	{
		pBCC = pVehicle->GetPassenger( );
	}
	else */
	{
		pBCC = ToBaseCombatCharacter( pHitEnt );
	}


	if ( pBCC && !pBCC->BlocksLOS() ) 
	{
		if ( npcOwner->IRelationType( pBCC ) == D_HT )
			return true;
	}
#endif // 0

	if ( g_debug_wars_weapon.GetBool() )
	{
		NDebugOverlay::Line( barrelPos, targetPos, 255, 0, 0, false, 1.0 );
	}

	return false;
}
