//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "player_command.h"
#include "igamemovement.h"
#include "in_buttons.h"
#include "ipredictionsystem.h"
#include "hl2wars_player.h"
#include "iservervehicle.h"
#include "tier0/vprof.h"
#include "unit_base_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CMoveData g_MoveData;
CMoveData *g_pMoveData = &g_MoveData;
extern IGameMovement *g_pGameMovement;

IPredictionSystem *IPredictionSystem::g_pPredictionSystems = NULL;

//-----------------------------------------------------------------------------
// Sets up the move data for TF2
//-----------------------------------------------------------------------------
class CHL2WarsPlayerMove : public CPlayerMove
{
DECLARE_CLASS( CHL2WarsPlayerMove, CPlayerMove );

public:
	virtual void	SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void	RunCommand( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *moveHelper );
	virtual void	FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move );
};

// PlayerMove Interface
static CHL2WarsPlayerMove g_PlayerMove;

//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
CPlayerMove *PlayerMove()
{
	return &g_PlayerMove;
}


//-----------------------------------------------------------------------------
// Purpose: This is called pre player movement and copies all the data necessary
//          from the player for movement. (Server-side, the client-side version
//          of this code can be found in prediction.cpp.)
//-----------------------------------------------------------------------------
void CHL2WarsPlayerMove::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	player->AvoidPhysicsProps( ucmd );

	BaseClass::SetupMove( player, ucmd, pHelper, move );

	// Teleport
	if( ucmd->directmove )
	{
		trace_t tr;
		g_pMoveData->SetAbsOrigin( ucmd->vecmovetoposition );
	}

	// this forces horizontal movement
	/*CHL2WarsPlayer *pWarsPlayer = dynamic_cast<CHL2WarsPlayer*>(player);
	if (pWarsPlayer && pWarsPlayer->GetControlledUnit() )
	{
		move->m_vecAngles.x = 0;
		move->m_vecViewAngles.x = 0;

		CBaseEntity *pMoveParent = player->GetMoveParent();
		if (!pMoveParent)
		{
			move->m_vecAbsViewAngles = move->m_vecViewAngles;
		}
		else
		{
			matrix3x4_t viewToParent, viewToWorld;
			AngleMatrix( move->m_vecViewAngles, viewToParent );
			ConcatTransforms( pMoveParent->EntityToWorldTransform(), viewToParent, viewToWorld );
			MatrixAngles( viewToWorld, move->m_vecAbsViewAngles );
		}
	}*/
	//CASW_MoveData *pASWMove = static_cast<CASW_MoveData*>( move );
	//pASWMove->m_iForcedAction = ucmd->forced_action;

	// setup trace optimization
	g_pGameMovement->SetupMovementBounds( move );
}

extern void DiffPrint( bool bServer, int nCommandNumber, char const *fmt, ... );

