//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Simple 2D grid for flora, cover and props.
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

//-----------------------------------------------------------------------------
// Purpose: Data structs for grid
//-----------------------------------------------------------------------------
typedef CUtlVector< CWarsFlora * > FloraVector;

typedef struct wars_coverspot_t
{
	enum 
	{ 
		NOTSAVED			= 0x01, // Discarded when saving cover
		FROMNAVMESH			= 0x100 // Temp flag indicating the cover spot comes from the old nav mesh
	};

	int id;
	Vector position;
	unsigned short flags;
} wars_coverspot_t;
typedef CUtlVector< wars_coverspot_t > CoverVector;

typedef struct wars_gridentry_t
{
	FloraVector flora;
	CoverVector coverspots;
} wars_gridentry_t;

typedef CUtlVector< wars_gridentry_t > WarsGridVector;

//-----------------------------------------------------------------------------
// Purpose: Manages the grid
//-----------------------------------------------------------------------------
class CWarsGrid
{
public:
	void LevelInit();
	void LevelShutdown();

	WarsGridVector &GetGrid();
	wars_gridentry_t &GetGridEntry( int key );
	wars_gridentry_t &GetGridEntry( const Vector &vPos );

	bool LoadGridData();
	bool SaveGridData( KeyValues *pKVWars );

	// Cover
	int CreateCoverSpot( const Vector &vPos, unsigned int flags = 0 );
	bool DestroyCoverSpot( const Vector &vPos, int id, unsigned int excludeFlags = 0 );
	bool ConvertOldNavMeshCover();

	//-----------------------------------------------------------------------------
	// Purpose: Apply the functor to all cover in radius.
	//-----------------------------------------------------------------------------
	template < typename Functor >
	bool ForAllCoverInRadius( Functor &func, const Vector &vPosition, float fRadius )
	{
		CUtlVector<wars_coverspot_t *> foundCover;
		float fRadiusSqr = fRadius * fRadius;

		int originX = WARSGRID_KEYSINGLE( vPosition.x );
		int originY = WARSGRID_KEYSINGLE( vPosition.y );
		int shiftLimit = ceil( fRadius / WARSGRID_TILESIZE );

		for( int x = originX - shiftLimit; x <= originX + shiftLimit; ++x )
		{
			if ( x < 0 || x >= WARSGRID_GRIDSIZE )
				continue;

			for( int y = originY - shiftLimit; y <= originY + shiftLimit; ++y )
			{
				if ( y < 0 || y >= WARSGRID_GRIDSIZE )
					continue;

				CoverVector &coverVector = WarsGrid().GetGrid()[x + (y * WARSGRID_GRIDSIZE)].coverspots;
				for( int i = 0; i < coverVector.Count(); i++ )
				{
					wars_coverspot_t *pCover = &coverVector[i];
					if( pCover && pCover->position.DistToSqr( vPosition ) < fRadiusSqr )
					{
						foundCover.AddToTail( pCover );
					}
				}
			}
		}

		FOR_EACH_VEC( foundCover, idx )
		{
			wars_coverspot_t *pCover = foundCover[idx];
			if( func( pCover ) == false )
				return false;
		}

		return true;
	}

private:
	void LoadCover( KeyValues *pKVWars );
	bool SaveCover( KeyValues *pKVWars );

private:
	int m_nNextCoverID;
};

CWarsGrid &WarsGrid();

#endif // WARS_GRID_H