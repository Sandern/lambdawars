//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: The TF Game rules 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2wars_gamerules.h"
#include "ammodef.h"
#include "KeyValues.h"
#include "hl2wars_shareddefs.h"

#ifdef CLIENT_DLL

	#include "precache_register.h"
	#include "c_hl2wars_player.h"
	#include "c_playerresource.h"
#else
	#include "voice_gamemgr.h"
	#include "player_resource.h"
	#include "hl2wars_team.h"
#endif

#ifdef ENABLE_PYTHON
	#include "srcpy.h"
	#include <networkstringtabledefs.h>
#endif // ENABLE_PYTHON

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifndef CLIENT_DLL
ConVar sk_plr_dmg_grenade( "sk_plr_dmg_grenade","0");		
#endif

ConVar mm_max_players( "mm_max_players", "16", FCVAR_REPLICATED | FCVAR_CHEAT, "Max players for matchmaking system" );

REGISTER_GAMERULES_CLASS( CHL2WarsGameRules );


BEGIN_NETWORK_TABLE_NOBASE( CHL2WarsGameRules, DT_HL2WarsGameRules )

END_NETWORK_TABLE()


LINK_ENTITY_TO_CLASS( HL2Wars_gamerules, CHL2WarsGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( HL2WarsGameRulesProxy, DT_HL2WarsGameRulesProxy )


#ifdef CLIENT_DLL
	void RecvProxy_HL2WarsGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CHL2WarsGameRules *pRules = HL2WarsGameRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CHL2WarsGameRulesProxy, DT_HL2WarsGameRulesProxy )
		RecvPropDataTable( "HL2Wars_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_HL2WarsGameRules ), RecvProxy_HL2WarsGameRules )
	END_RECV_TABLE()
#else
	void *SendProxy_HL2WarsGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CHL2WarsGameRules *pRules = HL2WarsGameRules();
		Assert( pRules );
		pRecipients->SetAllRecipients();
		return pRules;
	}

	BEGIN_SEND_TABLE( CHL2WarsGameRulesProxy, DT_HL2WarsGameRulesProxy )
		SendPropDataTable( "HL2Wars_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_HL2WarsGameRules ), SendProxy_HL2WarsGameRules )
	END_SEND_TABLE()
#endif

bool CHL2WarsGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		V_swap(collisionGroup0,collisionGroup1);
	}

	// Anything unit collision group + WARS_COLLISION_GROUP_IGNORE_ALL_UNITS will not collide
	if( collisionGroup1 == WARS_COLLISION_GROUP_IGNORE_ALL_UNITS && 
			( collisionGroup0 >= WARS_COLLISION_GROUP_IGNORE_UNIT_START && collisionGroup0 <= WARS_COLLISION_GROUP_IGNORE_ALL_UNITS ) )
		return false;

	// Anything unit collision group + WARS_COLLISION_GROUP_IGNORE_ALL_UNITS
	// + WARS_COLLISION_GROUP_BUILDING + WARS_COLLISION_GROUP_IGNORE_ALL_UNITS_AND_BUILD will not collide
	if( collisionGroup1 == WARS_COLLISION_GROUP_IGNORE_ALL_UNITS_AND_BUILD &&
			( collisionGroup0 >= WARS_COLLISION_GROUP_IGNORE_UNIT_START && collisionGroup0 <= WARS_COLLISION_GROUP_IGNORE_ALL_UNITS_AND_BUILD ) )
		return false;

	// If collision group is one of the ignore groups, and the other group one of the unit groups, then don't collide
	if( collisionGroup0 >= WARS_COLLISION_GROUP_IGNORE_UNIT_START && collisionGroup0 <= WARS_COLLISION_GROUP_IGNORE_UNIT_END )
	{
		if( collisionGroup1 >= WARS_COLLISION_GROUP_UNIT_START && collisionGroup1 <= WARS_COLLISION_GROUP_UNIT_END )
		{
			if( collisionGroup0 == collisionGroup1-WARS_COLLISION_SUPPORTED_UNITS-1 )
				return false;
		}
		else if( collisionGroup1 >= WARS_COLLISION_GROUP_IGNORE_UNIT_START && collisionGroup1 <= WARS_COLLISION_GROUP_IGNORE_UNIT_END )
		{
			if( collisionGroup0 == collisionGroup1 )
				return false;
		}
		collisionGroup0 = COLLISION_GROUP_NPC;
	}

	// Revert to npc collision group
	if( collisionGroup1 >= WARS_COLLISION_GROUP_IGNORE_UNIT_START && collisionGroup1 <= WARS_COLLISION_GROUP_UNIT_END )
		collisionGroup1 = COLLISION_GROUP_NPC;
	if( collisionGroup0 >= WARS_COLLISION_GROUP_IGNORE_UNIT_START && collisionGroup0 <= WARS_COLLISION_GROUP_UNIT_END )
		collisionGroup0 = COLLISION_GROUP_NPC;

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}

