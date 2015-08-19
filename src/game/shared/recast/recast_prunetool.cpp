//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
// Adapted for Lambda Wars

#include "cbase.h"

#include "recast/recast_mesh.h"
#include "recast/recast_common.h"

#include "detour/DetourNavMesh.h"
#include "detour/DetourCommon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Copy/paste from Recast int array


static void floodNavmesh(dtNavMesh* nav, NavmeshFlags* flags, dtPolyRef start, unsigned char flag)
{
	// If already visited, skip.
	if (flags->getFlags(start))
		return;
		
	PolyRefArray openList;
	openList.push(start);

	while (openList.size())
	{
		const dtPolyRef ref = openList.pop();
		// Get current poly and tile.
		// The API input has been cheked already, skip checking internal data.
		const dtMeshTile* tile = 0;
		const dtPoly* poly = 0;
		nav->getTileAndPolyByRefUnsafe(ref, &tile, &poly);

		// Visit linked polygons.
		for (unsigned int i = poly->firstLink; i != DT_NULL_LINK; i = tile->links[i].next)
		{
			const dtPolyRef neiRef = tile->links[i].ref;
			// Skip invalid and already visited.
			if (!neiRef || flags->getFlags(neiRef))
				continue;
			// Mark as visited
			flags->setFlags(neiRef, flag);
			// Visit neighbours
			openList.push(neiRef);
		}
	}
}

static void disableUnvisitedPolys(dtNavMesh* nav, NavmeshFlags* flags)
{
	for (int i = 0; i < nav->getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = ((const dtNavMesh*)nav)->getTile(i);
		if (!tile->header) continue;
		const dtPolyRef base = nav->getPolyRefBase(tile);
		for (int j = 0; j < tile->header->polyCount; ++j)
		{
			const dtPolyRef ref = base | (unsigned int)j;
			if (!flags->getFlags(ref))
			{
				unsigned short f = 0;
				nav->getPolyFlags(ref, &f);
				nav->setPolyFlags(ref, f | SAMPLE_POLYFLAGS_DISABLED);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Marks unreachable polygons for samplePositions with 
//			SAMPLE_POLYFLAGS_DISABLED flag.
//-----------------------------------------------------------------------------
bool CRecastMesh::DisableUnreachablePolygons( const CUtlVector< Vector > &samplePositions )
{
	NavmeshFlags *flags = new NavmeshFlags;
	flags->init( m_navMesh );

	for( int i = 0; i < samplePositions.Count(); i++ )
	{
		floodNavmesh( m_navMesh, flags, GetPolyRef( samplePositions[i] ), 1 );
	}

	disableUnvisitedPolys( m_navMesh, flags );

	delete flags;

	return true;
}
