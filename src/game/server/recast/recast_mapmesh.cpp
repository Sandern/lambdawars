//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// Note: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast/recast_mapmesh.h"
#include "recast/recast_mesh.h"
#include "builddisp.h"
#include "gamebspfile.h"
//#include "datacache/imdlcache.h"
#include <filesystem.h>
#include "SkyCamera.h"

#include "ChunkyTriMesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar recast_mapmesh_loadbrushes("recast_mapmesh_loadbrushes", "1");
static ConVar recast_mapmesh_loadstaticprops("recast_mapmesh_loadstaticprops", "1");
static ConVar recast_mapmesh_loaddynamicprops("recast_mapmesh_loaddynamicprops", "1");
static ConVar recast_mapmesh_loaddisplacements("recast_mapmesh_loaddisplacements", "1");

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapMesh::CMapMesh() : m_chunkyMesh(0)
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapMesh::~CMapMesh()
{
	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: Clears loaded map data
//-----------------------------------------------------------------------------
void CMapMesh::Clear()
{
	m_Vertices.Purge();
	m_Triangles.Purge();
	m_Normals.Purge();

	if( m_chunkyMesh )
	{
		delete m_chunkyMesh;
		m_chunkyMesh = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Test if triangle is in valid area. Area should exist and 
//			not be skybox.
//-----------------------------------------------------------------------------
#define SURFACE_TOOLSNODRAW "TOOLS/TOOLSNODRAW"
#define SURFACE_TOOLSBLACK "TOOLS/TOOLSBLACK"
bool CMapMesh::IsTriangleInValidArea( const Vector *vTriangle )
{
	Vector vCenter, vNormal;
	vCenter = (vTriangle[0] + vTriangle[1] + vTriangle[2]) / 3.0f;
	vNormal = CrossProduct( vTriangle[1] - vTriangle[0], vTriangle[2] - vTriangle[0] );
	VectorNormalize( vNormal );

	// Should have an area in the direction the triangle is facing
	// Move 2 units into the area, which usually seems enought to get the proper area
	int area = engine->GetArea( vCenter + (vNormal * 2.0f) );
	if( area == 0 )
		return false;

	// The area should not be connected to a skybox entity
	CSkyCamera *pCur = GetSkyCameraList();
	while ( pCur )
	{
		if ( engine->CheckAreasConnected( area, pCur->m_skyboxData.area ) )
		{
			return false;
		}

		pCur = pCur->m_pNext;
	}

	// Perform trace to get the surface property
	trace_t tr;
	UTIL_TraceLine( vCenter + (-vNormal*1.0f), vCenter + (vNormal*1.0f), MASK_NPCWORLDSTATIC, NULL, COLLISION_GROUP_NONE, &tr );
	if( tr.DidHit() )
	{
		// Can't be in solid
		if( tr.startsolid || tr.allsolid )
			return false;

		// Don't care about sky
		if( tr.surface.flags & (SURF_SKY|SURF_SKY2D) )
			return false;

		// Filter triangles with certain materials
		const surfacedata_t *psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );
		if( psurf )
		{
			//Msg("Surface prop tri: %s, game.material: %c\n", tr.surface.name, psurf->game.material);
			
			if( V_strncmp( tr.surface.name, SURFACE_TOOLSNODRAW, sizeof( SURFACE_TOOLSNODRAW ) ) == 0 )
				return false;
			if( V_strncmp( tr.surface.name, SURFACE_TOOLSBLACK, sizeof( SURFACE_TOOLSBLACK ) ) == 0 )
				return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Load displacment verts and triangles
//-----------------------------------------------------------------------------
bool CMapMesh::GenerateDispVertsAndTris( void *fileContent, CUtlVector<float> &verts, CUtlVector<int> &triangles )
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
	int i;
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
		for (i = 0; i < numVerts; ++i )
		{
			const Vector &dispVert = pVert[i].m_Vert;
			verts.AddToTail( dispVert[0] );
			verts.AddToTail( dispVert[2] );
			verts.AddToTail( dispVert[1] );
		}

		unsigned short *pIndex = coreDisp.GetRenderIndexList();
		int numIndices = coreDisp.GetRenderIndexCount();
		
		Vector trisVerts[3];
		i = 0;
		while( i + 2 < numIndices )
		{
			for( int k = 0; k < 3; k++ )
			{
				int idx = iStartingVertIndex + pIndex[ i + k ];
				const float *vert = &verts[idx*3];
				for( int l = 0; l < 3; l++ )
				{
					trisVerts[l].Init( vert[0], vert[2], vert[1] );
				}
			}

			if( !IsTriangleInValidArea( trisVerts ) )
			{
				i += 3;
				continue;
			}

			triangles.AddToTail( iStartingVertIndex + pIndex[ i ] );
			triangles.AddToTail( iStartingVertIndex + pIndex[ i + 1 ] );
			triangles.AddToTail( iStartingVertIndex + pIndex[ i + 2 ] );

			i += 3;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: get model vcollide_t
//-----------------------------------------------------------------------------
vcollide_t *LoadModelPhysCollide( const char *pModelName )
{
	const model_t *pModel = modelinfo->FindOrLoadModel( pModelName );
	if( !pModel )
	{
		Warning("LoadModelPhysCollide: unable to load model %s\n", pModelName);
		return NULL;
	}

	return modelinfo->GetVCollide( pModel );
}

//-----------------------------------------------------------------------------
// Purpose: Add vertices and triangles of a collision model to mesh
//-----------------------------------------------------------------------------
void CMapMesh::AddCollisionModelToMesh( const matrix3x4_t &transform, CPhysCollide const *pCollisionModel, 
	CUtlVector<float> &verts, CUtlVector<int> &triangles, int filterContents )
{
	ICollisionQuery *pCollisionQuery = physcollision->CreateQueryModel( (CPhysCollide *)pCollisionModel );
	if( !pCollisionQuery )
	{
		Warning("AddCollisionModelToMesh: could not create collision query model\n");
		return;
	}

	Vector trisVerts[3];
	Vector vCenter;

	for( int i = 0; i < pCollisionQuery->ConvexCount(); i++ )
	{
		int nTris = pCollisionQuery->TriangleCount( i );
		for( int j = 0; j < nTris; j++ )
		{
			pCollisionQuery->GetTriangleVerts( i, j, trisVerts );

			if( !IsTriangleInValidArea( trisVerts ) )
				continue;

			if( filterContents != CONTENTS_EMPTY )
			{
				// UGLY! used for filtering out water of the world collidable.
				// Preferable we should just have some way to get the material.
				vCenter = (trisVerts[0] + trisVerts[1] + trisVerts[2]) / 3.0f;
				Vector offset(0, 0, 2.0f);
				if( enginetrace->GetPointContents_WorldOnly(vCenter-offset, filterContents) & filterContents &&
					(enginetrace->GetPointContents_WorldOnly(vCenter+offset, filterContents) & filterContents) == 0 )
				{
					continue;
				}
			}

			int iStartingVertIndex = verts.Count() / 3;
			for ( int k = 0; k < 3; k++ )
			{
				Vector out;
				VectorTransform( trisVerts[k].Base(), transform, out.Base() );
				verts.AddToTail( out[0] );
				verts.AddToTail( out[2] );
				verts.AddToTail( out[1] );
			}

			triangles.AddToTail( iStartingVertIndex );
			triangles.AddToTail( iStartingVertIndex + 1 );
			triangles.AddToTail( iStartingVertIndex + 2 );
		}
	}

	physcollision->DestroyQueryModel( pCollisionQuery );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMapMesh::GenerateStaticPropData( void *fileContent, CUtlVector<float> &verts, CUtlVector<int> &triangles )
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

		CUtlVector<vcollide_t *> modelsVCollides;
		modelsVCollides.EnsureCapacity( dictEntries );

		for( int i = 0; i < dictEntries; i++ )
		{
			staticPropData.Get( &staticPropDictLump, sizeof( staticPropDictLump ) );
			Msg("%d: %s\n", i, staticPropDictLump.m_Name);

			vcollide_t *vcollide = LoadModelPhysCollide( staticPropDictLump.m_Name );
			if( !vcollide )
			{
				modelsVCollides.AddToTail( NULL );
				continue;
			}
			modelsVCollides.AddToTail( vcollide );
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

			vcollide_t *vcollide = modelsVCollides[staticProp.m_PropType];

			matrix3x4_t transform; // model to world transformation
			AngleMatrix( staticProp.m_Angles, staticProp.m_Origin, transform);

			int j;

			switch( staticProp.m_Solid )
			{
			case SOLID_VPHYSICS:
				propsWithCollision++;

				if( vcollide )
				{
					for( j = 0; j < vcollide->solidCount; j++ )
					{
						AddCollisionModelToMesh( transform,  
							modelsVCollides[staticProp.m_PropType]->solids[j], verts, triangles );
					}
				}
				break;
			case SOLID_BBOX:
				propsWithCollisionBB++;
				// TODO?
				Warning("CMapMesh::GenerateStaticPropData: bbox for static props is not implemented yet.\n");
				break;
			case SOLID_NONE:
				break;
			default:
				Warning("CMapMesh::GenerateStaticPropData: Unexpected collision for static prop\n");
			}

			//Msg("%d: %f %f %f\n", i, staticProp.m_Origin.x, staticProp.m_Origin.y, staticProp.m_Origin.z);
		}

		Msg("Listened %d static prop entries, of which %d have vphysics, and %d bbox\n", staticPropEntries, propsWithCollision, propsWithCollisionBB);
	}
	else
	{
		Warning("CMapMesh::GenerateStaticPropData: Could not find static prop lump!\n");
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMapMesh::GenerateDynamicPropData( CUtlVector<float> &verts, CUtlVector<int> &triangles )
{
	for( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity != NULL; pEntity = gEntList.NextEnt( pEntity ) )
	{
		if( FClassnameIs( pEntity, "prop_dynamic" ) )
		{
			if( pEntity->IsSolidFlagSet(FSOLID_NOT_SOLID) )
				continue;

			//DevMsg("Checking prop dynamic: %s, parent: %s\n", STRING( pEntity->GetEntityName() ), STRING( pEntity->m_iParent ) );

			// Consider named or parented dynamic props to be really dynamic. They should either
			// be an obstacle on the mesh, or have a nav_blocker entity around them.
			if( pEntity->GetEntityName() != NULL_STRING || pEntity->m_iParent != NULL_STRING || pEntity->GetMoveParent() != NULL )
				continue;

			//DevMsg("\tAdding dynamic prop to map mesh\n");

			if( pEntity->GetSolid() == SOLID_VPHYSICS )
			{
				matrix3x4_t transform; // model to world transformation
				AngleMatrix( pEntity->GetAbsAngles(), pEntity->GetAbsOrigin(), transform);

				IPhysicsObject *pPhysObj = pEntity->VPhysicsGetObject();
				if( pPhysObj )
				{
					AddCollisionModelToMesh( transform, pPhysObj->GetCollide(), verts, triangles );
				}
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static int CalcBrushContents( void *fileContent, int nodeidx )
{
	int contents = 0;

	BSPHeader_t *header = (BSPHeader_t *)fileContent;
	dleaf_t *leafs = (dleaf_t *)((char *)fileContent + header->lumps[LUMP_LEAFS].fileofs);
	unsigned short *leafbrushes = (unsigned short *)((char *)fileContent + header->lumps[LUMP_LEAFBRUSHES].fileofs);
	dnode_t *nodes = (dnode_t *)((char *)fileContent + header->lumps[LUMP_NODES].fileofs);
	dbrush_t *brushes = (dbrush_t *)((char *)fileContent + header->lumps[LUMP_BRUSHES].fileofs);

	// Keep going until terminated by leafs.
	// Each node has exactly two children, which can be either another node or a leaf. 
	// A child node has two further children, and so on until all branches of the tree are terminated with leaves, which have no children.
	// https://developer.valvesoftware.com/wiki/Source_BSP_File_Format
	while( true )
	{
		if( nodeidx < 0 )
		{
			// negative numbers are -(leafs+1), not nodes
			int leafidx = -(nodeidx+1);
			dleaf_t &leaf = leafs[leafidx];
			
			for( int i = 0; i < leaf.numleafbrushes; i++ )
			{
				contents |= brushes[leafbrushes[leaf.firstleafbrush+i]].contents;
			}

			return contents;
		}

		dnode_t &node = nodes[nodeidx];
		contents |= CalcBrushContents( fileContent, node.children[0] );
		nodeidx = node.children[1];
	}

	return contents;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMapMesh::GenerateBrushData( void *fileContent, CUtlVector<float> &verts, CUtlVector<int> &triangles )
{
	BSPHeader_t *header = (BSPHeader_t *)fileContent;

	// Load Brush Models
	CUtlVector< vcollide_t > parsedphysmodels;

	dmodel_t *brushmodels = (dmodel_t *)((char *)fileContent + header->lumps[LUMP_MODELS].fileofs);
	int nbrushmodels = (header->lumps[LUMP_MODELS].filelen) / sizeof(dmodel_t);
	parsedphysmodels.EnsureCount( nbrushmodels );

	// Load PhysModel data
	byte *physmodels = (byte *)((char *)fileContent + header->lumps[LUMP_PHYSCOLLIDE].fileofs);
	byte *basephysmodels = physmodels;
	int nphysmodelsize = header->lumps[LUMP_PHYSCOLLIDE].filelen;

	dphysmodel_t physModel;
	// Variable length, etc. Last is null.
	do
	{
		V_memcpy( &physModel, physmodels, sizeof(physModel) );
		physmodels += sizeof(physModel);

		if( physModel.dataSize > 0 )
		{
			physcollision->VCollideLoad( &parsedphysmodels[ physModel.modelIndex ], physModel.solidCount, (const char *)physmodels, physModel.dataSize + physModel.keydataSize );
			physmodels += physModel.dataSize;
			physmodels += physModel.keydataSize;
		}
		
		if( (int)(physmodels - basephysmodels) > nphysmodelsize )
			break;

	} while ( physModel.dataSize > 0 );

	for( int i = 0; i < nbrushmodels; i++ )
	{
		dmodel_t *pModel = &brushmodels[ i ];

		int contents = CalcBrushContents( fileContent, pModel->headnode );

		// Only parse the first brush (world) and clips
		if( i != 0 /*&& (contents & (CONTENTS_MONSTERCLIP)) == 0*/ )
		{
			continue;
		}

		matrix3x4_t transform; // model to world transformation
		AngleMatrix( vec3_angle, pModel->origin, transform);

		DevMsg( "Brush %d: solid count %d, contents: %d\n", i, parsedphysmodels[i].solidCount, contents );
		for( int j = 0; j < parsedphysmodels[i].solidCount; j++ )
		{
			AddCollisionModelToMesh( transform, parsedphysmodels[i].solids[j], verts, triangles, CONTENTS_WATER );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMapMesh::Load()
{
	Clear();

	// nav filename is derived from map filename
	char filename[256];
	V_snprintf( filename, sizeof( filename ), "maps\\%s.bsp", STRING( gpGlobals->mapname ) );

	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::READ_ONLY );
	if ( !filesystem->ReadFile( filename, "MOD", fileBuffer ) )	// this ignores .nav files embedded in the .bsp ...
	{
		Warning("Recast LoadMapData: unable to read bsp");
		return false;
	}

	int length = fileBuffer.TellMaxPut();
	void *fileContent = fileBuffer.Base();

	BSPHeader_t *header = (BSPHeader_t *)fileContent;

#if 0
	// Load vertices
	dvertex_t *vertices = (dvertex_t *)((char *)fileContent + header->lumps[LUMP_VERTEXES].fileofs);
	int nvertsLump = (header->lumps[LUMP_VERTEXES].filelen) / sizeof(dvertex_t);
	for( int i = 0; i < nvertsLump; i++ )
	{
		m_Vertices.AddToTail( vertices[i].point[0] );
		m_Vertices.AddToTail( vertices[i].point[2] );
		m_Vertices.AddToTail( vertices[i].point[1] );
	}

	// Load edges and Surfedge array
	//dedge_t *edges = (dedge_t *)((char *)fileContent + header->lumps[LUMP_EDGES].fileofs);
	//int nEdges = (header->lumps[LUMP_EDGES].filelen) / sizeof(dedge_t);
	//int *surfedge = (int *)((char *)fileContent + header->lumps[LUMP_SURFEDGES].fileofs);
	//int nSurfEdges = (header->lumps[LUMP_SURFEDGES].filelen) / sizeof(int);

	// Load texinfo
	//texinfo_s *texinfo = (texinfo_s *)((char *)fileContent + header->lumps[LUMP_TEXINFO].fileofs);
	//int nTexInfo = (header->lumps[LUMP_TEXINFO].filelen) / sizeof(texinfo_s);

	// Load triangles, parse from faces
	dface_t *faces = (dface_t *)((char *)fileContent + header->lumps[LUMP_FACES].fileofs);
	int nFaces = (header->lumps[LUMP_FACES].filelen) / sizeof(dface_t);
	for( int i = 0; i < nFaces; i++ )
	{
		if ( faces[i].dispinfo == -1 )
		{
			// Test texture info
			if( faces[i].texinfo > 0 && faces[i].texinfo < nTexInfo )
			{
				texinfo_s &faceTexinfo = texinfo[faces[i].texinfo];
				if( faceTexinfo.flags & (SURF_SKY2D|SURF_SKY|SURF_TRIGGER|SURF_HINT|SURF_SKIP) )
				{
					continue;
				}
			}

			// Create vertex at face origin
			float x = 0, y = 0, z = 0;
			for( int j = faces[i].firstedge; j < faces[i].firstedge+faces[i].numedges; j++ )
			{
				float *p1 = m_Vertices.Base() + (edges[abs(surfedge[j])].v[0] * 3);
				float *p2 = m_Vertices.Base() + (edges[abs(surfedge[j])].v[1] * 3);
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

			int connectingVertIdx = m_Vertices.Count() / 3;
			m_Vertices.AddToTail( x );
			m_Vertices.AddToTail( y );
			m_Vertices.AddToTail( z );

			//Msg( "\tface: %d, firstedge: %d, numedges: %d\n", i, faces[i].firstedge, faces[i].numedges );
			// Turn the face into a set of triangles
			for( int j = faces[i].firstedge; j < faces[i].firstedge+faces[i].numedges; j++ )
			{
				if( surfedge[j] < 0 )
				{
					m_Triangles.AddToTail( edges[-surfedge[j]].v[1] );
					m_Triangles.AddToTail( edges[-surfedge[j]].v[0] );
				}
				else
				{
					m_Triangles.AddToTail( edges[surfedge[j]].v[0] );
					m_Triangles.AddToTail( edges[surfedge[j]].v[1] );
				}
				m_Triangles.AddToTail( connectingVertIdx );
			}

		}
	}
#endif // 0

	if( recast_mapmesh_loaddisplacements.GetBool() )
		GenerateDispVertsAndTris( fileContent, m_Vertices, m_Triangles );
	if( recast_mapmesh_loadstaticprops.GetBool() )
		GenerateStaticPropData( fileContent, m_Vertices, m_Triangles );
	if( recast_mapmesh_loaddynamicprops.GetBool() )
		GenerateDynamicPropData( m_Vertices, m_Triangles );
	if( recast_mapmesh_loadbrushes.GetBool() )
		GenerateBrushData( fileContent, m_Vertices, m_Triangles );

#if defined(_DEBUG)
	for( int i = 0; i < m_Triangles.Count(); i++ )
	{
		if( m_Triangles[i] < 0 || m_Triangles[i] >= GetNumVerts() )
		{
			Warning("Triangle %d has invalid vertex index: %d\n", i, m_Triangles[i] );
		}
	}
#endif // _DEBUG

	Msg( "Recast Load map data for %s: %d verts and %d tris (bsp size: %d, version: %d)\n", filename, GetNumVerts(), GetNumTris(), length, header->m_nVersion );

	// Calculate normals.
	float *verts = m_Vertices.Base();
	m_Normals.EnsureCapacity( GetNumTris()*3 );
	for (int i = 0; i < GetNumTris()*3; i += 3)
	{
		const float* v0 = verts + m_Triangles[i]*3;
		const float* v1 = verts + m_Triangles[i+1]*3;
		const float* v2 = verts + m_Triangles[i+2]*3;
		float e0[3], e1[3];
		for (int j = 0; j < 3; ++j)
		{
			e0[j] = v1[j] - v0[j];
			e1[j] = v2[j] - v0[j];
		}
		float* n = m_Normals.Base() + i;
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

	// Chunky mesh
	m_chunkyMesh = new rcChunkyTriMesh;
	if (!m_chunkyMesh)
	{
		Warning("buildTiledNavigation: Out of memory 'm_chunkyMesh'.\n");
		//ctx->log(RC_LOG_ERROR, "buildTiledNavigation: Out of memory 'm_chunkyMesh'.");
		return false;
	}
	if (!rcCreateChunkyTriMesh(GetVerts(), GetTris(), GetNumTris(), 256, m_chunkyMesh))
	{
		Warning("buildTiledNavigation: Failed to build chunky mesh.\n");
		//ctx->log(RC_LOG_ERROR, "buildTiledNavigation: Failed to build chunky mesh.");
		return false;
	}	

	return true;
}