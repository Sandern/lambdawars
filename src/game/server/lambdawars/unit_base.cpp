//====== Copyright © Sandern Corporation, All rights reserved. ===========//
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
#include "unit_baseanimstate.h"
#include "wars_weapon_shared.h"

#include "sendprop_priorities.h"
#include "networkstringtable_gamedll.h"

#ifdef ENABLE_PYTHON
	#include "srcpy.h"
#endif // ENABLE_PYTHON

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar g_debug_rangeattacklos("g_debug_rangeattacklos", "0", FCVAR_CHEAT);
static ConVar g_debug_checkthrowtolerance( "g_debug_checkthrowtolerance", "0" );
ConVar g_unit_force_minimal_sendtable("g_unit_force_minimal_sendtable", "0", FCVAR_CHEAT);
static ConVar g_unit_minimal_sendtable_updaterate("g_unit_minimal_sendtable_updaterate", "0.4", FCVAR_CHEAT);
static ConVar unit_nextserveranimupdatetime("unit_nextserveranimupdatetime", "0.2", FCVAR_CHEAT);

//-----------------------------------------------------------------------------
// Grenade tossing
//-----------------------------------------------------------------------------
//
//	FIXME: Create this in a better fashion!
//
extern ConVar sv_gravity;
Vector VecCheckThrowTolerance( CBaseEntity *pEdict, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flTolerance, int iCollisionGroup )
{
	flSpeed = Max( 1.0f, flSpeed );

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

TossGrenadeAnimEventHandler::TossGrenadeAnimEventHandler( const char *pEntityName, float fSpeed )
{
	V_strncpy( m_EntityName, pEntityName, 40 );
	m_fSpeed = fSpeed;
}

void TossGrenadeAnimEventHandler::SetGrenadeClass( const char *pGrenadeClass )
{
	V_strncpy( m_EntityName, pGrenadeClass, 40 );
}

bool TossGrenadeAnimEventHandler::GetTossVector( CUnitBase *pUnit, const Vector &vecStartPos, const Vector &vecTarget, int iCollisionGroup, Vector *vecOut )
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

CBaseEntity *TossGrenadeAnimEventHandler::TossGrenade( CUnitBase *pUnit, Vector &vecStartPos, Vector &vecTarget, int iCollisionGroup )
{
	if( !pUnit )
		return NULL;

	// Try and spit at our target
	Vector	vecToss;				
	if ( GetTossVector( pUnit, vecStartPos, vecTarget, iCollisionGroup, &vecToss ) == false )
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

#ifdef ENABLE_PYTHON
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
		boost::python::object items = d.attr("items")();
		boost::python::object iterator = items.attr("__iter__")();
		boost::python::ssize_t length = boost::python::len(items); 
		for( boost::python::ssize_t u = 0; u < length; u++ )
		{
			boost::python::object item = iterator.attr( PY_NEXT_METHODNAME )();

			try 
			{
				event = boost::python::extract<int>( item[0] );
			}
			catch( boost::python::error_already_set & )
			{
				PyErr_Print();
				continue;
			}

			SetAnimEventHandler( event, item[1] );
		}
	}
	catch( boost::python::error_already_set & )
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
	catch( boost::python::error_already_set & )
	{
		PyErr_Clear();
		// Not an handler object. Assume unbound method that handles the event.
		handler.m_pHandler = NULL;
	}
	handler.m_pyInstance = pyhandler;
	m_AnimEventMap.Insert(event, handler);
}
#endif // ENABLE_PYTHON

//=============================================================================
// Unit Send Tables
//
// Note1: Don't try to split vecOrigin and vecAngles over multiple tables.
//		  The first few frames seem to send this data from all tables (Valve BUG?)
//		  This ends up taking a big hit in Overrun due spawning many enemies at a high rate.
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose:  Send proxies for tables
//-----------------------------------------------------------------------------
void* SendProxy_SendCommanderDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the unit entity
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

