//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "wars_grid.h"
#include "filesystem.h"

#include "editor/editorwarsmapmgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static WarsGridVector s_WarsGrid;
static wars_gridentry_t empty;

static CWarsGrid s_WarsGridInstance;
CWarsGrid &WarsGrid() { return s_WarsGridInstance; }

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsGrid::LevelInit()
{
	s_WarsGrid.SetCount( WARSGRID_TILESIZE * WARSGRID_TILESIZE );

	// Load in data
	LoadGridData();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsGrid::LevelShutdown()
{
	s_WarsGrid.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
WarsGridVector &CWarsGrid::GetGrid()
{
	return s_WarsGrid;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
wars_gridentry_t &CWarsGrid::GetGridEntry( int key )
{
	if( key < 0 || key > s_WarsGrid.Count() )
	{
		Warning("GetGridEntry: accessing outside bounds of grid at key %d\n", key );
		return empty;
	}
	return s_WarsGrid[key];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
wars_gridentry_t &CWarsGrid::GetGridEntry( const Vector &vPos )
{
	return GetGridEntry( WARSGRID_KEY( vPos ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWarsGrid::LoadGridData()
{
	char curMap[MAX_PATH];
	CEditorWarsMapMgr::BuildCurrentWarsPath( curMap, sizeof( curMap ) );

	KeyValues *pKVWars = new KeyValues( "WarsMap" );
	KeyValues::AutoDelete autodelete( pKVWars );
	if( pKVWars->LoadFromFile( filesystem, curMap, NULL ) )
	{
		LoadCover( pKVWars );
	}
	else
	{
		LoadCover( NULL );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWarsGrid::SaveGridData( KeyValues *pKVWars )
{
	SaveCover( pKVWars );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsGrid::LoadCover( KeyValues *pKVWars )
{
	m_nNextCoverID = 0;

	if( pKVWars )
	{
		KeyValues *pCover = pKVWars->FindKey( "cover" );
		if( pCover )
		{
			for ( KeyValues *pKey = pCover->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
			{
				if ( V_strcmp( pKey->GetName(), "spot" ) )
					continue;

				Vector vecPos;
				UTIL_StringToVector( vecPos.Base(), pKey->GetString( "position" ) );
				int flags = pKey->GetInt( "flags", 0 );

				CreateCoverSpot( vecPos, flags );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWarsGrid::SaveCover( KeyValues *pKVWars )
{
	// Remove old key
	KeyValues *pCoverKey = pKVWars->FindKey( "cover" );
	if( pCoverKey != NULL )
	{
		pKVWars->RemoveSubKey( pCoverKey );
	}
	
	// Create new key with updated cover
	pCoverKey = pKVWars->FindKey( "cover", true );

	FOR_EACH_VEC( s_WarsGrid, gridit )
	{
		wars_gridentry_t &entry = s_WarsGrid[gridit];

		FOR_EACH_VEC( entry.coverspots, it )
		{
			wars_coverspot_t &spot = entry.coverspots[it];
			if( spot.flags & wars_coverspot_t::NOTSAVED )
			{
				continue;
			}

			KeyValues *pKVNew = new KeyValues("spot");
			pKVNew->SetString( "position", UTIL_VarArgs( "%.2f %.2f %.2f", XYZ( spot.position ) ) );
			pKVNew->SetInt( "flags", spot.flags );
			pCoverKey->AddSubKey( pKVNew );
		}
	}

	return true;
}

//----------------------------------------------------------------------------- 
// Purpose: 
//-----------------------------------------------------------------------------
int CWarsGrid::CreateCoverSpot( const Vector &vPos, unsigned int flags )
{
	wars_gridentry_t &entry = GetGridEntry( vPos );
	entry.coverspots.AddToTail();

	wars_coverspot_t &spot = entry.coverspots.Tail();
	spot.id = m_nNextCoverID++;
	spot.position = vPos;
	spot.flags = flags;

	return spot.id;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWarsGrid::DestroyCoverSpot( const Vector &vPos, int id, unsigned int excludeFlags )
{
	wars_gridentry_t &entry = GetGridEntry( vPos );
	FOR_EACH_VEC( entry.coverspots, it )
	{
		if( entry.coverspots[it].flags & excludeFlags )
		{
			continue;
		}

		if( entry.coverspots[it].id == id )
		{
			entry.coverspots.Remove( it );
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWarsGrid::ConvertOldNavMeshCover()
{
	FOR_EACH_VEC( s_WarsGrid, gridit )
	{
		wars_gridentry_t &entry = s_WarsGrid[gridit];

		FOR_EACH_VEC( entry.coverspots, it )
		{
			wars_coverspot_t &spot = entry.coverspots[it];
			if( (spot.flags & wars_coverspot_t::FROMNAVMESH) == 0 )
			{
				continue;
			}

			spot.flags &= ~(wars_coverspot_t::FROMNAVMESH|wars_coverspot_t::NOTSAVED);
		}
	}
	return true;
}