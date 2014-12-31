//====== Copyright � Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// Note: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast/recast_mesh.h"
#include "builddisp.h"

#include <filesystem.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Load displacment verts and triangles
//-----------------------------------------------------------------------------
bool CRecastMesh::GenerateDispVertsAndTris( void *fileContent, CUtlVector<float> &verts, CUtlVector<int> &triangles )
{
	BSPHeader_t *header = (BSPHeader_t *)fileContent;

	dvertex_t *vertices = (dvertex_t *)((char *)fileContent + header->lumps[LUMP_VERTEXES].fileofs);
	dedge_t *edges = (dedge_t *)((char *)fileContent + header->lumps[LUMP_EDGES].fileofs);
	int *surfedge = (int *)((char *)fileContent + header->lumps[LUMP_SURFEDGES].fileofs);
	dface_t *faces = (dface_t *)((char *)fileContent + header->lumps[LUMP_FACES].fileofs);
	int nFaces = (header->lumps[LUMP_FACES].filelen) / sizeof(dface_t);

	ddispinfo_t *dispInfoArray = (ddispinfo_t *)((char *)fileContent + header->lumps[LUMP_DISPINFO].fileofs);
	int nDispInfo = (header->lumps[LUMP_DISPINFO].filelen) / sizeof(ddispinfo_t);
	CDispVert *dispVerts = (CDispVert *)((char *)fileContent + header->lumps[LUMP_DISP_VERTS].fileofs);
	CDispTri *dispTri = (CDispTri *)((char *)fileContent + header->lumps[LUMP_DISP_TRIS].fileofs);

	// Build mapping from index to face
	int nMemSize = nFaces * sizeof(unsigned short);
	unsigned short *dispIndexToFaceIndex = (unsigned short*)stackalloc( nMemSize );
	V_memset( dispIndexToFaceIndex, 0xFF, nMemSize );
	
    for ( int i = 0; i < nFaces; i++ )
    {
        if ( faces[i].dispinfo == -1 || faces[i].dispinfo >= nDispInfo )
            continue;

		dispIndexToFaceIndex[faces[i].dispinfo] = (unsigned short)i;
    }

	int iCurVert = 0;
	int iCurTri = 0;
	for( int dispInfoIdx = 0; dispInfoIdx < nDispInfo; dispInfoIdx++ )
	{
		ddispinfo_t &dispInfo = dispInfoArray[dispInfoIdx];
		int nVerts = NUM_DISP_POWER_VERTS( dispInfo.power );
		int nTris = NUM_DISP_POWER_TRIS( dispInfo.power );

		// Find the face of this dispinfo
		unsigned short nFaceIndex = dispIndexToFaceIndex[dispInfoIdx];
		if ( nFaceIndex == 0xFFFF )
			continue;

		CCoreDispInfo coreDisp;
		CCoreDispSurface *pDispSurf = coreDisp.GetSurface();
		pDispSurf->SetPointStart( dispInfo.startPosition );
		pDispSurf->SetContents( dispInfo.contents );
	
		coreDisp.InitDispInfo( dispInfo.power, dispInfo.minTess, dispInfo.smoothingAngle, dispVerts + iCurVert, dispTri + iCurTri, 0, NULL );

		dface_t *pFaces = &faces[ nFaceIndex ];
		pDispSurf->SetHandle( nFaceIndex );

		if ( pFaces->numedges > 4 )
			continue;

		Vector surfPoints[4];
		pDispSurf->SetPointCount( pFaces->numedges );
		int j;
		for ( j = 0; j < pFaces->numedges; j++ )
		{
			int eIndex = surfedge[pFaces->firstedge+j];
			if ( eIndex < 0 )
			{
				VectorCopy( vertices[edges[-eIndex].v[1]].point, surfPoints[j] );
			}
			else
			{
				VectorCopy( vertices[edges[eIndex].v[0]].point, surfPoints[j] );
			}
		}

		for ( j = 0; j < 4; j++ )
		{
			pDispSurf->SetPoint( j, surfPoints[j] );
		}

		pDispSurf->FindSurfPointStartIndex();
		pDispSurf->AdjustSurfPointData();

		// Create the displacement from the set data
		coreDisp.Create();

		iCurVert += nVerts;
		iCurTri += nTris;

		// Create geometry for recast navigation mesh this data
		int iStartingVertIndex = verts.Count() / 3;

		int numVerts = coreDisp.GetSize();
		CoreDispVert_t *pVert = coreDisp.GetDispVertList();
		for (int i = 0; i < numVerts; ++i )
		{
			const Vector &dispVert = pVert[i].m_Vert;
			verts.AddToTail( dispVert[0] );
			verts.AddToTail( dispVert[2] );
			verts.AddToTail( dispVert[1] );
		}

		unsigned short *pIndex = coreDisp.GetRenderIndexList();
		int numIndices = coreDisp.GetRenderIndexCount();
		for ( int i = 0; i < numIndices - 2; ++i )
		{
			triangles.AddToTail( iStartingVertIndex + pIndex[ i ] );
			triangles.AddToTail( iStartingVertIndex + pIndex[ i + 1 ] );
			triangles.AddToTail( iStartingVertIndex + pIndex[ i + 2 ] );
		}
	}

	return true;
}

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
	V_snprintf( filename, sizeof( filename ), "%s", STRING( engine->GetLevelName() ) );
	V_StripExtension( filename, filename, 256 );
	V_snprintf( filename, sizeof( filename ), "%s.bsp", filename );
	//Msg("CLIENT LOADING NAV MESH: %s\n", filename);
