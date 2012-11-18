//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
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
#ifndef DISABLE_PYTHON
	UnitBaseAirNavigator( boost::python::object outer );
#endif // DISABLE_PYTHON

	virtual void		Update( UnitBaseMoveCommand &mv );
	virtual CheckGoalStatus_t	MoveUpdateWaypoint();

	virtual bool		ShouldConsiderNavMesh( void );

	virtual bool		TestRoute( const Vector &vStartPos, const Vector &vEndPos );

	int GetTestRouteMask();
	void SetTestRouteMask( int mask );

private:
	float m_fCurrentHeight;
	int m_iTestRouteMask;
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

#endif // UNIT_AIRNAVIGATOR_H