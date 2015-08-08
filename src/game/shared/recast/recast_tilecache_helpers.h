//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_TILECACHE_HELPER_H
#define RECAST_TILECACHE_HELPER_H

#ifdef _WIN32
#pragma once
#endif

#include "detourtilecache/DetourTileCache.h"
#include "detourtilecache/DetourTileCacheBuilder.h"

class CMapMesh;

struct FastLZCompressor : public dtTileCacheCompressor
{
	virtual int maxCompressedSize(const int bufferSize);
	
	virtual dtStatus compress(const unsigned char* buffer, const int bufferSize,
							  unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize);
	
	virtual dtStatus decompress(const unsigned char* compressed, const int compressedSize,
								unsigned char* buffer, const int maxBufferSize, int* bufferSize);
};

struct LinearAllocator : public dtTileCacheAlloc
{
	unsigned char* buffer;
	int capacity;
	int top;
	int high;
	
	LinearAllocator(const int cap);
	
	~LinearAllocator();

	void resize(const int cap);
	
	virtual void reset();
	
	virtual void* alloc(const int size);
	
	virtual void free(void* /*ptr*/);
};

class COffMeshConnection;

struct MeshProcess : public dtTileCacheMeshProcess
{
	MeshProcess( const char *meshName );

	virtual void process(struct dtNavMeshCreateParams* params,
						 unsigned char* polyAreas, unsigned short* polyFlags);

private:
	void parseAll();

#ifndef CLIENT_DLL
	void parseConnection( COffMeshConnection *pOffMeshConn );
#endif // CLIENT_DLL

private:
	int meshFlags;

	CUtlVector< float > offMeshConnVerts;
	CUtlVector< float > offMeshConnRad;
	CUtlVector< unsigned char > offMeshConnDir;
	CUtlVector< unsigned char > offMeshConnArea;
	CUtlVector< unsigned short > offMeshConnFlags;
	CUtlVector< unsigned int > offMeshConnId;
};

#endif // RECAST_TILECACHE_HELPER_H