#else
	V_snprintf( filename, sizeof( filename ), "maps\\%s.bsp", STRING( gpGlobals->mapname ) );
#endif // CLIENT_DLL

	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::READ_ONLY );
	if ( !filesystem->ReadFile( filename, "MOD", fileBuffer ) )	// this ignores .nav files embedded in the .bsp ...
	{
		Warning("Recast LoadMapData: unable to read bsp");
		return false;
	}

	int length = fileBuffer.TellMaxPut();
	void *fileContent = fileBuffer.Base();

	BSPHeader_t *header = (BSPHeader_t *)fileContent;

	// Load vertices
	dvertex_t *vertices = (dvertex_t *)((char *)fileContent + header->lumps[LUMP_VERTEXES].fileofs);
	int nvertsLump = (header->lumps[LUMP_VERTEXES].filelen) / sizeof(dvertex_t);
	for( int i = 0; i < nvertsLump; i++ )
	{
		verts.AddToTail( vertices[i].point[0] );
		verts.AddToTail( vertices[i].point[2] );
		verts.AddToTail( vertices[i].point[1] );
	}

	// Load edges and Surfedge array
	dedge_t *edges = (dedge_t *)((char *)fileContent + header->lumps[LUMP_EDGES].fileofs);
	//int nEdges = (header->lumps[LUMP_EDGES].filelen) / sizeof(dedge_t);
	int *surfedge = (int *)((char *)fileContent + header->lumps[LUMP_SURFEDGES].fileofs);
	//int nSurfEdges = (header->lumps[LUMP_SURFEDGES].filelen) / sizeof(int);

	// Load triangles, parse from faces
	dface_t *faces = (dface_t *)((char *)fileContent + header->lumps[LUMP_FACES].fileofs);
	int nFaces = (header->lumps[LUMP_FACES].filelen) / sizeof(dface_t);
	Msg("nFaces: %d\n", nFaces);
	for( int i = 0; i < nFaces; i++ )
	{
		if ( faces[i].dispinfo == -1 )
		{
			// Create vertex at face origin
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

			int connectingVertIdx = verts.Count() / 3;
			verts.AddToTail( x );
			verts.AddToTail( y );
			verts.AddToTail( z );

			//Msg( "\tface: %d, firstedge: %d, numedges: %d\n", i, faces[i].firstedge, faces[i].numedges );
			// Turn the face into a set of triangles
			for( int j = faces[i].firstedge; j < faces[i].firstedge+faces[i].numedges; j++ )
			{
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
		else
		{
			// Displacement
		}
	}

	GenerateDispVertsAndTris( fileContent, verts, triangles );
	
	// Copy result
	m_nverts = verts.Count() / 3;
	m_verts = new float[verts.Count()];
	V_memcpy( m_verts, verts.Base(), verts.Count() * sizeof(float) );

	m_ntris = triangles.Count() / 3;
	m_tris = new int[triangles.Count()];
	V_memcpy( m_tris, triangles.Base(), triangles.Count() * sizeof(int) );

#if defined(_DEBUG)
	for( int i = 0; i < triangles.Count(); i++ )
	{
		if( triangles[i] < 0 || triangles[i] >= m_nverts )
		{
			Warning("Triangle %d has invalid vertex index: %d\n", i, triangles[i] );
		}
	}
#endif // _DEBUG

	Msg( "Recast Load map data for %s: %d verts and %d tris (bsp size: %d, version: %d)\n", filename, m_nverts, m_ntris, length, header->m_nVersion );

#if defined(_DEBUG) || defined(CALC_GEOM_NORMALS)
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
#endif // CALC_GEOM_NORMALS

	return true;
}