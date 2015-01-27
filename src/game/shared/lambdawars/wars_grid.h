//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:		
//
//=============================================================================//

#ifndef WARS_GRID_H
#define WARS_GRID_H

#ifdef _WIN32
#pragma once
#endif

class CWarsFlora;

#define WARSGRID_TILESIZE 256
#define WARSGRID_GRIDSIZE (COORD_EXTENT/WARSGRID_TILESIZE)
#define WARSGRID_KEYSINGLE( x ) (int)((MAX_COORD_INTEGER+x) / WARSGRID_TILESIZE)
#define WARSGRID_KEY( position ) (int)((MAX_COORD_INTEGER+position.x) / WARSGRID_TILESIZE) + ((int)((MAX_COORD_INTEGER+position.y) / WARSGRID_TILESIZE) * WARSGRID_GRIDSIZE)

typedef CUtlVector< CWarsFlora * > FloraVector;

typedef struct wars_coverspot_t
{
	int id;
	Vector position;
	unsigned short flags;
} wars_coverspot_t;

typedef struct wars_gridentry_t
{
	FloraVector flora;
	CUtlVector< wars_coverspot_t > coverspots;
} wars_gridentry_t;

typedef CUtlVector< wars_gridentry_t > WarsGridVector;

class CWarsGrid
{
public:
	static void LevelInit();
	static void LevelShutdown();

	static WarsGridVector &GetGrid();
	static wars_gridentry_t &GetGridEntry( int key );
	static wars_gridentry_t &GetGridEntry( const Vector &vPos );
};

#endif // WARS_GRID_H