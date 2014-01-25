//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "prediction.h"
#include "c_baseplayer.h"
#include "igamemovement.h"
#include "c_hl2wars_player.h"
#include "unit_base_shared.h"
#include "IClientVehicle.h"
#include "tier0/vprof.h"
#include "prediction_private.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CMoveData g_MoveData;
CMoveData *g_pMoveData = &g_MoveData;
extern IGameMovement *g_pGameMovement;

//extern bool StrategicValidMove( const Vector &vStart, const Vector &vDestination, CBaseTrace *pTrace  );

class CHL2WarsPrediction : public CPrediction
{
DECLARE_CLASS( CHL2WarsPrediction, CPrediction );

public:
	CHL2WarsPrediction();

	virtual void	SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void	RunCommand( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *moveHelper );
	virtual void	FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move );

	virtual void	CheckError( int nSlot, C_BasePlayer *player, int commands_acknowledged );
	virtual void	CheckUnitError( int nSlot, int commands_acknowledged );

protected:

	bool			m_bUnitOriginTypedescriptionSearched;
	CUtlVector< const typedescription_t * >	m_UnitOriginTypeDescription; // A vector in cases where the .x, .y, and .z are separately listed

};

CHL2WarsPrediction::CHL2WarsPrediction() 
	: m_bUnitOriginTypedescriptionSearched(false)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsPrediction::CheckError( int nSlot, C_BasePlayer *player, int commands_acknowledged )
{
#if !defined( NO_ENTITY_PREDICTION )
	// Infesed - check marine prediction error as well as player's:
	CheckUnitError(nSlot, commands_acknowledged);
#endif

	BaseClass::CheckError( nSlot, player, commands_acknowledged );
}

