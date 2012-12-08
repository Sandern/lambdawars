//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose:	
//
//=============================================================================//

#ifndef UNIT_NAVIGATOR_H
#define UNIT_NAVIGATOR_H

#ifdef _WIN32
#pragma once
#endif

#include "unit_component.h"
#include "nav.h"

// Debug
#define NavDbgMsg( fmt, ... )				\
	if( unit_navigator_debug.GetBool() )	\
		DevMsg( fmt, ##__VA_ARGS__ );		\

extern ConVar unit_navigator_debug;
extern ConVar unit_navigator_debug_inrange;

// Forward declarations
class UnitBaseMoveCommand;
class CNavArea;

// Goal types
enum UnitGoalTypes
{
	GOALTYPE_NONE = 0,
	GOALTYPE_INVALID,
	GOALTYPE_POSITION,
	GOALTYPE_TARGETENT,
	GOALTYPE_POSITION_INRANGE,
	GOALTYPE_TARGETENT_INRANGE,
};

// Goal flags
enum UnitGoalFlags
{
	GF_NOCLEAR = (1 << 0),
	GF_REQTARGETALIVE = (1 << 1),
	GF_USETARGETDIST = (1 << 2 ),
	GF_NOLOSREQUIRED = (1 << 3 ), // No line check (just get in sense range)
	GF_REQUIREVISION = (1 << 4 ), // Fog of war
	GF_OWNERISTARGET = (1 << 5 ), // Bumped into entity that is being owned by the target
	GF_DIRECTPATH = (1 << 6), // Build direct path, don't do path finding. For special cases.
};


// Status of the current goal if any.
enum CheckGoalStatus_t
{
	CHS_NOGOAL = 0,
	CHS_HASGOAL, // Got a goal and moving towards it
	CHS_ATGOAL, // At goal, will dispatch success at the end of the navigation update (unless noclear is specified)
	CHS_FAILED, // Goal failed due some reason (target died, blocked resolving failed, etc)
	CHS_CLIMB, // Start waypoint at which we should start climbing (placed in the from area)
	CHS_CLIMBDEST, // End point of climbing (placed into the new area)
	CHS_EDGEDOWN, // Fall down, likely a small gap between the two navigation meshes too.
	CHS_EDGEDOWNDEST,
	CHS_BLOCKED, // Indicates we are currently blocked and are trying to resolve it
};

// Status of being blocked
enum BlockedStatus_t
{
	BS_NONE = 0,
	BS_LITTLE,
	BS_MUCH,
	BS_STUCK,
	BS_GIVEUP, 
};

//-----------------------------------------------------------------------------
// Purpose: Waypoint class
//-----------------------------------------------------------------------------
class UnitBaseWaypoint
{
public:
	UnitBaseWaypoint()
	{
		memset( this, 0, sizeof(*this) );
		vecLocation	= vec3_invalid;
		flPathDistGoal = -1;
		navDir = NUM_DIRECTIONS;
	}
	UnitBaseWaypoint( const Vector &vecPosition, float flYaw=0.0f )
	{
		memset( this, 0, sizeof(*this) );
		flPathDistGoal = -1;
		vecLocation = vecPosition;
		flYaw = flYaw;
		navDir = NUM_DIRECTIONS;
	}
	UnitBaseWaypoint( const UnitBaseWaypoint &from )
	{
		memcpy( this, &from, sizeof(*this) );
		flPathDistGoal = -1;
		pNext = pPrev = NULL;
		navDir = NUM_DIRECTIONS;
	}

	UnitBaseWaypoint &operator=( const UnitBaseWaypoint &from )
	{
		memcpy( this, &from, sizeof(*this) );
		flPathDistGoal = -1;
		pNext = pPrev = NULL;
		return *this;
	}

	~UnitBaseWaypoint()
	{
		if ( pNext )
		{
			pNext->pPrev = pPrev;
		}
		if ( pPrev )
		{
			pPrev->pNext = pNext;
		}
	}

	//---------------------------------

	void					SetNext( UnitBaseWaypoint *p );
	UnitBaseWaypoint *		GetNext()				{ return pNext; }
	const UnitBaseWaypoint *GetNext() const			{ return pNext; }

	UnitBaseWaypoint *		GetPrev()				{ return pPrev; }
	const UnitBaseWaypoint *GetPrev() const			{ return pPrev; }

	UnitBaseWaypoint *		GetLast();

	//---------------------------------

	const Vector &		GetPos() const				{ return vecLocation; }
	void 				SetPos(const Vector &newPos) { vecLocation = newPos; }


	//---------------------------------
	//
	// Basic info
	//
	Vector			vecLocation;
	float			flYaw;				// Waypoint facing dir

	float			flToleranceX;
	float			flToleranceY;
	Vector			areaSlope;
	NavDirType		navDir;

	CheckGoalStatus_t SpecialGoalStatus; // Leave 0 to be ignored

	//---------------------------------
	//
	// Precalculated distances
	//
	float			flPathDistGoal;

	//---------------------------------
	//
	// Nav Area info
	//
	CNavArea *pFrom;
	CNavArea *pTo;

private:
	UnitBaseWaypoint *pNext;
	UnitBaseWaypoint *pPrev;
};

inline void UnitBaseWaypoint::SetNext( UnitBaseWaypoint *p )	
{ 
	if (pNext) 
	{
		pNext->pPrev = NULL; 
	}

	pNext = p; 

	if ( pNext ) 
	{
		if ( pNext->pPrev )
			pNext->pPrev->pNext = NULL;

		pNext->pPrev = this; 
	}
}

// Path class
//-----------------------------------------------------------------------------
// Purpose: Path. Maintains the list of waypoints to a goal.
//-----------------------------------------------------------------------------
class UnitBasePath
{
public:
	UnitBasePath() 
	{
		Clear();
	}
	UnitBasePath( const UnitBasePath &src )
	{
		m_iGoalType = src.m_iGoalType;
		m_vGoalPos = src.m_vGoalPos;
		m_vGoalInRangePos = src.m_vGoalInRangePos;
		m_waypointTolerance = src.m_waypointTolerance;
		m_fGoalTolerance = src.m_fGoalTolerance;
		m_iGoalFlags = src.m_iGoalFlags;
		m_fMinRange = src.m_fMinRange;
		m_hTarget = src.m_hTarget;
		m_bAvoidEnemies = src.m_bAvoidEnemies;

		// Copy waypoints
		if( src.m_pWaypointHead )
		{
			UnitBaseWaypoint *pPrev, *pCur;
			m_pWaypointHead = new UnitBaseWaypoint(*src.m_pWaypointHead);
			pPrev = m_pWaypointHead;
			pCur = m_pWaypointHead->GetNext();
			while( pCur )
			{
				pCur = new UnitBaseWaypoint(*pCur);
				pPrev->SetNext(pCur);
				pPrev = pCur;
				pCur = pCur->GetNext();
			}
		}
		else
		{
			m_pWaypointHead = NULL;
		}
	}

	bool CurWaypointIsGoal() 
	{
		if( m_pWaypointHead && m_pWaypointHead->GetNext() == NULL )
			return true;
		return false;
	}

	UnitBaseWaypoint *GetCurWaypoint() { return m_pWaypointHead; }

	inline float GetToleranceCurWaypoint() 
	{
		return CurWaypointIsGoal() ? m_fGoalTolerance : m_waypointTolerance;
	}

	void Clear()
	{
		m_iGoalType = GOALTYPE_NONE;
		m_iGoalFlags = 0;
		m_hTarget = NULL;
		m_bAvoidEnemies = true;
		SetWaypoint(NULL);
	}

	void SetWaypoint( UnitBaseWaypoint *pWayPoint )
	{
		m_pWaypointHead = pWayPoint;
	}

	void Advance()
	{
		if ( CurWaypointIsGoal() )
			return;

		// -------------------------------------------------------
		// If I have another waypoint advance my path
		// -------------------------------------------------------
		if (GetCurWaypoint()->GetNext()) 
		{
			UnitBaseWaypoint *pNext = GetCurWaypoint()->GetNext();

			delete GetCurWaypoint();
			m_pWaypointHead = pNext;

			return;
		}
		// -------------------------------------------------
		//  This is an error catch that should *not* happen
		//  It means a route was created with no goal
		// -------------------------------------------------
		else 
		{
			DevMsg( "!!ERROR!! Force end of route with no goal!\n");
		}
	}

public:
	int m_iGoalType;
	UnitBaseWaypoint *m_pWaypointHead;
	Vector m_vGoalPos;
	Vector m_vGoalInRangePos;
	float m_waypointTolerance;
	float m_fGoalTolerance;
	int m_iGoalFlags;
	float m_fMinRange, m_fMaxRange;
	EHANDLE m_hTarget;
	bool m_bAvoidEnemies;
};

#define CONSIDER_SIZE 48
#define MAX_TESTDIRECTIONS 8

//-----------------------------------------------------------------------------
// Purpose: Navigation class. Path building/updating and local obstacle avoidance.
//-----------------------------------------------------------------------------
class UnitBaseNavigator : public UnitComponent
{
	DECLARE_CLASS(UnitBaseNavigator, UnitComponent);
public:
	friend class CUnitBase;

#ifndef DISABLE_PYTHON
	UnitBaseNavigator( boost::python::object outer );
#endif // DISABLE_PYTHON

	// Core
	virtual void		Reset();
	virtual void		StopMoving();
	void				DispatchOnNavComplete();
	void				DispatchOnNavFailed();
	virtual void		Update( UnitBaseMoveCommand &mv );
	virtual void		UpdateFacingTargetState( bool bIsFacing );
	virtual void		UpdateIdealAngles( UnitBaseMoveCommand &MoveCommand, Vector *pathdir=NULL );
	virtual void		UpdateGoalStatus( UnitBaseMoveCommand &MoveCommand, CheckGoalStatus_t GoalStatus );
	virtual void		CalcMove( UnitBaseMoveCommand &MoveCommand, QAngle angles, float speed );

	// flow stuff
	float				GetDensityMultiplier();
	Vector				GetWishVelocity() { return m_vLastWishVelocity; }

	virtual float		GetEntityBoundingRadius( CBaseEntity *pEnt );
	virtual void		RegenerateConsiderList( Vector &vPathDir, CheckGoalStatus_t GoalStatus );
	virtual bool		ShouldConsiderEntity( CBaseEntity *pEnt );
	virtual bool		ShouldConsiderNavMesh( void );

	float				ComputeDensityAndAvgVelocity( int iPos, Vector *pAvgVelocity );
	float				ComputeEntityDensity( const Vector &vPos, CBaseEntity *pEnt );

	float				ComputeUnitCost( int iPos, Vector *pFinalVelocity, CheckGoalStatus_t GoalStatus, 
								UnitBaseMoveCommand &MoveCommand, Vector &vPathDir, float &fGoalDist );
	Vector				ComputeVelocity( CheckGoalStatus_t GoalStatus, UnitBaseMoveCommand &MoveCommand, Vector &vPathDir, float &fGoalDist );

	float				CalculateAvgDistHistory();

	// Path updating
	virtual CheckGoalStatus_t UpdateGoalAndPath( UnitBaseMoveCommand &MoveCommand, Vector &vPathDir, float &fGoalDist );
	virtual bool		IsInRangeGoal( UnitBaseMoveCommand &MoveCommand );
	virtual CheckGoalStatus_t	MoveUpdateWaypoint();
	Vector				ComputeWaypointTarget( const Vector &start, UnitBaseWaypoint *pEnd );
	virtual void		AdvancePath();
	virtual bool		UpdateReactivePath( bool bNoRecomputePath = false );

	virtual bool		IsCompleteInArea( CNavArea *pArea, const Vector &vPos );
	//virtual bool		TestPosition( const Vector &vPosition );
	virtual bool		TestRouteEnd( UnitBaseWaypoint *pWaypoint );
	virtual bool		TestRoute( const Vector &vStartPos, const Vector &vEndPos );

	BlockedStatus_t		GetBlockedStatus( void );
	virtual void		ResetBlockedStatus( void );
	virtual void		UpdateBlockedStatus( void );

	// Goals
	virtual bool		SetGoal( Vector &destination, float goaltolerance=64.0f, int goalflags=0, bool avoidenemies=true );
	virtual bool		SetGoalTarget( CBaseEntity *pTarget, float goaltolerance=64.0f, int goalflags=0, bool avoidenemies=true );
	virtual bool		SetGoalInRange( Vector &destination, float maxrange, float minrange=0.0f, float goaltolerance=0.0f, int goalflags=0, bool avoidenemies=true );
	virtual bool		SetGoalTargetInRange( CBaseEntity *pTarget, float maxrange, float minrange=0.0f, float goaltolerance=0.0f, int goalflags=0, bool avoidenemies=true );

	bool				SetVectorGoal( const Vector &dir, float targetDist, float minDist = 0, bool fShouldDeflect = false );
	virtual bool		SetVectorGoalFromTarget( Vector &destination, float minDist, float goaltolerance=64.0f) { return SetGoal(destination, goaltolerance); }

	virtual void		UpdateGoalInRange( float maxrange, float minrange=0.0f );
	virtual void		UpdateGoalInfo( void );
	virtual float		GetGoalDistance( void );

	// Path finding
	virtual bool		FindPath( int goaltype, const Vector &vDestination, float fGoalTolerance, int goalflags=0, float fMinRange=0.0f, float fMaxRange=0.0f );
	virtual bool		DoFindPathToPos();
	virtual bool		DoFindPathToPosInRange();

	// Route buiding
	virtual UnitBaseWaypoint *	BuildLocalPath( const Vector &pos );
	virtual UnitBaseWaypoint *	BuildWayPointsFromRoute(CNavArea *goalArea, UnitBaseWaypoint *pWayPoint, int prevdir=-1);
	virtual UnitBaseWaypoint *	BuildNavAreaPath( const Vector &pos );
	virtual UnitBaseWaypoint *	BuildRoute();

	// Getters/Setters for path
#ifndef DISABLE_PYTHON
	void SetPath( boost::python::object path );
	inline boost::python::object PyGetPath() { return m_refPath; }
#endif // DISABLE_PYTHON
	inline UnitBasePath *GetPath() { return m_pPath; }

	// Facing
	float				GetIdealYaw();
	void				SetIdealYaw( float fIdealYaw );
	CBaseEntity *		GetFacingTarget();
	void				SetFacingTarget( CBaseEntity *pFacingTarget );
	const Vector &		GetFacingTargetPos();
	void				SetFacingTargetPos( Vector &vFacingTargetPos );

	// Misc
	//void				CalculateDestinationInRange( Vector *pResult, const Vector &vGoalPos, float minrange, float maxrange);
	bool				FindVectorGoal( Vector *pResult, const Vector &dir, float targetDist, float minDist=0 );
	static void			CalculateDeflection( const Vector &start, const Vector &dir, const Vector &normal, Vector *pResult );

	// Debug
	virtual void		DrawDebugRouteOverlay();	
	virtual void		DrawDebugInfo();

public:
	// Facing settings
	float m_fIdealYawTolerance;
	float m_fFacingCone;

	// Override goal / path velocity
	Vector m_vForceGoalVelocity;

	// Override avoidance behavior
	bool m_bNoAvoid;

protected:
	CheckGoalStatus_t m_LastGoalStatus;

private:
	// Facing
	float m_fIdealYaw;
	EHANDLE m_hFacingTarget;
	Vector m_vFacingTargetPos;
	bool m_bFacingFaceTarget;

	// Path and pathfinding
	UnitBasePath *m_pPath;
#ifndef DISABLE_PYTHON
	boost::python::object m_refPath;
#endif // DISABLE_PYTHON
	float m_fGoalDistance;
	Vector m_vGoalDir;

	// Position checking
	BlockedStatus_t m_BlockedStatus;
	float m_fBlockedStartTime;
	float m_fNextLastPositionCheck;
	float m_fLastPathRecomputation;
	float m_fNextReactivePathUpdate;
	float m_fNextAllowPathRecomputeTime;
	bool m_bNoNavAreasNearby;
	Vector m_vLastPosition;

	// Misc
	float m_fClimbHeight;
	Vector m_vecClimbDirection;

	// Flow
	struct consider_pos_t {
		float m_fDensity;
	};
	struct consider_entity_t {
		EHANDLE m_pEnt;
		consider_pos_t positions[MAX_TESTDIRECTIONS];
	};

	consider_entity_t m_ConsiderList[CONSIDER_SIZE];
	int m_iConsiderSize;

	Vector m_vTestDirections[MAX_TESTDIRECTIONS];
	Vector m_vTestPositions[MAX_TESTDIRECTIONS];
	int m_iUsedTestDirections;

	float m_fLastComputedDensity;
	float m_fLastBestDensity;
	float m_fLastBestCost;
	float m_fLastComputedDist;
	float m_fLastBestDist;
	Vector m_vLastWishVelocity;
	float m_fDiscomfortWeight;

	struct dist_entry_t {
		dist_entry_t( float fDist, float fTimeStamp ) : m_fDist(fDist), m_fTimeStamp(fTimeStamp) {}
		float m_fDist;
		float m_fTimeStamp;
	};
	CUtlVector<dist_entry_t> m_DistHistory;
	float m_fNextAvgDistConsideration;
	float m_fLastAvgDist;

	struct seed_entry_t {
		seed_entry_t( Vector2D vPos, float fTimeStamp ) : m_vPos(vPos), m_fTimeStamp(fTimeStamp) {}
		Vector2D m_vPos;
		float m_fTimeStamp;
	};
	CUtlVector<seed_entry_t> m_Seeds;

	Vector m_vDebugVelocity;
};

// ---------------------------------------------------------------
// Inlines
inline float UnitBaseNavigator::GetEntityBoundingRadius( CBaseEntity *pEnt )
{
	return pEnt->CollisionProp()->BoundingRadius2D();
}

inline BlockedStatus_t UnitBaseNavigator::GetBlockedStatus( void )
{
	return m_BlockedStatus;
}

inline float UnitBaseNavigator::GetIdealYaw()
{
	return m_fIdealYaw;
}

inline void UnitBaseNavigator::SetIdealYaw( float fIdealYaw )
{
	m_fIdealYaw = fIdealYaw;
	m_bFacingFaceTarget = false;
}

inline CBaseEntity *UnitBaseNavigator::GetFacingTarget()
{
	return m_hFacingTarget;
}

inline void UnitBaseNavigator::SetFacingTarget( CBaseEntity *pFacingTarget )
{
	m_hFacingTarget = pFacingTarget;
	m_bFacingFaceTarget = false;
}

inline const Vector &UnitBaseNavigator::GetFacingTargetPos()
{
	return m_vFacingTargetPos;
}

inline void UnitBaseNavigator::SetFacingTargetPos( Vector &vFacingTargetPos )
{
	m_vFacingTargetPos = vFacingTargetPos;
	m_bFacingFaceTarget = false;
}

#endif // UNIT_NAVIGATOR_H
