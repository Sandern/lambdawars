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


class UnitBaseAirNavigator : public UnitBaseNavigator
{
	DECLARE_CLASS(UnitBaseAirNavigator, UnitBaseNavigator);

public:
#ifdef ENABLE_PYTHON
	UnitBaseAirNavigator( boost::python::object outer );
#endif // ENABLE_PYTHON

	virtual void		Update( UnitBaseMoveCommand &mv );
	virtual CheckGoalStatus_t	MoveUpdateWaypoint( UnitBaseMoveCommand &MoveCommand );

	virtual UnitBaseWaypoint *	BuildLocalPath( const Vector &pos );

	virtual bool		ShouldConsiderNavMesh( void );

	virtual bool		TestRoute( const Vector &vStartPos, const Vector &vEndPos );

	int GetTestRouteMask();
	void SetTestRouteMask( int mask );

	bool GetTestRouteWorldOnly();
	void SetTestRouteWorldOnly( bool enable );

	bool GetUseSimplifiedRouteBuilding();
	void SetUseSimplifiedRouteBuilding( bool enable );

private:
	float m_fCurrentHeight;
	int m_iTestRouteMask;
	bool m_bTestRouteWorldOnly;
	bool m_bUseSimplifiedRouteBuilding;
};

// Inlines
inline bool UnitBaseAirNavigator::ShouldConsiderNavMesh( void )
{
	return false; // Never add density from nav areas
}

inline int UnitBaseAirNavigator::GetTestRouteMask()
{
	return m_iTestRouteMask;
}

inline void UnitBaseAirNavigator::SetTestRouteMask( int mask )
{
	m_iTestRouteMask = mask;
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