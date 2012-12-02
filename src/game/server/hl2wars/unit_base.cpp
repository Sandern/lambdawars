//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_base.h"
#include "unit_base_shared.h"
#include "unit_sense.h"
#include "hl2wars_shareddefs.h"
#include "basegrenade_shared.h"
#include "unit_navigator.h"
#include "hl2wars_player.h"
#include "animation.h"

#ifdef HL2WARS_ASW_DLL
	#include "sendprop_priorities.h"
#endif // HL2WARS_ASW_DLL

#ifndef DISABLE_PYTHON
	#include "src_python.h"
#endif // DISABLE_PYTHON

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Grenade tossing
//-----------------------------------------------------------------------------
//
//	FIXME: Create this in a better fashion!
//
ConVar g_debug_checkthrowtolerance( "g_debug_checkthrowtolerance", "0" );
extern ConVar sv_gravity;
Vector VecCheckThrowTolerance( CBaseEntity *pEdict, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flTolerance, int iCollisionGroup )
{
	flSpeed = MAX( 1.0f, flSpeed );

	float flGravity = sv_gravity.GetFloat();

	Vector vecGrenadeVel = (vecSpot2 - vecSpot1);

	// throw at a constant time
	float time = vecGrenadeVel.Length( ) / flSpeed;
	vecGrenadeVel = vecGrenadeVel * (1.0 / time);

	// adjust upward toss to compensate for gravity loss
	vecGrenadeVel.z += flGravity * time * 0.5;

	Vector vecApex = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
	vecApex.z += 0.5 * flGravity * (time * 0.5) * (time * 0.5);


	trace_t tr;
	UTIL_TraceLine( vecSpot1, vecApex, MASK_SOLID, pEdict, iCollisionGroup, &tr );
	if (tr.fraction != 1.0)
	{
		// fail!
		if ( g_debug_checkthrowtolerance.GetBool() )
		{
			NDebugOverlay::Line( vecSpot1, vecApex, 255, 0, 0, true, 5.0 );
		}

		return vec3_origin;
	}

	if ( g_debug_checkthrowtolerance.GetBool() )
	{
		NDebugOverlay::Line( vecSpot1, vecApex, 0, 255, 0, true, 5.0 );
	}

	UTIL_TraceLine( vecApex, vecSpot2, MASK_SOLID_BRUSHONLY, pEdict, iCollisionGroup, &tr );
	if ( tr.fraction != 1.0 )
	{
		bool bFail = true;

		// Didn't make it all the way there, but check if we're within our tolerance range
		if ( flTolerance > 0.0f )
		{
			float flNearness = ( tr.endpos - vecSpot2 ).LengthSqr();
			if ( flNearness < Square( flTolerance ) )
			{
				if ( g_debug_checkthrowtolerance.GetBool() )
				{
					NDebugOverlay::Sphere( tr.endpos, vec3_angle, flTolerance, 0, 255, 0, 0, true, 5.0 );
				}

				bFail = false;
			}
		}
		
		if ( bFail )
		{
			if ( g_debug_checkthrowtolerance.GetBool() )
			{
				NDebugOverlay::Line( vecApex, vecSpot2, 255, 0, 0, true, 5.0 );
				NDebugOverlay::Sphere( tr.endpos, vec3_angle, flTolerance, 255, 0, 0, 0, true, 5.0 );
			}
			return vec3_origin;
		}
	}

	if ( g_debug_checkthrowtolerance.GetBool() )
	{
		NDebugOverlay::Line( vecApex, vecSpot2, 0, 255, 0, true, 5.0 );
	}

	return vecGrenadeVel;
}

TossGrenadeAnimEventHandler::TossGrenadeAnimEventHandler(const char *pEntityName, float fSpeed)
{
	Q_strncpy(m_EntityName, pEntityName, 40);
	m_fSpeed = fSpeed;
}

bool TossGrenadeAnimEventHandler::GetTossVector(CUnitBase *pUnit, const Vector &vecStartPos, const Vector &vecTarget, int iCollisionGroup, Vector *vecOut )
{
	if( !pUnit || !vecOut )
		return false;

	// Try the most direct route
	Vector vecToss = VecCheckThrowTolerance( pUnit, vecStartPos, vecTarget, m_fSpeed, (10.0f*12.0f), iCollisionGroup );

	// If this failed then try a little faster (flattens the arc)
	if ( vecToss == vec3_origin )
	{
		vecToss = VecCheckThrowTolerance( pUnit, vecStartPos, vecTarget, m_fSpeed * 1.5f, (10.0f*12.0f), iCollisionGroup );
		if ( vecToss == vec3_origin )
			return false;
	}

	// Save out the result
	if ( vecOut )
	{
		*vecOut = vecToss;
	}

	return true;
}