#ifdef CLIENT_DLL
	
#ifdef ENABLE_PYTHON
extern INetworkStringTable *g_pStringTableInfoPanel;

boost::python::object CHL2WarsGameRules::GetTableInfoString(const char *entry)
{
	if( !entry )
	{
		return boost::python::object();
	}

	const char *data = NULL;
	int length = 0;

	if ( NULL == g_pStringTableInfoPanel )
		return boost::python::object();

	int index = g_pStringTableInfoPanel->FindStringIndex( entry );

	if (index != ::INVALID_STRING_INDEX)
		data = (const char *)g_pStringTableInfoPanel->GetStringUserData(index, &length);

	if (!data || !data[0])
		return boost::python::object(); // nothing to show

	return boost::python::object( data );
}
#endif // ENABLE_PYTHON

#else

// --------------------------------------------------------------------------------------------------- //
// Voice helper
// --------------------------------------------------------------------------------------------------- //

class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
	{
		// Dead players can only be heard by other dead team mates
		if ( pTalker->IsAlive() == false )
		{
			if ( pListener->IsAlive() == false )
				return ( pListener->InSameTeam( pTalker ) );

			return false;
		}

		return ( pListener->InSameTeam( pTalker ) );
	}
};
CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;



// --------------------------------------------------------------------------------------------------- //
// Globals.
// --------------------------------------------------------------------------------------------------- //
static const char *s_PreserveEnts[] =
{
	"player",
	"viewmodel",
	"worldspawn",
	"soundent",
	"ai_network",
	"ai_hint",
	"HL2Wars_gamerules",
	"HL2Wars_team_manager",
	"HL2Wars_team_unassigned",
	"HL2Wars_team_blue",
	"HL2Wars_team_red",
	"HL2Wars_player_manager",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sprite",
	"env_sun",
	"env_wind",
	"env_fog_controller",
	"func_brush",
	"func_wall",
	"func_illusionary",
	"info_node",
	"info_target",
	"info_node_hint",
	"info_player_red",
	"info_player_blue",
	"point_viewcontrol",
	"shadow_control",
	"sky_camera",
	"scene_manager",
	"trigger_soundscape",
	"point_commentary_node",
	"func_precipitation",
	"func_team_wall",
	"", // END Marker
};

// --------------------------------------------------------------------------------------------------- //
// Global helper functions.
// --------------------------------------------------------------------------------------------------- //

// World.cpp calls this but we don't use it in HL2Wars.
void InitBodyQue()
{
}


// --------------------------------------------------------------------------------------------------- //
// CHL2WarsGameRules implementation.
// --------------------------------------------------------------------------------------------------- //

