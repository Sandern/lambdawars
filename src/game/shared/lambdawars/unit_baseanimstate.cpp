//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "unit_baseanimstate.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ------------------------------------------------------------------------------------------------ //
// UnitBaseAnimState implementation.
// ------------------------------------------------------------------------------------------------ //
#ifdef ENABLE_PYTHON
UnitBaseAnimState::UnitBaseAnimState(boost::python::object outer) : UnitComponent(outer)
{
}
#endif // ENABLE_PYTHON

UnitBaseAnimState::~UnitBaseAnimState()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float UnitBaseAnimState::GetAnimTimeInterval( void ) const
{
#ifdef CLIENT_DLL
	return gpGlobals->frametime;
#else
	return GetOuter()->GetAnimTimeInterval();
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseAnimState::GetOuterAbsVelocity( Vector& vel ) const
{
#if defined( CLIENT_DLL )
	m_pOuter->EstimateAbsVelocity( vel );
#else
	vel = m_pOuter->GetAbsVelocity();
#endif

	// Special case: track train. Substract base velocity.
	CBaseEntity *pEnt = m_pOuter->GetGroundEntity();
	if( pEnt && pEnt->IsBaseTrain() )
	{
		Vector baseVel;
#if defined( CLIENT_DLL )
		pEnt->EstimateAbsVelocity( baseVel );
#else
		baseVel = pEnt->GetAbsVelocity();
#endif
		vel -= baseVel;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float UnitBaseAnimState::GetOuterXYSpeed() const
{
	Vector vel;
	GetOuterAbsVelocity( vel );
	return vel.Length2D();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseAnimState::SetOuterPoseParameter( int iParam, float flValue )
{
	// Make sure to set all the history values too, otherwise the server can overwrite them.
	GetOuter()->SetPoseParameter( iParam, flValue );
}