CBaseEntity *TossGrenadeAnimEventHandler::TossGrenade(CUnitBase *pUnit, Vector &vecStartPos, Vector &vecTarget, int iCollisionGroup)
{
	if( !pUnit )
		return NULL;

	// Try and spit at our target
	Vector	vecToss;				
	if ( GetTossVector( pUnit, vecStartPos, vecTarget, pUnit->CalculateIgnoreOwnerCollisionGroup(), &vecToss ) == false )
	{
		return NULL;
	}

	// Find what our vertical theta is to estimate the time we'll impact the ground
	///Vector vecToTarget = ( vTarget - vSpitPos );
	//VectorNormalize( vecToTarget );
	float flVelocity = VectorNormalize( vecToss );
	//float flCosTheta = DotProduct( vecToTarget, vecToss );
	//float flTime = (vSpitPos-vTarget).Length2D() / ( flVelocity * flCosTheta );

	// Emit a sound where this is going to hit so that targets get a chance to act correctly
	//CSoundEnt::InsertSound( SOUND_DANGER, vTarget, (15*12), flTime, this );

	CBaseGrenade *pGrenade = dynamic_cast<CBaseGrenade *>(CreateEntityByName( m_EntityName ));
	if( !pGrenade )
		return NULL;
	pGrenade->SetAbsOrigin( vecStartPos );
	pGrenade->SetAbsAngles( vec3_angle );
	pGrenade->SetOwnerNumber( pUnit->GetOwnerNumber() );
    pGrenade->AddFOWFlags(FOWFLAG_HIDDEN|FOWFLAG_NOTRANSMIT);
	DispatchSpawn( pGrenade );
	pGrenade->SetThrower( pUnit );
	pGrenade->SetOwnerEntity( pUnit );
						
	pGrenade->SetAbsVelocity( vecToss * flVelocity );

	// Tumble through the air
	pGrenade->SetLocalAngularVelocity(
		QAngle( random->RandomFloat( -250, -500 ),
				random->RandomFloat( -250, -500 ),
				random->RandomFloat( -250, -500 ) ) );
	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
EmitSoundAnimEventHandler::EmitSoundAnimEventHandler( const char *soundscript )
{
	Q_strncpy(m_SoundScript, soundscript, 256);
}

void EmitSoundAnimEventHandler::HandleEvent(CUnitBase *pUnit, animevent_t *event)
{
	if( !pUnit )
		return;
	pUnit->EmitSound(m_SoundScript, event->eventtime);   
}

#ifndef DISABLE_PYTHON
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
AnimEventMap::AnimEventMap()
{
	SetDefLessFunc( m_AnimEventMap );
}	

AnimEventMap::AnimEventMap( AnimEventMap &animeventmap )
{
	SetDefLessFunc( m_AnimEventMap );

	FOR_EACH_MAP_FAST(animeventmap.m_AnimEventMap, idx)
	{
		m_AnimEventMap.Insert(animeventmap.m_AnimEventMap.Key(idx), animeventmap.m_AnimEventMap.Element(idx));
	}
}	

AnimEventMap::AnimEventMap( AnimEventMap &animeventmap, boost::python::dict d )
{
	SetDefLessFunc( m_AnimEventMap );

	FOR_EACH_MAP_FAST(animeventmap.m_AnimEventMap, idx)
	{
		m_AnimEventMap.Insert(animeventmap.m_AnimEventMap.Key(idx), animeventmap.m_AnimEventMap.Element(idx));
	}

	AddAnimEventHandlers(d);
}

AnimEventMap::AnimEventMap( boost::python::dict d )
{
	SetDefLessFunc( m_AnimEventMap );

	AddAnimEventHandlers(d);
}

void AnimEventMap::AddAnimEventHandlers( boost::python::dict d )
{
	try 
	{
		int event;
		bp::object objectKey, objectValue;
		const bp::object objectKeys = d.iterkeys();
		const bp::object objectValues = d.itervalues();
		unsigned long ulCount = bp::len(d); 
		for( unsigned long u = 0; u < ulCount; u++ )
		{
			objectKey = objectKeys.attr( "next" )();
			objectValue = objectValues.attr( "next" )();

			try 
			{
				event = bp::extract<int>(objectKey);
			}
			catch( bp::error_already_set & )
			{
				PyErr_Print();
				continue;
			}

			SetAnimEventHandler(event, objectValue);
		}
	}
	catch( bp::error_already_set & )
	{
		PyErr_Print();
	}
}

void AnimEventMap::SetAnimEventHandler(int event, boost::python::object pyhandler)
{
	animeventhandler_t handler;
	try 
	{
		handler.m_pHandler = boost::python::extract<BaseAnimEventHandler *>(pyhandler);
	}
	catch( bp::error_already_set & )
	{
		PyErr_Clear();
		// Not an handler object. Assume unbound method that handles the event.
		handler.m_pHandler = NULL;
	}
	handler.m_pyInstance = pyhandler;
	m_AnimEventMap.Insert(event, handler);
}
#endif // DISABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose:  Send proxies
//-----------------------------------------------------------------------------
void* SendProxy_SendCommanderDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the weapon entity
	CUnitBase *pUnit = (CUnitBase*)pVarData;
	if ( pUnit )
	{
		// Only send this chunk of data to the commander of the unit
		CHL2WarsPlayer *pPlayer = pUnit->GetCommander();
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
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendCommanderDataTable );

void* SendProxy_SendNormalDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the unit entity
	CUnitBase *pUnit = (CUnitBase*)pVarData;
	if ( pUnit )
	{
		// Send this table to all players except the commander
		CHL2WarsPlayer *pPlayer = pUnit->GetCommander();
		if ( pPlayer )
		{
			pRecipients->ClearRecipient( pPlayer->GetClientIndex() );
		}
	}

	return (void*)pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendNormalDataTable );

//-----------------------------------------------------------------------------
// Purpose:  Send tables
//-----------------------------------------------------------------------------
void SendProxy_Origin( const SendProp *, const void *, const void *, DVariant *, int, int );

BEGIN_SEND_TABLE_NOBASE( CUnitBase, DT_CommanderExclusive )
	// Send high res version for the commander
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),

#ifdef HL2WARS_ASW_DLL
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 0), 13, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesX ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 1), 13, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesY ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 2), 13, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesZ ),

	// This data is only send to the commander for prediction
	SendPropEHandle		( SENDINFO( m_hGroundEntity ), SPROP_CHANGES_OFTEN ),
	//SendPropVector		( SENDINFO( m_vecBaseVelocity ), 20, 0, -1000, 1000 ),
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecVelocity, 0), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecVelocity, 1), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecVelocity, 2), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),

	// Update SetDefaultEyeOffset when changing the range!
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecViewOffset, 0), 10, SPROP_ROUNDDOWN, -256.0, 256.0f),
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecViewOffset, 1), 10, SPROP_ROUNDDOWN, -256.0, 256.0f),
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecViewOffset, 2), 12, SPROP_CHANGES_OFTEN,	-1.0f, 1024.0f),
#else
	SendPropAngle( SENDINFO_VECTORELEM2(m_angRotation, 0, x), 13, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesX ),
	SendPropAngle( SENDINFO_VECTORELEM2(m_angRotation, 1, y), 13, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesY ),
	SendPropAngle( SENDINFO_VECTORELEM2(m_angRotation, 2, z), 13, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesZ ),

	// This data is only send to the commander for prediction
	SendPropEHandle		( SENDINFO( m_hGroundEntity ), SPROP_CHANGES_OFTEN ),
	SendPropVector		( SENDINFO( m_vecBaseVelocity ), 20, 0, -1000, 1000 ),
	SendPropFloat		( SENDINFO_VECTORELEM2(m_vecVelocity, 0, x), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),
	SendPropFloat		( SENDINFO_VECTORELEM2(m_vecVelocity, 1, y), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),
	SendPropFloat		( SENDINFO_VECTORELEM2(m_vecVelocity, 2, z), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),

	// Update SetDefaultEyeOffset when changing the range!
	SendPropFloat		( SENDINFO_VECTORELEM2(m_vecViewOffset, 0, x), 10, SPROP_ROUNDDOWN, -256.0, 256.0f),
	SendPropFloat		( SENDINFO_VECTORELEM2(m_vecViewOffset, 1, y), 10, SPROP_ROUNDDOWN, -256.0, 256.0f),
	SendPropFloat		( SENDINFO_VECTORELEM2(m_vecViewOffset, 2, z), 12, SPROP_CHANGES_OFTEN,	-1.0f, 1024.0f),