void* SendProxy_SendMinimalDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	CUnitBase *pUnit = (CUnitBase*)pVarData;
	if ( pUnit )
	{
		for( int i = 0; i < gpGlobals->maxClients; i++ )
		{
			if( !pUnit->UseMinimalSendTable( i ) )
				pRecipients->ClearRecipient( i );
		}
	}

	return (void*)pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendMinimalDataTable );

void* SendProxy_SendFullDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the unit entity
	CUnitBase *pUnit = (CUnitBase*)pVarData;
	if ( pUnit )
	{
		for( int i = 0; i < gpGlobals->maxClients; i++ )
		{
			if( pUnit->UseMinimalSendTable( i ) )
				pRecipients->ClearRecipient( i );
		}
	}

	return (void*)pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendFullDataTable );

void* SendProxy_SingleSelectionDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the unit entity
	CUnitBase *pUnit = (CUnitBase*)pVarData;
	if ( pUnit )
	{
		if( pUnit->AlwaysSendFullSelectionData() )
			return (void*)pVarData; // Send full res to all players

		pRecipients->ClearAllRecipients();
		const CUtlVector< CHandle< CHL2WarsPlayer > > &selectedByPlayers = pUnit->GetSelectedByPlayers();
		for( int i = 0; i < selectedByPlayers.Count(); i++ )
		{
			if( selectedByPlayers[i] && selectedByPlayers[i]->CountUnits() == 1 )
				pRecipients->SetRecipient( selectedByPlayers[i].GetEntryIndex() - 1 );
		}
	}

	return (void*)pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SingleSelectionDataTable );

void* SendProxy_MultiOrNoneSelectionDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the unit entity
	CUnitBase *pUnit = (CUnitBase*)pVarData;
	if ( pUnit )
	{
		if( pUnit->AlwaysSendFullSelectionData() )
		{
			pRecipients->ClearAllRecipients(); // Never use this table
			return (void*)pVarData; 
		}

		// Inverse of SendProxy_SingleSelectionDataTable: Clear all players that have this unit as single selection
		const CUtlVector< CHandle< CHL2WarsPlayer > > &selectedByPlayers = pUnit->GetSelectedByPlayers();
		for( int i = 0; i < selectedByPlayers.Count(); i++ )
		{
			if( selectedByPlayers[i] && selectedByPlayers[i]->CountUnits() == 1 )
				pRecipients->ClearRecipient( selectedByPlayers[i].GetEntryIndex() - 1 );
		}
	}

	return (void*)pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_MultiOrNoneSelectionDataTable );

extern void SendProxy_SimulationTime( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );

//-----------------------------------------------------------------------------
// Purpose:  Send proxies for variables
//-----------------------------------------------------------------------------
void SendProxy_Origin( const SendProp *, const void *, const void *, DVariant *, int, int );

void SendProxy_Health( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CUnitBase *unit = (CUnitBase*)pStruct;
	Assert( unit );

	pOut->m_Int = (int)floor( unit->HealthFraction() * ((1 << SENDPROP_HEALTH_BITS_LOW) - 1) );
}

void SendProxy_Energy( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CUnitBase *unit = (CUnitBase*)pStruct;
	Assert( unit );

	pOut->m_Int = (int)floor( (unit->GetEnergy() / (float)unit->GetMaxEnergy()) * ((1 << SENDPROP_HEALTH_BITS_LOW) - 1) );
}

//-----------------------------------------------------------------------------
// Purpose:  Send tables
//-----------------------------------------------------------------------------
BEGIN_SEND_TABLE_NOBASE( CUnitBase, DT_CommanderExclusive )
	// Send high res version for the commander
#if 0
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),

	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 0), 13, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesX ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 1), 13, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesY ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 2), 13, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesZ ),
