//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "wars_orders.h"
#include "unit_base_shared.h"
#include "hl2wars_util_shared.h"
#include "src_python.h"

#ifdef CLIENT_DLL
	#include "c_hl2wars_player.h"
#else
	#include "hl2wars_player.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_formation_type( "unit_orders_formation_type", "0", FCVAR_CHEAT|FCVAR_REPLICATED );
ConVar unit_orders_testratio_lookahead( "unit_orders_testratio_lookahead", "20", FCVAR_CHEAT|FCVAR_REPLICATED );
ConVar unit_orders_spread( "unit_orders_spread", "18.0", FCVAR_CHEAT|FCVAR_REPLICATED );

#ifndef CLIENT_DLL
	ConVar unit_orders_debug_positions( "unit_orders_debug_positions", "0", FCVAR_CHEAT );
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
GroupMoveOrder::GroupMoveOrder( CHL2WarsPlayer *pPlayer, const Vector &vPosition ) : m_pPlayer(pPlayer), m_vPosition(vPosition)
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GroupMoveOrder::Apply( void )
{
	if( !m_Units.Count() )
		return;

	// First generate the positions for arrival
	if( sv_formation_type.GetInt() == 0 )
		ComputeFormationPositions();
	else
		ComputeSquareFormation();
            
	// Second we are going to decide who is going to move where
	// First calculate the midpoints of each group (the units and the target positions)
	Vector unitmidpoint, formationmidpoint, dir, dirpos;
	int i, j, bestj;
	float bestdot, bestratio, dot;

	ComputeMidPointUnits( unitmidpoint );
	ComputeFormationMidPoint( formationmidpoint );

	dir = unitmidpoint - formationmidpoint;
	dir.z = 0.0;
	VectorNormalize( dir );

	unitmidpoint = unitmidpoint + dir * 16000.0;
	formationmidpoint = formationmidpoint + -dir*16000.0;
	float lenline = (unitmidpoint-formationmidpoint).Length2D();

#ifndef CLIENT_DLL
	if( unit_orders_debug_positions.GetBool() )
	{
        NDebugOverlay::Cross3D(unitmidpoint, 32.0, 255, 0, 0, false, 5);
        NDebugOverlay::Cross3D(formationmidpoint, 32.0, 0, 255, 0, false, 5);
        NDebugOverlay::Line(unitmidpoint, formationmidpoint, 0, 0, 255, false, 5);
                
		for( int i = 0; i < m_Units.Count(); i++ )
		{
            Vector projected;
			Project( unitmidpoint, formationmidpoint, m_Units[i]->GetAbsOrigin(), projected );
            NDebugOverlay::Line(projected, m_Units[i]->GetAbsOrigin(), 0, 255, 255, false, 5);
            float ratio = (projected-unitmidpoint).Length2D() / (formationmidpoint-unitmidpoint).Length2D();
            NDebugOverlay::Text(projected, UTIL_VarArgs("%f", ratio), true, 5);
		}
	}
#endif // CLIENT_DLL

    // Then project the units and positions on the line formed by the midpoints
    // Sort these values
	CUtlVector< UnitRatio_t > unitratios;
	CUtlVector< PositionRatio_t > positionratios;

    ProjectAndSortUnits( unitmidpoint, formationmidpoint, unitratios );
    ProjectAndSortPositions( unitmidpoint, formationmidpoint, positionratios );

	int maxlookahead = unit_orders_testratio_lookahead.GetInt();

	// Calculate arrival angle
	const MouseTraceData_t &rightpressed = m_pPlayer->GetMouseDataRightPressed();
	const MouseTraceData_t &rightreleased = m_pPlayer->GetMouseDataRightReleased();

	QAngle angle(0,0,0);
	if( ( rightpressed.m_vWorldOnlyEndPos - rightreleased.m_vWorldOnlyEndPos ).Length2D() > 16.0f )
		  angle = UTIL_CalculateDirection( rightpressed.m_vWorldOnlyEndPos, rightreleased.m_vWorldOnlyEndPos );

	// Get selection
	bp::list selection = UtlVectorToListByValue< CUnitBase * >( m_Units );

    // Finally order each unit
    // We look ahead a bit for better positions
    // We do this by looking at the dot product between the target position-unit and the midpoints line
    // If aligned closer with the midpoint line, then that position is better for the unit.
	for( i = 0; i < unitratios.Count(); i++ )
	{
		UnitRatio_t &ratio1 = unitratios[i];

		PositionRatio_t &ratio2 = positionratios[i];
		bestj = 0;
		dirpos = ratio1.unit->GetAbsOrigin() - m_vPosition;
		dirpos.z = 0.0;
		VectorNormalize(dirpos);
		bestdot = DotProduct(dir, dirpos);
		bestratio = ratio2.ratio;

		for( j = 0; j < MIN( maxlookahead, positionratios.Count() ); j++ )
		{
			ratio2 = positionratios[i];

            // Do not swap when the position is much further along the line.
            if( abs( bestratio - ratio2.ratio ) * lenline > 48.0f )
                break;
                    
            // Now swap if this position-unit line is more aligned with the midline
            dirpos = ratio1.unit->GetAbsOrigin() - m_vPosition;
            dirpos.z = 0.0;
            VectorNormalize(dirpos);
            dot = DotProduct(dir, dirpos);
            if( dot > bestdot )
			{
                bestj = j;
                bestdot = dot;
                bestratio = ratio2.ratio;
			}
		}

		// Order unit using the best found position
		OrderUnit( ratio1.unit, positionratios[bestj].position, angle, selection );
		positionratios.Remove( bestj );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GroupMoveOrder::ComputeSquareFormation( void )
{
	m_Positions.RemoveAll();
	int sizesqrt = ceil(sqrt((double)m_Units.Count()));
	int hsizesqrt = (int)(sizesqrt/2.0);

	int x, y;
	for( int i = 0; i < sizesqrt; i++ )
	{
		for( int j = 0; j < sizesqrt; j++ )
		{
			x = m_vPosition.x + (i - hsizesqrt)*64.0f;
			y = m_vPosition.y + (j - hsizesqrt)*64.0f;
			m_Positions.AddToTail( Vector( x, y, m_vPosition.z ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GroupMoveOrder::ComputeFormationPositions( void )
{
	static Vector offset(0, 0, 48.0);

	float fSpread = unit_orders_spread.GetFloat();

	m_Positions.RemoveAll();
	positioninfo_t info( m_vPosition + offset, -Vector(fSpread, fSpread, 0), Vector(fSpread, fSpread, 18), 0, 2048.0f, 0.0f, 0.0f, NULL, 0.0f, 256.0f, MASK_NPCSOLID_BRUSHONLY );
	while( m_Positions.Count() < m_Units.Count() )
	{
		UTIL_FindPosition(info);
		if( !info.m_bSuccess )
			break;

#ifndef CLIENT_DLL
		if( unit_orders_debug_positions.GetBool() )
		{
			NDebugOverlay::Cross3D(info.m_vPosition, 64.0, 255, 255, 0, false, 5.0);
		}
#endif // CLIENT_DLL

		m_Positions.AddToTail(info.m_vPosition);
	}
            
	// If we did not find enough positions, just fill the rest with the goal position
	while( m_Positions.Count() < m_Units.Count() )
		m_Positions.AddToTail( m_vPosition );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GroupMoveOrder::ComputeFormationMidPoint( Vector &vMidPoint )
{
	vMidPoint.Init( 0, 0, 0 );
	for( int i = 0; i < m_Positions.Count(); i++ )
		vMidPoint += m_Positions[i];
    vMidPoint /= m_Positions.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GroupMoveOrder::ComputeMidPointUnits( Vector &vMidPoint )
{
	vMidPoint.Init( 0, 0, 0 );
	for( int i = 0; i < m_Units.Count(); i++ )
		vMidPoint += m_Units[i]->GetAbsOrigin();
    vMidPoint /= m_Units.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GroupMoveOrder::Project( const Vector &point1, const Vector &point2, const Vector &toproject, Vector &projected )
{
	float m = (point2.y - point1.y) / (point2.x - point1.x);
	float b = point1.y - (m * point1.x);

	projected.x = (m * toproject.y + toproject.x - m * b) / (m * m + 1);
	projected.y = (m * m * toproject.y + m * toproject.x + b) / (m * m + 1);
	projected.z = toproject.z;
}

//-----------------------------------------------------------------------------
// Purpose: Sort functions
//-----------------------------------------------------------------------------
int UnitRatioSort( GroupMoveOrder::UnitRatio_t const *a, GroupMoveOrder::UnitRatio_t const *b )
{
    if( (*a).ratio > (*b).ratio )
		return 1;
	else if( (*a).ratio < (*b).ratio )
		return -1;
    return 0;
}

int PositionRatioSort( GroupMoveOrder::PositionRatio_t const *a, GroupMoveOrder::PositionRatio_t const *b )
{
    if( (*a).ratio > (*b).ratio )
		return 1;
	else if( (*a).ratio < (*b).ratio )
		return -1;
    return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GroupMoveOrder::ProjectAndSortUnits( const Vector &unitmidpoint, const Vector &formationmidpoint, CUtlVector< UnitRatio_t > &result  )
{
	float ratio;
	Vector projected;
	for( int i = 0; i < m_Units.Count(); i++ )
	{
		Project( unitmidpoint, formationmidpoint, m_Units[i]->GetAbsOrigin(), projected );
		ratio = ( projected-unitmidpoint ).Length2D() / (formationmidpoint-unitmidpoint).Length2D();
		result.AddToTail( UnitRatio_t( m_Units[i], ratio ) );
	}

	result.Sort( UnitRatioSort );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GroupMoveOrder::ProjectAndSortPositions( const Vector &unitmidpoint, const Vector &formationmidpoint, CUtlVector< PositionRatio_t > &result  )
{
	float ratio;
	Vector projected;
	for( int i = 0; i < m_Positions.Count(); i++ )
	{
		Project( unitmidpoint, formationmidpoint, m_Positions[i], projected );
		ratio = ( projected-unitmidpoint ).Length2D() / (formationmidpoint-unitmidpoint).Length2D();
		result.AddToTail( PositionRatio_t( m_Positions[i], ratio ) );
	}

	result.Sort( PositionRatioSort );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GroupMoveOrder::OrderUnit( CUnitBase *pUnit, const Vector &targetposition, const QAngle &angle, bp::list selection )
{
	bool findhidespot = m_Units.Count() < 10;
	pUnit->GetPyHandle().attr("MoveOrderInternal")( targetposition, angle, selection, m_vPosition, findhidespot );
}