#endif // HL2WARS_ASW_DLL
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CUnitBase, DT_NormalExclusive )
#ifdef HL2WARS_ASW_DLL
	SendPropVectorXY( SENDINFO( m_vecOrigin ), 				 CELL_BASEENTITY_ORIGIN_CELL_BITS, SPROP_CELL_COORD_LOWPRECISION | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, CBaseEntity::SendProxy_CellOriginXY, SENDPROP_NONLOCALPLAYER_ORIGINXY_PRIORITY ),
	SendPropFloat   ( SENDINFO_VECTORELEM( m_vecOrigin, 2 ), CELL_BASEENTITY_ORIGIN_CELL_BITS, SPROP_CELL_COORD_LOWPRECISION | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, CBaseEntity::SendProxy_CellOriginZ, SENDPROP_NONLOCALPLAYER_ORIGINZ_PRIORITY ),

	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 0), 10, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesX ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 1), 10, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesY ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 2), 10, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesZ ),
#else
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),

	SendPropAngle( SENDINFO_VECTORELEM2(m_angRotation, 0, x), 9, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesX ),
	SendPropAngle( SENDINFO_VECTORELEM2(m_angRotation, 1, y), 9, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesY ),
	SendPropAngle( SENDINFO_VECTORELEM2(m_angRotation, 2, z), 9, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesZ ),