void CHL2WarsPlayerMove::RunCommand( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *moveHelper )
{
	CHL2WarsPlayer *pWarsPlayer = static_cast<CHL2WarsPlayer*>( player );
	Assert( pWarsPlayer );

	StartCommand( player, ucmd );

	// Set globals appropriately
	gpGlobals->curtime		=  player->m_nTickBase * TICK_INTERVAL;
	gpGlobals->frametime	=  player->m_bGamePaused ? 0 : TICK_INTERVAL;

	// Add and subtract buttons we're forcing on the player
	ucmd->buttons |= player->m_afButtonForced;
	ucmd->buttons &= ~player->m_afButtonDisabled;

	if ( player->m_bGamePaused )
	{
		// If no clipping and cheats enabled and noclipduring game enabled, then leave
		//  forwardmove and angles stuff in usercmd
		if ( player->GetMoveType() == MOVETYPE_NOCLIP &&
			sv_cheats->GetBool() )// && 
			//sv_noclipduringpause.GetBool() )
		{
			gpGlobals->frametime = TICK_INTERVAL;
		}
	}

	/*
	// TODO:  We can check whether the player is sending more commands than elapsed real time
	cmdtimeremaining -= ucmd->msec;
	if ( cmdtimeremaining < 0 )
	{
	//	return;
	}
	*/

	g_pGameMovement->StartTrackPredictionErrors( player );

	//CommentarySystem_PePlayerRunCommand( player, ucmd );

	// Do weapon selection
	/*if ( ucmd->weaponselect != 0 )
	{
		CBaseCombatWeapon *weapon = dynamic_cast< CBaseCombatWeapon * >( CBaseEntity::Instance( ucmd->weaponselect ) );
		if (weapon)
			pWarsPlayer->ASWSelectWeapon(weapon, 0); // ucmd->weaponsubtype);  // infested - subtype var used for sending marine profile index instead
	}*/

	IServerVehicle *pVehicle = player->GetVehicle();

	// Latch in impulse.
	if ( ucmd->impulse )
	{
		// Discard impulse commands unless the vehicle allows them.
		// FIXME: UsingStandardWeapons seems like a bad filter for this. The flashlight is an impulse command, for example.
		if ( !pVehicle || player->UsingStandardWeaponsInVehicle() )
		{
			player->m_nImpulse = ucmd->impulse;
		}
	}

	// Update player input button states
	VPROF_SCOPE_BEGIN( "player->UpdateButtonState" );
	player->UpdateButtonState( ucmd->buttons );
	VPROF_SCOPE_END();

	CheckMovingGround( player, TICK_INTERVAL );

	g_pMoveData->m_vecOldAngles = player->pl.v_angle;

	// Copy from command to player unless game .dll has set angle using fixangle
	if ( player->pl.fixangle == FIXANGLE_NONE )
	{
		player->pl.v_angle = ucmd->viewangles;
	}
	else if( player->pl.fixangle == FIXANGLE_RELATIVE )
	{
		player->pl.v_angle = ucmd->viewangles + player->pl.anglechange;
	}

	// TrackIR
	//player->SetEyeAngleOffset(ucmd->headangles);
	//player->SetEyeOffset(ucmd->headoffset);
	// TrackIR

	// Call standard client pre-think
	RunPreThink( player );

	// Call Think if one is set
	RunThink( player, TICK_INTERVAL );

	// Setup input.
	SetupMove( player, ucmd, moveHelper, g_pMoveData );

	// Let the game do the movement.
	if( pWarsPlayer && !pWarsPlayer->GetControlledUnit() )
	{
		if ( !pVehicle )
		{
			VPROF( "g_pGameMovement->ProcessMovement()" );
			Assert( g_pGameMovement );
			g_pGameMovement->ProcessMovement( player, g_pMoveData );
		}
		else
		{
			VPROF( "pVehicle->ProcessMovement()" );
			pVehicle->ProcessMovement( player, g_pMoveData );
		}
	}

	// Copy output
	FinishMove( player, ucmd, g_pMoveData );

	// Let server invoke any needed impact functions
	VPROF_SCOPE_BEGIN( "moveHelper->ProcessImpacts" );
	moveHelper->ProcessImpacts();
	VPROF_SCOPE_END();

	// put lag compensation here so it affects weapons
	//lagcompensation->StartLagCompensation( player, LAG_COMPENSATE_BOUNDS );
	RunPostThink( player );
	//lagcompensation->FinishLagCompensation( player );

	// let the player drive marine movement here
	//pWarsPlayer->DriveMarineMovement( ucmd, moveHelper );
	if( pWarsPlayer->GetControlledUnit() )
		pWarsPlayer->GetControlledUnit()->GetIUnit()->UserCmd( ucmd );

	g_pGameMovement->FinishTrackPredictionErrors( player );

	FinishCommand( player );

	// Let time pass
	player->m_nTickBase++;
}

//-----------------------------------------------------------------------------
// Purpose: This is called post player movement to copy back all data that
//          movement could have modified and that is necessary for future
//          movement. (Server-side, the client-side version of this code can 
//          be found in prediction.cpp.)
//-----------------------------------------------------------------------------
void CHL2WarsPlayerMove::FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move )
{
	// Call the default FinishMove code.
	BaseClass::FinishMove( player, ucmd, move );

	/*IServerVehicle *pVehicle = player->GetVehicle();
	if (pVehicle && gpGlobals->frametime != 0)
	{
		pVehicle->FinishMove( player, ucmd, move );
	}*/

	// Strategic mode specific stuff
	// Don't filter. Always update.
	CHL2WarsPlayer *pPlayer = ToHL2WarsPlayer(player);
	if( !pPlayer ) //|| pPlayer->IsStrategicModeOn() == false )
		return;

	// Copy mouse aim
	pPlayer->SetCameraOffset( ucmd->m_vCameraOffset );
	pPlayer->UpdateMouseData( ucmd->m_vMouseAim );
}