//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef _HL2WARS_NAV_PATHFIND_H_
#define _HL2WARS_NAV_PATHFIND_H_

#include "unit_base_shared.h"
#include "nav.h"

//--------------------------------------------------------------------------------------------------------------
/**
* Functor used with NavAreaBuildPath()
*/
class UnitShortestPathCost
{
public:
	UnitShortestPathCost( CUnitBase *pUnit, bool bNoSpecialTravel=false ) : m_pUnit(pUnit) , m_bNoSpecialTravel(bNoSpecialTravel)
	{
		m_fBoundingRadius = m_pUnit->CollisionProp()->BoundingRadius2D();
	}

	bool IsAreaValid( CNavArea *area, CNavArea *fromArea = NULL, NavTraverseType how = NUM_TRAVERSE_TYPES )
	{
		if( (area->GetAttributes() & NAV_MESH_JUMP) )
		{
			//Msg("#%d jump not supported (area %d)\n", m_pUnit->entindex(), area->GetID());
			return false;
		}

		if( (area->GetAttributes() & NAV_MESH_CROUCH) )
		{
			//Msg("#%d crouch not supported (area %d)\n", m_pUnit->entindex(), area->GetID());
			return false;
		}

		// Must fit in the nav area. Take surrounding area's in consideration
		//float fTolX = area->GetSizeX();
		//float fTolY =  area->GetSizeY();

		if( fromArea )
		{
			if( how == GO_NORTH || how == GO_SOUTH )
			{
				float fTolX = area->GetSizeX() + fromArea->GetSizeX();
				if( m_fBoundingRadius > fTolX )
					return false;

				float fTolY =  area->GetSizeY();
				if( m_fBoundingRadius > fTolY )
					return false;
			}
			else if( how == GO_WEST || how == GO_EAST )
			{
				float fTolY = area->GetSizeY() + fromArea->GetSizeY();
				if( m_fBoundingRadius > fTolY )
					return false;

				float fTolX =  area->GetSizeX();
				if( m_fBoundingRadius > fTolX )
					return false;
			}
		}
		/*
		// Per direction, check if the bounding radius fits
		// If not, try to expand the tolerance by adding adj areas
		if( m_fBoundingRadius > fTolX )
		{
			return false;
		}

		if( m_fBoundingRadius > fTolY )
		{
			//area->GetAdjacentArea(
			return false;
		}*/

		// Figure out normal, calculate and check min slope
		Vector normal;
		area->ComputeNormal( &normal );
		float fSlope = DotProduct( normal, Vector(0, 0, 1) );
		if( fSlope < m_pUnit->m_fMinSlope )
		{
			return false;
		}

		return true;
	}

	float operator() ( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, NavTraverseType how, float length )
	{
		if ( fromArea == NULL )
		{
			// first area in path, no cost
			return 0.0f;
		}
		else
		{
			// First check if the target area is valid
			if( !IsAreaValid( area ) )
			{
				return -1;
			}

			// compute distance traveled along path so far
			float dist;

			
			if ( ladder )
			{
				//dist = ladder->m_length;
				return -1;
			}
			else if ( length > 0.0 )
			{
				dist = length;
			}
			else
			{
				dist = ( area->GetCenter() - fromArea->GetCenter() ).Length();
			}

			float cost = dist + fromArea->GetCostSoFar();

			// if this is a "crouch" area, add penalty
			/*if ( area->GetAttributes() & NAV_MESH_CROUCH )
			{
				const float crouchPenalty = 20.0f;		// 10
				cost += crouchPenalty * dist;
			}*/

			// Compute height difference
			float heightdiff = fromArea->ComputeAdjacentConnectionHeightChange( area );
				
			// Don't consider insignificant height differences
			if( fabs(heightdiff) > 16.0f )
			{
				if( m_bNoSpecialTravel )
					return -1;

				if( heightdiff > 0 )
				{
					// Means we need to climb up or jump up
					if( heightdiff > m_pUnit->m_fMaxClimbHeight )
						return -1;

					cost += heightdiff * 3.0f;
				}
				else
				{
					// Only allow save drops (so don't get any damage)
					if( fabs(heightdiff) > m_pUnit->m_fSaveDrop )
						return -1;

					cost += fabs(heightdiff) * 3.0f;
				}
			}

			// if this is a "jump" area, add penalty
			/*if ( area->GetAttributes() & NAV_MESH_JUMP )
			{
				const float jumpPenalty = 5.0f;
				cost += jumpPenalty * dist;
			}*/

			return cost;
		}
	}

private:
	CUnitBase *m_pUnit;
	float m_fBoundingRadius;
	bool m_bNoSpecialTravel;
};


