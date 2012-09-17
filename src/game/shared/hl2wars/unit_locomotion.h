//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: Defines the locomotion of a ground moving unit.
//
//=============================================================================//

#ifndef UNIT_LOCOMOTION_H
#define UNIT_LOCOMOTION_H

#ifdef _WIN32
#pragma once
#endif

#include "util_shared.h"
#include "vprof.h"
#include "unit_component.h"
#include "util_shared.h"

float UnitComputePathDirection( const Vector &start, const Vector &end, Vector &pDirection );
float Unit_ClampYaw( float yawSpeedPerSec, float current, float target, float time );

//-----------------------------------------------------------------------------
// The movement command. This controls the movement.
//-----------------------------------------------------------------------------
class UnitBaseMoveCommand
{
public:
	UnitBaseMoveCommand()
	{
		memset( this, 0, sizeof(*this) );
	}

	void Clear()
	{
		forwardmove = 0.0f;
		sidemove = 0.0f;
		upmove = 0.0f;
		jump = false;
	}

	inline CBaseEntity *GetBlocker() { return m_hBlocker; }

	// Command
	float forwardmove;
	float sidemove;
	float upmove;
	QAngle idealviewangles;
	float interval;
	bool jump;

	// Data ( copied before moving, applied after )
	Vector origin; 
	Vector velocity;  
	QAngle viewangles;

	// Out
	float totaldistance;
	Vector outwishvel;
	float outstepheight;
	EHANDLE m_hBlocker;
	Vector blocker_hitpos;
	Vector blocker_dir;
	float stopdistance;

	// Settings
	float maxspeed;
	float yawspeed;
};

//-----------------------------------------------------------------------------
// Step argument and result structs
//-----------------------------------------------------------------------------
struct UnitCheckStepArgs_t
{
	Vector				vecStart;
	Vector				vecStepDir;
	float				stepSize;
	float				stepHeight;
	float				stepDownMultiplier;
	float				minStepLanding;
	unsigned			collisionMask;
};


struct UnitCheckStepResult_t
{
	Vector			endPoint;
	Vector			hitPos;
	Vector			hitNormal;
	bool			fStartSolid;
	CBaseEntity *	pBlocker;
};

//-----------------------------------------------------------------------------
// Nav Trace Filter
//-----------------------------------------------------------------------------
class CTraceFilterUnitNav : public CTraceFilterSimple
{
public:
	CTraceFilterUnitNav( CUnitBase *pProber, const IHandleEntity *passedict, int collisionGroup );
	bool ShouldHitEntity( IHandleEntity *pEntity, int contentsMask );

private:
	CUnitBase *m_pProber;
	bool m_bIgnoreTransientEntities;
	bool m_bCheckCollisionTable;
};

//-----------------------------------------------------------------------------
// Unit Base Locomotion class. Deals with movement.
//-----------------------------------------------------------------------------
class UnitBaseLocomotion : public UnitComponent
{
public:
	//friend class CUnitBase;
#ifndef DISABLE_PYTHON
	UnitBaseLocomotion( boost::python::object outer );
#endif // DISABLE_PYTHON

	// Main function
	virtual void PerformMovement( UnitBaseMoveCommand &mv );

	// Facing
	virtual void	MoveFacing();

	// Main move function
	virtual void	Move( float interval, UnitBaseMoveCommand &mv );

	// Setup
	virtual void SetupMove( UnitBaseMoveCommand &mv );
	virtual void FinishMove( UnitBaseMoveCommand &mv );

	// Acceleration
	virtual bool	CanAccelerate();
	virtual void	Accelerate( Vector& wishdir, float wishspeed, float accel);
	virtual void	AirAccelerate( Vector& wishdir, float wishspeed, float accel );

	// Position
	virtual void	CategorizePosition();

	// If I were to stop moving, how much distance would I walk before I'm halted?
	virtual float	GetStopDistance();

	// Walk
	virtual bool	ShouldWalk();
	virtual void	WalkMove();
	virtual void	AirMove( void );
	virtual void	FullWalkMove();
	virtual void	HandleJump() {}

	// Main ground move functions
	void				GroundMove();
	//bool				GroundMove( const Vector &vecActualStart, const Vector &vecDesiredEnd, 
	//	unsigned int collisionMask, unsigned flags );
	bool				CheckStep( const UnitCheckStepArgs_t &args, UnitCheckStepResult_t *pResult );

	// Tracing
	void	TraceUnitBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm );

	// Friction
	void	Friction( void );

	// Decompoosed gravity
	void	StartGravity( void );
	void	FinishGravity( void );

	int UnitTryMove( trace_t *steptrace );
	virtual int		TryUnitMove( Vector *pFirstDest=NULL, trace_t *pFirstTrace=NULL );
	virtual void	StepMove( Vector &vecDestination, trace_t &trace );

	// Slide off of the impacting object
	// returns the blocked flags:
	// 0x01 == floor
	// 0x02 == step / wall
	int				ClipVelocity( Vector& in, Vector& normal, Vector& out, float overbounce );

	void SetupMovementBounds( UnitBaseMoveCommand &mv );

public:
	float stepsize;
	int unitsolidmask;
	float surfacefriction;

	float acceleration;
	float airacceleration;
	float worldfriction;
	float stopspeed;

	Vector blocker_hitpos;

protected:
	UnitBaseMoveCommand *mv;

private:
	Vector m_vecMins, m_vecMaxs;

#ifdef HL2WARS_ASW_DLL
	ITraceListData	*m_pTraceListData;
#else
	CTraceListData *m_pTraceListData;
#endif // HL2WARS_ASW_DLL
};

//-----------------------------------------------------------------------------
// Traces player movement + position
//-----------------------------------------------------------------------------
inline void UnitBaseLocomotion::TraceUnitBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm )
{
	//VPROF( "UnitBaseLocomotion::TraceUnitBBox" );
	VPROF_BUDGET( "UnitBaseLocomotion::TraceUnitBBox", VPROF_BUDGETGROUP_UNITS );
#if 1
	Ray_t ray;
	ray.Init( start, end, m_vecMins, m_vecMaxs );
	CTraceFilterUnitNav filter(m_pOuter, m_pOuter, collisionGroup);
	//CTraceFilterSimple filter(m_pOuter, collisionGroup);
#ifdef HL2WARS_ASW_DLL
	if ( m_pTraceListData && m_pTraceListData->CanTraceRay(ray) )
	{
		enginetrace->TraceRayAgainstLeafAndEntityList( ray, m_pTraceListData, fMask, &filter, &pm );
#else
	if ( m_pTraceListData )
	{
		enginetrace->TraceRayAgainstLeafAndEntityList( ray, *m_pTraceListData, fMask, &filter, &pm );
#endif // HL2WARS_ASW_DLL
	}
	else
	{
		enginetrace->TraceRay( ray, fMask, &filter, &pm );
	}
#else
	/*Ray_t ray;
	ray.Init( start, end, GetOuter()->WorldAlignMins(), GetOuter()->WorldAlignMaxs() );
	UTIL_TraceRay( ray, fMask, GetOuter(), collisionGroup, &pm );*/

	//UTIL_TraceEntity(m_pOuter, start, end, unitsolidmask, &pm );

	CTraceFilterUnitNav filter(m_pOuter, false, m_pOuter, collisionGroup);
	UTIL_TraceEntity(m_pOuter, start, end, unitsolidmask, &filter, &pm );
#endif // HL2WARS_ASW_DLL
}

#endif // UNIT_LOCOMOTION_H