CHL2WarsGameRules::CHL2WarsGameRules()
{
	m_bLevelInitialized = false;
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHL2WarsGameRules::~CHL2WarsGameRules()
{
}

#ifdef ENABLE_PYTHON
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsGameRules::InitGamerules()
{
	InitTeams();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsGameRules::ShutdownGamerules()
{
	DestroyTeams();
}
#endif // ENABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsGameRules::DestroyTeams()
{
	// Remove teams from global list
	int i;

	for( i = 0; i < m_Teams.Count(); i++ )
	{
		g_Teams.FindAndRemove( m_Teams[i] );
	}

	// Remove our team entities
	for(i=0; i<m_Teams.Count(); i++)
	{
		if( m_Teams[i] )
			UTIL_Remove( m_Teams[i] );
	}
	m_Teams.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *pszTeamNames[] =
{
	"#HL2Wars_Team_Unassigned",
	"#HL2Wars_Team_Spectator",
	"#HL2Wars_Team_One",
	"#HL2Wars_Team_Two",
	"#HL2Wars_Team_Three",
	"#HL2Wars_Team_Four",
};
void CHL2WarsGameRules::InitTeams( void )
{
	Assert( g_Teams.Count() == 0 );

	g_Teams.Purge();	// just in case

	CTeam *pUnassigned = static_cast<CTeam*>(CreateEntityByName( "hl2wars_team_unassigned" ));
	Assert( pUnassigned );
	pUnassigned->Init( pszTeamNames[TEAM_UNASSIGNED], TEAM_UNASSIGNED );
	g_Teams.AddToTail( pUnassigned );
	m_Teams.AddToTail( pUnassigned );

	CTeam *pSpectator = static_cast<CTeam*>(CreateEntityByName( "hl2wars_team_manager" ));
	Assert( pSpectator );
	pSpectator->Init( pszTeamNames[TEAM_SPECTATOR], TEAM_SPECTATOR );
	g_Teams.AddToTail( pSpectator );
	m_Teams.AddToTail( pSpectator );

	/// Team 1-4
	CTeam *pTeamOne = static_cast<CTeam*>(CreateEntityByName( "hl2wars_team_unassigned" ));
	Assert( pTeamOne );
	pTeamOne->Init( pszTeamNames[WARS_TEAM_ONE], WARS_TEAM_ONE );
	g_Teams.AddToTail( pTeamOne );
	m_Teams.AddToTail( pTeamOne );

	CTeam *pTeamTwo = static_cast<CTeam*>(CreateEntityByName( "hl2wars_team_unassigned" ));
	Assert( pTeamTwo );
	pTeamTwo->Init( pszTeamNames[WARS_TEAM_TWO], WARS_TEAM_TWO );
	g_Teams.AddToTail( pTeamTwo );
	m_Teams.AddToTail( pTeamTwo );

	CTeam *pTeamThree = static_cast<CTeam*>(CreateEntityByName( "hl2wars_team_unassigned" ));
	Assert( pTeamThree );
	pTeamThree->Init( pszTeamNames[WARS_TEAM_THREE], WARS_TEAM_THREE );
	g_Teams.AddToTail( pTeamThree );
	m_Teams.AddToTail( pTeamThree );

	CTeam *pTeamFour = static_cast<CTeam*>(CreateEntityByName( "hl2wars_team_unassigned" ));
	Assert( pTeamFour );
	pTeamFour->Init( pszTeamNames[WARS_TEAM_FOUR], WARS_TEAM_FOUR );
	g_Teams.AddToTail( pTeamFour );
	m_Teams.AddToTail( pTeamFour );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CHL2WarsGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	CHL2WarsPlayer *pPlayer = ToHL2WarsPlayer( pEdict );

	// Handle some player commands here as they relate more directly to gamerules state
	if ( pPlayer->ClientCommand( args ) )
	{
		return true;
	}
	else if ( BaseClass::ClientCommand( pEdict, args ) )
	{
		return true;
	}
	return false;
}

void CHL2WarsGameRules::UpdateVoiceManager()
{
	GetVoiceGameMgr()->Update( gpGlobals->frametime );
}

Vector DropToGround( 
					CBaseEntity *pMainEnt, 
					const Vector &vPos, 
					const Vector &vMins, 
					const Vector &vMaxs )
{
	trace_t trace;
	UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
	return trace.endpos;
}

void CHL2WarsGameRules::Precache( void ) 
{ 
	// Tracers
	PrecacheEffect( "GunshipTracer" );
	PrecacheEffect( "StriderTracer" );
	PrecacheEffect( "HunterTracer" );
	PrecacheEffect( "GaussTracer" );
	PrecacheEffect( "AirboatGunHeavyTracer" );
	PrecacheEffect( "AirboatGunTracer" );
	PrecacheEffect( "HelicopterTracer" );
	PrecacheEffect( "AR2Tracer" );
	PrecacheEffect( "AR2Explosion" );
	PrecacheEffect( "AR2Impact" );
	PrecacheEffect( "ChopperMuzzleFlash" );
	PrecacheEffect( "GunshipMuzzleFlash" );
	PrecacheEffect( "HunterMuzzleFlash" );

	// Impact
	PrecacheEffect( "ImpactJeep" );
	PrecacheEffect( "ImpactGauss" );
	PrecacheEffect( "Impact" );
	PrecacheEffect( "AirboatGunImpact" );
	PrecacheEffect( "HelicopterImpact" );
	PrecacheEffect( "cball_bounce" );
	PrecacheEffect( "cball_explode" );
	PrecacheEffect( "bloodspray" );

	// TODO: Move followning precache calls to python
	PrecacheEffect( "ManhackSparks" );

	PrecacheEffect( "StriderMuzzleFlash" ); 
	PrecacheEffect( "VortDispel" );
	PrecacheEffect( "HelicopterMegaBomb" );
	PrecacheEffect( "BoltImpact" );

	BaseClass::Precache(); 
}

void TestSpawnPointType( const char *pEntClassName )
{
	// Find the next spawn spot.
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, pEntClassName );

	while( pSpot )
	{
		// check if pSpot is valid
		if( g_pGameRules->IsSpawnPointValid( pSpot, NULL ) )
		{
			// the successful spawn point's location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 0, 255, 0, 100, 60 );

			// drop down to ground
			Vector GroundPos = DropToGround( NULL, pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

			// the location the player will spawn at
			NDebugOverlay::Box( GroundPos, VEC_HULL_MIN, VEC_HULL_MAX, 0, 0, 255, 100, 60 );

			// draw the spawn angles
			QAngle spotAngles = pSpot->GetLocalAngles();
			Vector vecForward;
			AngleVectors( spotAngles, &vecForward );
			NDebugOverlay::HorzArrow( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin() + vecForward * 32, 10, 255, 0, 0, 255, true, 60 );
		}
		else
		{
			// failed spawn point location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 255, 0, 0, 100, 60 );
		}

		// increment pSpot
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}
}

void CHL2WarsGameRules::PlayerSpawn( CBasePlayer *p )
{	
	CHL2WarsPlayer *pPlayer = ToHL2WarsPlayer( p );

	int team = pPlayer->GetTeamNumber();

	if( team != TEAM_SPECTATOR )
	{
		pPlayer->SetMaxSpeed( 600 );
	}
}

/* create some proxy entities that we use for transmitting data */
void CHL2WarsGameRules::CreateStandardEntities()
{
	// Create the player resource
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create( "player_manager", vec3_origin, vec3_angle );

	// Create the entity that will send our data to the client.
#ifdef _DEBUG
	CBaseEntity *pEnt = 
#endif
		CBaseEntity::Create( "HL2Wars_gamerules", vec3_origin, vec3_angle );
	Assert( pEnt );
}
int CHL2WarsGameRules::SelectDefaultTeam()
{
	int team = TEAM_UNASSIGNED;

	return team;
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Init CS ammo definitions
//-----------------------------------------------------------------------------

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			1	

// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if ( !bInitted )
	{
		bInitted = true;


		def.AddAmmoType("AR2",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	8,		3,		60,			BULLET_IMPULSE(200, 1225), 0 );
		def.AddAmmoType("AlyxGun",			DMG_BULLET,					TRACER_LINE,			5,		3,		150,		BULLET_IMPULSE(200, 1225), 0 );
		def.AddAmmoType("Pistol",			DMG_BULLET,					TRACER_LINE_AND_WHIZ,	5,		3,		150,		BULLET_IMPULSE(200, 1225), 0 );
		def.AddAmmoType("SMG1",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	4,		3,		225,		BULLET_IMPULSE(200, 1225), 0 );
		def.AddAmmoType("357",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	40,		30,		12,			BULLET_IMPULSE(800, 5000), 0 );
		def.AddAmmoType("XBowBolt",			DMG_BULLET,					TRACER_LINE,			100,	10,		10,			BULLET_IMPULSE(800, 8000), 0 );

		def.AddAmmoType("Buckshot",			DMG_BULLET | DMG_BUCKSHOT,	TRACER_LINE,			8,		3,		30,			BULLET_IMPULSE(400, 1200), 0 );
		def.AddAmmoType("RPG_Round",		DMG_BURN,					TRACER_NONE,			100,	50,		3,			0, 0 );
		def.AddAmmoType("SMG1_Grenade",		DMG_BURN,					TRACER_NONE,			100,	50,		3,			0, 0 );
		def.AddAmmoType("SniperRound",		DMG_BULLET | DMG_SNIPER,	TRACER_NONE,			20,		100,	30,			BULLET_IMPULSE(650, 6000), 0 );
		def.AddAmmoType("SniperPenetratedRound", DMG_BULLET | DMG_SNIPER, TRACER_NONE,			10,		100,	30,			BULLET_IMPULSE(150, 6000), 0 );
		def.AddAmmoType("Grenade",			DMG_BURN,					TRACER_NONE,			150,	75,		5,			0, 0);
		def.AddAmmoType("Thumper",			DMG_SONIC,					TRACER_NONE,			10, 10, 2, 0, 0 );
		def.AddAmmoType("Gravity",			DMG_CLUB,					TRACER_NONE,			0,	0, 8, 0, 0 );
		//		def.AddAmmoType("Extinguisher",		DMG_BURN,					TRACER_NONE,			0,	0, 100, 0, 0 );
		def.AddAmmoType("Battery",			DMG_CLUB,					TRACER_NONE,			NULL, NULL, NULL, 0, 0 );
		def.AddAmmoType("GaussEnergy",		DMG_SHOCK,					TRACER_NONE,			15,		15, 0, BULLET_IMPULSE(650, 8000), 0 ); // hit like a 10kg weight at 400 in/s
		def.AddAmmoType("CombineCannon",	DMG_BULLET,					TRACER_LINE,			3, 40, NULL, 1.5 * 750 * 12, 0 ); // hit like a 1.5kg weight at 750 ft/s
		def.AddAmmoType("AirboatGun",		DMG_AIRBOAT,				TRACER_LINE,			3,		3,		NULL,					BULLET_IMPULSE(10, 600), 0 );

		def.AddAmmoType("StriderMinigun",	DMG_BULLET,					TRACER_LINE,			5, 5, 15, 1.0 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED ); // hit like a 1.0kg weight at 750 ft/s

		def.AddAmmoType("StriderMinigunDirect",	DMG_BULLET,				TRACER_LINE,			2, 2, 15, 1.0 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED ); // hit like a 1.0kg weight at 750 ft/s
		def.AddAmmoType("HelicopterGun",	DMG_BULLET,					TRACER_LINE_AND_WHIZ,	3, 6,	225,	BULLET_IMPULSE(400, 1225), AMMO_FORCE_DROP_IF_CARRIED | AMMO_INTERPRET_PLRDAMAGE_AS_DAMAGE_TO_PLAYER );
		def.AddAmmoType("AR2AltFire",		DMG_DISSOLVE,				TRACER_NONE,			0, 0, 3, 0, 0 );
		def.AddAmmoType("Grenade",			DMG_BURN,					TRACER_NONE,			150,		75,		5,		0, 0);

		def.AddAmmoType("Hopwire",			DMG_BLAST,					TRACER_NONE,			150,		75,		3,		0, 0);
		def.AddAmmoType("CombineHeavyCannon",	DMG_BULLET,				TRACER_LINE,			40,	40, NULL, 10 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED ); // hit like a 10 kg weight at 750 ft/s
		def.AddAmmoType("ammo_proto1",			DMG_BULLET,				TRACER_LINE,			0, 0, 10, 0, 0 );
	}


	return &def;
}


#ifndef CLIENT_DLL

bool CHL2WarsGameRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	if ( pEntity )
	{
		return PyClientConnected(ENTINDEX(pEntity), pszName, pszAddress, reject, maxrejectlen) && BaseClass::ClientConnected(pEntity, pszName, pszAddress, reject, maxrejectlen);
	}
	return PyClientConnected(-1, pszName, pszAddress, reject, maxrejectlen) && BaseClass::ClientConnected(pEntity, pszName, pszAddress, reject, maxrejectlen);
}

void CHL2WarsGameRules::ClientDisconnected( edict_t *client ) 
{ 
	BaseClass::ClientDisconnected(client); 

	if ( client )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( client );
		PyClientDisconnected(pPlayer);
	}
	else
	{
		PyClientDisconnected(NULL);
	}
}

const char *CHL2WarsGameRules::GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer )
{
	return "";
}

