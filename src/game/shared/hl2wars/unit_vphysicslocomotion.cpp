//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_vphysicslocomotion.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Movement class
//-----------------------------------------------------------------------------
#ifdef ENABLE_PYTHON
UnitVPhysicsLocomotion::UnitVPhysicsLocomotion( boost::python::object outer ) : UnitBaseLocomotion(outer)
{

}
#endif // ENABLE_PYTHON

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void UnitVPhysicsLocomotion::GroundMove()
{
	IPhysicsObject *pPhysObj = GetOuter()->VPhysicsGetObject();
	if( !pPhysObj )
	{
		Warning( "UnitVPhysicsLocomotion: No VPhysics object for entity #%d\n", GetOuter()->entindex() );
		return;
	}

	pPhysObj->SetVelocity( &(mv->velocity), NULL );
}