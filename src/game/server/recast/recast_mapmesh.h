//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_MAPMESH_H
#define RECAST_MAPMESH_H

#ifdef _WIN32
#pragma once
#endif

struct rcChunkyTriMesh;

class CMapMesh
{
public:
	CMapMesh();
	~CMapMesh();

	bool Load( bool offMeshConnectionsOnly = false );
	void Clear();

	const float *GetVerts();
	int GetNumVerts();
	const int *GetTris();
	int GetNumTris();
	const float *GetNorms();
	const rcChunkyTriMesh *GetChunkyMesh();

	CUtlVector< float > &GetOffMeshConnVerts() { return m_OffMeshConnVerts; }
	CUtlVector< float > &GetOffMeshConnRad() { return m_OffMeshConnRad; }
	CUtlVector< unsigned char > &GetOffMeshConnDir() { return m_OffMeshConnDir; }
	CUtlVector< unsigned char > &GetOffMeshConnArea() { return m_OffMeshConnArea; }
	CUtlVector< unsigned short > &GetOffMeshConnFlags() { return m_OffMeshConnFlags; }
	CUtlVector< unsigned int > &GetOffMeshConnId() { return m_OffMeshConnId; }
	int GetOffMeshConnCount() { return m_OffMeshConnId.Count(); }

private:
	virtual bool GenerateDispVertsAndTris( void *fileContent, CUtlVector<float> &verts, CUtlVector<int> &triangles );
	virtual bool GenerateStaticPropData( void *fileContent, CUtlVector<float> &verts, CUtlVector<int> &triangles );
	virtual bool GenerateDynamicPropData( CUtlVector<float> &verts, CUtlVector<int> &triangles );
	virtual bool GenerateBrushData( void *fileContent, CUtlVector<float> &verts, CUtlVector<int> &triangles );
	virtual bool GenerateOffMeshConnections();

private:
	CUtlVector< float > m_Vertices;
	CUtlVector< int > m_Triangles; // Indices into m_Vertices
	CUtlVector< float > m_Normals;

	rcChunkyTriMesh* m_chunkyMesh;

	// Offmesh connections
	CUtlVector< float > m_OffMeshConnVerts;
	CUtlVector< float > m_OffMeshConnRad;
	CUtlVector< unsigned char > m_OffMeshConnDir;
	CUtlVector< unsigned char > m_OffMeshConnArea;
	CUtlVector< unsigned short > m_OffMeshConnFlags;
	CUtlVector< unsigned int > m_OffMeshConnId;
};

inline const float *CMapMesh::GetVerts()
{
	return m_Vertices.Base();
}
inline int CMapMesh::GetNumVerts()
{
	return m_Vertices.Count() / 3;
}

inline const int *CMapMesh::GetTris()
{
	return m_Triangles.Base();
}
inline int CMapMesh::GetNumTris()
{
	return m_Triangles.Count() / 3;
}

inline const float *CMapMesh::GetNorms()
{
	return m_Normals.Base();
}

inline const rcChunkyTriMesh *CMapMesh::GetChunkyMesh()
{
	return m_chunkyMesh;
}

#endif // RECAST_MAPMESH_H