const char *CHL2WarsGameRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( !pPlayer )  // dedicated server output
		return NULL;

	const char *pszFormat = NULL;

	if ( bTeamOnly )
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
			pszFormat = "HL2Wars_Chat_Spec";
		else
		{
			if (pPlayer->m_lifeState != LIFE_ALIVE )
				pszFormat = "HL2Wars_Chat_Team_Dead";
			else
				pszFormat = "HL2Wars_Chat_Team";
		}
	}
	else
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR)
			pszFormat = "HL2Wars_Chat_All_Spec";
		else
		{
			if (pPlayer->m_lifeState != LIFE_ALIVE )
				pszFormat = "HL2Wars_Chat_All_Dead";
			else
				pszFormat = "HL2Wars_Chat_All";
		}
	}

	return pszFormat;
}

#ifdef ENABLE_PYTHON
boost::python::object CHL2WarsGameRules::GetNextLevelName( bool bRandom )
{
	char buf[MAX_PATH];
	BaseClass::GetNextLevelName(buf, MAX_PATH, bRandom);
	return boost::python::object(buf);
}
#endif // ENABLE_PYTHON

#endif

float CHL2WarsGameRules::GetAmmoDamage( CBaseEntity *pAttacker, CBaseEntity *pVictim, int nAmmoType )
{
	float flDamage = 0.0f;
	CAmmoDef *pAmmoDef = GetAmmoDef();

	if ( pAmmoDef->DamageType( nAmmoType ) & DMG_SNIPER )
	{
		// If this damage is from a SNIPER, we do damage based on what the bullet
		// HITS, not who fired it. All other bullets have their damage values
		// arranged according to the owner of the bullet, not the recipient.
		if ( pVictim->IsPlayer() )
		{
			// Player
			flDamage = pAmmoDef->PlrDamage( nAmmoType );
		}
		else
		{
			// NPC or breakable
			flDamage = pAmmoDef->NPCDamage( nAmmoType );
		}
	}
	else
	{
		flDamage = BaseClass::GetAmmoDamage( pAttacker, pVictim, nAmmoType );
	}

	return flDamage;
}

float CHL2WarsGameRules::GetMapElapsedTime( void )
{
	return gpGlobals->curtime;
}
