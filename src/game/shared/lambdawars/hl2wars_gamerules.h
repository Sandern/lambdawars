//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: The TF Game rules object
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef HL2WARS_GAMERULES_H
#define HL2WARS_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif


#include "teamplay_gamerules.h"
#include "convar.h"
#include "gamevars_shared.h"

#ifdef CLIENT_DLL
	#include "c_baseplayer.h"
#else
	#include "player.h"
	#include "hl2wars_player.h"
	#include "utlqueue.h"
	#include "playerclass_info_parse.h"

#endif


#ifdef CLIENT_DLL
	#define CHL2WarsGameRules C_HL2WarsGameRules
	#define CHL2WarsGameRulesProxy C_HL2WarsGameRulesProxy
#endif


class CHL2WarsGameRulesProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CHL2WarsGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CHL2WarsViewVectors : public CViewVectors
{
public:
	CHL2WarsViewVectors( 
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight
#if defined ( HL2Wars_USE_PRONE )
		,Vector vProneHullMin,
		Vector vProneHullMax,
		Vector vProneView
#endif
		) :
			CViewVectors( 
				vView,
				vHullMin,
				vHullMax,
				vDuckHullMin,
				vDuckHullMax,
				vDuckView,
				vObsHullMin,
				vObsHullMax,
				vDeadViewHeight )
	{
#if defined( HL2Wars_USE_PRONE )
		m_vProneHullMin = vProneHullMin;
		m_vProneHullMax = vProneHullMax;
		m_vProneView = vProneView;
#endif 
	}
#if defined ( HL2Wars_USE_PRONE )
	Vector m_vProneHullMin;
	Vector m_vProneHullMax;	
	Vector m_vProneView;
#endif
};

class CHL2WarsGameRules : public CTeamplayRules
{
public:
	DECLARE_CLASS( CHL2WarsGameRules, CTeamplayRules );

	virtual bool IsTeamplay( void ) { return false; }   // is this deathmatch game being played with team rules?

	virtual const unsigned char *GetEncryptionKey( void ) { return (unsigned char *)"a1b2c3d4"; }
	
#ifdef ENABLE_PYTHON
#ifndef CLIENT_DLL
	virtual void InitGamerules();
	virtual void ShutdownGamerules();
#endif // CLIENT_DLL
#endif // ENABLE_PYTHON

	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.
	
	CHL2WarsGameRules();
	virtual ~CHL2WarsGameRules();

	void UpdateVoiceManager();

	void InitTeams( void );

	virtual const char *GetPlayerClassname() { return NULL; }

	// CBaseEntity overrides.
public:

	// Called when game rules are destroyed by CWorld
	virtual void LevelShutdown( void ) { return; }

	virtual void Precache( void );

	// Functions to verify the single/multiplayer status of a game
	virtual const char *GetGameDescription( void ) { return "Lambda Wars"; } 

	virtual void			OnServerHibernating() {}

	// Client connection/disconnection
	virtual bool ClientConnected( edict_t *pEntity, const char *name, const char *address, char *reject, int maxrejectlen ); // a client just connected to the server (player hasn't spawned yet)
	virtual bool PyClientConnected( int clientindex, const char *name, const char *address, char *reject, int maxrejectlen ) { return true; }
	virtual void ClientDisconnected( edict_t *client ); // a client just disconnected from the server
	virtual void PyClientDisconnected( CBasePlayer *client ) {}
	virtual void ClientActive( CBasePlayer *client ) {}

	// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *player );     // called by CBasePlayer::Spawn just before releasing player into the game

	//virtual bool AllowAutoTargetCrosshair( void ) { return BaseClass::AllowAutoTargetCrosshair(); }
	virtual bool ClientCommand( CBaseEntity *edict, const CCommand &args );  // handles the user commands;  returns TRUE if command handled properly

	// Informs about player changes
	virtual void PlayerChangedOwnerNumber( CBasePlayer *player, int oldownernumber, int newownernumber ) {}

	// ..
	void CreateStandardEntities( void );

	virtual const char *GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer );
	virtual const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );

	int SelectDefaultTeam( void );

#ifdef ENABLE_PYTHON
	// ...
	virtual boost::python::object GetNextLevelName( bool bRandom = false );
#endif // ENABLE_PYTHON

	float GetIntermissionEndTime() { return m_flIntermissionEndTime; }
	void SetintermissionEndTime( float endtime ) { m_flIntermissionEndTime = endtime; }

	bool GetGameOver() { return g_fGameOver; }
	void SetGameOver( bool gameover ) { g_fGameOver = gameover; }

private:
	void DestroyTeams();

private:
	bool m_bLevelInitialized;

	CUtlVector< EHANDLE > m_Teams; // List of team entities created by this gamerules.

#endif

	// Damage rules for ammo types
	virtual float GetAmmoDamage( CBaseEntity *pAttacker, CBaseEntity *pVictim, int nAmmoType );

public:
	float GetMapElapsedTime();		// How much time has elapsed since the map started.
};

//-----------------------------------------------------------------------------
// Gets us at the team fortress game rules
//-----------------------------------------------------------------------------

inline CHL2WarsGameRules* HL2WarsGameRules()
{
	return static_cast<CHL2WarsGameRules*>(g_pGameRules);
}


#endif // HL2WARS_GAMERULES_H
