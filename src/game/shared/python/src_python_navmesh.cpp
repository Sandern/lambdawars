//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "src_python_navmesh.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"
#include "nav_area.h"
#include "wars_mapboundary.h"
#include "hl2wars_nav_pathfind.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar g_pynavmesh_debug("g_pynavmesh_debug", "0", FCVAR_CHEAT|FCVAR_REPLICATED);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool NavMeshAvailable()
{
	return TheNavMesh->IsLoaded();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool NavMeshTestHasArea( Vector &pos, float beneathLimt )
{
	return TheNavMesh->GetNavArea( pos, beneathLimt ) != NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float NavMeshGetPathDistance( Vector &vStart, Vector &vGoal, bool anyz, float maxdist, bool bNoTolerance, CUnitBase *pUnit )
{
	if( g_pynavmesh_debug.GetInt() > 1 )
		DevMsg("NavMeshGetPathDistance: anyz: %d, maxdist: %f, bNoTolerance: %d, unit: %d\n", anyz, maxdist, bNoTolerance, pUnit);

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
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector NavMeshGetPositionNearestNavArea( const Vector &pos, float beneathlimit, bool bCheckBlocked )
{
	CNavArea *pArea;
	//pArea = TheNavMesh->GetNearestNavArea(pos, false, 64.0f);
	pArea = TheNavMesh->GetNavArea(pos, beneathlimit, bCheckBlocked);
	if( pArea ) {
		Vector vAreaPos(pos);
		vAreaPos.z = pArea->GetZ(pos);
		return vAreaPos;
	}
	return vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CreateNavArea( const Vector &corner, const Vector &otherCorner )
{
#ifndef CLIENT_DLL
	return TheNavMesh->CreateNavArea( corner, otherCorner );
#else
	return -1;
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CreateNavAreaByCorners( const Vector &nwCorner, const Vector &neCorner, const Vector &seCorner, const Vector &swCorner )
{
#ifndef CLIENT_DLL
	return TheNavMesh->CreateNavArea( nwCorner, neCorner, seCorner, swCorner );
#else
	return -1;
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DestroyNavArea( int id )
{
	CNavArea *area;
	area = TheNavMesh->GetNavAreaByID( id );
	if( area )
	{
		TheNavAreas.FindAndRemove( area );
		TheNavMesh->DestroyArea( area );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DestroyAllNavAreas()
{
	TheNavMesh->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector RandomNavAreaPosition( )
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

		mins.x = MIN(mins.x, othermins.x);
		mins.y = MIN(mins.y, othermins.y);
		mins.z = MIN(mins.z, othermins.z);
		maxs.x = MAX(maxs.x, othermaxs.x);
		maxs.y = MAX(maxs.y, othermaxs.y);
		maxs.z = MAX(maxs.z, othermaxs.z);
	}

	return RandomNavAreaPositionWithin( mins, maxs );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector RandomNavAreaPositionWithin( const Vector &mins, const Vector &maxs )
{
	Vector random( 
		mins.x + ((float)rand() / RAND_MAX) * (maxs.x - mins.x),
		mins.y + ((float)rand() / RAND_MAX) * (maxs.y - mins.y),
		maxs.z
	);
	CNavArea *pArea = TheNavMesh->GetNearestNavArea( random, false, 10000.0f, false, false );
	if( !pArea )
	{
		if( g_pynavmesh_debug.GetBool() )
			DevMsg("RandomNavAreaPosition: No area found within Mins: %f %f %f, Maxs: %f %f %f, Random: %f %f %f\n", 
					mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z, random.x, random.y, random.z);
		return vec3_origin;
	}
	Vector vRandomPoint = pArea->GetRandomPoint();
	vRandomPoint.z += 32.0f;

	if( g_pynavmesh_debug.GetBool() )
		DevMsg("RandomNavAreaPosition: Found position %f %f %f\n", vRandomPoint.x, vRandomPoint.y, vRandomPoint.z);

	return pArea->GetCenter();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int GetNavAreaAt( const Vector &pos, float beneathlimit )
{
	CNavArea *pArea = TheNavMesh->GetNavArea( pos, beneathlimit );
	if( !pArea )
		return -1;

	return pArea->GetID();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::list GetNavAreasAtBB( const Vector &mins, const Vector &maxs )
{
	bp::list l;
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
#ifndef CLIENT_DLL
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
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SetAreasBlocked( bp::list areas, bool blocked )
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
				area->SetAttributes( area->GetAttributes()|NAV_MESH_NAV_BLOCKER );
			else
				area->SetAttributes( area->GetAttributes()&(~NAV_MESH_NAV_BLOCKER) );
		}
	}
#endif // CLIENT_DLL
}

#ifndef CLIENT_DLL
static ConVar g_debug_coveredbynavareas("g_debug_coveredbynavareas", "0", FCVAR_CHEAT);
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool IsBBCoveredByNavAreas( const Vector &mins, const Vector &maxs, float tolerance, bool bRequireIsFlat, float fFlatTol )
{
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
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool TryMergeSurrounding( int id, float tolerance )
{
#ifdef CLIENT_DLL
	return false;
#else
	CNavArea *pArea = TheNavMesh->GetNavAreaByID( id );
	if( !pArea )
	{
		Warning("Invalid nav area %d\n", id);
		return false;
	}

	return TheNavMesh->TryMergeSingleArea( pArea, tolerance );
#endif // CLIENT_DLL
}

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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
typedef struct HidingSpotResult_t
{
	HidingSpotResult_t( HidingSpot *spot, float dist ) : pSpot(spot), fDist(dist) {}

	HidingSpot *pSpot;
	float fDist;
} HidingSpotResult_t;

#define USE_NAVAREA_BASED_DIST
class HidingSpotCollector
{
public:
	HidingSpotCollector( CUtlVector< HidingSpotResult_t > &HidingSpots, const Vector &vPos, float fRadius, CUnitBase *pUnit ) : m_HidingSpots( HidingSpots ), m_vPos( vPos ), m_pUnit(pUnit)
	{
#ifndef USE_NAVAREA_BASED_DIST
		m_fRadiusSqr = fRadius * fRadius;
#else
		m_fRadius = fRadius;
		m_pOrderArea = TheNavMesh->GetNearestNavArea(vPos, false, fRadius, false, false);
#endif // USE_NAVAREA_BASED_DIST
	}

	bool operator() ( CNavArea *area )
	{
		// Figure out normal and skip if the slope is crap
		Vector normal;
		area->ComputeNormal( &normal );
		float fSlope = DotProduct( normal, Vector(0, 0, 1) );
		if( fSlope < 0.83f )
		{
			return false;
		}

		const HidingSpotVector *spots = area->GetHidingSpots();

		for( int i = 0; i < spots->Count(); i++ )
		{
			HidingSpot *pSpot = spots->Element( i );
			const Vector &vPos = pSpot->GetPosition();
			float fDist = (vPos - m_vPos).Length2D();

#ifndef USE_NAVAREA_BASED_DIST
			float fDist = (vPos - m_vPos).LengthSqr();
			if( fDist < m_fRadiusSqr )
				m_HidingSpots.AddToTail( HidingSpotResult_t( pSpot, fDist ) );
#else
			CNavArea *pTestArea = TheNavMesh->GetNearestNavArea( vPos, false, m_fRadius, false, false );

			float fPathDist;
			if( !m_pUnit )
			{
				ShortestPathCost costFunc;
				fPathDist = NavAreaTravelDistance<ShortestPathCost>( m_pOrderArea, pTestArea, costFunc, m_fRadius * 2.0f );
			}
			else
			{
				UnitShortestPathCost costFunc( m_pUnit, false );
				fPathDist = NavAreaTravelDistance<UnitShortestPathCost>( m_pOrderArea, pTestArea, costFunc, m_fRadius * 2.0f );
			}

			if( fPathDist >= 0 && fDist < m_fRadius )
			{
				m_HidingSpots.AddToTail( HidingSpotResult_t( pSpot, fDist ) );
			}

#endif // USE_NAVAREA_BASED_DIST
		}

		return true;
	}

private:
#ifndef USE_NAVAREA_BASED_DIST
	float m_fRadiusSqr;
#else
	CUnitBase *m_pUnit;
	CNavArea *m_pOrderArea;
	float m_fRadius;
#endif // USE_NAVAREA_BASED_DIST
	const Vector &m_vPos;
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::list GetHidingSpotsInRadius( const Vector &pos, float radius, CUnitBase *pUnit )
{
	bp::list l;

	// Get hiding spots in radius
	CUtlVector< HidingSpotResult_t > HidingSpots;
	HidingSpotCollector collector( HidingSpots, pos, radius, pUnit );
	TheNavMesh->ForAllAreasInRadius< HidingSpotCollector >( collector, pos, radius * 2.0f ); // Use a larger radius for testing the areas

	// Sort based on distance
	HidingSpots.Sort( HidingSpotCompare );

	// Python return result
	for( int i = 0; i < HidingSpots.Count(); i++ )
		l.append( bp::make_tuple( HidingSpots[i].pSpot->GetID(), HidingSpots[i].pSpot->GetPosition() ) );

	return l;
}