#endif // 0

	// This data is only send to the commander for prediction
	//SendPropVector		( SENDINFO( m_vecBaseVelocity ), 20, 0, -1000, 1000 ),
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecVelocity, 0), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecVelocity, 1), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecVelocity, 2), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),

	// Update SetDefaultEyeOffset when changing the range!
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecViewOffset, 0), 10, SPROP_ROUNDDOWN, -256.0, 256.0f),
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecViewOffset, 1), 10, SPROP_ROUNDDOWN, -256.0, 256.0f),
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecViewOffset, 2), 12, SPROP_CHANGES_OFTEN,	-1.0f, 1024.0f),

END_SEND_TABLE()

// Table used when out of "PVS". This table is send when DT_FullTable is not send!
BEGIN_SEND_TABLE_NOBASE( CUnitBase, DT_MinimalTable )
END_SEND_TABLE()

// Table used when the unit is in "PVS". This table is send when DT_MinimalTable is not send!
BEGIN_SEND_TABLE_NOBASE( CUnitBase, DT_FullTable )
	// Moved sending z component to main table so it gets always send, here it gives too much problems
	// The unit does not receive an update once the full table is selected again.
	//SendPropFloat   ( SENDINFO_VECTORELEM( m_vecOrigin, 2 ), CELL_BASEENTITY_ORIGIN_CELL_BITS, SPROP_CELL_COORD_LOWPRECISION | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, CBaseEntity::SendProxy_CellOriginZ, SENDPROP_NONLOCALPLAYER_ORIGINZ_PRIORITY ),

	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 0), 10, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesX ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 2), 10, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesZ ),
	
	SendPropInt		(SENDINFO( m_iMaxHealth ), 15, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO( m_iMaxEnergy ), 15, SPROP_UNSIGNED ),

	SendPropInt		(SENDINFO( m_takedamage ), 3, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO( m_lifeState ), 3, SPROP_UNSIGNED ),

	SendPropEHandle		( SENDINFO( m_hSquadUnit ) ),
	SendPropEHandle		( SENDINFO( m_hCommander ) ),
	SendPropEHandle		( SENDINFO( m_hEnemy ) ),

	SendPropBool( SENDINFO( m_bCrouching ) ),
	SendPropBool( SENDINFO( m_bClimbing ) ),
END_SEND_TABLE()

// Single Unit Selection Table. Only send when the player has exactly one unit selected.
// Used for displaying data in the hud
BEGIN_SEND_TABLE_NOBASE( CUnitBase, DT_SingleSelectionTable )
	SendPropInt		(SENDINFO( m_iHealth ), 15, SPROP_UNSIGNED|SPROP_CHANGES_OFTEN ),
	SendPropInt		(SENDINFO( m_iEnergy ), 15, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO( m_iKills ), 9, SPROP_UNSIGNED ),
END_SEND_TABLE()

// Send when the unit is in a multi selection or is not selected at all
BEGIN_SEND_TABLE_NOBASE( CUnitBase, DT_MultiOrNoneSelectionTable )
	SendPropInt		(SENDINFO( m_iHealth ), SENDPROP_HEALTH_BITS_LOW, SPROP_UNSIGNED/*|SPROP_CHANGES_OFTEN*/, SendProxy_Health ),
	SendPropInt		(SENDINFO( m_iEnergy ), SENDPROP_HEALTH_BITS_LOW, SPROP_UNSIGNED, SendProxy_Energy ),
END_SEND_TABLE()