#endif // HL2WARS_ASW_DLL
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST(CUnitBase, DT_UnitBase)
	SendPropString( SENDINFO( m_NetworkedUnitType ) ),

	SendPropInt		(SENDINFO(m_iHealth), 15, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_iMaxHealth), 15, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO( m_takedamage ), 3, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO( m_lifeState ), 3, SPROP_UNSIGNED ),

	// Send the same flags as done for the players. These flags are used to execute the animstate on the client
	SendPropInt		(SENDINFO(m_fFlags), PLAYER_FLAG_BITS, SPROP_UNSIGNED|SPROP_CHANGES_OFTEN ),	

	SendPropEHandle		( SENDINFO( m_hSquadUnit ) ),
	SendPropEHandle		( SENDINFO( m_hCommander ) ),
	SendPropEHandle		( SENDINFO( m_hEnemy ) ),

	SendPropBool( SENDINFO( m_bCrouching ) ),
	SendPropBool( SENDINFO( m_bClimbing ) ),

	SendPropInt		(SENDINFO(m_iEnergy), 15, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_iMaxEnergy), 15, SPROP_UNSIGNED ),

	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),

	// Excludes
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
	
	// playeranimstate and clientside animation takes care of these on the client
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	// Something with the legs ?
	SendPropExclude( "DT_BaseFlex" , "m_vecLean" ),
	SendPropExclude( "DT_BaseFlex" , "m_vecShift" ),

	// Data that only gets sent to the player controlling this unit
	SendPropDataTable( "commanderdata", 0, &REFERENCE_SEND_TABLE(DT_CommanderExclusive), SendProxy_SendCommanderDataTable ),
	// Data that gets sent to all other players
	SendPropDataTable( "normaldata", 0, &REFERENCE_SEND_TABLE(DT_NormalExclusive), SendProxy_SendNormalDataTable ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::Spawn( void )
{
	BaseClass::Spawn();

	// If owernumber wasn't changed yet, trigger on change once
	if( GetOwnerNumber() == 0 )
		OnChangeOwnerNumber(0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CUnitBase::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "unittype" ) )
	{
		SetUnitType( szValue );
		return true;
	}
	return BaseClass::KeyValue( szKeyName, szValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::SetUnitType( const char *unit_type )
{
	Q_strcpy( m_NetworkedUnitType.GetForModify(), unit_type );
	const char *pOldUnitType = STRING(m_UnitType);
	m_UnitType = AllocPooledString(unit_type);
	OnUnitTypeChanged(pOldUnitType);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CUnitBase::GetDensityMultiplier()
{
	if( GetNavigator() )
		return GetNavigator()->GetDensityMultiplier();
	return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	if( !pWeapon )
		return;

	BaseClass::Weapon_Equip( pWeapon );

	pWeapon->SetOwnerNumber( GetOwnerNumber() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static ConVar g_debug_rangeattacklos("g_debug_rangeattacklos", "0", FCVAR_CHEAT);
bool CUnitBase::HasRangeAttackLOS( const Vector &vTargetPos )
{
	if( GetActiveWeapon() )
	{
		m_bHasRangeAttackLOS = GetActiveWeapon()->WeaponLOSCondition(Weapon_ShootPosition(), vTargetPos, false);
	}
	else
	{
		trace_t result;
		CTraceFilterNoNPCsOrPlayer traceFilter( this, COLLISION_GROUP_NONE );
		UTIL_TraceLine( EyePosition(), vTargetPos, m_iAttackLOSMask, &traceFilter, &result );
		if( g_debug_rangeattacklos.GetBool() )
			NDebugOverlay::Line( EyePosition(), result.endpos, 0, 255, 0, true, 1.0f );
		m_bHasRangeAttackLOS = (result.fraction == 1.0f);
	}
	m_fLastRangeAttackLOSTime = gpGlobals->curtime;
	return m_bHasRangeAttackLOS;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::SetEnemy( CBaseEntity *pEnemy )
{
	if( !pEnemy || (pEnemy->IsUnit() && !pEnemy->IsAlive()) )
	{
		if( m_bHasEnemy )
		{
			// Did have an enemy, but is now cleared
			m_hEnemy = NULL;
			m_bHasEnemy = false;
#ifndef DISABLE_PYTHON
			SrcPySystem()->Run<const char *>( 
				SrcPySystem()->Get("DispatchEvent", GetPyInstance() ), 
				"OnEnemyLost"
			);
#endif // DISABLE_PYTHON
		}
		return;
	}
	
	if( pEnemy == m_hEnemy )
		return; // Nothing changed

	// New not null enemy
	m_hEnemy = pEnemy;
	m_bHasEnemy = true;
#ifndef DISABLE_PYTHON
	SrcPySystem()->Run<const char *, boost::python::object>( 
		SrcPySystem()->Get("DispatchEvent", GetPyInstance() ), 
		"OnNewEnemy", pEnemy->GetPyHandle()
	);
#endif // DISABLE_PYTHON
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::UpdateEnemy( UnitBaseSense &senses )
{
	CheckEnemyAlive();

	CBaseEntity *pEnemy = GetEnemy();
	CBaseEntity *pBest = senses.GetNearestEnemy();
	if( pBest == pEnemy )
		return;

	// Don't change enemy if very nearby
	if( pBest && pEnemy && pEnemy->GetAbsOrigin().DistToSqr( pBest->GetAbsOrigin() ) < (128.0*128.0f) )
		return;

	SetEnemy(pBest);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::CheckEnemyAlive()
{
	if( m_bHasEnemy )
	{
		if( !m_hEnemy || !m_hEnemy->FOWShouldShow( GetOwnerNumber() ) )
		{
			SetEnemy(NULL);
		}
		else if( m_hEnemy->IsUnit() )
		{
			if( !m_hEnemy->IsAlive() )
			{
				SetEnemy(NULL);
			}
			else
			{
				CUnitBase *pUnit = m_hEnemy->MyUnitPointer();
				if( pUnit && !pUnit->CanBeSeen() )
					SetEnemy(NULL);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CUnitBase::PassesDamageFilter( const CTakeDamageInfo &info )
{
	if( info.GetAttacker() )
	{
		CUnitBase *pUnit = info.GetAttacker()->MyUnitPointer();
		if( pUnit && pUnit->GetCommander() == NULL )
		{
			if( pUnit->IRelationType(this) != D_HT )
				return false;
		}
	}
	return BaseClass::PassesDamageFilter( info );
}

//-----------------------------------------------------------------------------
// Purpose: Return dist. to enemy (closest of origin/head/feet)
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CUnitBase::EnemyDistance( CBaseEntity *pEnemy, bool bConsiderSizeUnit )
{
	if( !pEnemy )
	{		
#ifndef DISABLE_PYTHON
		PyErr_SetString(PyExc_Exception, "Invalid enemy" );
		throw boost::python::error_already_set(); 
#endif // DISABLE_PYTHON
		return 0.0f;
	}

	Vector enemyDelta = pEnemy->WorldSpaceCenter() - WorldSpaceCenter();

	// NOTE: We ignore rotation for computing height.  Assume it isn't an effect
	// we care about, so we simply use OBBSize().z for height.  
	// Otherwise you'd do this:
	// pEnemy->CollisionProp()->WorldSpaceSurroundingBounds( &enemyMins, &enemyMaxs );
	// float enemyHeight = enemyMaxs.z - enemyMins.z;

	float enemyHeight = pEnemy->CollisionProp()->OBBSize().z;
	float myHeight = CollisionProp()->OBBSize().z;

	// max distance our centers can be apart with the boxes still overlapping
	float flMaxZDist = ( enemyHeight + myHeight ) * 0.5f;

	// see if the enemy is closer to my head, feet or in between
	if ( enemyDelta.z > flMaxZDist )
	{
		// enemy feet above my head, compute distance from my head to his feet
		enemyDelta.z -= flMaxZDist;
	}
	else if ( enemyDelta.z < -flMaxZDist )
	{
		// enemy head below my feet, return distance between my feet and his head
		enemyDelta.z += flMaxZDist;
	}
	else
	{
		// boxes overlap in Z, no delta
		enemyDelta.z = 0;
	}

	if( bConsiderSizeUnit )
		return MAX(enemyDelta.Length() - pEnemy->CollisionProp()->BoundingRadius2D(), 0.0f);
	return enemyDelta.Length();
}

float CUnitBase::TargetDistance( const Vector &pos, CBaseEntity *pTarget, bool bConsiderSizeUnit )
{
	Vector enemyDelta = pTarget->WorldSpaceCenter() - pos;

	// NOTE: We ignore rotation for computing height.  Assume it isn't an effect
	// we care about, so we simply use OBBSize().z for height.  
	// Otherwise you'd do this:
	// pEnemy->CollisionProp()->WorldSpaceSurroundingBounds( &enemyMins, &enemyMaxs );
	// float enemyHeight = enemyMaxs.z - enemyMins.z;

	float enemyHeight = pTarget->CollisionProp()->OBBSize().z;
	float myHeight = CollisionProp()->OBBSize().z;

	// max distance our centers can be apart with the boxes still overlapping
	float flMaxZDist = ( enemyHeight + myHeight ) * 0.5f;

	// see if the enemy is closer to my head, feet or in between
	if ( enemyDelta.z > flMaxZDist )
	{
		// enemy feet above my head, compute distance from my head to his feet
		enemyDelta.z -= flMaxZDist;
	}
	else if ( enemyDelta.z < -flMaxZDist )
	{
		// enemy head below my feet, return distance between my feet and his head
		enemyDelta.z += flMaxZDist;
	}
	else
	{
		// boxes overlap in Z, no delta
		enemyDelta.z = 0;
	}

	if( bConsiderSizeUnit )
	{
		return enemyDelta.Length() - pTarget->CollisionProp()->BoundingRadius2D();
	}
	return enemyDelta.Length();
}

bool CUnitBase::FInAimCone( CBaseEntity *pEntity, float fMinDot/*==0.994f*/ )
{
	if( !pEntity )
		return false;
	return FInAimCone( pEntity->BodyTarget( EyePosition() ), fMinDot );
}

bool CUnitBase::FInAimCone( const Vector &vecSpot, float fMinDot/*=0.994f*/ )
{
	Vector los = ( vecSpot - GetAbsOrigin() );

	// do this in 2D
	los.z = 0;
	VectorNormalize( los );

	Vector facingDir = BodyDirection2D( );

	float flDot = DotProduct( los, facingDir );

	if ( flDot > fMinDot )
		return true;

	return false;
}

// sets which player commands this unit
void CUnitBase::SetCommander( CHL2WarsPlayer *player )
{
	if ( m_hCommander.Get() == player )
	{
		return;
	}

	m_hCommander = player;
}

#ifndef DISABLE_PYTHON
void CUnitBase::SetNavigator( boost::python::object navigator )
{
	if( navigator.ptr() == Py_None )
	{
		m_pNavigator = NULL;
		m_pyNavigator = boost::python::object();
		return;
	}

	try {
		m_pNavigator = boost::python::extract<UnitBaseNavigator *>(navigator);
		m_pyNavigator = navigator;
	} catch(boost::python::error_already_set &) {
		PyErr_Print();
		PyErr_Clear();
		m_pNavigator = NULL;
		m_pyNavigator = boost::python::object();
		return;
	}
}
#endif // DISABLE_PYTHON

/*
void CUnitBase::SetExpresser( boost::python::object expresser )
{
	if( expresser.ptr() == Py_None )
	{
		m_pExpresser = NULL;
		m_pyExpresser = boost::python::object();
		return;
	}

	try {
		m_pExpresser = boost::python::extract<UnitExpresser *>(expresser);
		m_pyExpresser = expresser;
	} catch(boost::python::error_already_set &) {
		PyErr_Print();
		PyErr_Clear();
		m_pExpresser = NULL;
		m_pyExpresser = boost::python::object();
		return;
	}
}

UnitExpresser *CUnitBase::GetExpresser() 
{ 
	return m_pExpresser; 
}
*/

IResponseSystem *CUnitBase::GetResponseSystem()
{
	extern IResponseSystem *g_pResponseSystem;
	// Expressive NPC's use the general response system
	return g_pResponseSystem;
}

#ifndef DISABLE_PYTHON
void CUnitBase::SetAnimEventMap( boost::python::object animeventmap )
{
	if( animeventmap.ptr() == Py_None )
	{
		m_pAnimEventMap = NULL;
		m_pyAnimEventMap = boost::python::object();
		return;
	}
	m_pAnimEventMap = boost::python::extract<AnimEventMap *>(animeventmap);
	m_pyAnimEventMap = animeventmap;
}
#endif // DISABLE_PYTHON

void CUnitBase::HandleAnimEvent( animevent_t *pEvent )
{
	if( m_pAnimEventMap )
	{
		int idx = m_pAnimEventMap->m_AnimEventMap.Find(pEvent->Event());
		if( m_pAnimEventMap->m_AnimEventMap.IsValidIndex(idx) )
		{
			if( m_pAnimEventMap->m_AnimEventMap[idx].m_pHandler )
			{
				m_pAnimEventMap->m_AnimEventMap[idx].m_pHandler->HandleEvent(this, pEvent);
			}
			else
			{
#ifndef DISABLE_PYTHON
				// Assume it's an unbound method
				try {
					m_pAnimEventMap->m_AnimEventMap[idx].m_pyInstance(GetPyInstance(), pEvent);
				} catch( bp::error_already_set &) {
					PyErr_Print();
					PyErr_Clear();
				}
#endif // DISABLE_PYTHON
			}
			return;
		}
	}


	// FIXME: why doesn't this code pass unhandled events down to its parent?
	// Came from my weapon?
	//Adrian I'll clean this up once the old event system is phased out.
	int nEvent = pEvent->Event();
	if ( pEvent->pSource != this || ( pEvent->type & AE_TYPE_NEWEVENTSYSTEM && pEvent->type & AE_TYPE_WEAPON ) || ( nEvent >= EVENT_WEAPON && nEvent <= EVENT_WEAPON_LAST ) )
	{
		Weapon_HandleAnimEvent( pEvent );
	}
	else
	{
		BaseClass::HandleAnimEvent( pEvent );
	}
}

void CUnitBase::SetCrouching( bool crouching )
{
	m_bCrouching = crouching;	
}

void CUnitBase::SetClimbing( bool climbing )
{
	m_bClimbing = climbing;
}

int	CUnitBase::OnTakeDamage( const CTakeDamageInfo &info )
{
	bool bFullHealth = (m_iHealth >= m_iMaxHealth);
	int rv = BaseClass::OnTakeDamage( info );
	if( bFullHealth && m_iHealth < m_iMaxHealth )
		OnLostFullHealth();
	return rv;
}

int CUnitBase::TakeHealth( float flHealth, int bitsDamageType )
{
	bool bFullHealth = (m_iHealth >= m_iMaxHealth);
	int rv = BaseClass::TakeHealth( flHealth, bitsDamageType );
	if( !bFullHealth && m_iHealth >= m_iMaxHealth )
		OnFullHealth();
	return rv;
}
