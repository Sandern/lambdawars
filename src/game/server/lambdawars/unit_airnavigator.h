//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Defines the locomotion of an air moving unit.
//
//=============================================================================//

#ifndef UNIT_AIRNAVIGATOR_H
#define UNIT_AIRNAVIGATOR_H

#ifdef _WIN32
#pragma once
#endif

#include "unit_navigator.h"
#include "unit_airlocomotion.h"

class UnitBaseAirNavigator : public UnitBaseNavigator
{
	DECLARE_CLASS(UnitBaseAirNavigator, UnitBaseNavigator);

public:
#ifdef ENABLE_PYTHON
	UnitBaseAirNavigator( boost::python::object outer );
#endif // ENABLE_PYTHON

	virtual void		Update( UnitAirMoveCommand &mv );
	virtual CheckGoalStatus_t	MoveUpdateWaypoint( UnitBaseMoveCommand &MoveCommand );

	virtual UnitBaseWaypoint *	BuildLocalPath( const Vector &pos );

	virtual bool		ShouldConsiderNavMesh( void );

	//virtual bool		TestRoute( const Vector &vStartPos, const Vector &vEndPos );

	bool GetTestRouteWorldOnly();
	void SetTestRouteWorldOnly( bool enable );

	bool GetUseSimplifiedRouteBuilding();
	void SetUseSimplifiedRouteBuilding( bool enable );

	virtual CRecastMesh *GetNavMesh();

private:
	float m_fCurrentHeight;
	float m_fDesiredHeight;
	bool m_bTestRouteWorldOnly;
	bool m_bUseSimplifiedRouteBuilding;
};

// Inlines
inline bool UnitBaseAirNavigator::ShouldConsiderNavMesh( void )
{
	return false; // Never add density from nav areas
}

inline bool UnitBaseAirNavigator::GetTestRouteWorldOnly()
{
	return m_bTestRouteWorldOnly;
}

inline void UnitBaseAirNavigator::SetTestRouteWorldOnly( bool enable )
{
	m_bTestRouteWorldOnly = enable;
}

inline bool UnitBaseAirNavigator::GetUseSimplifiedRouteBuilding()
{
	return m_bUseSimplifiedRouteBuilding;
}

inline void UnitBaseAirNavigator::SetUseSimplifiedRouteBuilding( bool enable )
{
	m_bUseSimplifiedRouteBuilding = enable;
}

#endif // UNIT_AIRNAVIGATOR_H