//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*

===== tf_client.cpp ========================================================

  HL2 client/server game specific stuff

*/

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "entitylist.h"
#include "physics.h"
#include "game.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "shake.h"
#include "player_resource.h"
#include "engine/IEngineSound.h"
#include "hl2wars_player.h"
#include "hl2wars_gamerules.h"
#include "tier0/vprof.h"

#ifdef ENABLE_PYTHON
	#include "srcpy.h"
	#include "srcpy_gamerules.h"
#endif // ENABLE_PYTHON

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );

extern bool			g_fGameOver;


void FinishClientPutInServer( CHL2WarsPlayer *pPlayer )
{
	pPlayer->InitialSpawn();
	pPlayer->Spawn();

	char sName[128];
	Q_strncpy( sName, pPlayer->GetPlayerName(), sizeof( sName ) );
	
	// First parse the name and remove any %'s
	for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
	{
		// Replace it with a space
		if ( *pApersand == '%' )
				*pApersand = ' ';
	}

	// notify other clients of player joining the game
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_connected", sName[0] != 0 ? sName : "<unconnected>" );
}

/*
===========
ClientPutInServer

called each time a player is spawned into the game
============
*/
//#include "inetchannel.h"
void ClientPutInServer( edict_t *pEdict, const char *playername )
{
	// Allocate a CBaseTFPlayer for pev, and call spawn
	const char *pClassname = "player";
	if( HL2WarsGameRules() )
	{
		const char *pGPClassname = HL2WarsGameRules()->GetPlayerClassname();
		if( pGPClassname )
			pClassname = pGPClassname;
	}

	CHL2WarsPlayer *pPlayer = CHL2WarsPlayer::CreatePlayer( pClassname, pEdict );
	if( !pPlayer )
	{
		pPlayer = CHL2WarsPlayer::CreatePlayer( "player", pEdict );
		Warning("ClientPutInServer: Failed to spawn entity %s. Falling back to player.\n", pClassname);
	}
	pPlayer->SetPlayerName( playername );

	/*
	INetChannelInfo *nci = engine->GetPlayerNetInfo(pPlayer->entindex());
	INetChannel *nc = (INetChannel *)nci;
	Msg("Buffer size before: %d\n", nc->GetBufferSize());
	nc->SetMaxBufferSize(true, 1000000000);
	Msg("Buffer size after: %d\n", nc->GetBufferSize());
	*/
}

void ClientActive( edict_t *pEdict, bool bLoadGame )
{
	// Can't load games in CS!
	Assert( !bLoadGame );

	CHL2WarsPlayer *pPlayer = ToHL2WarsPlayer( CBaseEntity::Instance( pEdict ) );
	FinishClientPutInServer( pPlayer );

#ifdef ENABLE_PYTHON
	// Give a full update of the networked python entities
	// NOTE: Only dedicated servers and the listened host. Listened and clients are done in ClientConnect
	if( engine->IsDedicatedServer() || ENTINDEX(pEdict) > 1 )
	{
		FullClientUpdatePyNetworkCls( pPlayer );
	}

#ifdef CLIENT_DLL
	char pLevelName[_MAX_PATH];
	Q_FileBase(engine->GetLevelName(), pLevelName, _MAX_PATH);
#else
	const char *pLevelName = STRING(gpGlobals->mapname);
#endif

	if( SrcPySystem()->IsPythonRunning() )
	{
		// Notify gamemgr (before other client active's)
		boost::python::object ca;
		try {
			ca = SrcPySystem()->Get("_ClientActive", "gamemgr");
			ca(*pPlayer);
		} catch(...) {
			PyErr_Print();
			PyErr_Clear();
		}

		// Send clientactive signal
		try {
			bp::dict kwargs;
			kwargs["sender"] = bp::object();
			kwargs["client"] = pPlayer->GetPyHandle();
			bp::object signal = SrcPySystem()->Get("clientactive", "core.signals", true);
			SrcPySystem()->CallSignal( signal, kwargs );
			//SrcPySystem()->SignalCheckResponses( PyEval_CallObjectWithKeywords( method.ptr(), NULL, kwargs.ptr() ) );

			signal = SrcPySystem()->Get("map_clientactive", "core.signals", true)[pLevelName];
			SrcPySystem()->CallSignal( signal, kwargs );
			//SrcPySystem()->SignalCheckResponses( PyEval_CallObjectWithKeywords( method.ptr(), NULL, kwargs.ptr() ) );
		} catch( bp::error_already_set & ) {
			Warning("Failed to retrieve clientactive signal:\n");
			PyErr_Print();
		}
	}
#endif // ENABLE_PYTHON

	// Notify gamerules
	if( HL2WarsGameRules() )
		HL2WarsGameRules()->ClientActive(pPlayer);
}

void ClientFullyConnect( edict_t *pEntity )
{

}


