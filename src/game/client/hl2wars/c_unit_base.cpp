//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_unit_base.h"
#include "unit_base_shared.h"
#include "gamestringpool.h"
#include "model_types.h"
#include "cdll_bounded_cvars.h"

#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>

#include "c_hl2wars_player.h"
#include "hl2wars_util_shared.h"
#include "iinput.h"

#include "unit_baseanimstate.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar	cl_wars_smooth		( "cl_wars_smooth", "1", 0, "Smooth unit's render origin after prediction errors" );
static ConVar	cl_wars_smoothtime	( 
	"cl_wars_smoothtime", 
	"0.1", 
	0, 
	"Smooth unit's render origin after prediction error over this many seconds",
	true, 0.01,	// min/max is 0.01/2.0
	true, 2.0
	 );

//-----------------------------------------------------------------------------
// Purpose: Recv proxies
//-----------------------------------------------------------------------------
void RecvProxy_Unit_LocalVelocityX( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CUnitBase *pUnit = (CUnitBase *) pStruct;

	Assert( pUnit );

	float flNewVel_x = pData->m_Value.m_Float;
	
	Vector vecVelocity = pUnit->GetLocalVelocity();	

	if( vecVelocity.x != flNewVel_x )	// Should this use an epsilon check?
	{
		//if (vecVelocity.x > 30.0f)
		//{
			
		//}		
		vecVelocity.x = flNewVel_x;
		pUnit->SetLocalVelocity( vecVelocity );
	}
}

void RecvProxy_Unit_LocalVelocityY( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CUnitBase *pUnit = (CUnitBase *) pStruct;

	Assert( pUnit );

	float flNewVel_y = pData->m_Value.m_Float;

	Vector vecVelocity = pUnit->GetLocalVelocity();

	if( vecVelocity.y != flNewVel_y )
	{
		vecVelocity.y = flNewVel_y;
		pUnit->SetLocalVelocity( vecVelocity );
	}
}

void RecvProxy_Unit_LocalVelocityZ( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CUnitBase *pUnit = (CUnitBase *) pStruct;
	
	Assert( pUnit );

	float flNewVel_z = pData->m_Value.m_Float;

	Vector vecVelocity = pUnit->GetLocalVelocity();

	if( vecVelocity.z != flNewVel_z )
	{
		vecVelocity.z = flNewVel_z;
		pUnit->SetLocalVelocity( vecVelocity );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Recv tables
//-----------------------------------------------------------------------------
extern void RecvProxy_LocalVelocityX( const CRecvProxyData *pData, void *pStruct, void *pOut );
extern void RecvProxy_LocalVelocityY( const CRecvProxyData *pData, void *pStruct, void *pOut );
extern void RecvProxy_LocalVelocityZ( const CRecvProxyData *pData, void *pStruct, void *pOut );
extern void RecvProxy_SimulationTime( const CRecvProxyData *pData, void *pStruct, void *pOut );

BEGIN_RECV_TABLE_NOBASE( CUnitBase, DT_CommanderExclusive )
	// Hi res origin and angle
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[0], m_angRotation[0] ) ),
	RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[1], m_angRotation[1] ) ),
	RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[2], m_angRotation[2] ) ),

	// Only received by the commander
	RecvPropVector		( RECVINFO( m_vecBaseVelocity ) ),
	RecvPropFloat		( RECVINFO(m_vecVelocity[0]), 0, RecvProxy_Unit_LocalVelocityX ),
	RecvPropFloat		( RECVINFO(m_vecVelocity[1]), 0, RecvProxy_Unit_LocalVelocityY ),
	RecvPropFloat		( RECVINFO(m_vecVelocity[2]), 0, RecvProxy_Unit_LocalVelocityZ ),
	RecvPropFloat		( RECVINFO(m_vecViewOffset[0]) ),
	RecvPropFloat		( RECVINFO(m_vecViewOffset[1]) ),
	RecvPropFloat		( RECVINFO(m_vecViewOffset[2]) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( CUnitBase, DT_NormalExclusive )
	RecvPropVectorXY( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ), 0, C_BaseEntity::RecvProxy_CellOriginXY ),
	RecvPropFloat( RECVINFO_NAME( m_vecNetworkOrigin[2], m_vecOrigin[2] ), 0, C_BaseEntity::RecvProxy_CellOriginZ ),

	RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[0], m_angRotation[0] ) ),
	RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[1], m_angRotation[1] ) ),
	RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[2], m_angRotation[2] ) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( CUnitBase, DT_MinimalTable )
	//RecvPropInt( RECVINFO(m_flSimulationTime), 0, RecvProxy_SimulationTime ),
	RecvPropVectorXY( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ), 0, C_BaseEntity::RecvProxy_CellOriginXY ),
	//RecvPropFloat( RECVINFO_NAME( m_vecNetworkOrigin[2], m_vecOrigin[2] ), 0, C_BaseEntity::RecvProxy_CellOriginZ ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( CUnitBase, DT_FullTable )
	//RecvPropInt( RECVINFO(m_flSimulationTime), 0, RecvProxy_SimulationTime ),

	RecvPropString(  RECVINFO( m_NetworkedUnitType ) ),

	RecvPropInt		(RECVINFO(m_iHealth)),
	RecvPropInt		(RECVINFO(m_iMaxHealth)),
	RecvPropInt		(RECVINFO(m_fFlags)),
	RecvPropInt		(RECVINFO( m_takedamage)),
	RecvPropInt		(RECVINFO( m_lifeState)),

	RecvPropEHandle		( RECVINFO( m_hSquadUnit ) ),
	RecvPropEHandle		( RECVINFO( m_hCommander ) ),
	RecvPropEHandle		( RECVINFO( m_hEnemy ) ),
	//RecvPropEHandle		( RECVINFO( m_hGroundEntity ) ),

	RecvPropBool( RECVINFO( m_bCrouching ) ),
	RecvPropBool( RECVINFO( m_bClimbing ) ),

	RecvPropInt		(RECVINFO(m_iEnergy)),
	RecvPropInt		(RECVINFO(m_iMaxEnergy)),
	//RecvPropInt		(RECVINFO(m_iKills)),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( CUnitBase, DT_SelectedOnlyTable )
	RecvPropInt		(RECVINFO(m_iKills)),
