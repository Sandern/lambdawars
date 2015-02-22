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

ConVar g_pynavmesh_debug("g_pynavmesh_debug", "0", FCVAR_CHEAT|FCVAR_REPLICATED);
ConVar g_pynavmesh_debug_hidespot("g_pynavmesh_debug_hidespot", "0", FCVAR_CHEAT|FCVAR_REPLICATED);
//ConVar g_pynavmesh_hidespot_searchmethod("g_pynavmesh_hidespot_searchmethod", "0", FCVAR_CHEAT|FCVAR_REPLICATED);

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
// TODO: Remove unused parameters
//-----------------------------------------------------------------------------
float NavMeshGetPathDistance( Vector &vStart, Vector &vGoal, bool anyz, float maxdist, bool bNoTolerance, CBaseEntity *pUnit )
{
	if( g_pynavmesh_debug.GetInt() > 1 )
		DevMsg("NavMeshGetPathDistance: anyz: %d, maxdist: %f, bNoTolerance: %d, unit: %d\n", anyz, maxdist, bNoTolerance, pUnit);

	CRecastMesh *pMesh = pUnit ? RecastMgr().GetMesh( RecastMgr().FindBestMeshForEntity( pUnit ) ) : RecastMgr().GetMesh( DEFAULT_MESH );
	if( !pMesh )
		return 0;

	return pMesh->FindPathDistance( vStart, vGoal );

#if 0
	CNavArea *startArea, *goalArea;
	if( bNoTolerance )
	{
		startArea = TheNavMesh->GetNavArea(vStart);
		goalArea = TheNavMesh->GetNavArea(vGoal);
	}
	else
	{
		startArea = TheNavMesh->GetNearestNavArea(vStart, anyz, maxdist, false, false);
		goalArea = TheNavMesh->GetNearestNavArea(vGoal, anyz, maxdist, false, false);
	}
	if( !startArea || !goalArea )
	{
		if( g_pynavmesh_debug.GetBool() )
			DevMsg("NavMeshGetPathDistance: No start=%d or goal=%d area\n", startArea, goalArea);
		return -1;
	}

	if( g_pynavmesh_debug.GetInt() > 1 )
		DevMsg("NavMeshGetPathDistance: startArea: %d, goalArea: %d\n", startArea->GetID(), goalArea->GetID());

	if( !pUnit )
	{
		ShortestPathCost costFunc;
		return NavAreaTravelDistance<ShortestPathCost>(startArea, goalArea, costFunc, maxdist);
	}
	else
	{
		UnitShortestPathCost costFunc( pUnit, false );
		return NavAreaTravelDistance<UnitShortestPathCost>(startArea, goalArea, costFunc, maxdist);
	}
#endif // 0
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
int CreateNavArea( const Vector &corner, const Vector &otherCorner )
{
	return -1;
#ifndef CLIENT_DLL
	//return TheNavMesh->CreateNavArea( corner, otherCorner );
#else
	return -1;
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CreateNavAreaByCorners( const Vector &nwCorner, const Vector &neCorner, const Vector &seCorner, const Vector &swCorner )
{
	return -1;
#ifndef CLIENT_DLL
	//return TheNavMesh->CreateNavArea( nwCorner, neCorner, seCorner, swCorner );
#else
	return -1;
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DestroyNavArea( unsigned int id )
{
#if 0 // TODO
	CNavArea *area = TheNavMesh->GetNavAreaByID( id );
	if( area )
	{
		TheNavAreas.FindAndRemove( area );
		TheNavMesh->DestroyArea( area );
	}
#endif // 0
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
	CUtlVector< CBaseEntity *> startPoints;
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "info_start_wars" );
	while( pEnt )
	{
		startPoints.AddToTail( pEnt );
		pEnt = gEntList.FindEntityByClassname( pEnt, "info_start_wars" );
	}
	const Vector *vStartPoint = startPoints.Count() > 0 ? &startPoints[random->RandomInt(0, startPoints.Count()-1)]->GetAbsOrigin() : NULL;
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

#if 0
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int GetActiveNavMesh()
{
	CNavArea *pArea = TheNavMesh->GetSelectedArea();
	if( !pArea )
		return -1;

	return pArea->GetID();
}
#endif // 0

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

#if 0
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
boost::python::list GetNavAreasAtBB( const Vector &mins, const Vector &maxs )
{
	boost::python::list l;
	CNavArea *area;
	Extent extent;
	Vector vAbsMins2, vAbsMaxs2;

	// TODO: Use ForAllAreasOverlappingExtent?
	FOR_EACH_VEC( TheNavAreas, it )
	{
		area = TheNavAreas[ it ];

		area->GetExtent( &extent );

		vAbsMins2 = extent.lo;
		vAbsMaxs2 = extent.hi;

		if( vAbsMins2[2] > vAbsMaxs2[2] )
		{
			float z = vAbsMaxs2[2];
			vAbsMaxs2[2] = vAbsMins2[2];
			vAbsMins2[2] = z;
		}
		else if( vAbsMins2[2] == vAbsMaxs2[2] )
		{
			vAbsMaxs2[2] += 1.0f;
		}

		// Does it intersects with our bounding box?
		if( IsBoxIntersectingBox( mins, maxs, 
			vAbsMins2, vAbsMaxs2 ) )
		{
			l.append( area->GetID() );
		}
	}

	return l;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SplitAreasAtBB( const Vector &mins, const Vector &maxs )
{
	CNavArea *area, *other;
	Extent extent;
	Vector vAbsMins2, vAbsMaxs2;

	CUtlVector< CNavArea * > Areas;

	FOR_EACH_VEC( TheNavAreas, it )
	{
		area = TheNavAreas[ it ];

		area->GetExtent( &extent );

		vAbsMins2 = extent.lo;
		vAbsMaxs2 = extent.hi;

		// seems the extents are only 2d valid hi and lo
		if( vAbsMins2[2] > vAbsMaxs2[2] )
		{
			float z = vAbsMaxs2[2];
			vAbsMaxs2[2] = vAbsMins2[2];
			vAbsMins2[2] = z;
		}
		else if( vAbsMins2[2] == vAbsMaxs2[2] )
		{
			vAbsMaxs2[2] += 1.0f;
		}

		// does it intersects?
		if( IsBoxIntersectingBox( mins, maxs, 
			vAbsMins2, vAbsMaxs2 ) )
		{
			Areas.AddToTail( area );
		}
	}

	// now split them
	CUtlVector<int> ids;
	for( int i = 0; i < Areas.Count(); i++ )
	{
		area = Areas[i];
	
		// First split up into areas covering the given bounds
		if( area->SplitEdit( false, TheNavMesh->SnapToGrid(mins.x, true), &other, &area ) )
		{
			ids.AddToTail(other->GetID());
		}
		//TheNavMesh->StripNavigationAreas();

		if( area->SplitEdit( false, TheNavMesh->SnapToGrid(maxs.x, true), &area, &other ) )
		{
			ids.AddToTail(other->GetID());
		}
		//TheNavMesh->StripNavigationAreas();

		if( area->SplitEdit( true, TheNavMesh->SnapToGrid(mins.y, true), &other, &area ) )
		{
			ids.AddToTail(other->GetID());
		}
		//TheNavMesh->StripNavigationAreas();

		if( area->SplitEdit( true, TheNavMesh->SnapToGrid(maxs.y, true), &area, &other ) )
		{
			ids.AddToTail(other->GetID());
		}
		ids.AddToTail(area->GetID());
		//TheNavMesh->StripNavigationAreas();
	}

	CNavArea::ClearCachedPath(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SetAreasBlocked( boost::python::list areas, bool blocked, CBaseEntity *pOwner )
{
#ifndef CLIENT_DLL
	CNavArea *area;
	int id;
	int length = boost::python::len(areas);
	for(int i=0; i<length; i++)
	{
		id = boost::python::extract<int>( areas[i] );
		area = TheNavMesh->GetNavAreaByID( id );
		if( area )
		{
			if( blocked ) 
			{
				area->SetAttributes( area->GetAttributes()|NAV_MESH_NAV_BLOCKER );
			}
			else
			{
				area->SetAttributes( area->GetAttributes()&(~NAV_MESH_NAV_BLOCKER) );
			}
			area->SetOwner( pOwner );
		}
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool IsAreaBlocked( int areaid )
{
	CNavArea *area = TheNavMesh->GetNavAreaByID( areaid );
	if( area )
	{
		return (area->GetAttributes() & NAV_MESH_NAV_BLOCKER) != 0;
	}

	PyErr_SetString(PyExc_Exception, "Could not find area for given id" );
	throw boost::python::error_already_set();
}
#endif // 0

#ifndef CLIENT_DLL
static ConVar g_debug_coveredbynavareas("g_debug_coveredbynavareas", "0", FCVAR_CHEAT);
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool IsBBCoveredByNavAreas( const Vector &mins, const Vector &maxs, float tolerance, bool bRequireIsFlat, float fFlatTol )
{
#if 0
	float fArea, fGoalArea;
	CNavArea *area;
	Extent extent;
	Vector vAbsMins2, vAbsMaxs2;
	Vector vNormal;

	fGoalArea = (maxs.x - mins.x) * (maxs.y - mins.y);
	fArea = 0.0f;

	if( !TheNavMesh->IsLoaded() )
		return true;

	// TODO: Use ForAllAreasOverlappingExtent?
	FOR_EACH_VEC( TheNavAreas, it )
	{
		area = TheNavAreas[ it ];

		area->GetExtent( &extent );

		vAbsMins2 = extent.lo;
		vAbsMaxs2 = extent.hi;

		if( vAbsMins2[2] > vAbsMaxs2[2] )
		{
			float z = vAbsMaxs2[2];
			vAbsMaxs2[2] = vAbsMins2[2];
			vAbsMins2[2] = z;
		}
		else if( vAbsMins2[2] == vAbsMaxs2[2] )
		{
			vAbsMaxs2[2] += 1.0f;
		}

		// Does it intersects with our bounding box?
		if( IsBoxIntersectingBox( mins, maxs, 
			vAbsMins2, vAbsMaxs2 ) )
		{

			fArea += area->GetSizeX() * area->GetSizeY();

			if( bRequireIsFlat )
			{
				area->ComputeNormal( &vNormal );
				if( vNormal[2] < fFlatTol )
				{
#ifndef CLIENT_DLL
					if( g_debug_coveredbynavareas.GetBool() )
						DevMsg("IsBBCoveredByNavAreas: Normal area %d is %f (> %f)\n", area->GetID(), vNormal[2], fFlatTol);
#endif // CLIENT_DLL
					return false;
				}
			}
		}
	}

	if( fArea > fGoalArea - (fGoalArea * tolerance) )
		return true;
#ifndef CLIENT_DLL
	if( g_debug_coveredbynavareas.GetBool() )
		DevMsg("IsBBCoveredByNavAreas: goal area %f, total area: %f, tol: %f\n", fGoalArea, fArea, tolerance);
#endif // CLIENT_DLL
	return false;
#else
	return true; // TODO!
#endif // 0
}

#if 0
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool TryMergeSurrounding( int id, float tolerance )
{
	CNavArea *pArea = TheNavMesh->GetNavAreaByID( id );
	if( !pArea )
	{
		Warning("Invalid nav area %d\n", id);
		return false;
	}

	return TheNavMesh->TryMergeSingleArea( pArea, tolerance );
}
#endif // 0

#if 0
#ifndef CLIENT_DLL
CON_COMMAND_F( nav_verifyareas, "", FCVAR_CHEAT)
{
	Msg("Verifying areas\n");
	CUtlVector<int> tested;

	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];
		int id = area->GetID();

		bool bAlreadyTested = false;
		FOR_EACH_VEC( tested, it3 )
		{
			if( tested[it3] == id )
			{
				bAlreadyTested = true;
				break;
			}
		}
		if( bAlreadyTested )
			continue;

		int iCount = 0;
		FOR_EACH_VEC( TheNavAreas, it2 )
		{
			CNavArea *area2 = TheNavAreas[ it2 ];

			if( area->GetID() == area2->GetID() )
			{
				iCount++;
				
			}
		}	

		if( iCount > 1 )
		{
			Warning("Area %d occurs multiple times in the nav area list! (%d times)\n", id, iCount);
		}
	}
	Msg("Total of %d areas\n", TheNavAreas.Count());
}
#endif // CLIENT_DLL
#endif // 0

#define HIDESPOT_DEBUG_DURATION 0.25f

#if 0
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
typedef struct HidingSpotResult_t
{
	HidingSpotResult_t( HidingSpot *spot, float dist ) : pSpot(spot), fDist(dist) {}

	HidingSpot *pSpot;
	float fDist;
} HidingSpotResult_t;

class HidingSpotCollector
{
public:
	HidingSpotCollector( CUtlVector< HidingSpotResult_t > &HidingSpots, const Vector &vPos, float fRadius, CUnitBase *pUnit, const Vector *pSortPos ) : 
			m_HidingSpots( HidingSpots ), m_vPos( vPos ), m_pUnit(pUnit), m_pOrderArea(NULL), m_pSortPos( pSortPos )
	{
		m_fRadius = fRadius;

		if( vPos.IsValid() )
			m_pOrderArea = TheNavMesh->GetNearestNavArea(vPos, false, 1250.0f + fRadius, false, true);
		else
			Warning( "HidingSpotCollector: Invalid input position %f %f %f\n", vPos.x, vPos.y, vPos.z );

		if( g_pynavmesh_debug_hidespot.GetBool() )
		{
			char buf[512];
			if( m_pOrderArea )
			{
				Q_snprintf( buf, 512, "HidingSpotCollector: HAS nav mesh! %d spots. Test Radius: %f", m_pOrderArea->GetHidingSpots()->Count(), m_fRadius );
				NDebugOverlay::Text( m_pOrderArea->GetCenter(), buf, false, HIDESPOT_DEBUG_DURATION );
			}
			else
			{
				Q_snprintf( buf, 512, "HidingSpotCollector: no nav mesh! Test Radius: %f", m_fRadius );
				NDebugOverlay::Text( vPos, buf, false, HIDESPOT_DEBUG_DURATION );
			}
		}
	}

	bool operator() ( CNavArea *area )
	{
		// Figure out normal and skip if the slope is crap
		Vector normal;
		area->ComputeNormal( &normal );
		float fSlope = DotProduct( normal, Vector(0, 0, 1) );
		if( fSlope < 0.83f )
		{
			if( g_pynavmesh_debug_hidespot.GetBool() )
				NDebugOverlay::Text( area->GetCenter(), "HidingSpotCollector: skipping area due slope", false, HIDESPOT_DEBUG_DURATION );
			return true;
		}

		const HidingSpotVector *spots = area->GetHidingSpots();

		if( g_pynavmesh_debug_hidespot.GetBool() && area != m_pOrderArea )
		{
			char buf[512];
			Q_snprintf( buf, 512, "HidingSpotCollector: Nav mesh with %d spots", spots->Count() );
			NDebugOverlay::Text( area->GetCenter(), buf, false, HIDESPOT_DEBUG_DURATION );
		}

		// Area must be reachable
		// Temporary disable blocked for this area if the spot is in this area
		//int attributes = area->GetAttributes();
		//area->SetAttributes( attributes&(~NAV_MESH_NAV_BLOCKER) );
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
		//area->SetAttributes( attributes );

		if( fPathDist < 0 )
		{
			if( g_pynavmesh_debug_hidespot.GetBool() )
			{
				for( int i = 0; i < spots->Count(); i++ )
					NDebugOverlay::Box( spots->Element( i )->GetPosition(), -Vector(8, 8, 8), Vector(8, 8, 8), 255, 0, 0, true, HIDESPOT_DEBUG_DURATION );
			}

			return true;
		}

		// Collect spots in range
		for( int i = 0; i < spots->Count(); i++ )
		{
			HidingSpot *pSpot = spots->Element( i );
			const Vector &vPos = pSpot->GetPosition();
			float fDist = ( vPos - m_vPos ).Length2D();

			//if( !pSpot->HasGoodCover() )
			//	continue;

			if( fDist < m_fRadius )
			{
				// When m_pSortPos is defined, an alternative distance is calculated for sorting
				if( m_pSortPos )
					m_HidingSpots.AddToTail( HidingSpotResult_t( pSpot, ( vPos - *m_pSortPos ).Length2D() ) );
				else
					m_HidingSpots.AddToTail( HidingSpotResult_t( pSpot, fDist ) );

				if( g_pynavmesh_debug_hidespot.GetBool() )
					NDebugOverlay::Box( pSpot->GetPosition(), -Vector(8, 8, 8), Vector(8, 8, 8), 0, 255, 0, true, HIDESPOT_DEBUG_DURATION );
			}
			else
			{
				if( g_pynavmesh_debug_hidespot.GetBool() )
				{
					NDebugOverlay::Box( pSpot->GetPosition(), -Vector(8, 8, 8), Vector(8, 8, 8), 0, 0, 255, true, HIDESPOT_DEBUG_DURATION );
				}
			}
		}

		return true;
	}

private:
	CUnitBase *m_pUnit;
	CNavArea *m_pOrderArea;
	float m_fRadius;
	const Vector &m_vPos;
	const Vector *m_pSortPos;
	CUtlVector< HidingSpotResult_t > &m_HidingSpots;
};

static int HidingSpotCompare(const HidingSpotResult_t *pLeft, const HidingSpotResult_t *pRight )
{
	if( pLeft->fDist < pRight->fDist )
		return -1;
	else if( pLeft->fDist > pRight->fDist )
		return 1;
	return 0;
}

#ifdef CLIENT_DLL
	ConVar disable_hidingspots( "cl_disable_coverspots", "0", FCVAR_CHEAT );
#else
	ConVar disable_hidingspots( "sv_disable_coverspots", "0", FCVAR_CHEAT );
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
boost::python::list GetHidingSpotsInRadius( const Vector &pos, float radius, CUnitBase *pUnit, bool bSort, const Vector *pSortPos )
{
	boost::python::list l;

	if( !TheNavMesh->IsLoaded() || disable_hidingspots.GetBool() )
		return l;

	// Get hiding spots in radius
	CUtlVector< HidingSpotResult_t > HidingSpots;
	HidingSpotCollector collector( HidingSpots, pos, radius, pUnit, pSortPos );
	if( g_pynavmesh_hidespot_searchmethod.GetInt() == 1 )
	{
		TheNavMesh->ForAllAreasInRadius< HidingSpotCollector >( collector, pos, Max(1250.0f, radius * 2.0f) ); // Use a larger radius for testing the areas
	}
	else
	{
		Extent extent;
		extent.lo = Vector(pos.x - radius, pos.y - radius, pos.z - 128.0f );
		extent.hi = Vector(pos.x + radius, pos.y + radius, pos.z + 128.0f );
		TheNavMesh->ForAllAreasOverlappingExtent( collector, extent );
	}

	if( bSort )
	{
		// Sort based on distance
		HidingSpots.Sort( HidingSpotCompare );
	}

	// Python return result
	for( int i = 0; i < HidingSpots.Count(); i++ )
		l.append( boost::python::make_tuple( HidingSpots[i].pSpot->GetID(), HidingSpots[i].pSpot->GetPosition() ) );

	return l;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CreateHidingSpot( const Vector &pos, int &navareaid, bool notsaved, bool checkground )
{
	CNavArea *pArea = TheNavMesh->GetNearestNavArea( pos + Vector( 0, 0, HalfHumanHeight ), false, 10000.0f, checkground );
	if( !pArea )
		return -1;

	HidingSpot *spot = TheNavMesh->CreateHidingSpot();
	spot->SetPosition( pos );
	spot->SetFlags( notsaved ? HidingSpot::IN_COVER|HidingSpot::NOTSAVED : HidingSpot::IN_COVER );
	pArea->AddHidingSpot( spot );

	navareaid = pArea->GetID();
	return spot->GetID();
}

typedef struct HidingSpotAllResult_t
{
	HidingSpotAllResult_t( HidingSpot *spot, CNavArea *area ) : pSpot(spot), pArea(area) {}

	HidingSpot *pSpot;
	CNavArea *pArea;
} HidingSpotAllResult_t;

class HidingSpotCollectorAll
{
public:
	HidingSpotCollectorAll( CUtlVector<HidingSpotAllResult_t> &hidingspots ) : m_hidingSpots( hidingspots )
	{
	}

	bool operator() ( CNavArea *area )
	{
		const HidingSpotVector *spots = area->GetHidingSpots();
		for( int i = 0; i < spots->Count(); i++ )
		{
			HidingSpot *pSpot = spots->Element( i );
			m_hidingSpots.AddToTail( HidingSpotAllResult_t(pSpot, area) );
		}

		return true;
	}

private:
	CUtlVector<HidingSpotAllResult_t> &m_hidingSpots;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool DestroyHidingSpot( const Vector &pos, float tolerance )
{
	CUtlVector<HidingSpotAllResult_t> hidingspots;
	HidingSpotCollectorAll collector( hidingspots );
	TheNavMesh->ForAllAreasInRadius< HidingSpotCollectorAll >( collector, pos, 1250.0f );

	HidingSpot *pBestSpot = NULL;
	float fBestDist = MAX_COORD_FLOAT;
	CNavArea *pBestArea = NULL;
	for( int i = 0; i < hidingspots.Count(); i++ )
	{
		HidingSpot *pSpot = hidingspots[i].pSpot;

		float fDist = (pSpot->GetPosition() - pos).Length();

		if( !pBestSpot || fDist < fBestDist )
		{
			pBestSpot = pSpot;
			fBestDist = fDist;
			pBestArea = hidingspots[i].pArea;
		}
	}

	if( !pBestSpot || !pBestArea || fBestDist > tolerance )
		return false;

	return pBestArea->RemoveHidingSpot( pBestSpot );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool DestroyHidingSpotByID( unsigned int navareaid, unsigned int hidespotid )
{
	CNavArea *area = TheNavMesh->GetNavAreaByID( navareaid );
	if( !area )
		return false;

	bool deleted = false;

	const HidingSpotVector *spots = area->GetHidingSpots();
	for( int i = 0; i < spots->Count(); i++ )
	{
		HidingSpot *pSpot = spots->Element( i );
		if( pSpot->GetID() != hidespotid )
			continue;

		deleted = area->RemoveHidingSpot( pSpot );
		break;
	}

	return deleted;
}
#endif // 0

#ifdef CLIENT_DLL
	ConVar disable_hidingspots( "cl_disable_coverspots", "0", FCVAR_CHEAT );
#else
	ConVar disable_hidingspots( "sv_disable_coverspots", "0", FCVAR_CHEAT );
#endif // CLIENT_DLL

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

	// Python return result
	for( int i = 0; i < coverSpots.Count(); i++ )
		l.append( boost::python::make_tuple( coverSpots[i].pSpot->id, coverSpots[i].pSpot->position ) );

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