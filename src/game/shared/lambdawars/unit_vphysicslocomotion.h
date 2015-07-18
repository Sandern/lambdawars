//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#ifndef UNIT_VPHYSICSLOCOMOTION_H
#define UNIT_VPHYSICSLOCOMOTION_H

#ifdef _WIN32
#pragma once
#endif

#include "unit_locomotion.h"

//-----------------------------------------------------------------------------
// Unit Base Locomotion class. Deals with movement.
//-----------------------------------------------------------------------------
class UnitVPhysicsLocomotion : public UnitBaseLocomotion, public IMotionEvent
{
public:
#ifdef ENABLE_PYTHON
	UnitVPhysicsLocomotion( boost::python::object outer );
#endif // ENABLE_PYTHON
	~UnitVPhysicsLocomotion();

	virtual void SetupMove( UnitBaseMoveCommand &mv );
	virtual void FinishMove( UnitBaseMoveCommand &mv );

	virtual void	FullWalkMove();
	virtual float	GetStopDistance();
	virtual void	VPhysicsMove();
	virtual void	VPhysicsMoveStep();

	// No facing needed for roller mine
	virtual void	MoveFacing() {}

	// Does the simulation
	void CreateMotionController();
	void DestroyMotionController();

	virtual simresult_e	Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );

private:
	Vector m_vecLinear;
	AngularImpulse m_vecAngular;

	IPhysicsMotionController *m_pMotionController;
};

#endif // UNIT_VPHYSICSLOCOMOTION_H