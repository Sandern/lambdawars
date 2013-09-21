//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "gamemovement.h"
#include "movevars_shared.h"
#include "unit_base_shared.h"
#include "wars_mapboundary.h"
#include "collisionutils.h"
#include "hl2wars_util_shared.h"

#ifdef CLIENT_DLL
	#include "c_hl2wars_player.h"
	#include "input.h"
#else
	#include "hl2wars_player.h"
#endif

#include "engine/ivdebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
	ConVar cl_showmovementspeed( "cl_showmovementspeed", "-1", FCVAR_CHEAT, "Show the (client) movement speed." );
#else
	ConVar sv_showmovementspeed( "sv_showmovementspeed", "-1", FCVAR_CHEAT, "Show the (server) movement speed." );
	ConVar sv_strategic_debug_movement( "sv_strategic_debug_movement", "-1", FCVAR_CHEAT, "" );
#endif // CLIENT_DLL
// ---------------------------------------------------------------------------------------- //
// Settings
// ---------------------------------------------------------------------------------------- //
static ConVar strategic_cam_maxspeed( "strategic_cam_maxspeed", "10000", FCVAR_REPLICATED, "Clamps the max speed of all strategic players." );

// ---------------------------------------------------------------------------------------- //
// CHL2WarsGameMovement class
// ---------------------------------------------------------------------------------------- //
class CHL2WarsGameMovement : public CGameMovement
{
public:
	DECLARE_CLASS( CHL2WarsGameMovement, CGameMovement );

	CHL2WarsGameMovement();
	virtual ~CHL2WarsGameMovement();

#if 0
	virtual Vector const&	GetPlayerMins( bool ducked ) const;
	virtual Vector const&	GetPlayerMaxs( bool ducked ) const;
	virtual const Vector&	GetPlayerMins( void ) const; // uses local player
	virtual const Vector&	GetPlayerMaxs( void ) const; // uses local player
#endif // 0

	void PlayerMove( void );
	virtual void	CheckParameters( void );

	void ApplyRotation();
	void StrategicPlayerMove();

	// The basic solid body movement clip that slides along multiple planes
	virtual int		TryStrategicMove( Vector *pFirstDest=NULL, trace_t *pFirstTrace=NULL );

	virtual unsigned int PlayerSolidMask( bool brushOnly = false );	///< returns the solid mask for the given player, so bots can have a more-restrictive set

	virtual void			TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm );

	// Traces the player bbox as it is swept from start to end
	virtual CBaseHandle		TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm );

	virtual bool CanAccelerate();

private:
	float m_fCamDistance, m_fCamDesiredDistance;
};


// Expose our interface.
static CHL2WarsGameMovement g_GameMovement;
IGameMovement *g_pGameMovement = ( IGameMovement * )&g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameMovement, IGameMovement,INTERFACENAME_GAMEMOVEMENT, g_GameMovement );


// ---------------------------------------------------------------------------------------- //
// CHL2WarsGameMovement.
// ---------------------------------------------------------------------------------------- //

CHL2WarsGameMovement::CHL2WarsGameMovement()
{
}

CHL2WarsGameMovement::~CHL2WarsGameMovement()
{
}

