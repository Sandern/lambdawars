//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Implementation of save/loading the Recast meshes
//
// Note: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast/recast_mgr.h"
#include "recast/recast_mesh.h"
#include "recast/recast_file.h"

#include "datacache/imdlcache.h"
#include <filesystem.h>

#include "detour/DetourNavMesh.h"
#include "detour/DetourNavMeshQuery.h"
#include "detourtilecache/DetourTileCache.h"
#include "detourtilecache/DetourTileCacheBuilder.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define FORMAT_BSPFILE "maps\\%s.bsp"
#define FORMAT_NAVFILE "maps\\%s.recast"
#define EXT_NAVFILE "recast"

static const int NAVMESHSET_MAGIC = 'M'<<24 | 'S'<<16 | 'E'<<8 | 'T'; //'MSET';
static const int NAVMESHSET_VERSION = 1;

struct NavMgrHeader
{
	int magic;
	int version;
	int numMeshes;
};

struct TileCacheSetHeader
{
	int cellSize;
	int cellHeight;
	int numTiles;
	dtNavMeshParams meshParams;
	dtTileCacheParams cacheParams;
	int lenName;
};

struct TileCacheTileHeader
{
	dtCompressedTileRef tileRef;
	int dataSize;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::Load( CUtlBuffer &fileBuffer )
{
	// Read header.
	TileCacheSetHeader header;
	fileBuffer.Get( &header, sizeof( header ) );
	if( !fileBuffer.IsValid() )
	{
		Msg( "Failed to read mesh header.\n" );
		return false;
	}

	m_cellSize = header.cellSize;
	m_cellHeight = header.cellHeight;

	if( header.lenName > 2048 )
	{
		Msg( "Mesh name too long. Invalid mesh file?\n" );
		return false;
	}

	char *szName = (char *)stackalloc( sizeof( char ) * (header.lenName + 1) );
	fileBuffer.Get( szName, header.lenName );
	szName[header.lenName] = 0;

	Init( szName );

	m_navMesh = dtAllocNavMesh();
	if (!m_navMesh)
	{
		return false;
	}
	dtStatus status = m_navMesh->init(&header.meshParams);
	if (dtStatusFailed(status))
	{
		return false;
	}

	m_tileCache = dtAllocTileCache();
	if (!m_tileCache)
	{
		return false;
	}
	status = m_tileCache->init(&header.cacheParams, m_talloc, m_tcomp, m_tmproc);
	if (dtStatusFailed(status))
	{
		Warning("Failed to init tile cache\n");
		return false;
	}
		
	// Read tiles.
	for (int i = 0; i < header.numTiles; ++i)
	{
		TileCacheTileHeader tileHeader;
		fileBuffer.Get( &tileHeader, sizeof(tileHeader) );
		if( !fileBuffer.IsValid() )
			return false;

		if (!tileHeader.tileRef || !tileHeader.dataSize)
			break;

		unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
		if (!data) 
			break;
		V_memset(data, 0, tileHeader.dataSize);
		fileBuffer.Get( data, tileHeader.dataSize );
		if( !fileBuffer.IsValid() )
			return false;
		
		dtCompressedTileRef tile = 0;
		m_tileCache->addTile(data, tileHeader.dataSize, DT_COMPRESSEDTILE_FREE_DATA, &tile);

		if (tile)
			m_tileCache->buildNavMeshTile(tile, m_navMesh);
	}

	// Init nav query
	status = m_navQuery->init(m_navMesh, 2048);
	if( dtStatusFailed(status) )
	{
		Warning("Could not init Detour navmesh query\n");
		return false;
	}

	Msg( "Loaded mesh %s\n", m_Name.Get() );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMgr::Load()
{
	Reset();

	MDLCACHE_CRITICAL_SECTION();

	char filename[256];
#ifdef CLIENT_DLL
	V_snprintf( filename, sizeof( filename ), "%s", STRING( engine->GetLevelName() ) );
	V_StripExtension( filename, filename, 256 );
	V_snprintf( filename, sizeof( filename ), "%s.%s", filename, EXT_NAVFILE );
#else
	V_snprintf( filename, sizeof( filename ), FORMAT_NAVFILE, STRING( gpGlobals->mapname ) );
#endif // CLIENT_DLL

	bool navIsInBsp = false;
	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::READ_ONLY );
	if ( !filesystem->ReadFile( filename, "MOD", fileBuffer ) )	// this ignores .nav files embedded in the .bsp ...
	{
		navIsInBsp = true;
		if ( !filesystem->ReadFile( filename, "BSP", fileBuffer ) )	// ... and this looks for one if it's the only one around.
		{
			return false;
		}
	}

	// Read header of nav mesh file
	NavMgrHeader header;
	fileBuffer.Get( &header, sizeof( header ) );
	if ( !fileBuffer.IsValid() || header.magic != NAVMESHSET_MAGIC )
	{
		Msg( "Invalid navigation file '%s'.\n", filename );
		return false;
	}

	// read file version number
	if ( !fileBuffer.IsValid() || header.version > NAVMESHSET_VERSION )
	{
		Msg( "Unknown navigation file version.\n" );
		return false;
	}

	// Read the meshes!
	for( int i = 0; i < header.numMeshes; i++ )
	{
		CRecastMesh *pMesh = new CRecastMesh();
		if( !pMesh->Load( fileBuffer ) )
			return false;

		m_Meshes.Insert( pMesh->GetName(), pMesh );
	}
	
	return true;
}

#ifndef CLIENT_DLL
bool CRecastMesh::Save( CUtlBuffer &fileBuffer )
{
	Msg( "Saving mesh %s (%d)\n", m_Name.Get(), m_Name.Length() );

	// Store header.
	TileCacheSetHeader header;
	header.cellSize = m_cellSize;
	header.cellHeight = m_cellHeight;
	header.numTiles = 0;
	header.lenName = m_Name.Length();
	for (int i = 0; i < m_tileCache->getTileCount(); ++i)
	{
		const dtCompressedTile* tile = m_tileCache->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) 
			continue;
		header.numTiles++;
	}
	memcpy(&header.cacheParams, m_tileCache->getParams(), sizeof(dtTileCacheParams));
	memcpy(&header.meshParams, m_navMesh->getParams(), sizeof(dtNavMeshParams));
	fileBuffer.Put( &header, sizeof( header ) );
	fileBuffer.Put( m_Name.Get(), m_Name.Length() );