//--------------------------------------------------------------------------------------------------------------
/**
 * Find path from startArea to goalArea via an A* search, using supplied cost heuristic.
 * If cost functor returns -1 for an area, that area is considered a dead end.
 * This doesn't actually build a path, but the path is defined by following parent
 * pointers back from goalArea to startArea.
 * If 'closestArea' is non-NULL, the closest area to the goal is returned (useful if the path fails).
 * If 'goalArea' is NULL, will compute a path as close as possible to 'goalPos'.
 * If 'goalPos' is NULL, will use the center of 'goalArea' as the goal position.
 * If 'maxPathLength' is nonzero, path building will stop when this length is reached.
 * Returns true if a path exists.
 */
#define IGNORE_NAV_BLOCKERS true
template< typename CostFunctor >
bool UnitNavAreaBuildPath( CNavArea *startArea, CNavArea *goalArea, const Vector *goalPos, CostFunctor &costFunc, CNavArea **closestArea = NULL, float maxPathLength = 0.0f, int teamID = TEAM_ANY, bool ignoreNavBlockers = false )
{
	VPROF_BUDGET( "NavAreaBuildPath", "NextBotSpiky" );

	if ( closestArea )
	{
		*closestArea = startArea;
	}

#ifndef CLIENT_DLL
	bool isDebug = ( g_DebugPathfindCounter-- > 0 );
#endif // CLIENT_DLL

	if (startArea == NULL)
		return false;

	CNavArea::ClearCachedPath();
	startArea->SetParent( NULL );

	if (goalArea != NULL && goalArea->IsBlocked( teamID, ignoreNavBlockers ))
		goalArea = NULL;

	if (goalArea == NULL && goalPos == NULL)
		return false;

	// if we are already in the goal area, build trivial path
	if (startArea == goalArea)
	{
		return true;
	}

	// determine actual goal position
	Vector actualGoalPos = (goalPos) ? *goalPos : goalArea->GetCenter();

	// start search
	CNavArea::ClearSearchLists();

	// compute estimate of path length
	/// @todo Cost might work as "manhattan distance"
	startArea->SetTotalCost( (startArea->GetCenter() - actualGoalPos).Length() );

	float initCost = costFunc( startArea, NULL, NULL, NULL, NUM_TRAVERSE_TYPES, -1.0f );	
	if (initCost < 0.0f)
		return false;
	startArea->SetCostSoFar( initCost );
	startArea->SetPathLengthSoFar( 0.0 );

	startArea->AddToOpenList();

	// keep track of the area we visit that is closest to the goal
	float closestAreaDist = startArea->GetTotalCost();
	float closestAreaCostsSoFar = initCost;

	// do A* search
	while( !CNavArea::IsOpenListEmpty() )
	{
		// get next area to check
		CNavArea *area = CNavArea::PopOpenList();

#ifndef CLIENT_DLL
		if ( isDebug )
		{
			area->DrawFilled( 0, 255, 0, 128, 30.0f );
		}
#endif // CLIENT_DLL

		if( area->GetParent() )
		{
			bool curAreaBlocked = area->GetParent()->IsBlocked( teamID, ignoreNavBlockers );
			bool newAreaBlocked = area->IsBlocked( teamID, ignoreNavBlockers );
			if( newAreaBlocked && area->GetOwner() == NULL )
			{
				// When moving into a blocked area, the area should be owned by something (might be goal)
				continue;
			}
			if( curAreaBlocked && newAreaBlocked )
			{
				// This is only allowed if both areas are owned by the same
				if( area->GetOwner() != area->GetParent()->GetOwner() )
					continue;

			}
			else if( curAreaBlocked && !newAreaBlocked )
			{
				// Don't consider area if current area is blocked a new area is not blocked
				// This allows us to go into blocked areas, but pass them onto a route to something else
				continue;
			}
		}

		// check if we have found the goal area or position
		if (area == goalArea || (goalArea == NULL && goalPos && area->Contains( *goalPos )))
		{
			if (closestArea)
			{
				*closestArea = area;
			}

			return true;
		}

		// search adjacent areas
		enum SearchType
		{
			SEARCH_FLOOR, SEARCH_LADDERS, SEARCH_ELEVATORS
		};
		SearchType searchWhere = SEARCH_FLOOR;
		int searchIndex = 0;

		int dir = NORTH;
		const NavConnectVector *floorList = area->GetAdjacentAreas( NORTH );

		bool ladderUp = true;
		const NavLadderConnectVector *ladderList = NULL;
		enum { AHEAD = 0, LEFT, RIGHT, BEHIND, NUM_TOP_DIRECTIONS };
		int ladderTopDir = AHEAD;
		bool bHaveMaxPathLength = ( maxPathLength > 0.0f );
		float length = -1;
		
		while( true )
		{
			CNavArea *newArea = NULL;
			NavTraverseType how;
			const CNavLadder *ladder = NULL;
			const CFuncElevator *elevator = NULL;

			//
			// Get next adjacent area - either on floor or via ladder
			//
			if ( searchWhere == SEARCH_FLOOR )
			{
				// if exhausted adjacent connections in current direction, begin checking next direction
				if ( searchIndex >= floorList->Count() )
				{
					++dir;

					if ( dir == NUM_DIRECTIONS )
					{
						// checked all directions on floor - check ladders next
						searchWhere = SEARCH_LADDERS;

						ladderList = area->GetLadders( CNavLadder::LADDER_UP );
						searchIndex = 0;
						ladderTopDir = AHEAD;
					}
					else
					{
						// start next direction
						floorList = area->GetAdjacentAreas( (NavDirType)dir );
						searchIndex = 0;
					}

					continue;
				}

				const NavConnect &floorConnect = floorList->Element( searchIndex );
				newArea = floorConnect.area;
				length = floorConnect.length;
				how = (NavTraverseType)dir;
				++searchIndex;

				if ( IsX360() && searchIndex < floorList->Count() )
				{
					PREFETCH360( floorList->Element( searchIndex ).area, 0  );
				}
			}
			else if ( searchWhere == SEARCH_LADDERS )
			{
				if ( searchIndex >= ladderList->Count() )
				{
					if ( !ladderUp )
					{
						// checked both ladder directions - check elevators next
						searchWhere = SEARCH_ELEVATORS;
						searchIndex = 0;
						ladder = NULL;
					}
					else
					{
						// check down ladders
						ladderUp = false;
						ladderList = area->GetLadders( CNavLadder::LADDER_DOWN );
						searchIndex = 0;
					}
					continue;
				}

				if ( ladderUp )
				{
					ladder = ladderList->Element( searchIndex ).ladder;

					// do not use BEHIND connection, as its very hard to get to when going up a ladder
					if ( ladderTopDir == AHEAD )
					{
						newArea = ladder->m_topForwardArea;
					}
					else if ( ladderTopDir == LEFT )
					{
						newArea = ladder->m_topLeftArea;
					}
					else if ( ladderTopDir == RIGHT )
					{
						newArea = ladder->m_topRightArea;
					}
					else
					{
						++searchIndex;
						ladderTopDir = AHEAD;
						continue;
					}

					how = GO_LADDER_UP;
					++ladderTopDir;
				}
				else
				{
					newArea = ladderList->Element( searchIndex ).ladder->m_bottomArea;
					how = GO_LADDER_DOWN;
					ladder = ladderList->Element(searchIndex).ladder;
					++searchIndex;
				}

				if ( newArea == NULL )
					continue;

				length = -1.0f;
			}
			else // if ( searchWhere == SEARCH_ELEVATORS )
			{
				const NavConnectVector &elevatorAreas = area->GetElevatorAreas();

				elevator = area->GetElevator();

				if ( elevator == NULL || searchIndex >= elevatorAreas.Count() )
				{
					// done searching connected areas
					elevator = NULL;
					break;
				}

				newArea = elevatorAreas[ searchIndex++ ].area;
				if ( newArea->GetCenter().z > area->GetCenter().z )
				{
					how = GO_ELEVATOR_UP;
				}
				else
				{
					how = GO_ELEVATOR_DOWN;
				}

				length = -1.0f;
			}


			// don't backtrack
			Assert( newArea );
			if ( newArea == area->GetParent() )
				continue;
			if ( newArea == area ) // self neighbor?
				continue;

			bool curAreaBlocked = area->IsBlocked( teamID, ignoreNavBlockers );
			bool newAreaBlocked = newArea->IsBlocked( teamID, ignoreNavBlockers );
			if( newAreaBlocked && newArea->GetOwner() == NULL )
			{
				// When moving into a blocked area, the area should be owned by something (might be goal)
				continue;
			}
			if( curAreaBlocked && newAreaBlocked )
			{
				// This is only allowed if both areas are owned by the same
				if( newArea->GetOwner() != area->GetOwner() )
					continue;

			}
			else if( curAreaBlocked && !newAreaBlocked )
			{
				// Don't consider area if current area is blocked a new area is not blocked
				// This allows us to go into blocked areas, but pass them onto a route to something else
				continue;
			}

			float newCostSoFar = costFunc( newArea, area, ladder, elevator, how, length );
			
			// check if cost functor says this area is a dead-end
			if ( newCostSoFar < 0.0f )
				continue;

			// Safety check against a bogus functor.  The cost of the path
			// A...B, C should always be at least as big as the path A...B.
			Assert( newCostSoFar >= area->GetCostSoFar() );

			// And now that we've asserted, let's be a bit more defensive.
			// Make sure that any jump to a new area incurs some pathfinsing
			// cost, to avoid us spinning our wheels over insignificant cost
			// benefit, floating point precision bug, or busted cost functor.
			float minNewCostSoFar = area->GetCostSoFar() * 1.00001 + 0.00001;
			newCostSoFar = MAX( newCostSoFar, minNewCostSoFar );
				
			// stop if path length limit reached
			if ( bHaveMaxPathLength )
			{
				// keep track of path length so far
				float deltaLength = ( newArea->GetCenter() - area->GetCenter() ).Length();
				float newLengthSoFar = area->GetPathLengthSoFar() + deltaLength;
				if ( newLengthSoFar > maxPathLength )
					continue;
				
				newArea->SetPathLengthSoFar( newLengthSoFar );
			}

			if ( ( newArea->IsOpen() || newArea->IsClosed() ) && newArea->GetCostSoFar() <= newCostSoFar )
			{
				// this is a worse path - skip it
				continue;
			}
			else
			{
				// compute estimate of distance left to go
				float distSq = Max( ( newArea->GetCenter() - actualGoalPos ).LengthSqr(), 0.0f );
				float newCostRemaining = ( distSq > 0.0 ) ? FastSqrt( distSq ) : 0.0 ;

				// track closest area to goal in case path fails
				if ( closestArea && newCostRemaining < closestAreaDist )
				{
					*closestArea = newArea;
					
					closestAreaDist = newCostRemaining;
					closestAreaCostsSoFar = newCostSoFar;
				}
				
				newArea->SetCostSoFar( newCostSoFar );
				newArea->SetTotalCost( newCostSoFar + newCostRemaining );

				if ( newArea->IsClosed() )
				{
					newArea->RemoveFromClosedList();
				}

				if ( newArea->IsOpen() )
				{
					// area already on open list, update the list order to keep costs sorted
					newArea->UpdateOnOpenList();
				}
				else
				{
					newArea->AddToOpenList();
				}

				newArea->SetParent( area, how );
			}
		}

		// we have searched this area
		area->AddToClosedList();
	}

	return false;
}

#endif // _HL2WARS_NAV_PATHFIND_H_