#if 0
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ducked - 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector& CHL2WarsGameMovement::GetPlayerMins( bool ducked ) const
{
	return ducked ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ducked - 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector& CHL2WarsGameMovement::GetPlayerMaxs( bool ducked ) const
{	
	return ducked ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX;
}

static Vector g_vWarsHullMins(-128, -128, 0);
static Vector g_vWarsHullMaxs(128, 128, 96);

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector& CHL2WarsGameMovement::GetPlayerMins( void ) const
{
	if ( player->IsObserver() )
	{
		return VEC_OBS_HULL_MIN;	
	}
	else
	{
		CHL2WarsPlayer *warsplayer = dynamic_cast<CHL2WarsPlayer *>( player );
		if( warsplayer->IsStrategicModeOn() )
			return g_vWarsHullMins;
		else
			return player->m_Local.m_bDucked  ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector& CHL2WarsGameMovement::GetPlayerMaxs( void ) const
{	
	if ( player->IsObserver() )
	{
		return VEC_OBS_HULL_MAX;	
	}
	else
	{
		CHL2WarsPlayer *warsplayer = dynamic_cast<CHL2WarsPlayer *>( player );
		if( warsplayer->IsStrategicModeOn() )
			return g_vWarsHullMaxs;
		else
			return player->m_Local.m_bDucked  ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX;
	}
}
#endif // 0

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHL2WarsGameMovement::CanAccelerate()
{
	if( player->GetMoveType() != MOVETYPE_STRATEGIC )
	{
		return CGameMovement::CanAccelerate();
	}

	// Can always accelerate in rts mode.
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsGameMovement::PlayerMove()
{
	// Other movetype than strategic? use base class
	if( player->GetMoveType() != MOVETYPE_STRATEGIC )
	{
		CGameMovement::PlayerMove();
		return;
	}

	VPROF( "CHL2WarsGameMovement::PlayerMove" );

	CheckParameters();

	// clear output applied velocity
	mv->m_outWishVel.Init();
	mv->m_outJumpVel.Init();

	MoveHelper( )->ResetTouchList();                    // Assume we don't touch anything

	ReduceTimers();

	AngleVectors (mv->m_vecViewAngles, &m_vecForward, &m_vecRight, &m_vecUp );  // Determine movement angles

	SetGroundEntity( NULL );

	// Store off the starting water level
	m_nOldWaterLevel = player->GetWaterLevel();

	m_nOnLadder = 0;

	StrategicPlayerMove();
}

void CHL2WarsGameMovement::CheckParameters( void )
{
	// Other movetype than strategic? use base class
	if( player->GetMoveType() != MOVETYPE_STRATEGIC )
	{
		CGameMovement::CheckParameters();
		return;
	}

	QAngle	v_angle;

	if ( player->GetFlags() & FL_FROZEN ||
		 player->GetFlags() & FL_ONTRAIN /*|| 
		 IsDead()*/ )
	{
		mv->m_flForwardMove = 0;
		mv->m_flSideMove    = 0;
		mv->m_flUpMove      = 0;
	}

	DecayPunchAngle();

	// Take angles from command.
	//if ( !IsDead() )
	{
		v_angle = mv->m_vecAngles;
		v_angle = v_angle + player->m_Local.m_vecPunchAngle;

		// Now adjust roll angle
		if ( player->GetMoveType() != MOVETYPE_ISOMETRIC  &&
			 player->GetMoveType() != MOVETYPE_NOCLIP )
		{
			mv->m_vecAngles[ROLL]  = CalcRoll( v_angle, mv->m_vecVelocity, sv_rollangle.GetFloat(), sv_rollspeed.GetFloat() );
		}
		else
		{
			mv->m_vecAngles[ROLL] = 0.0; // v_angle[ ROLL ];
		}
		mv->m_vecAngles[PITCH] = v_angle[PITCH];
		mv->m_vecAngles[YAW]   = v_angle[YAW];
	}
	/*else
	{
		mv->m_vecAngles = mv->m_vecOldAngles;
	}*/

	// Set dead player view_offset
	/*if ( IsDead() )
	{
		player->SetViewOffset( VEC_DEAD_VIEWHEIGHT );
	}*/

	// Adjust client view angles to match values used on server.
	if ( mv->m_vecAngles[YAW] > 180.0f )
	{
		mv->m_vecAngles[YAW] -= 360.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsGameMovement::StrategicPlayerMove()
{
	trace_t pm;
	Vector move;
	Vector forward, right, up;

	Vector wishvel;
	Vector wishdir;
	float wishspeed, heightchange;
	float maxspeed = strategic_cam_maxspeed.GetFloat(); // Server defined max speed

	CHL2WarsPlayer *warsplayer = dynamic_cast<CHL2WarsPlayer *>( player );
	if( !warsplayer )
		return;

	float speed = warsplayer->GetCamSpeed();
	float maxacceleration = warsplayer->GetCamAcceleration();
	float friction = warsplayer->GetCamFriction();
	float stopspeed = warsplayer->GetCamStopSpeed();

	// Determine our current height
	warsplayer->CalculateHeight( mv->GetAbsOrigin() );

	// Determine movement angles
	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  

	up.z = 0.0f;
	right.z = 0.0f;

	VectorNormalize (up); 
	VectorNormalize (right);

	VectorNormalize (forward); // Zoom direction

	// Copy movement amounts
	// Assume max move is 450.0, so we can scale it to the correct speed value
	// NOTE: Swapped fmove and umove in terms of usage.
	float smove = (mv->m_flSideMove/450.0f) * speed; 
	float fmove = (mv->m_flForwardMove/450.0f) * speed; 
	float umove = (mv->m_flUpMove/450.0f) * speed; // Zoom when there is no map boundary

	for (int i=0 ; i<2 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = up[i]*fmove + right[i]*smove;
	wishvel[2] = 0.0f;

	if( warsplayer->IsCamValid() && warsplayer->GetCamMaxHeight() > 2.0f )
		;//wishvel[2] = -warsplayer->GetCamHeight() * 4.0f; // Force player origin down
	else
		wishvel = wishvel + forward*umove*4.0;		// Zooming is forward

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// Clamp to user defined wish speed
	//
	if (wishspeed > speed )
	{
		VectorScale (wishvel, speed/wishspeed, wishvel);
		wishspeed = speed;
	}

	//
	// Clamp to server defined max speed
	//
	if (wishspeed > maxspeed )
	{
		VectorScale (wishvel, maxspeed/wishspeed, wishvel);
		wishspeed = maxspeed;
	}

	if ( maxacceleration > 0.0 )
	{
		// Set pmove velocity
		Accelerate ( wishdir, wishspeed, maxacceleration );

		float spd = VectorLength( mv->m_vecVelocity );
		if (spd < 1.0f)
		{
			mv->m_vecVelocity.Init();
		}
		else
		{
			// Bleed off some speed, but if we have less than the bleed
			//  threshhold, bleed the theshold amount.
			float control = (spd < stopspeed) ? stopspeed : spd;

			// Add the amount to the drop amount.
			float drop = control * friction * gpGlobals->frametime;

			// scale the velocity
			float newspeed = spd - drop;
			if (newspeed < 0)
				newspeed = 0;

			// Determine proportion of old speed we are using.
			newspeed /= spd;
			VectorScale( mv->m_vecVelocity, newspeed, mv->m_vecVelocity );
		}
	}
	else
	{
		VectorCopy( wishvel, mv->m_vecVelocity );
	}

#ifdef CLIENT_DLL
	if( cl_showmovementspeed.GetInt() == player->entindex() )
	{
		engine->Con_NPrintf( 1, "CLIENT Movement speed: %6.1f | maxspeed: %6.1f | userspeed: %6.1f | pos:  %6.1f %6.1f %6.1f | mins: %6.1f %6.1f %6.1f | maxs: %6.1f %6.1f %6.1f", 
			mv->m_vecVelocity.Length(), maxspeed, speed, mv->GetAbsOrigin().x, mv->GetAbsOrigin().y, mv->GetAbsOrigin().z,
			GetPlayerMins().x, GetPlayerMins().y, GetPlayerMins().z, GetPlayerMaxs().x, GetPlayerMaxs().y, GetPlayerMaxs().z );
		engine->Con_NPrintf( 2, "CLIENT Cam Max Height: %6.1f | ground pos: %6.1f %6.1f %6.1f", 
			warsplayer->GetCamMaxHeight(), warsplayer->GetCamGroundPos().x, warsplayer->GetCamGroundPos().y, warsplayer->GetCamGroundPos().z );
	}
#else
	if( sv_showmovementspeed.GetInt() == player->entindex() )
	{
		engine->Con_NPrintf( 4, "SERVER Movement speed: %6.1f | maxspeed: %6.1f | userspeed: %6.1f | pos:  %6.1f %6.1f %6.1f | mins: %6.1f %6.1f %6.1f | maxs: %6.1f %6.1f %6.1f", 
			mv->m_vecVelocity.Length(), maxspeed, speed, mv->GetAbsOrigin().x, mv->GetAbsOrigin().y, mv->GetAbsOrigin().z,
			GetPlayerMins().x, GetPlayerMins().y, GetPlayerMins().z, GetPlayerMaxs().x, GetPlayerMaxs().y, GetPlayerMaxs().z );
		engine->Con_NPrintf( 5, "SERVER Cam Max Height: %6.1f | ground pos: %6.1f %6.1f %6.1f", 
			warsplayer->GetCamMaxHeight(), warsplayer->GetCamGroundPos().x, warsplayer->GetCamGroundPos().y, warsplayer->GetCamGroundPos().z );
	}

	if( sv_strategic_debug_movement.GetInt() == player->entindex() )
	{
		NDebugOverlay::Box( mv->GetAbsOrigin(), -Vector(16, 16, 16), Vector(16, 16, 16), 0, 255, 0, 255, gpGlobals->frametime );
	}
#endif // CLIENT_DLL

	// Store current height
	heightchange = mv->GetAbsOrigin().z;

	if( warsplayer->IsCamValid() )
	{
		//Vector vCamOffsetZ = Vector(0.0f, 0.0f, warsplayer->GetCameraOffset().z);
		//TracePlayerBBox( mv->GetAbsOrigin(), warsplayer->GetCamGroundPos()+vCamOffsetZ, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );
		TracePlayerBBox( mv->GetAbsOrigin(), warsplayer->GetCamGroundPos()+Vector(0, 0, warsplayer->GetCamMaxHeight()-48.0f), PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );
		mv->SetAbsOrigin( pm.endpos );
		//NDebugOverlay::Box( warsplayer->GetCamGroundPos()+vCamOffsetZ, -Vector(16, 16, 16), Vector(16, 16, 16), 255, 0, 0, 255, gpGlobals->frametime );
	}

	// Try moving, only obstructed by the map boundaries.
	Vector destination;
	VectorMA( mv->GetAbsOrigin(), gpGlobals->frametime, mv->m_vecVelocity, destination );

	TracePlayerBBox( mv->GetAbsOrigin(), destination, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );
	if( pm.fraction == 1 )
	{

		mv->SetAbsOrigin( pm.endpos );
	}
	else
	{
		// Try moving straight along out normal path.
		TryStrategicMove();
	}

	// Determine new height and return player to the ground
	warsplayer->CalculateHeight( mv->GetAbsOrigin() );

	if( warsplayer->IsCamValid() )
	{
		TracePlayerBBox( mv->GetAbsOrigin(), warsplayer->GetCamGroundPos(), PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );
		mv->SetAbsOrigin( pm.endpos );
		//NDebugOverlay::Box( pm.endpos, -Vector(16, 16, 16), Vector(16, 16, 16), 0, 255, 0, 255, gpGlobals->frametime );

		// Determine new height again :)
		warsplayer->CalculateHeight( mv->GetAbsOrigin() );
	}

	CheckVelocity();

	// Zero out velocity if in noaccel mode
	if ( maxacceleration < 0.0f )
	{
		mv->m_vecVelocity.Init();
	}
}

unsigned int CHL2WarsGameMovement::PlayerSolidMask( bool brushOnly )
{
	if( player->GetMoveType() != MOVETYPE_STRATEGIC )
	{
		return CGameMovement::PlayerSolidMask( brushOnly );
	}

	return CONTENTS_PLAYERCLIP;//|CONTENTS_MOVEABLE;
}

//-----------------------------------------------------------------------------
// Traces player movement + position
//-----------------------------------------------------------------------------
inline void CHL2WarsGameMovement::TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm )
{
	if( player->GetMoveType() != MOVETYPE_STRATEGIC )
	{
		CGameMovement::TracePlayerBBox( start, end, fMask, collisionGroup, pm );
		return;
	}

	VPROF( "CGameMovement::TracePlayerBBox" );

	Ray_t ray;
	ray.Init( start, end, GetPlayerMins(), GetPlayerMaxs() );

	CTraceFilterWars traceFilter( mv->m_nPlayerHandle.Get(), collisionGroup );

	enginetrace->TraceRay( ray, fMask, &traceFilter, &pm );
}

CBaseHandle CHL2WarsGameMovement::TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm )
{
	if( player->GetMoveType() != MOVETYPE_STRATEGIC )
	{
		return CGameMovement::TestPlayerPosition( pos, collisionGroup, pm );
	}


	Ray_t ray;
	ray.Init( pos, pos, GetPlayerMins(), GetPlayerMaxs() );

	CTraceFilterWars traceFilter( mv->m_nPlayerHandle.Get(), collisionGroup );

	enginetrace->TraceRay( ray, PlayerSolidMask(), &traceFilter, &pm );

	if ( (pm.contents & PlayerSolidMask()) && pm.m_pEnt )
	{
		return pm.m_pEnt->GetRefEHandle();
	}
	else
	{	
		return INVALID_EHANDLE_INDEX;
	}
}

//#define PLAYER_GETTING_STUCK_TESTING
#define	MAX_CLIP_PLANES		5

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CHL2WarsGameMovement::TryStrategicMove( Vector *pFirstDest, trace_t *pFirstTrace )
{
	int			bumpcount, numbumps;
	Vector		dir;
	float		d;
	int			numplanes;
	Vector		planes[MAX_CLIP_PLANES];
	Vector		primal_velocity, original_velocity;
	Vector      new_velocity;
	int			i, j;
	trace_t	pm;
	Vector		end;
	float		time_left, allFraction;
	int			blocked;		
	
	numbumps  = 4;           // Bump up to four times
	
	blocked   = 0;           // Assume not blocked
	numplanes = 0;           //  and not sliding along any planes

	VectorCopy (mv->m_vecVelocity, original_velocity);  // Store original velocity
	VectorCopy (mv->m_vecVelocity, primal_velocity);
	
	allFraction = 0;
	time_left = gpGlobals->frametime;   // Total time for this movement operation.

	new_velocity.Init();

	for (bumpcount=0 ; bumpcount < numbumps; bumpcount++)
	{
		if ( mv->m_vecVelocity.Length() == 0.0 )
			break;

		// Assume we can move all the way from the current origin to the
		//  end point.
		VectorMA( mv->GetAbsOrigin(), time_left, mv->m_vecVelocity, end );

		// See if we can make it from origin to end point.
		TracePlayerBBox( mv->GetAbsOrigin(), end, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );

		allFraction += pm.fraction;

		// If we started in a solid object, or we were in solid space
		//  the whole way, zero out our velocity and return that we
		//  are blocked by floor and wall.
		if (pm.allsolid)
		{	
#if defined( PLAYER_GETTING_STUCK_TESTING )
			Msg( "Trapped!!! :(\n" );
#endif
			// entity is trapped in another solid
			VectorCopy (vec3_origin, mv->m_vecVelocity);
			return 4;
		}

		// If we moved some portion of the total distance, then
		//  copy the end position into the pmove.origin and 
		//  zero the plane counter.
		if( pm.fraction > 0 )
		{	
			if ( numbumps > 0 && pm.fraction == 1 )
			{
				// There's a precision issue with terrain tracing that can cause a swept box to successfully trace
				// when the end position is stuck in the triangle.  Re-run the test with an uswept box to catch that
				// case until the bug is fixed.
				// If we detect getting stuck, don't allow the movement
				trace_t stuck;
				TracePlayerBBox( pm.endpos, pm.endpos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, stuck );

				if ( stuck.startsolid || stuck.fraction != 1.0f )
				{
					//Msg( "Player will become stuck!!!\n" );
					VectorCopy (vec3_origin, mv->m_vecVelocity);
					break;
				}
			}

#if defined( PLAYER_GETTING_STUCK_TESTING )
			trace_t foo;
			TracePlayerBBox( pm.endpos, pm.endpos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, foo );
			if ( foo.startsolid || foo.fraction != 1.0f )
			{
				Msg( "Player will become stuck!!!\n" );
			}
#endif
			// actually covered some distance
			mv->SetAbsOrigin( pm.endpos);
			VectorCopy (mv->m_vecVelocity, original_velocity);
			numplanes = 0;
		}

		// If we covered the entire distance, we are done
		//  and can return.
		if (pm.fraction == 1)
		{
			 break;		// moved the entire distance
		}

		// Save entity that blocked us (since fraction was < 1.0)
		//  for contact
		// Add it if it's not already in the list!!!
		MoveHelper( )->AddToTouched( pm, mv->m_vecVelocity );

		// If the plane we hit has a high z component in the normal, then
		//  it's probably a floor
		if (pm.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
		}
		// If the plane has a zero z component in the normal, then it's a 
		//  step or wall
		if (!pm.plane.normal[2])
		{
			blocked |= 2;		// step / wall
		}

		// Reduce amount of m_flFrameTime left by total time left * fraction
		//  that we covered.
		time_left -= time_left * pm.fraction;

		// Did we run out of planes to clip against?
		if (numplanes >= MAX_CLIP_PLANES)
		{	
			// this shouldn't really happen
			//  Stop our movement if so.
			VectorCopy (vec3_origin, mv->m_vecVelocity);
			//Con_DPrintf("Too many planes 4\n");

			break;
		}

		// Set up next clipping plane
		VectorCopy (pm.plane.normal, planes[numplanes]);
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		//

		// reflect player velocity 
		// Only give this a try for first impact plane because you can get yourself stuck in an acute corner by jumping in place
		//  and pressing forward and nobody was really using this bounce/reflection feature anyway...
		if ( numplanes == 1 &&
			player->GetMoveType() == MOVETYPE_WALK &&
			player->GetGroundEntity() == NULL )	
		{
			for ( i = 0; i < numplanes; i++ )
			{
				if ( planes[i][2] > 0.7  )
				{
					// floor or slope
					ClipVelocity( original_velocity, planes[i], new_velocity, 1 );
					VectorCopy( new_velocity, original_velocity );
				}
				else
				{
					ClipVelocity( original_velocity, planes[i], new_velocity, 1.0 + sv_bounce.GetFloat() * (1 - player->m_surfaceFriction) );
				}
			}

			VectorCopy( new_velocity, mv->m_vecVelocity );
			VectorCopy( new_velocity, original_velocity );
		}
		else
		{
			for (i=0 ; i < numplanes ; i++)
			{
				ClipVelocity (
					original_velocity,
					planes[i],
					mv->m_vecVelocity,
					1);

				for (j=0 ; j<numplanes ; j++)
					if (j != i)
					{
						// Are we now moving against this plane?
						if (mv->m_vecVelocity.Dot(planes[j]) < 0)
							break;	// not ok
					}
				if (j == numplanes)  // Didn't have to clip, so we're ok
					break;
			}
			
			// Did we go all the way through plane set
			if (i != numplanes)
			{	// go along this plane
				// pmove.velocity is set in clipping call, no need to set again.
				;  
			}
			else
			{	// go along the crease
				if (numplanes != 2)
				{
					VectorCopy (vec3_origin, mv->m_vecVelocity);
					break;
				}
				CrossProduct (planes[0], planes[1], dir);
				dir.NormalizeInPlace();
				d = dir.Dot(mv->m_vecVelocity);
				VectorScale (dir, d, mv->m_vecVelocity );
			}

			//
			// if original velocity is against the original velocity, stop dead
			// to avoid tiny occilations in sloping corners
			//
			d = mv->m_vecVelocity.Dot(primal_velocity);
			if (d <= 0)
			{
				//Con_DPrintf("Back\n");
				VectorCopy (vec3_origin, mv->m_vecVelocity);
				break;
			}
		}
	}

	if ( allFraction == 0 )
	{
		VectorCopy (vec3_origin, mv->m_vecVelocity);
	}

#if 0
	// Check if they slammed into a wall
	float fSlamVol = 0.0f;

	float fLateralStoppingAmount = primal_velocity.Length2D() - mv->m_vecVelocity.Length2D();
	if ( fLateralStoppingAmount > PLAYER_MAX_SAFE_FALL_SPEED * 2.0f )
	{
		fSlamVol = 1.0f;
	}
	else if ( fLateralStoppingAmount > PLAYER_MAX_SAFE_FALL_SPEED )
	{
		fSlamVol = 0.85f;
	}

	PlayerRoughLandingEffects( fSlamVol );
#endif // 0

	return blocked;
}