	// Store tiles.
	for (int i = 0; i < m_tileCache->getTileCount(); ++i)
	{
		const dtCompressedTile* tile = m_tileCache->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;

		TileCacheTileHeader tileHeader;
		tileHeader.tileRef = m_tileCache->getTileRef(tile);
		tileHeader.dataSize = tile->dataSize;
		fileBuffer.Put( &tileHeader, sizeof( tileHeader ) );

		fileBuffer.Put( tile->data, tile->dataSize );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return the filename for this map's "nav map" file
//-----------------------------------------------------------------------------
const char *CRecastMgr::GetFilename( void ) const
{
	// filename is local to game dir for Steam, so we need to prepend game dir for regular file save
	char gamePath[256];
	engine->GetGameDir( gamePath, 256 );

	// persistant return value
	static char filename[256];
	V_snprintf( filename, sizeof( filename ), "%s\\" FORMAT_NAVFILE, gamePath, STRING( gpGlobals->mapname ) );

	return filename;
}

//-----------------------------------------------------------------------------
// Purpose: Saves the generated navigation meshes
//-----------------------------------------------------------------------------
bool CRecastMgr::Save()
{
	const char *filename = GetFilename();
	if( filename == NULL )
		return false;

	CUtlBuffer fileBuffer( 4096, 1024*1024 );

	NavMgrHeader header;
	header.magic = NAVMESHSET_MAGIC;
	header.version = NAVMESHSET_VERSION;
	header.numMeshes = m_Meshes.Count();
	fileBuffer.Put( &header, sizeof( header ) );

	for ( int i = m_Meshes.First(); i != m_Meshes.InvalidIndex(); i = m_Meshes.Next(i ) )
	{
		m_Meshes[i]->Save( fileBuffer );
	}

	if ( !filesystem->WriteFile( filename, "MOD", fileBuffer ) )
	{
		Warning( "Unable to save %d bytes to %s\n", fileBuffer.Size(), filename );
		return false;
	}

	unsigned int navSize = filesystem->Size( filename );
	DevMsg( "Size of nav file '%s' is %u bytes.\n", filename, navSize );

	return true;
}

#endif // CLIENT_DLL
