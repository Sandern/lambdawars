//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// Note: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast/recast_mesh.h"

#include <filesystem.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::LoadMapData()
{
	CUtlVector<float> verts;
	CUtlVector<int> triangles;

	// nav filename is derived from map filename
	char filename[256];
#ifdef CLIENT_DLL
	Q_snprintf( filename, sizeof( filename ), "%s", STRING( engine->GetLevelName() ) );
	V_StripExtension( filename, filename, 256 );
	Q_snprintf( filename, sizeof( filename ), "%s.bsp", filename );
	//Msg("CLIENT LOADING NAV MESH: %s\n", filename);
#else
	Q_snprintf( filename, sizeof( filename ), "maps\\%s.bsp", STRING( gpGlobals->mapname ) );
#endif // CLIENT_DLL

	void *fileBuffer = NULL;
	int length = filesystem->ReadFileEx( filename, "GAME", &fileBuffer, false, false );

#if 0
	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::READ_ONLY );
	if ( !filesystem->ReadFile( filename, "MOD", fileBuffer ) )	// this ignores .nav files embedded in the .bsp ...
	{
		Warning("Recast LoadMapData: unable to read bsp");
		return false;
	}
#endif // 0

	BSPHeader_t *header = (BSPHeader_t *)fileBuffer;

	// Load vertices
	dvertex_t *vertices = (dvertex_t *)((char *)fileBuffer + header->lumps[LUMP_VERTEXES].fileofs);
	int nvertsLump = (header->lumps[LUMP_VERTEXES].filelen) / sizeof(dvertex_t);
	//m_nverts = (header->lumps[LUMP_VERTEXES].filelen) / sizeof(dvertex_t);
	//m_verts = new float[3*m_nverts];
	for( int i = 0; i < nvertsLump; i++ )
	{
		verts.AddToTail( vertices[i].point[0] );
		verts.AddToTail( vertices[i].point[2] );
		verts.AddToTail( vertices[i].point[1] );
		//m_verts[i*3] = vertices[i].point[0];
		//m_verts[i*3+1] = vertices[i].point[2];
		//m_verts[i*3+2] = vertices[i].point[1];
		//Msg("vert %d: %f %f %f\n", i, m_verts[i*3], m_verts[i*3+1], m_verts[i*3+2]);
	}

	// Load edges and Surfedge array
	dedge_t *edges = (dedge_t *)((char *)fileBuffer + header->lumps[LUMP_EDGES].fileofs);
	//int nEdges = (header->lumps[LUMP_EDGES].filelen) / sizeof(dedge_t);

	int *surfedge = (int *)((char *)fileBuffer + header->lumps[LUMP_SURFEDGES].fileofs);
	//int nSurfEdges = (header->lumps[LUMP_SURFEDGES].filelen) / sizeof(int);

	// Load triangles, parse from faces
	dface_t *faces = (dface_t *)((char *)fileBuffer + header->lumps[LUMP_FACES].fileofs);
	int nFaces = (header->lumps[LUMP_FACES].filelen) / sizeof(dface_t);
	Msg("nFaces: %d\n", nFaces);
	for( int i = 0; i < nFaces; i++ )
	{
		float x = 0, y = 0, z = 0;
		for( int j = faces[i].firstedge; j < faces[i].firstedge+faces[i].numedges; j++ )
		{
			float *p1 = verts.Base() + (edges[abs(surfedge[j])].v[0] * 3);
			float *p2 = verts.Base() + (edges[abs(surfedge[j])].v[1] * 3);
			x += p1[0];
			y += p1[1];
			z += p1[2];
			x += p2[0];
			y += p2[1];
			z += p2[2];
		}

		x /= (float)(faces[i].numedges * 2);
		y /= (float)(faces[i].numedges * 2);
		z /= (float)(faces[i].numedges * 2);

		//Msg("Face %d origin: %f %f %f\n", i, x, y, z);

		int connectingVertIdx = verts.Count() / 3;
		verts.AddToTail( x );
		verts.AddToTail( y );
		verts.AddToTail( z );

		//Msg( "\tface: %d, firstedge: %d, numedges: %d\n", i, faces[i].firstedge, faces[i].numedges );
		for( int j = faces[i].firstedge; j < faces[i].firstedge+faces[i].numedges; j++ )
		{
			// Create triangle 1
			if( surfedge[j] < 0 )
			{
				triangles.AddToTail( edges[-surfedge[j]].v[1] );
				triangles.AddToTail( edges[-surfedge[j]].v[0] );
			}
			else
			{
				triangles.AddToTail( edges[surfedge[j]].v[0] );
				triangles.AddToTail( edges[surfedge[j]].v[1] );
			}
			triangles.AddToTail( connectingVertIdx );
		}
	}

	m_nverts = verts.Count() / 3;
	m_verts = new float[verts.Count()];
	V_memcpy( m_verts, verts.Base(), verts.Count() * sizeof(float) );

	m_ntris = triangles.Count() / 3;
	m_tris = new int[triangles.Count()];
	V_memcpy( m_tris, triangles.Base(), triangles.Count() * sizeof(int) );

	for( int i = 0; i < triangles.Count(); i++ )
	{
		if( triangles[i] < 0 || triangles[i] >= m_nverts )
		{
			Warning("Triangle %d has invalid vertex index: %d\n", i, triangles[i] );
		}
	}

	Msg( "Recast Load map data for %s: %d verts and %d tris (bsp size: %d, version: %d)\n", filename, m_nverts, m_ntris, length, header->m_nVersion );

	// Calculate normals.
	m_normals = new float[m_ntris*3];
	for (int i = 0; i < m_ntris*3; i += 3)
	{
		const float* v0 = &m_verts[m_tris[i]*3];
		const float* v1 = &m_verts[m_tris[i+1]*3];
		const float* v2 = &m_verts[m_tris[i+2]*3];
		float e0[3], e1[3];
		for (int j = 0; j < 3; ++j)
		{
			e0[j] = v1[j] - v0[j];
			e1[j] = v2[j] - v0[j];
		}
		float* n = &m_normals[i];
		n[0] = e0[1]*e1[2] - e0[2]*e1[1];
		n[1] = e0[2]*e1[0] - e0[0]*e1[2];
		n[2] = e0[0]*e1[1] - e0[1]*e1[0];
		float d = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
		if (d > 0)
		{
			d = 1.0f/d;
			n[0] *= d;
			n[1] *= d;
			n[2] *= d;
		}
	}

	return true;
}