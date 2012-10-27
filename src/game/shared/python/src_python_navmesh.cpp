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

bool NavMeshAvailable()
{
	return TheNavMesh->IsLoaded();
}

bool NavMeshTestHasArea( Vector &pos, float beneathLimt )
{
	return TheNavMesh->GetNavArea( pos, beneathLimt ) != NULL;
}

float NavMeshGetPathDistance( Vector &vStart, Vector &vGoal, bool anyz, float maxdist, bool bNoTolerance, CUnitBase *pUnit )
{
	CNavArea *startArea, *goalArea;
	if( bNoTolerance )
	{
		startArea = TheNavMesh->GetNavArea(vStart);
		goalArea = TheNavMesh->GetNavArea(vGoal);
	}
	else
	{
		startArea = TheNavMesh->GetNearestNavArea(vStart, anyz, maxdist);
		goalArea = TheNavMesh->GetNearestNavArea(vGoal, anyz, maxdist);
	}
	if( !startArea || !goalArea )
		return -1;

	if( !pUnit )
	{
		ShortestPathCost costFunc;
		return NavAreaTravelDistance<ShortestPathCost>(startArea, goalArea, costFunc);
	}
	else
	{
		UnitShortestPathCost costFunc( pUnit, false );
		return NavAreaTravelDistance<UnitShortestPathCost>(startArea, goalArea, costFunc);
	}
}

Vector NavMeshGetPositionNearestNavArea( const Vector &pos, float beneathlimit )
{
	CNavArea *pArea;
	//pArea = TheNavMesh->GetNearestNavArea(pos, false, 64.0f);
	pArea = TheNavMesh->GetNavArea(pos, beneathlimit);
	if( pArea ) {
		Vector vAreaPos(pos);
		vAreaPos.z = pArea->GetZ(pos);
		return vAreaPos;
	}
	return vec3_origin;
}

int CreateNavArea( const Vector &corner, const Vector &otherCorner )
{
#ifndef CLIENT_DLL
	return TheNavMesh->CreateNavArea( corner, otherCorner );
#else
	return -1;
#endif // CLIENT_DLL
}

int CreateNavAreaByCorners( const Vector &nwCorner, const Vector &neCorner, const Vector &seCorner, const Vector &swCorner )
{
#ifndef CLIENT_DLL
	return TheNavMesh->CreateNavArea( nwCorner, neCorner, seCorner, swCorner );
#else
	return -1;
#endif // CLIENT_DLL
}

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

void DestroyAllNavAreas()
{
	TheNavMesh->Reset();
}

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

	Vector random( 
		mins.x + ((float)rand() / RAND_MAX) * (maxs.x - mins.x),
		mins.y + ((float)rand() / RAND_MAX) * (maxs.y - mins.y),
		maxs.z
	);
	CNavArea *pArea = TheNavMesh->GetNearestNavArea( random );
	if( !pArea )
		return vec3_origin;

	return pArea->GetCenter();
}

int GetNavAreaAt( const Vector &pos, float beneathlimit )
{
	CNavArea *pArea = TheNavMesh->GetNavArea( pos, beneathlimit );
	if( !pArea )
		return -1;

	return pArea->GetID();
}

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
		//area->ComputeTolerance();
		//other->ComputeTolerance();

		if( area->SplitEdit( false, TheNavMesh->SnapToGrid(maxs.x, true), &area, &other ) )
		{
			ids.AddToTail(other->GetID());
		}
		//TheNavMesh->StripNavigationAreas();
		//area->ComputeTolerance();
		//other->ComputeTolerance();

		if( area->SplitEdit( true, TheNavMesh->SnapToGrid(mins.y, true), &other, &area ) )
		{
			ids.AddToTail(other->GetID());
		}
		//TheNavMesh->StripNavigationAreas();
		//area->ComputeTolerance();
		//other->ComputeTolerance();

		if( area->SplitEdit( true, TheNavMesh->SnapToGrid(maxs.y, true), &area, &other ) )
		{
			ids.AddToTail(other->GetID());
		}
		ids.AddToTail(area->GetID());
		//TheNavMesh->StripNavigationAreas();
		//area->ComputeTolerance();
		//other->ComputeTolerance();
	}

#if 0
	Msg("Split result.\n");
	for( int i = 0; i < ids.Count(); i++ )
	{
		Msg("Split result id: %d\n", ids[i]);
	}
#endif // 0
#endif // CLIENT_DLL
}

void SetAreasBlocked( bp::list areas, bool blocked )
{
#ifndef CLIENT_DLL
	CNavArea *area, *adj;
	int id, iCount, k, j;
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
			
			// Tell adjs to recompute tolerance
			for( k = 0; k<NUM_DIRECTIONS; ++k )
			{
				NavDirType dir = (NavDirType)k;
				iCount = area->GetAdjacentCount( dir );
				for( j = 0; j < iCount; ++j )
				{
					adj = area->GetAdjacentArea( dir, j );
					if( !adj )
						continue;
					adj->ComputeTolerance();
				}
			}
		}
	}
#endif // CLIENT_DLL
}

#ifndef CLIENT_DLL
static ConVar g_debug_coveredbynavareas("g_debug_coveredbynavareas", "0", FCVAR_CHEAT);
#endif // CLIENT_DLL

bool IsBBCoveredByNavAreas( const Vector &mins, const Vector &maxs, float tolerance, bool bRequireIsFlat, float fFlatTol )
{
	float fArea, fGoalArea;
	CNavArea *area;
	Extent extent;
	Vector vAbsMins2, vAbsMaxs2;
	Vector vNormal;

	fGoalArea = (maxs.x - mins.x) * (maxs.y - mins.y);
	fArea = 0.0f;

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

class HidingSpotCollector
{
public:
	HidingSpotCollector( CUtlVector< HidingSpot * > &HidingSpots, const Vector &vPos, float fRadius ) : m_HidingSpots( HidingSpots ), m_vPos( vPos )
	{
		m_fRadiusSqr = fRadius * fRadius;
	}

	bool operator() ( CNavArea *area )
	{
		const HidingSpotVector *spots = area->GetHidingSpots();

		for( int i = 0; i < spots->Count(); i++ )
		{
			const Vector &vPos = spots->Element( i )->GetPosition();

			if( (vPos - m_vPos).LengthSqr() < m_fRadiusSqr )
				m_HidingSpots.AddToTail( spots->Element( i ) );
		}

		return true;
	}

private:
	float m_fRadiusSqr;
	const Vector &m_vPos;
	CUtlVector< HidingSpot * > &m_HidingSpots;
};

bp::list GetHidingSpotsInRadius( const Vector &pos, float radius )
{
	bp::list l;
	CUtlVector< HidingSpot * > HidingSpots;
	HidingSpotCollector collector( HidingSpots, pos, radius );

	TheNavMesh->ForAllAreasInRadius< HidingSpotCollector >( collector, pos, radius );

	for( int i = 0; i < HidingSpots.Count(); i++ )
	{
		l.append( bp::make_tuple( HidingSpots[i]->GetID(), HidingSpots[i]->GetPosition() ) );
	}

	return l;
}