//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "wars_grid.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static WarsGridVector s_WarsGrid;
static wars_gridentry_t empty;

void CWarsGrid::LevelInit()
{
	s_WarsGrid.SetCount( WARSGRID_TILESIZE * WARSGRID_TILESIZE );

	// Load in data
}

void CWarsGrid::LevelShutdown()
{
	s_WarsGrid.RemoveAll();
}

WarsGridVector &CWarsGrid::GetGrid()
{
	return s_WarsGrid;
}

wars_gridentry_t &CWarsGrid::GetGridEntry( int key )
{
	if( key < 0 || key > s_WarsGrid.Count() )
	{
		Warning("GetGridEntry: accessing outside bounds of grid at key %d\n", key );
		return empty;
	}
	return s_WarsGrid[key];
}

wars_gridentry_t &CWarsGrid::GetGridEntry( const Vector &vPos )
{
	return GetGridEntry( WARSGRID_KEY( vPos ) );
}