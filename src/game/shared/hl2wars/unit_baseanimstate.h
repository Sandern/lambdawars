//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
//=============================================================================//

#ifndef UNIT_BASEANIMSTATE_H
#define UNIT_BASEANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "unit_component.h"

class UnitBaseAnimState : public UnitComponent
{
public:
	DECLARE_CLASS_NOBASE( UnitBaseAnimState );

#ifdef ENABLE_PYTHON
	UnitBaseAnimState( boost::python::object outer );
#endif // ENABLE_PYTHON

	virtual ~UnitBaseAnimState();

	virtual void Update( float eyeYaw, float eyePitch ) {}

	// Get the correct interval
	float	GetAnimTimeInterval( void ) const;

	// Check if the unit has this activity
	bool HasActivity( Activity actDesired );

	// Allow inheriting classes to override SelectWeightedSequence
	virtual int SelectWeightedSequence( Activity activity );
};

// Inlines
inline int UnitBaseAnimState::SelectWeightedSequence( Activity activity ) 
{
	return GetOuter()->SelectWeightedSequence( activity ); 
}

inline bool UnitBaseAnimState::HasActivity( Activity actDesired ) 
{ 
	return SelectWeightedSequence( actDesired ) > 0; 
}

#endif // UNIT_BASEANIMSTATE_H