//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#ifndef WARS_SHAREDDEFS_H
#define WARS_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

#include "wars_vprof.h"

#define WARS_COLLISION_SUPPORTED_UNITS 16
enum WarsCollision_Group_t
{
	// Don't change order without changing the ShouldCollide code!
	WARS_COLLISION_GROUP_IGNORE_UNIT_START = LAST_SHARED_COLLISION_GROUP,
	WARS_COLLISION_GROUP_IGNORE_UNIT_END = WARS_COLLISION_GROUP_IGNORE_UNIT_START+WARS_COLLISION_SUPPORTED_UNITS,
	WARS_COLLISION_GROUP_UNIT_START,
	WARS_COLLISION_GROUP_UNIT_END = WARS_COLLISION_GROUP_UNIT_START+WARS_COLLISION_SUPPORTED_UNITS,
	WARS_COLLISION_GROUP_IGNORE_ALL_UNITS, // Ignore collisions from WARS_COLLISION_GROUP_IGNORE_UNIT_START up to this one
	WARS_COLLISION_GROUP_BUILDING,
	WARS_COLLISION_GROUP_IGNORE_ALL_UNITS_AND_BUILD, // Ignore collisions from WARS_COLLISION_GROUP_IGNORE_UNIT_START up to this one
};

// Teams
enum warsteams_e
{
	WARS_TEAM_ONE = LAST_SHARED_TEAM+1,
	WARS_TEAM_TWO,
	WARS_TEAM_THREE,
	WARS_TEAM_FOUR,
};

//--------------
// HL2 SPECIFIC
//--------------
#define DMG_SNIPER			(DMG_LASTGENERICFLAG<<1)	// This is sniper damage
#define DMG_MISSILEDEFENSE	(DMG_LASTGENERICFLAG<<2)	// The only kind of damage missiles take. (special missile defense)

//-----------------------------------------------------------------------------
// Purpose: Fog of War parameters
//-----------------------------------------------------------------------------
// Struct for positions
typedef struct FowPos_t {
	int x;
	int y;
#ifdef CLIENT_DLL
	float sortslope;
#endif // CLIENT_DLL
} FowPos_t;

#define FOW_WORLDSIZE 32768
#define FOWMAXPLAYERS 16

#define FOWFLAG_HIDDEN			( 1 << 0 )
#define FOWFLAG_NOTRANSMIT		( 1 << 1 )
#define FOWFLAG_UPDATER			( 1 << 2 )
#define FOWFLAG_INITTRANSMIT	( 1 << 3 ) // Send the initial location of the entity, so neutral buildings are known for players
#define FOWFLAG_KEEPCOLINFOW	( 1 << 4 ) // Keep collision on client side even when in the fog of war (specific for neutral buildings)

#define FOWFLAG_UNITS_MASK		(FOWFLAG_HIDDEN|FOWFLAG_NOTRANSMIT|FOWFLAG_UPDATER)
#define FOWFLAG_BUILDINGS_MASK	(FOWFLAG_NOTRANSMIT|FOWFLAG_UPDATER)
#define FOWFLAG_BUILDINGS_NEUTRAL_MASK	(FOWFLAG_NOTRANSMIT|FOWFLAG_UPDATER|FOWFLAG_INITTRANSMIT|FOWFLAG_KEEPCOLINFOW)
#define FOWFLAG_ALL_MASK		(FOWFLAG_HIDDEN|FOWFLAG_NOTRANSMIT|FOWFLAG_UPDATER)
#define FOWFLAG_PARENTMASK		(FOWFLAG_HIDDEN|FOWFLAG_NOTRANSMIT) // Child entities only inherit these flags from their parents if this flag is set

#define FOWFLAG_BITCOUNT			5

//-----------------------------------------------------------------------------
// Purpose: Custom way to clear or read fow
//-----------------------------------------------------------------------------
struct fogofwar_t
{
	int			m_iX;
	int			m_iY;
	int			m_iRadius;
	int			m_iOwnerNumber;
	float		m_fDelay;			// Delay before started: delay = gpGlobals->curtime + delay;
	float		m_fLifeTime;		// usage: lifetime = gpGlobals->curtime + lifetime; Use -1 for live forever.
};

#endif // WARS_SHAREDDEFS_H