/*
===============
const char *GetGameDescription()

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription()
{
	if ( g_pGameRules ) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "Half-Life 2: Wars";
}


//-----------------------------------------------------------------------------
// Purpose: Precache game-specific models & sounds
//-----------------------------------------------------------------------------
void ClientGamePrecache( void )
{
	// Materials used by the client effects
	CBaseEntity::PrecacheModel( "sprites/white.vmt" );
	CBaseEntity::PrecacheModel( "sprites/physbeam.vmt" );
}


// called by ClientKill and DeadThink
void respawn( CBaseEntity *pEdict, bool fCopyCorpse )
{
	if (gpGlobals->coop || gpGlobals->deathmatch)
	{
		if ( fCopyCorpse )
		{
			// make a copy of the dead body for appearances sake
			dynamic_cast< CBasePlayer* >( pEdict )->CreateCorpse();
		}

		// respawn player
		pEdict->Spawn();
	}
	else
	{       // restart the entire server
		engine->ServerCommand("reload\n");
	}
}
void GameStartFrame( void )
{
	VPROF( "GameStartFrame" );

	if ( g_pGameRules )
		g_pGameRules->Think();

	if ( g_fGameOver )
		return;

	gpGlobals->teamplay = false;

	//extern void Bot_RunAll();
	//Bot_RunAll();
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
void InstallGameRules()
{
#ifdef ENABLE_PYTHON
	// Install python gamerules
	if( SrcPySystem()->IsPythonRunning() )
	{
		boost::python::object liig;
		try {
			liig = SrcPySystem()->Get("_LevelInitInstallGamerules", "core.gamerules.info");
			liig();
		} catch(...) {
			PyErr_Print();
			PyErr_Clear();
		}
	}
#endif // ENABLE_PYTHON

	if( g_pGameRules == NULL )	// Fallback
		CreateGameRulesObject( "CHL2WarsGameRules" );
}

//------------------------------------------------------------------------------
// A small wrapper around SV_Move that never clips against the supplied entity.
//------------------------------------------------------------------------------
static bool TestEntityPosition2 ( CBasePlayer *pPlayer )
{	
	trace_t	trace;
	UTIL_TraceEntity( pPlayer, pPlayer->GetAbsOrigin(), pPlayer->GetAbsOrigin(), MASK_PLAYERSOLID, &trace );
	return (trace.startsolid == 0);
}


//------------------------------------------------------------------------------
// Searches along the direction ray in steps of "step" to see if 
// the entity position is passible.
// Used for putting the player in valid space when toggling off noclip mode.
//------------------------------------------------------------------------------
static int FindPassableSpace2( CBasePlayer *pPlayer, const Vector& direction, float step, Vector& oldorigin )
{
	int i;
	for ( i = 0; i < 100; i++ )
	{
		Vector origin = pPlayer->GetAbsOrigin();
		VectorMA( origin, step, direction, origin );
		pPlayer->SetAbsOrigin( origin );
		if ( TestEntityPosition2( pPlayer ) )
		{
			VectorCopy( pPlayer->GetAbsOrigin(), oldorigin );
			return 1;
		}
	}
	return 0;
}

// Toggle strategic mode
void CC_Player_StrategicMode( void )
{
	if ( !sv_cheats->GetBool() )
		return;

	CHL2WarsPlayer *pPlayer = ToHL2WarsPlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	CPlayerState *pl = pPlayer->PlayerData();
	Assert( pl );

	if (pPlayer->GetMoveType() != MOVETYPE_STRATEGIC)
	{
		pPlayer->SetStrategicMode(true);
		return;
	}

	pPlayer->SetStrategicMode(false);

	Vector oldorigin = pPlayer->GetAbsOrigin();
	ClientPrint( pPlayer, HUD_PRINTCONSOLE, "strategic mode OFF\n");
	if ( !TestEntityPosition2( pPlayer ) )
	{
		Vector forward, right, up;

		AngleVectors ( pl->v_angle, &forward, &right, &up);

		// Try to move into the world
		if ( !FindPassableSpace2( pPlayer, forward, 1, oldorigin ) )
		{
			if ( !FindPassableSpace2( pPlayer, right, 1, oldorigin ) )
			{
				if ( !FindPassableSpace2( pPlayer, right, -1, oldorigin ) )		// left
				{
					if ( !FindPassableSpace2( pPlayer, up, 1, oldorigin ) )	// up
					{
						if ( !FindPassableSpace2( pPlayer, up, -1, oldorigin ) )	// down
						{
							if ( !FindPassableSpace2( pPlayer, forward, -1, oldorigin ) )	// back
							{
								Msg( "Can't find the world\n" );
							}
						}
					}
				}
			}
		}

		pPlayer->SetAbsOrigin( oldorigin );
	}
}

static ConCommand noclip("strategic_mode", CC_Player_StrategicMode, "Toggle. Become a strategic player.", FCVAR_CHEAT);
