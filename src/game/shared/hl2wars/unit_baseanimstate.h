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

#ifndef DISABLE_PYTHON
	UnitBaseAnimState( boost::python::object outer );
#endif // DISABLE_PYTHON

	virtual ~UnitBaseAnimState();

	virtual void Update( float eyeYaw, float eyePitch ) {}

	// Get the correct interval
	float	GetAnimTimeInterval( void ) const;
};

#endif // UNIT_BASEANIMSTATE_H