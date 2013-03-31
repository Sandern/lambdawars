//========= Copyright 2013, Sandern Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "tilebase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTileBase::CTileBase( void )
{
#ifndef CLIENT_DLL
	SetDensityMapType( DENSITY_NONE );
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTileBase::Spawn( void )
{
	BaseClass::Spawn();

    SetSolid( SOLID_BBOX );
    SetMoveType( MOVETYPE_NONE );

#ifndef CLIENT_DLL
	AddFlag( FL_STATICPROP );
#else
	InvalidatePhysicsRecursive( POSITION_CHANGED | ANGLES_CHANGED | VELOCITY_CHANGED );
    UpdatePartitionListEntry();
    UpdateVisibility();
	// CreateShadow() // TODO: Seems bugged
             
    // Reset interpolation history, so it doesn't contains shit
    // -> shouldn't do anything for client side created entities...
    //    Apparently it does do something, since it fucks up the angles of all tiles.
    // ResetLatched() 
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTileBase::UpdateModel( void )
{

}