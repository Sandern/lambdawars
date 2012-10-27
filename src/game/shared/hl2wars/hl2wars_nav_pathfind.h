//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
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

	float operator() ( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length )
	{
		if ( fromArea == NULL )
		{
			// first area in path, no cost
			return 0.0f;
		}
		else
		{
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
#ifndef CLIENT_DLL
					// Means we need to climb up or jump up
					if( heightdiff > m_pUnit->m_fMaxClimbHeight )
						return -1;
#endif // CLIENT_DLL
					cost += heightdiff * 2;
				}
				else
				{
#ifndef CLIENT_DLL
					// Only allow save drops (so don't get any damage)
					if( fabs(heightdiff) > m_pUnit->m_fSaveDrop )
						return -1;
#endif // CLIENT_DLL
					cost += fabs(heightdiff) * 1.1f;
				}
			}

			// Must fit in the nav area. Take surrounding area's in consideration
			float fTolX = MIN(area->GetTolerance(WEST), area->GetTolerance(EAST)) + area->GetSizeX();
			float fTolY =  MIN(area->GetTolerance(NORTH), area->GetTolerance(SOUTH)) + area->GetSizeY();

			if(m_fBoundingRadius > fTolX || m_fBoundingRadius > fTolY )
			{
				return -1;
			}

			if( (area->GetAttributes() & NAV_MESH_JUMP) )
			{
				//Msg("#%d jump not supported\n", m_pUnit->entindex());
				return -1;
			}

			if( (area->GetAttributes() & NAV_MESH_CROUCH) )
			{
				//Msg("#%d crouch not supported\n", m_pUnit->entindex());
				return -1;
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

#endif // _HL2WARS_NAV_PATHFIND_H_