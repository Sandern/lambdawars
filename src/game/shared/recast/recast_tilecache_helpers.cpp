//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// Note: Recasts expects "y" to be up, so y and z must be swapped everywhere. 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast_tilecache_helpers.h"
#include "recast/recast_mesh.h"
#ifndef CLIENT_DLL
#include "recast/recast_offmesh_connection.h"
#endif // CLIENT_DLL

#include "detour/DetourNavMeshBuilder.h"
#include "detour/DetourCommon.h"
#include "detourtilecache/DetourTileCache.h"
#include "detourtilecache/DetourTileCacheBuilder.h"

#include "fastlz/fastlz.h"

// memdbgon must be the last include file in a .cpp file!!!
//#include "tier0/memdbgon.h"

int FastLZCompressor::maxCompressedSize(const int bufferSize)
{
	return (int)(bufferSize* 1.05f);
}
	
dtStatus FastLZCompressor::compress(const unsigned char* buffer, const int bufferSize,
							unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize)
{

	*compressedSize = fastlz_compress((const void *const)buffer, bufferSize, compressed);
	return DT_SUCCESS;
}
	
dtStatus FastLZCompressor::decompress(const unsigned char* compressed, const int compressedSize,
							unsigned char* buffer, const int maxBufferSize, int* bufferSize)
{
	*bufferSize = fastlz_decompress(compressed, compressedSize, buffer, maxBufferSize);
	return *bufferSize < 0 ? DT_FAILURE : DT_SUCCESS;
}

LinearAllocator::LinearAllocator(const int cap) : buffer(0), capacity(0), top(0), high(0)
{
	resize(cap);
}
	
LinearAllocator::~LinearAllocator()
{
	dtFree(buffer);
}

void LinearAllocator::resize(const int cap)
{
	if (buffer) dtFree(buffer);
	buffer = (unsigned char*)dtAlloc(cap, DT_ALLOC_PERM);
	capacity = cap;
}
	
void LinearAllocator::reset()
{
	high = dtMax(high, top);
	top = 0;
}
	
void* LinearAllocator::alloc(const int size)
{
	if (!buffer)
		return 0;
	if (top+size > capacity)
		return 0;
	unsigned char* mem = &buffer[top];
	top += size;
	return mem;
}
	
void LinearAllocator::free(void* /*ptr*/)
{
	// Empty
}


MeshProcess::MeshProcess( const char *meshName )
{
	meshFlags = 0;
#ifndef CLIENT_DLL
	if( V_strncmp( meshName, "human", V_strlen( meshName ) ) == 0 )
	{
		meshFlags |= SF_OFFMESHCONN_HUMAN;
	}
	else if( V_strncmp( meshName, "medium", V_strlen( meshName ) )== 0 )
	{
		meshFlags |= SF_OFFMESHCONN_MEDIUM;
	}
	else if( V_strncmp( meshName, "large", V_strlen( meshName ) )== 0 )
	{
		meshFlags |= SF_OFFMESHCONN_LARGE;
	}
	else if( V_strncmp( meshName, "verylarge", V_strlen( meshName ) )== 0 )
	{
		meshFlags |= SF_OFFMESHCONN_VERYLARGE;
	}
	else if( V_strncmp( meshName, "air", V_strlen( meshName ) )== 0 )
	{
		meshFlags |= SF_OFFMESHCONN_AIR;
	}
#endif // CLIENT_DLL
}

static void TestAgentRadius( const char *meshName, float &fAgentRadius )
{
	float meshAgentRadius, stub;
	CRecastMesh::ComputeMeshSettings( meshName, meshAgentRadius, stub, stub, stub, stub, stub );
	if( meshAgentRadius > fAgentRadius ) 
	{
		fAgentRadius = meshAgentRadius;
	}
}

