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
#include "builddisp.h"
#include "gamebspfile.h"
#include "datacache/imdlcache.h"

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
	
    for( int i = 0; i < nFaces; i++ )
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
		if( nFaceIndex == 0xFFFF )
			continue;

		CCoreDispInfo coreDisp;
		CCoreDispSurface *pDispSurf = coreDisp.GetSurface();
		pDispSurf->SetPointStart( dispInfo.startPosition );
		pDispSurf->SetContents( dispInfo.contents );
	
		coreDisp.InitDispInfo( dispInfo.power, dispInfo.minTess, dispInfo.smoothingAngle, dispVerts + iCurVert, dispTri + iCurTri, 0, NULL );

		dface_t *pFaces = &faces[ nFaceIndex ];
		pDispSurf->SetHandle( nFaceIndex );

		if( pFaces->numedges > 4 )
			continue;

		Vector surfPoints[4];
		pDispSurf->SetPointCount( pFaces->numedges );
		int j;
		for( j = 0; j < pFaces->numedges; j++ )
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

		for( j = 0; j < 4; j++ )
		{
			pDispSurf->SetPoint( j, surfPoints[j] );
		}

		pDispSurf->FindSurfPointStartIndex();
		pDispSurf->AdjustSurfPointData();

		if( pDispSurf->GetPointCount() != 4 )
			continue;

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

		int i = 0;
		while( i + 2 < numIndices )
		{
			triangles.AddToTail( iStartingVertIndex + pIndex[ i ] );
			triangles.AddToTail( iStartingVertIndex + pIndex[ i + 1 ] );
			triangles.AddToTail( iStartingVertIndex + pIndex[ i + 2 ] );

			i += 3;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void LoadStaticProp( const char *pModelName, CUtlVector<float> &modelVerts, CUtlVector<int> &modelTris )
{
	MDLHandle_t h = mdlcache->FindMDL( pModelName );
	if( h == MDLHANDLE_INVALID )
	{
		Warning("LoadStaticProp: unable to get mdl handle for %s\n", pModelName);
		return;
	}

	vertexFileHeader_t *vertexData = mdlcache->GetVertexData( h );
	if( !vertexData )
	{
		Warning("LoadStaticProp: unable to get vertexData for %s\n", pModelName);
		return;
	}

	Msg( "Num vertices for %s: %d\n", pModelName, vertexData->numLODVertexes[0] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::GenerateStaticPropData( void *fileContent, CUtlVector<float> &verts, CUtlVector<int> &triangles )
{
	BSPHeader_t *header = (BSPHeader_t *)fileContent;

	// Find the static prop game lump
	dgamelump_t *staticPropLumpHeader = NULL;
	dgamelumpheader_t *gamelumpheader = (dgamelumpheader_t *)((char *)fileContent + header->lumps[LUMP_GAME_LUMP].fileofs);
	for( int i = 0; i < gamelumpheader->lumpCount; i++ )
	{
		dgamelump_t *gamelump = (dgamelump_t *)((char *)gamelumpheader + sizeof( dgamelumpheader_t ) + (i * sizeof( dgamelump_t )));
		if( gamelump->id == GAMELUMP_STATIC_PROPS )
		{
			if( gamelump->version == GAMELUMP_STATIC_PROPS_VERSION )
			{
				staticPropLumpHeader = gamelump;
			}
			else
			{
				Warning("CRecastMesh::GenerateStaticPropData: Found static prop lump with version %d, but expected %d!\n", gamelump->version, GAMELUMP_STATIC_PROPS_VERSION );
			}
			break;
		}
	}

	if( staticPropLumpHeader )
	{
		// Read models
		CUtlBuffer staticPropData( (void *)((char *)fileContent + staticPropLumpHeader->fileofs), staticPropLumpHeader->filelen, CUtlBuffer::READ_ONLY );
		int dictEntries = staticPropData.GetInt();
		Msg("Listening %d static prop dict entries\n", dictEntries);
		StaticPropDictLump_t staticPropDictLump;

		CUtlVector< CUtlVector< float > > modelVerts;
		CUtlVector< CUtlVector< int > > modelTris;
		modelVerts.EnsureCapacity( dictEntries );
		modelTris.EnsureCapacity( dictEntries );

		for( int i = 0; i < dictEntries; i++ )
		{
			staticPropData.Get( &staticPropDictLump, sizeof( staticPropDictLump ) );
			Msg("%d: %s\n", i, staticPropDictLump.m_Name);

			LoadStaticProp( staticPropDictLump.m_Name, modelVerts[i], modelTris[i] );
		}

		// Read static prop leafs
		int leafEntries = staticPropData.GetInt();
		StaticPropLeafLump_t staticPropLeaf;
		Msg("Listening %d static prop leaf entries\n", leafEntries);
		for( int i = 0; i < leafEntries; i++ )
		{
			staticPropData.Get( &staticPropLeaf, sizeof( staticPropLeaf ) );
		}

		// Read static props
		int staticPropEntries = staticPropData.GetInt();
		int propsWithCollision = 0;
		int propsWithCollisionBB = 0;
		StaticPropLump_t staticProp;
		for( int i = 0; i < staticPropEntries; i++ )
		{
			staticPropData.Get( &staticProp, sizeof( staticProp ) );

			matrix3x4_t rotationMatrix; // model to world transformation
			AngleMatrix( staticProp.m_Angles, staticProp.m_Origin, rotationMatrix);

			int j;
			//Vector worldPos;

			switch( staticProp.m_Solid )
			{
			case SOLID_VPHYSICS:
				propsWithCollision++;

				j = 0;
				while( j < modelTris[staticProp.m_PropType].Count() )
				{
					// Add transformed vertices
					//VectorTransform( vertPos, rotationMatrix, worldPos );

					// Add tris based on the vertices added

					j += 3;
				}
				break;
			case SOLID_BBOX:
				propsWithCollisionBB++;
				break;
			case SOLID_NONE:
				break;
			default:
				Warning("CRecastMesh::GenerateStaticPropData: Unexpected collision for static prop\n");
			}

			//Msg("%d: %f %f %f\n", i, staticProp.m_Origin.x, staticProp.m_Origin.y, staticProp.m_Origin.z);
		}

		Msg("Listened %d static prop entries, of which %d have vphysics, and %d bbox\n", staticPropEntries, propsWithCollision, propsWithCollisionBB);
	}
	else
	{
		Warning("CRecastMesh::GenerateStaticPropData: Could not find static prop lump!\n");
	}

	// Prop hulls?
#if 0
	dvertex_t *prophullverts = (dvertex_t *)((char *)fileContent + header->lumps[LUMP_PROPHULLVERTS].fileofs);
	int nprophullverts = (header->lumps[LUMP_PROPHULLVERTS].filelen) / sizeof(dvertex_t);

	//dprophull_t *prophull = (dprophull_t *)((char *)fileContent + header->lumps[LUMP_PROPHULLS].fileofs);
	int nprophull = (header->lumps[LUMP_PROPHULLS].filelen) / sizeof(dprophull_t);

	dprophulltris_t *prophulltris = (dprophulltris_t *)((char *)fileContent + header->lumps[LUMP_PROPTRIS].fileofs);
	int nprophulltris = (header->lumps[LUMP_PROPTRIS].filelen) / sizeof(dprophulltris_t);

	Msg("nprophullverts: %d, nprophull: %d, nprophulltris: %d\n", nprophullverts, nprophull, nprophulltris);

	// Add vertices
	int iStartingVertIndex = verts.Count() / 3;
	for( int i = 0; i < nprophullverts; i++ )
	{
		const Vector &v = prophullverts[i].point;
		verts.AddToTail( v[0] );
		verts.AddToTail( v[2] );
		verts.AddToTail( v[1] );
	}

	// Add triangles
	for( int i = 0; i < nprophulltris; i++ )
	{
		const dprophulltris_t &pht = prophulltris[i];

		int idx = pht.m_nIndexStart;
		while( idx < pht.m_nIndexCount )
		{
			triangles.AddToTail( iStartingVertIndex + idx );
			triangles.AddToTail( iStartingVertIndex + idx + 1 );
			triangles.AddToTail( iStartingVertIndex + idx + 2 );

			idx += 3;
		}
	}
#endif // 0

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
		/*else
		{
			// Displacement
		}*/
	}

	GenerateDispVertsAndTris( fileContent, verts, triangles );
	GenerateStaticPropData( fileContent, verts, triangles );

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