void CHL2WarsPrediction::CheckUnitError( int nSlot, int commands_acknowledged )
{
	C_HL2WarsPlayer	*player;
	Vector		origin;
	Vector		delta;
	float		len;
	static int	pos = 0;

	// Not in the game yet
	if ( !engine->IsInGame() )
		return;

	// Not running prediction
	if ( !cl_predict->GetInt() )
		return;

	player = C_HL2WarsPlayer::GetLocalHL2WarsPlayer( nSlot );
	if ( !player )
		return;

	C_UnitBase* pUnit = dynamic_cast<C_UnitBase *>(player->GetControlledUnit());
	if (!pUnit)
		return;

	// Not predictable yet (flush entity packet?)
	if ( !pUnit->IsIntermediateDataAllocated() )
		return;

	origin = pUnit->GetNetworkOrigin();

	const void *slot = pUnit->GetPredictedFrame( commands_acknowledged - 1 );
	if ( !slot )
		return;

	if ( !m_bUnitOriginTypedescriptionSearched )
	{
		m_bUnitOriginTypedescriptionSearched = true;
		const typedescription_t *td = CPredictionCopy::FindFlatFieldByName( "m_vecNetworkOrigin", pUnit->GetPredDescMap() );
		if ( td ) 
		{
			m_UnitOriginTypeDescription.AddToTail( td );
		}
	}

	if ( !m_UnitOriginTypeDescription.Count() )
		return;

	Vector predicted_origin;

	memcpy( (Vector *)&predicted_origin, (Vector *)( (byte *)slot + m_UnitOriginTypeDescription[ 0 ]->flatOffset[ TD_OFFSET_PACKED ] ), sizeof( Vector ) );

	// Compare what the server returned with what we had predicted it to be
	VectorSubtract ( predicted_origin, origin, delta );

	len = VectorLength( delta );
	if (len > MAX_PREDICTION_ERROR )
	{	
		// A teleport or something, clear out error
		len = 0;
	}
	else
	{
		if ( len > MIN_PREDICTION_EPSILON )
		{
			pUnit->NotePredictionError( delta );

#if 0
			if ( cl_showerror.GetInt() >= 1 )
			{
				con_nprint_t np;
				np.fixed_width_font = true;
				np.color[0] = 1.0f;
				np.color[1] = 0.95f;
				np.color[2] = 0.7f;
				np.index = 20 + ( ++pos % 20 );
				np.time_to_live = 2.0f;

				engine->Con_NXPrintf( &np, "marine pred error %6.3f units (%6.3f %6.3f %6.3f)", len, delta.x, delta.y, delta.z );
			}
#endif // 0
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsPrediction::SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, 
	CMoveData *move )
{
	// Call the default SetupMove code.
	BaseClass::SetupMove( player, ucmd, pHelper, move );

	//C_HL2WarsPlayer *pWarsPlayer = (C_HL2WarsPlayer*)player;
	//Assert( pWarsPlayer );

	// Teleport
	if( ucmd->directmove )
	{
		g_pMoveData->SetAbsOrigin( ucmd->vecmovetoposition );
	}
	
	/*C_HL2WarsPlayer *pWarsPlayer = static_cast<C_HL2WarsPlayer*>( player );
	if ( !asw_allow_detach.GetBool() )
	{		
		if ( pWarsPlayer && pWarsPlayer->GetControlledUnit() )
		{
			// this forces horizontal movement
			move->m_vecAngles.x = 0;
			move->m_vecViewAngles.x = 0;
		}
	}*/

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
	//CASW_MoveData *pASWMove = static_cast<CASW_MoveData*>( move );
	//pASWMove->m_iForcedAction = ucmd->forced_action;

	// setup trace optimization
	g_pGameMovement->SetupMovementBounds( move );
}

extern void DiffPrint( bool bServer, int nCommandNumber, char const *fmt, ... );

//-----------------------------------------------------------------------------
// Purpose: Predicts a single movement command for player
// Input  : *moveHelper - 
//			*player - 
//			*u - 
//-----------------------------------------------------------------------------
void CHL2WarsPrediction::RunCommand( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *moveHelper )
{
#if !defined( NO_ENTITY_PREDICTION )
	VPROF( "CPrediction::RunCommand" );
#if defined( _DEBUG )
	char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "runcommand%04d", ucmd->command_number );
	PREDICTION_TRACKVALUECHANGESCOPE( sz );
#endif

	C_HL2WarsPlayer *pWarsPlayer = (C_HL2WarsPlayer*)player;
	Assert( pWarsPlayer );

	StartCommand( player, ucmd );

	// Set globals appropriately
	gpGlobals->curtime		= player->m_nTickBase * TICK_INTERVAL;
	gpGlobals->frametime	= TICK_INTERVAL;
	
	g_pGameMovement->StartTrackPredictionErrors( player );

	// TODO
	// TODO:  Check for impulse predicted?

	// Do weapon selection
	/*
	if ( ucmd->weaponselect != 0 )
	{
		C_BaseCombatWeapon *weapon = ToBaseCombatWeapon( CBaseEntity::Instance( ucmd->weaponselect ) );
		if (weapon)
		{
			pWarsPlayer->ASWSelectWeapon(weapon, 0); //ucmd->weaponsubtype);		// asw - subtype var used for sending marine profile index instead
		}
	}*/

	// Latch in impulse.
	IClientVehicle *pVehicle = player->GetVehicle();
	if ( ucmd->impulse )
	{
		// Discard impulse commands unless the vehicle allows them.
		// FIXME: UsingStandardWeapons seems like a bad filter for this. 
		// The flashlight is an impulse command, for example.
		if ( !pVehicle || player->UsingStandardWeaponsInVehicle() )
		{
			player->m_nImpulse = ucmd->impulse;
		}
	}

	// Get button states
	player->UpdateButtonState( ucmd->buttons );

	// TODO
	//	CheckMovingGround( player, ucmd->frametime );

	// TODO
	//	g_pMoveData->m_vecOldAngles = player->pl.v_angle;

	// Copy from command to player unless game .dll has set angle using fixangle
	// if ( !player->pl.fixangle )
	{
		player->SetLocalViewAngles( ucmd->viewangles );
	}

	// Call standard client pre-think
	RunPreThink( player );

	// Call Think if one is set
	RunThink( player, TICK_INTERVAL );

	// Setup input.
	{

		SetupMove( player, ucmd, moveHelper, g_pMoveData );
	}

	// Run regular player movement if we're not controlling a marine
	if ( !pWarsPlayer->GetControlledUnit() )
	{
		if ( !pVehicle )
		{
			Assert( g_pGameMovement );
			g_pGameMovement->ProcessMovement( player, g_pMoveData );
		}
		else
		{
			pVehicle->ProcessMovement( player, g_pMoveData );
		}
	}

// 	if ( !asw_allow_detach.GetBool() && pWarsPlayer->GetMarine() )
// 	{
// 		g_pMoveData->SetAbsOrigin( pWarsPlayer->GetMarine()->GetAbsOrigin() );
// 	}

	FinishMove( player, ucmd, g_pMoveData );

	RunPostThink( player );

	// let the player drive marine movement here
	if( pWarsPlayer->GetControlledUnit() )
		pWarsPlayer->GetControlledUnit()->GetIUnit()->UserCmd( ucmd );

	g_pGameMovement->FinishTrackPredictionErrors( player );	

	FinishCommand( player );

	player->m_nTickBase++;
#endif
}

void CHL2WarsPrediction::FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move )
{
	// Call the default FinishMove code.
	BaseClass::FinishMove( player, ucmd, move );
}

// Expose interface to engine
static CHL2WarsPrediction g_Prediction;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CHL2WarsPrediction, IPrediction, VCLIENT_PREDICTION_INTERFACE_VERSION, g_Prediction );

CPrediction *prediction = &g_Prediction;