void MeshProcess::process(struct dtNavMeshCreateParams* params,
						unsigned char* polyAreas, unsigned short* polyFlags)
{
	// Update poly flags from areas.
	for (int i = 0; i < params->polyCount; ++i)
	{
		if (polyAreas[i] == DT_TILECACHE_WALKABLE_AREA)
			polyAreas[i] = SAMPLE_POLYAREA_GROUND;

		if (polyAreas[i] == SAMPLE_POLYAREA_GROUND ||
			polyAreas[i] == SAMPLE_POLYAREA_GRASS ||
			polyAreas[i] == SAMPLE_POLYAREA_ROAD)
		{
			polyFlags[i] = SAMPLE_POLYFLAGS_WALK;
		}
		else if (polyAreas[i] == SAMPLE_POLYAREA_WATER)
		{
			polyFlags[i] = SAMPLE_POLYFLAGS_SWIM;
		}
		else if (polyAreas[i] == SAMPLE_POLYAREA_DOOR)
		{
			polyFlags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
		}
	}

#ifndef CLIENT_DLL
	// Pass in off-mesh connections.
	// TODO: Currently this is always server side, but we don't really need them client side atm.
	offMeshConnVerts.Purge();
	offMeshConnRad.Purge();
	offMeshConnDir.Purge();
	offMeshConnArea.Purge();
	offMeshConnFlags.Purge();
	offMeshConnId.Purge();

	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "recast_offmesh_connection" );
	while( pEnt )
	{
		COffMeshConnection *pOffMeshConn = dynamic_cast< COffMeshConnection * >( pEnt );
		if( pOffMeshConn )
		{
			// Only parse connections for which we have the flag set
			int spawnFlags = pOffMeshConn->GetSpawnFlags();
			if( (spawnFlags & meshFlags) != 0 ) 
			{
				CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, pOffMeshConn->m_target );
				if( pTarget )
				{
					// Horrifying
					int vertOffset = offMeshConnVerts.Count();
					offMeshConnVerts.AddMultipleToTail( 6 );

					const Vector &vStart = pOffMeshConn->GetAbsOrigin();
					const Vector &vEnd = pTarget->GetAbsOrigin();

					offMeshConnVerts[vertOffset] = vStart.x;
					offMeshConnVerts[vertOffset+1] = vStart.z;
					offMeshConnVerts[vertOffset+2] = vStart.y;
					offMeshConnVerts[vertOffset+3] = vEnd.x;
					offMeshConnVerts[vertOffset+4] = vEnd.z;
					offMeshConnVerts[vertOffset+5] = vEnd.y;

					float rad = 0.0f;
					if( spawnFlags & SF_OFFMESHCONN_HUMAN )
						TestAgentRadius( "human", rad );
					if( spawnFlags & SF_OFFMESHCONN_MEDIUM )
						TestAgentRadius( "medium", rad );
					if( spawnFlags & SF_OFFMESHCONN_LARGE )
						TestAgentRadius( "large", rad );
					if( spawnFlags & SF_OFFMESHCONN_VERYLARGE )
						TestAgentRadius( "verylarge", rad );
					if( spawnFlags & SF_OFFMESHCONN_AIR )
						TestAgentRadius( "air", rad );

					offMeshConnRad.AddToTail( rad );

					offMeshConnId.AddToTail( 10000 + offMeshConnId.Count() );

					if( spawnFlags & SF_OFFMESHCONN_JUMPEDGE )
					{
						offMeshConnArea.AddToTail( SAMPLE_POLYAREA_JUMP );
						offMeshConnFlags.AddToTail( SAMPLE_POLYFLAGS_JUMP );
						offMeshConnDir.AddToTail( 0 ); // one direction
					}
					else
					{
						offMeshConnArea.AddToTail( SAMPLE_POLYAREA_GROUND );
						offMeshConnFlags.AddToTail( SAMPLE_POLYFLAGS_WALK );
						offMeshConnDir.AddToTail( 1 ); // bi direction
					}

					//Msg("Parsed offmesh connection with rad %f, start %f %f %f, end %f %f %f\n", rad,
					//	vStart.x, vStart.y, vStart.z, vEnd.x, vEnd.y, vEnd.z);
				}
				else
				{
					Warning("Offmesh connection at (%f %f %f) has no valid target (%s)\n", 
						pOffMeshConn->GetAbsOrigin().x, pOffMeshConn->GetAbsOrigin().y, pOffMeshConn->GetAbsOrigin().z,
						pOffMeshConn->m_target != NULL_STRING ? STRING( pOffMeshConn->m_target ) : "<null>");
				}
			}
		}

		// Find next off mesh connection
		pEnt = gEntList.FindEntityByClassname( pEnt, "recast_offmesh_connection" );
	}

	params->offMeshConVerts = offMeshConnVerts.Base();
	params->offMeshConRad = offMeshConnRad.Base();
	params->offMeshConDir =  offMeshConnDir.Base();
	params->offMeshConAreas = offMeshConnArea.Base();
	params->offMeshConFlags = offMeshConnFlags.Base();
	params->offMeshConUserID = offMeshConnId.Base();
	params->offMeshConCount = offMeshConnId.Count();

	//DevMsg("Parsed %d offmesh connections\n", offMeshConnId.Count());
#endif // CLIENT_DLL
}