// Main Unit Send Table. Always send
IMPLEMENT_SERVERCLASS_ST( CUnitBase, DT_UnitBase )
	SendPropVectorXY( SENDINFO( m_vecOrigin ), 				 CELL_BASEENTITY_ORIGIN_CELL_BITS, SPROP_CELL_COORD_LOWPRECISION | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, CBaseEntity::SendProxy_CellOriginXY, SENDPROP_NONLOCALPLAYER_ORIGINXY_PRIORITY ),
	SendPropFloat( SENDINFO_VECTORELEM( m_vecOrigin, 2 ), CELL_BASEENTITY_ORIGIN_CELL_BITS, SPROP_CELL_COORD_LOWPRECISION | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, CBaseEntity::SendProxy_CellOriginZ, SENDPROP_NONLOCALPLAYER_ORIGINZ_PRIORITY ),
	SendPropAngle( SENDINFO_VECTORELEM( m_angRotation, 1 ), 10, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesY ),

	// Send the same flags as done for the players. These flags are used to execute the animstate on the client
	// The only flag used right now is FL_ONGROUND, so just send one bit..
	SendPropInt		(SENDINFO( m_fFlags ), 1, SPROP_UNSIGNED|SPROP_CHANGES_OFTEN ),	

	SendPropInt		(SENDINFO( m_NetworkedUnitTypeSymbol ), MAX_GAMEDBNAMES_STRING_BITS, SPROP_UNSIGNED ),

	// Data that only gets sent to the player controlling this unit
	SendPropDataTable( "commanderdata", 0, &REFERENCE_SEND_TABLE(DT_CommanderExclusive), SendProxy_SendCommanderDataTable ),
	// Data that gets sent when unit is outside the pvs (and no other table is send)
	SendPropDataTable( "minimaldata", 0, &REFERENCE_SEND_TABLE(DT_MinimalTable), SendProxy_SendMinimalDataTable ),
	// Data that gets sent when unit is inside the pvs
	SendPropDataTable( "fulldata", 0, &REFERENCE_SEND_TABLE(DT_FullTable), SendProxy_SendFullDataTable ),
	// Data that gets sent when unit is the single selected unit of the player
	SendPropDataTable( "singleselectiondata", 0, &REFERENCE_SEND_TABLE(DT_SingleSelectionTable), SendProxy_SingleSelectionDataTable ),
	// Data that gets sent when unit is in a multi selection of the player or is not selected at all
	SendPropDataTable( "multiornoselectiondata", 0, &REFERENCE_SEND_TABLE(DT_MultiOrNoneSelectionTable), SendProxy_MultiOrNoneSelectionDataTable ),

	// Excludes
	//SendPropExclude( "DT_BaseEntity", "m_flSimulationTime" ),

	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),

	// Excludes
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),

	SendPropExclude( "DT_BaseAnimating", "m_nNewSequenceParity" ),
	SendPropExclude( "DT_BaseAnimating", "m_nResetEventsParity" ),
	SendPropExclude( "DT_BaseAnimating", "m_nMuzzleFlashParity" ),
	
	// playeranimstate and clientside animation takes care of these on the client
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	// Something with the legs ?
	SendPropExclude( "DT_BaseFlex" , "m_vecLean" ),
	SendPropExclude( "DT_BaseFlex" , "m_vecShift" ),

	// BaseCombatcharacter excludes
	// Don't need weapons inventory synced
	SendPropExclude( "DT_BCCLocalPlayerExclusive" , "m_hMyWeapons" ),
	
END_SEND_TABLE()