END_RECV_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED( UnitBase, DT_UnitBase )

BEGIN_NETWORK_TABLE( CUnitBase, DT_UnitBase )
	RecvPropDataTable( "minimaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_MinimalTable) ),
	RecvPropDataTable( "normaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_NormalExclusive) ),
	RecvPropDataTable( "fulldata", 0, 0, &REFERENCE_RECV_TABLE(DT_FullTable) ),
	RecvPropDataTable( "commanderdata", 0, 0, &REFERENCE_RECV_TABLE(DT_CommanderExclusive) ),
	RecvPropDataTable( "selecteddata", 0, 0, &REFERENCE_RECV_TABLE(DT_SelectedOnlyTable) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( CUnitBase )
	DEFINE_PRED_FIELD( m_flSimulationTime, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),

	//DEFINE_PRED_FIELD( m_vecVelocity, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	//DEFINE_PRED_FIELD( m_flAnimTime, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),	
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),	
	DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nResetEventsParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_hGroundEntity, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_vecBaseVelocity, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.05 ),
END_PREDICTION_DATA()

//-----------------------------------------------------------------------------
// Purpose: Default on hover paints the health bar
//-----------------------------------------------------------------------------
void CUnitBase::OnHoverPaint()
{
	//DrawHealthBar( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void CUnitBase::Spawn( )
{
	BaseClass::Spawn();

	SetGlobalFadeScale( 0.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void CUnitBase::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	// Created?
	if( updateType == DATA_UPDATE_CREATED )
	{
		// If owernumber wasn't changed yet, trigger on change once
		if( GetOwnerNumber() == 0 )
			OnChangeOwnerNumber(0);
	}

	// Check if the player's faction changed ( Might want to add a string table )
	if( m_UnitType == NULL_STRING || Q_strncmp( STRING(m_UnitType), m_NetworkedUnitType, MAX_PATH ) != 0 ) 
	{
		const char *pOldUnitType = STRING(m_UnitType);
		m_UnitType = AllocPooledString(m_NetworkedUnitType);
		OnUnitTypeChanged(pOldUnitType);
	}

	// Check change commander
	if( m_hOldCommander != m_hCommander )
	{
		UpdateVisibility();
		m_hOldCommander = m_hCommander;
	}

	// Check change active weapon
	if( m_hOldActiveWeapon != GetActiveWeapon() )
	{
		OnActiveWeaponChanged();
		m_hOldActiveWeapon = GetActiveWeapon();
	}

	// Check for enemy, make sure we are hating on the client (for correct effects)
	if( m_hOldEnemy != m_hEnemy )
	{
		if( m_bForcedEnemyHate )
		{
			if( m_hOldEnemy )
				RemoveEntityRelationship( m_hOldEnemy );
			m_bForcedEnemyHate = false;
		}

		m_hOldEnemy = m_hEnemy;

		if( m_hEnemy && IRelationType( m_hEnemy ) != D_HT )
		{
			AddEntityRelationship( m_hEnemy, D_HT, 0 );
			m_bForcedEnemyHate = true;
		}
	}

	// Health changed?
	if( m_iHealth != m_iOldHealth )
	{
		if( m_iOldHealth > m_iHealth )
			m_fLastTakeDamageTime = gpGlobals->curtime;
		m_iOldHealth = m_iHealth;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CUnitBase::DrawModel( int flags, const RenderableInstance_t &instance )
{
	if( m_bIsBlinking )
	{
		flags |= STUDIO_ITEM_BLINK;
		if( m_fBlinkTimeOut != -1 && m_fBlinkTimeOut < gpGlobals->curtime )
			m_bIsBlinking = false;
	}

	return BaseClass::DrawModel( flags, instance );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const Vector& CUnitBase::GetRenderOrigin( void )
{
	//if( m_pAnimState )
	//	return m_pAnimState->GetRenderOrigin();
	return BaseClass::GetRenderOrigin(); 
	//if( m_bCustomLightingOrigin )
	//	return BaseClass::GetRenderOrigin() + m_vCustomLightingOrigin;
	//return BaseClass::GetRenderOrigin(); 
}

#if 0
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CUnitBase::SetupBones( matrix3x4a_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime )
{
	// Skip if not in screen. SetupBones is really expensive and we shouldn't need it if not in screen.
	// Note: Don't do this if pBoneToWorldOut is not NULL, since some functions like ragdolls expect output.
	C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
	if ( !pBoneToWorldOut && pPlayer && !pPlayer->GetControlledUnit() )
	{
		int iX, iY;
		bool bResult = GetVectorInScreenSpace( GetAbsOrigin(), iX, iY );
		if ( !bResult || iX < -320 || iX > ScreenWidth()+320 || iY < -320 || iY > ScreenHeight() + 320 )
			return false;
	}
	return BaseClass::SetupBones( pBoneToWorldOut, nMaxBones, boneMask, currentTime );
}
#endif // 0

//-----------------------------------------------------------------------------
// Purpose: Makes the unit play the blink effect (highlights the unit)
//-----------------------------------------------------------------------------
void CUnitBase::Blink( float blink_time )
{
	m_bIsBlinking = true;
	m_fBlinkTimeOut = blink_time != -1 ? gpGlobals->curtime + blink_time : -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CUnitBase::ShouldDraw( void )
{
	if( GetCommander() && GetCommander() == C_HL2WarsPlayer::GetLocalHL2WarsPlayer() )
	{
		if( !input->CAM_IsThirdPerson() )
			return false;
	}
	return BaseClass::ShouldDraw();
}

ConVar cl_unit_interp_disable( "cl_unit_interp_disable", "0.1", FCVAR_NONE, "" );
ConVar cl_unit_interp_rate( "cl_unit_interp_rate", "0.1", FCVAR_NONE, "Interpolation for units client side." );

#define round(x) ((x)>=0)?(int)((x)+0.5):(int)((x)-0.5)

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::UpdateClientSideAnimation()
{
#if 1
	if( m_fNextSimulationUpdate < gpGlobals->curtime )
	{
		m_flOldSimulationTime = m_flSimulationTime;
		m_flSimulationTime = gpGlobals->curtime;
		OnLatchInterpolatedVariables( LATCH_SIMULATION_VAR );

		//float w = (m_fNextSimulationUpdate - m_fSimulationBaseClock) / cl_unit_interp_rate.GetFloat();
		//float wnext = round( w );
		//float nextw = (wnext - w);
		//if( nextw < 0 )
			m_fNextSimulationUpdate = gpGlobals->curtime + cl_unit_interp_rate.GetFloat();
		//else
		//	m_fNextSimulationUpdate = gpGlobals->curtime + cl_unit_interp_rate.GetFloat() * (wnext - w);
		//Msg("Calculated next update: %f\n", cl_unit_interp_rate.GetFloat() * (wnext - w));
	}
#endif // 0

	if( m_bUpdateClientAnimations )
	{
		// Yaw and Pitch are updated in UserCmd if the unit has a commander
		if( !GetCommander() )
		{
			if( GetActiveWeapon() )
			{
				AimGun();
			}
			else
			{
				m_fEyePitch = EyeAngles()[PITCH];
				m_fEyeYaw = EyeAngles()[YAW];
			}
		}

		if( GetSequence() != -1 )
			FrameAdvance(gpGlobals->frametime);

		if( m_pAnimState )
		{
			m_pAnimState->Update(m_fEyeYaw, m_fEyePitch);
		}
	}

	if( GetSequence() != -1 )
		OnLatchInterpolatedVariables((1<<0));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::InitPredictable( C_BasePlayer *pOwner )
{
	SetLocalVelocity(vec3_origin);
	BaseClass::InitPredictable( pOwner );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::PostDataUpdate( DataUpdateType_t updateType )
{
	// Base simulation time on when the origin changed
	bool originChanged = ( m_vecOldUnitOrigin != GetLocalOrigin() ) ? true : false;
	if( originChanged )
	{
		m_flOldSimulationTime = m_flSimulationTime;
		m_flSimulationTime = gpGlobals->curtime;
		m_fNextSimulationUpdate = gpGlobals->curtime + cl_unit_interp_rate.GetFloat();

		m_vecOldUnitOrigin = GetLocalOrigin();
	}

	bool bPredict = ShouldPredict();
	if ( bPredict )
	{
		SetSimulatedEveryTick( true );
	}
	else
	{
		SetSimulatedEveryTick( false );

		// estimate velocity for non local players
		float flTimeDelta = m_flSimulationTime - m_flOldSimulationTime;
		if ( flTimeDelta > 0  && !IsEffectActive(EF_NOINTERP) )
		{
			Vector newVelo = (GetNetworkOrigin() - GetOldOrigin()  ) / flTimeDelta;
			SetAbsVelocity( newVelo);
		}
	}

	// if player has switched into this marine, set it to be prediction eligible
	if (bPredict)
	{
		// C_BaseEntity assumes we're networking the entity's angles, so pretend that it
		// networked the same value we already have.
		//SetNetworkAngles( GetLocalAngles() );
		SetPredictionEligible( true );
	}
	else
	{
		SetPredictionEligible( false );
	}

	BaseClass::PostDataUpdate( updateType );

	if ( GetPredictable() && !bPredict )
	{
		MDLCACHE_CRITICAL_SECTION();
		ShutdownPredictable();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::NotePredictionError( const Vector &vDelta )
{
	Vector vOldDelta;

	GetPredictionErrorSmoothingVector( vOldDelta );

	// sum all errors within smoothing time
	m_vecPredictionError = vDelta + vOldDelta;

	// remember when last error happened
	m_flPredictionErrorTime = gpGlobals->curtime;
 
	ResetLatched(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::GetPredictionErrorSmoothingVector( Vector &vOffset )
{
	if ( engine->IsPlayingDemo() || !cl_wars_smooth.GetInt() || !cl_predict->GetBool() )
	{
		vOffset.Init();
		return;
	}

	float errorAmount = ( gpGlobals->curtime - m_flPredictionErrorTime ) / cl_wars_smoothtime.GetFloat();

	if ( errorAmount >= 1.0f )
	{
		vOffset.Init();
		return;
	}
	
	errorAmount = 1.0f - errorAmount;

	vOffset = m_vecPredictionError * errorAmount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BasePlayer *CUnitBase::GetPredictionOwner()
{
	return GetCommander();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CUnitBase::ShouldPredict( void )
{
	if (C_BasePlayer::IsLocalPlayer(GetCommander()))
	{
		FOR_EACH_VALID_SPLITSCREEN_PLAYER( hh )
		{
			ACTIVE_SPLITSCREEN_PLAYER_GUARD( hh );

			C_HL2WarsPlayer* player = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
			if (player && player->GetControlledUnit() == this)
			{
				return true;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUnitBase::EstimateAbsVelocity( Vector& vel )
{
#if 1
	// FIXME: Unit velocity doesn't seems correct
	if( ShouldPredict() )
	{
		vel = GetAbsVelocity();
		return;
	}
	return BaseClass::EstimateAbsVelocity(vel);
#else
	vel = GetAbsVelocity();
#endif // 0
}

#if 0
// units require extra interpolation time due to think rate
ConVar cl_unit_extra_interp( "cl_unit_extra_interp", "0.0", FCVAR_NONE, "Extra interpolation for units." );

float CUnitBase::GetInterpolationAmount( int flags )
{
	return BaseClass::GetInterpolationAmount( flags ) + cl_unit_extra_interp.GetFloat();
}
#endif // 0 