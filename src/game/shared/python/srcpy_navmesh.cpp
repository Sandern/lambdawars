//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy_navmesh.h"
#include "wars_mapboundary.h"
//#include "hl2wars_nav_pathfind.h"
#include "collisionutils.h"
#include "recast/recast_mgr.h"
#include "recast/recast_mesh.h"
#include "unit_base_shared.h"

#include "wars_grid.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar g_pynavmesh_debug("g_pynavmesh_debug", "0", FCVAR_CHEAT|FCVAR_REPLICATED);
static ConVar g_pynavmesh_debug_hidespot("g_pynavmesh_debug_hidespot", "0", FCVAR_CHEAT|FCVAR_REPLICATED);

#define DEFAULT_MESH "human"

//-----------------------------------------------------------------------------
// Purpose: Keep?
//-----------------------------------------------------------------------------
bool NavMeshAvailable()
{
	return RecastMgr().HasMeshes();
}

#if 0
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool NavMeshTestHasArea( Vector &pos, float beneathLimt )
{
	return TheNavMesh->GetNavArea( pos, beneathLimt ) != NULL;
}
#endif // 0

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float NavMeshGetPathDistance( Vector &vStart, Vector &vGoal, float maxdist, CBaseEntity *pUnit, float fBeneathLimit )
{
	if( g_pynavmesh_debug.GetInt() > 1 )
		DevMsg("NavMeshGetPathDistance: maxdist: %f, unit: %d, fBeneathLimit: %f\n", maxdist, pUnit, fBeneathLimit);

	CRecastMesh *pMesh = pUnit ? RecastMgr().GetMesh( RecastMgr().FindBestMeshForEntity( pUnit ) ) : RecastMgr().GetMesh( DEFAULT_MESH );
	if( !pMesh )
		return 0;

	return pMesh->FindPathDistance( vStart, vGoal, NULL, fBeneathLimit );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector NavMeshGetPositionNearestNavArea( const Vector &pos, float beneathlimit, float maxradius, CBaseEntity *pUnit )
{
	CRecastMesh *pMesh = pUnit ? RecastMgr().GetMesh( RecastMgr().FindBestMeshForEntity( pUnit ) ) : RecastMgr().GetMesh( DEFAULT_MESH );
	if( !pMesh )
		return vec3_origin;

	return pMesh->ClosestPointOnMesh( pos, beneathlimit, maxradius );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DestroyAllNavAreas()
{
	RecastMgr().Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector RandomNavAreaPosition( CBaseEntity *pUnit )
{
	if( !GetMapBoundaryList() )
	{
		Warning("RandomNavAreaPosition invalid call: requires map boundary.\n");
		return vec3_origin;
	}

	Vector mins, maxs;
	mins.x = mins.y = mins.z = MAX_COORD_FLOAT;
	maxs.x = maxs.y = maxs.z = -MAX_COORD_FLOAT;
	for( CBaseFuncMapBoundary *pEnt = GetMapBoundaryList(); pEnt != NULL; pEnt = pEnt->m_pNext )
	{
		Vector othermins, othermaxs;
		pEnt->GetMapBoundary( othermins, othermaxs );

		mins.x = Min(mins.x, othermins.x);
		mins.y = Min(mins.y, othermins.y);
		mins.z = Min(mins.z, othermins.z);
		maxs.x = Max(maxs.x, othermaxs.x);
		maxs.y = Max(maxs.y, othermaxs.y);
		maxs.z = Max(maxs.z, othermaxs.z);
	}

	return RandomNavAreaPositionWithin( mins, maxs, pUnit );
}

//-----------------------------------------------------------------------------
// Purpose: 
// TODO: Remove "tries" and add optional "unit" parameter for mesh selection
//-----------------------------------------------------------------------------
Vector RandomNavAreaPositionWithin( const Vector &mins, const Vector &maxs, CBaseEntity *pUnit )
{
	Vector center = (maxs - mins) / 2.0f;
	float radius = maxs.AsVector2D().DistTo(mins.AsVector2D());

	CRecastMesh *pMesh = pUnit ? RecastMgr().GetMesh( RecastMgr().FindBestMeshForEntity( pUnit ) ) : RecastMgr().GetMesh( DEFAULT_MESH );
	if( !pMesh )
		return vec3_origin;

	// The "findRandomPointAroundCircle" is not too random, so pick a random start position, which is used as starting point for the random
	// point on a poly on the navigation mesh. This function is not really used on the client, so not implemented right now for client.
#ifndef CLIENT_DLL
	CUtlVector< Vector > startPoints;
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "info_start_wars" );
	while( pEnt )
	{
		const Vector &vOrigin = pEnt->GetAbsOrigin();
		if( IsPointInBox( vOrigin, mins, maxs ) )
		{
			startPoints.AddToTail( pEnt->GetAbsOrigin() );
		}
		
		pEnt = gEntList.FindEntityByClassname( pEnt, "info_start_wars" );
	}
	Vector *vStartPoint = startPoints.Count() > 0 ? &startPoints[random->RandomInt(0, startPoints.Count()-1)] : NULL;
#else
	// TODO/Maybe. Not really needed.
	const Vector *vStartPoint = NULL;
#endif // 0

	Vector vRandomPoint = pMesh->RandomPointWithRadius( center, radius, vStartPoint );
	if( vRandomPoint != vec3_origin )
	{
		if( g_pynavmesh_debug.GetBool() )
			DevMsg("RandomNavAreaPosition: Found position %f %f %f (center: %f %f %f, radius: %f)\n", 
			vRandomPoint.x, vRandomPoint.y, vRandomPoint.z, center.x, center.y, center.z, radius );
		return vRandomPoint + Vector( 0, 0, 32.0f );
	}

	if( g_pynavmesh_debug.GetBool() )
		DevMsg("RandomNavAreaPosition: No position found within Mins: %f %f %f, Maxs: %f %f %f (center: %f %f %f, radius: %f)\n", 
				mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z, center.x, center.y, center.z, radius );
	return vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int GetNavAreaAt( const Vector &pos, float beneathlimit )
{
	CUnitBase *pUnit = NULL;
	CRecastMesh *pMesh = pUnit ? RecastMgr().GetMesh( RecastMgr().FindBestMeshForEntity( pUnit ) ) : RecastMgr().GetMesh( DEFAULT_MESH );
	if( !pMesh )
		return 0;
	return pMesh->GetPolyRef( pos, beneathlimit );
}

//-----------------------------------------------------------------------------
// Purpose: Tests if polys overlapping the bounds are within the flat tolerance.
//-----------------------------------------------------------------------------
bool NavIsAreaFlat( const Vector &mins, const Vector &maxs, float fFlatTol, CUnitBase *pUnit )
{
	CRecastMesh *pMesh = pUnit ? RecastMgr().GetMesh( RecastMgr().FindBestMeshForEntity( pUnit ) ) : RecastMgr().GetMesh( DEFAULT_MESH );
	if( !pMesh )
		return false;

	return pMesh->IsAreaFlat( (maxs + mins) / 2.0f, (maxs - mins) / 2.0f, fFlatTol );
}

#define HIDESPOT_DEBUG_DURATION 0.25f

#ifdef CLIENT_DLL
	ConVar disable_hidingspots( "cl_disable_coverspots", "0", FCVAR_CHEAT );
#else
	ConVar disable_hidingspots( "sv_disable_coverspots", "0", FCVAR_CHEAT );
#endif // CLIENT_DLL
ConVar coverspot_required_nav_radius( "coverspot_required_nav_radius", "2", FCVAR_CHEAT|FCVAR_REPLICATED );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
typedef struct CoverSpotResult_t
{
	CoverSpotResult_t( wars_coverspot_t *spot, float dist ) : pSpot(spot), fDist(dist) {}

	wars_coverspot_t *pSpot;
	float fDist;
} CoverSpotResult_t;

static int CoverCompare(const CoverSpotResult_t *pLeft, const CoverSpotResult_t *pRight )
{
	if( pLeft->fDist < pRight->fDist )
		return -1; 
	else if( pLeft->fDist > pRight->fDist )
		return 1;
	return 0;
}


class CoverSpotCollector
{
public:
	CoverSpotCollector( CUtlVector< CoverSpotResult_t > &coverSpots, const Vector &vPos, float fRadius, CUnitBase *pUnit, const Vector *pSortPos ) : 
			m_CoverSpots( coverSpots ), m_vPos( vPos ), m_pUnit(pUnit), m_pSortPos( pSortPos )
	{
		m_fRadius = fRadius;

		m_pMesh = pUnit ? RecastMgr().GetMesh( RecastMgr().FindBestMeshForEntity( pUnit ) ) : RecastMgr().GetMesh( DEFAULT_MESH );
		if( m_pMesh && !m_pMesh->IsLoaded() )
			m_pMesh = NULL;

#if 0
		if( vPos.IsValid() )
			m_pOrderArea = TheNavMesh->GetNearestNavArea(vPos, false, 1250.0f + fRadius, false, true);
		else
			Warning( "HidingSpotCollector: Invalid input position %f %f %f\n", vPos.x, vPos.y, vPos.z );

		if( g_pynavmesh_debug_hidespot.GetBool() )
		{
			char buf[512];
			if( m_pOrderArea )
			{
				V_snprintf( buf, 512, "HidingSpotCollector: HAS nav mesh! %d spots. Test Radius: %f", m_pOrderArea->GetHidingSpots()->Count(), m_fRadius );
				NDebugOverlay::Text( m_pOrderArea->GetCenter(), buf, false, HIDESPOT_DEBUG_DURATION );
			}
			else
			{
				V_snprintf( buf, 512, "HidingSpotCollector: no nav mesh! Test Radius: %f", m_fRadius );
				NDebugOverlay::Text( vPos, buf, false, HIDESPOT_DEBUG_DURATION );
			}
		}
#endif // 0
	}

	bool operator() ( wars_coverspot_t *coverSpot )
	{
#if 0
		// Figure out normal and skip if the slope is crap
		CNavArea *area = TheNavMesh->GetNearestNavArea(coverSpot->position, false, 1250.0f + m_fRadius, false, true);
		if( !area ) 
		{
			return true;
		}

		Vector normal;
		area->ComputeNormal( &normal );
		float fSlope = DotProduct( normal, Vector(0, 0, 1) );
		if( fSlope < 0.83f )
		{
			if( g_pynavmesh_debug_hidespot.GetBool() )
				NDebugOverlay::Text( area->GetCenter(), "HidingSpotCollector: skipping area due slope", false, HIDESPOT_DEBUG_DURATION );
			return true;
		}

		// Area must be reachable
		float fPathDist;
		if( !m_pUnit )
		{
			ShortestPathCost costFunc;
			fPathDist = NavAreaTravelDistance<ShortestPathCost>( m_pOrderArea, area, costFunc, 1250.0f + m_fRadius );
		}
		else
		{
			UnitShortestPathCost costFunc( m_pUnit, false );
			fPathDist = NavAreaTravelDistance<UnitShortestPathCost>( m_pOrderArea, area, costFunc, 1250.0f + m_fRadius );
		}
#endif // 0

		if( m_pMesh )
		{
			float fPathDist = m_pMesh->FindPathDistance( m_vPos, coverSpot->position );
			if( fPathDist < 0 )
			{
				return true;
			}
		}

		// Collect spots in range
		const Vector &vPos = coverSpot->position;
		float fDist = ( vPos - m_vPos ).Length2D();

		if( fDist < m_fRadius )
		{
			// When m_pSortPos is defined, an alternative distance is calculated for sorting
			if( m_pSortPos )
				m_CoverSpots.AddToTail( CoverSpotResult_t( coverSpot, ( vPos - *m_pSortPos ).Length2D() ) );
			else
				m_CoverSpots.AddToTail( CoverSpotResult_t( coverSpot, fDist ) );

			if( g_pynavmesh_debug_hidespot.GetBool() )
				NDebugOverlay::Box( vPos, -Vector(8, 8, 8), Vector(8, 8, 8), 0, 255, 0, true, HIDESPOT_DEBUG_DURATION );
		}
		else
		{
			if( g_pynavmesh_debug_hidespot.GetBool() )
			{
				NDebugOverlay::Box( vPos, -Vector(8, 8, 8), Vector(8, 8, 8), 0, 0, 255, true, HIDESPOT_DEBUG_DURATION );
			}
		}

		return true;
	}

private:
	CUnitBase *m_pUnit;
	CRecastMesh *m_pMesh;
	float m_fRadius;
	const Vector &m_vPos;
	const Vector *m_pSortPos;
	CUtlVector< CoverSpotResult_t > &m_CoverSpots;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
boost::python::list GetHidingSpotsInRadius( const Vector &pos, float radius, CUnitBase *pUnit, bool bSort, const Vector *pSortPos )
{
	boost::python::list l;

	if( disable_hidingspots.GetBool() )
		return l;

	// Get hiding spots in radius
	CUtlVector< CoverSpotResult_t > coverSpots;
	CoverSpotCollector collector( coverSpots, pos, radius, pUnit, pSortPos );
	WarsGrid().ForAllCoverInRadius( collector, pos, radius );

	if( bSort )
	{
		// Sort based on distance
		coverSpots.Sort( CoverCompare );
	}

	CRecastMesh *pMesh = pUnit ? RecastMgr().GetMesh( RecastMgr().FindBestMeshForEntity( pUnit ) ) : RecastMgr().GetMesh( DEFAULT_MESH );

	// Python return result
	float fRequiredNavRadius = coverspot_required_nav_radius.GetFloat();
	for( int i = 0; i < coverSpots.Count(); i++ )
	{
		// Should be on the navigation mesh! Search in a small radius, cover spots are always for soldiers.
		if( pMesh && fRequiredNavRadius )
		{
			Vector pos = coverSpots[i].pSpot->position;
			pos.z += 2.0f;
			if( !pMesh->TestRoute( pos - Vector(fRequiredNavRadius, 0, 0), pos + Vector(fRequiredNavRadius, 0, 0) ) ||
				!pMesh->TestRoute( pos - Vector(0, fRequiredNavRadius, 0), pos + Vector(0, fRequiredNavRadius, 0) ) )
			{
				continue;
			}
		}

		l.append( boost::python::make_tuple( coverSpots[i].pSpot->id, coverSpots[i].pSpot->position ) );
	}

	return l;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CreateHidingSpot( const Vector &pos, bool notsaved, bool checkground )
{
	return WarsGrid().CreateCoverSpot( pos, notsaved ? wars_coverspot_t::NOTSAVED : 0 );
}

bool DestroyHidingSpot( const Vector &pos, float tolerance, int num, unsigned int excludeFlags )
{
	// Get hiding spots in radius
	CUtlVector< CoverSpotResult_t > coverSpots;
	CoverSpotCollector collector( coverSpots, pos, tolerance, NULL, NULL );
	WarsGrid().ForAllCoverInRadius( collector, pos, tolerance );

	// Sort based on distance
	coverSpots.Sort( CoverCompare );

	for( int i = 0; i < coverSpots.Count(); i++ )
	{
		if( i >= num )
			break;
		if( coverSpots[i].pSpot->flags & excludeFlags )
			continue;
		DestroyHidingSpotByID( coverSpots[i].pSpot->position, coverSpots[i].pSpot->id );
	}

	return false;
}

bool DestroyHidingSpotByID( const Vector &pos, unsigned int hidespotid )
{
	return WarsGrid().DestroyCoverSpot( pos, hidespotid );
}