BEGIN_DATADESC( CUnitBase )
	DEFINE_FIELD( m_bCanBeSeen, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bUseCustomCanBeSeenCheck, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_bHasEnterOffset, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vEnterOffset, FIELD_VECTOR ),

	DEFINE_FIELD( m_UnitType, FIELD_STRING ),
	DEFINE_FIELD( m_NetworkedUnitTypeSymbol, FIELD_INTEGER ),
	DEFINE_FIELD( m_fLastTakeDamageTime, FIELD_FLOAT ),

	DEFINE_FIELD( m_iEnergy, FIELD_INTEGER ),
	DEFINE_FIELD( m_iMaxEnergy, FIELD_INTEGER ),
	DEFINE_FIELD( m_iKills, FIELD_INTEGER ),

	DEFINE_FIELD( m_hCommander, FIELD_EHANDLE ),
	
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::Spawn( void )
{
	BaseClass::Spawn();

	SetGlobalFadeScale( 0.0f );

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
void CUnitBase::SetUseMinimalSendTable( int iClientIndex, bool bUseMinimalSendTable )
{
	CServerNetworkProperty *netProp = static_cast<CServerNetworkProperty*>( GetNetworkable() );

	if( m_UseMinimalSendTable.IsBitSet( iClientIndex ) != bUseMinimalSendTable )
	{
		m_UseMinimalSendTable.Set( iClientIndex, bUseMinimalSendTable );

		if( !bUseMinimalSendTable )
		{
			// State might have changed
			NetworkStateChanged();

			// This unit is no longer using the minmal table for this player, 
			// so if low update rate is active, then we must clear it!
			if( netProp->TimerEventActive() )
			{
				netProp->SetUpdateInterval( 0 );
			}
		}
		else
		{
			// It might be the case that we can go into low update rate if not yet
			// This is only possible if the unit is out of view of all players
			// We only need some (slow) updates for the minimap!
			if( !netProp->TimerEventActive() && m_UseMinimalSendTable.IsAllSet() )
			{
				netProp->SetUpdateInterval( 0.4f );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Note, an entity can override the send table ( e.g., to send less data or to send minimal data for
//  objects ( prob. players ) that are not in the pvs.
// Input  : **ppSendTable - 
//			*recipient - 
//			*pvs - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
int CUnitBase::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );
	Assert( pRecipientEntity->IsPlayer() );

	CBasePlayer *pRecipientPlayer = static_cast<CBasePlayer*>( pRecipientEntity );

	// Don't send when in the fow for the recv player.
	if( !FOWShouldTransmit( pRecipientPlayer ) ) 
	{
		return FL_EDICT_DONTSEND;
	}

	return FL_EDICT_ALWAYS;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CUnitBase::UpdateTransmitState()
{
	// Should always use the full check
	return SetTransmitState( FL_EDICT_FULLCHECK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::SetUnitType( const char *unit_type )
{
	m_NetworkedUnitTypeSymbol = g_pStringTableGameDBNames->FindStringIndex( unit_type );
	if (m_NetworkedUnitTypeSymbol == INVALID_STRING_INDEX ) 
		m_NetworkedUnitTypeSymbol = g_pStringTableGameDBNames->AddString( CBaseEntity::IsServer(), unit_type );

	const char *pOldUnitType = STRING(m_UnitType);
	m_UnitType = AllocPooledString(unit_type);
	OnUnitTypeChanged(pOldUnitType);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::UpdateServerAnimation( void )
{
	VPROF_BUDGET( "CUnitBase::UpdateServerAnimation", VPROF_BUDGETGROUP_UNITS );

	if( !GetAnimState() )
		return;

	if( m_fNextServerAnimStateTime > gpGlobals->curtime )
		return;

	if( GetAnimState()->HasAimPoseParameters() )
	{
		if( !GetCommander() )
			AimGun();
		GetAnimState()->Update( m_fEyeYaw, m_fEyePitch );
		
	}
	else
	{
		const QAngle &eyeangles = EyeAngles();
		GetAnimState()->Update( eyeangles[YAW], eyeangles[PITCH] );
	}

	m_fNextServerAnimStateTime = gpGlobals->curtime + unit_nextserveranimupdatetime.GetFloat();
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
// Purpose: LOS tracing filter for units
//-----------------------------------------------------------------------------
class CUnitLOSFilter : public CTraceFilterSkipTwoEntities
{
	DECLARE_CLASS( CUnitLOSFilter, CTraceFilterSkipTwoEntities );
public:
	CUnitLOSFilter( IHandleEntity *pHandleEntity, IHandleEntity *pHandleEntity2, int collisionGroup ) :
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CUnitBase::HasRangeAttackLOS( const Vector &vTargetPos, CBaseEntity *pTarget )
{
	if( GetActiveWeapon() )
	{
		m_bHasRangeAttackLOS = GetActiveWeapon()->WeaponLOSCondition( GetLocalOrigin(), vTargetPos, false );
	}
	else
	{
		if( !pTarget )
			pTarget = GetEnemy();

		trace_t result;
		CUnitLOSFilter traceFilter( this, pTarget, GetCollisionGroup() );
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
bool CUnitBase::HasRangeAttackLOSTarget( CBaseEntity *pTarget )
{
	if( !pTarget || !pTarget->IsSolid() )
		return false;

	if( GetActiveWeapon() )
	{
		CWarsWeapon *pWeapon = dynamic_cast<CWarsWeapon *>( GetActiveWeapon() );
		if( pWeapon )
			m_bHasRangeAttackLOS = pWeapon->WeaponLOSCondition( GetLocalOrigin(), pTarget->BodyTarget( GetLocalOrigin() ), pTarget );
		else
			m_bHasRangeAttackLOS = false;
	}
	else
	{
		trace_t result;
		CUnitLOSFilter traceFilter( this, pTarget, GetCollisionGroup() );
		UTIL_TraceLine( EyePosition(), pTarget->BodyTarget( GetLocalOrigin() ), m_iAttackLOSMask, &traceFilter, &result );
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
			m_fLastEnemyChangeTime = gpGlobals->curtime;

#ifdef ENABLE_PYTHON
			SrcPySystem()->Run<const char *>( 
				SrcPySystem()->Get("DispatchEvent", GetPyInstance() ), 
				"OnEnemyLost"
			);
#endif // ENABLE_PYTHON
		}
		return;
	}
	
	if( pEnemy == m_hEnemy )
		return; // Nothing changed

	// New not null enemy
	m_hEnemy = pEnemy;
	m_bHasEnemy = true;
	m_fLastEnemyChangeTime = gpGlobals->curtime;

#ifdef ENABLE_PYTHON
	SrcPySystem()->Run<const char *, boost::python::object>( 
		SrcPySystem()->Get("DispatchEvent", GetPyInstance() ), 
		"OnNewEnemy", pEnemy->GetPyHandle()
	);
#endif // ENABLE_PYTHON
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::UpdateEnemy( UnitBaseSense &senses )
{
	VPROF_BUDGET( "CUnitBase::UpdateEnemy", VPROF_BUDGETGROUP_UNITS );

	CheckEnemyAlive();

	CBaseEntity *pEnemy = GetEnemy();
	CBaseEntity *pBest = senses.GetNearestEnemy();
	if( pBest == pEnemy )
		return;

	// Don't change enemy if very nearby
	if( pBest && pEnemy && pEnemy->GetAbsOrigin().DistToSqr( pBest->GetAbsOrigin() ) < m_fEnemyChangeToleranceSqr )
		return;

	SetEnemy(pBest);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::CheckEnemyAlive()
{
	VPROF_BUDGET( "CUnitBase::UpdateEnemy", VPROF_BUDGETGROUP_UNITS );

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
		if( pUnit && pUnit->GetCommander() == NULL && !pUnit->m_bFriendlyDamage && !info.IsForceFriendlyFire() )
		{
			if( pUnit->IRelationType(this) != D_HT )
				return false;
		}
	}
	return BaseClass::PassesDamageFilter( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CUnitBase::TargetDistance( const Vector &pos, CBaseEntity *pTarget, bool bConsiderSizeUnit )
{
	if( !pTarget )
	{		
#ifdef ENABLE_PYTHON
		PyErr_SetString(PyExc_Exception, "Invalid target" );
		throw boost::python::error_already_set(); 
#endif // ENABLE_PYTHON
		return 0.0f;
	}

	const Vector &targetPos = pTarget->WorldSpaceCenter();

	// For large units (i.e. buildings) use a trace
	float fBoundingRadius2D = pTarget->CollisionProp()->BoundingRadius2D();
	if( fBoundingRadius2D > 96.0f )
	{
		trace_t tr;
		Ray_t ray;
		ray.Init( pos, pTarget->EyePosition() ); // For buildings, the eye position is usually a better test position
		enginetrace->ClipRayToEntity( ray, MASK_SOLID, pTarget, &tr );
		if( tr.DidHit() )
		{
			return (pos - tr.endpos).Length2D();
		}
		else
		{
			Warning( "CUnitBase::TargetDistance: Entity #%d:%s not hit target entity #%d:%s!\n", 
						entindex(), GetClassname(), pTarget->entindex(), pTarget->GetClassname() );
			if( g_debug_rangeattacklos.GetBool() )
				NDebugOverlay::Line( pos, pTarget->EyePosition(), 255, 255, 0, true, 1.0f );
		}
	}

	Vector enemyDelta = targetPos - pos;

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
		return Max(enemyDelta.Length() - fBoundingRadius2D, 0.0f);
	return enemyDelta.Length();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CUnitBase::FInAimCone( CBaseEntity *pEntity, float fMinDot/*==0.994f*/ )
{
	if( !pEntity )
		return false;
	return FInAimCone( pEntity->BodyTarget( EyePosition() ), fMinDot );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose: sets which player commands this unit
//-----------------------------------------------------------------------------
void CUnitBase::SetCommander( CHL2WarsPlayer *player )
{
	if ( m_hCommander.Get() == player )
	{
		return;
	}

	m_hCommander = player;
}

#ifdef ENABLE_PYTHON
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
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
#endif // ENABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IResponseSystem *CUnitBase::GetResponseSystem()
{
	extern IResponseSystem *g_pResponseSystem;
	// Expressive NPC's use the general response system
	return g_pResponseSystem;
}

#ifdef ENABLE_PYTHON
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
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
#endif // ENABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
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
#ifdef ENABLE_PYTHON
				// Assume it's an unbound method
				try {
					m_pAnimEventMap->m_AnimEventMap[idx].m_pyInstance(GetPyInstance(), pEvent);
				} catch( boost::python::error_already_set &) {
					PyErr_Print();
					PyErr_Clear();
				}
#endif // ENABLE_PYTHON
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::SetCrouching( bool crouching )
{
	m_bCrouching = crouching;	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::SetClimbing( bool climbing )
{
	m_bClimbing = climbing;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CUnitBase::OnTakeDamage( const CTakeDamageInfo &info )
{
	bool bFullHealth = (m_iHealth >= m_iMaxHealth);
	int rv = BaseClass::OnTakeDamage( info );
	if( bFullHealth && m_iHealth < m_iMaxHealth )
		OnLostFullHealth();
	return rv;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CUnitBase::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if( info.GetAttacker() )
		m_fLastTakeDamageTime = gpGlobals->curtime;

	return BaseClass::OnTakeDamage_Alive( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CUnitBase::TakeHealth( float flHealth, int bitsDamageType )
{
	bool bFullHealth = (m_iHealth >= m_iMaxHealth);
	int rv = BaseClass::TakeHealth( flHealth, bitsDamageType );
	if( !bFullHealth && m_iHealth >= m_iMaxHealth )
		OnFullHealth();
	return rv;
}

//-----------------------------------------------------------------------------
// Purpose: Unit test code
//-----------------------------------------------------------------------------
class CUnitDummyTest : public CUnitBase
{
public:
	DECLARE_CLASS( CUnitDummyTest, CUnitBase );

	void Precache()
	{
		PrecacheModel("models/error.mdl");

		BaseClass::Precache();
	}

	void Spawn( void )
	{
		Precache();

		SetModel( "models/error.mdl" );

		BaseClass::Spawn();
	}
};
LINK_ENTITY_TO_CLASS( unit_dummy_test, CUnitDummyTest );
