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

#include "recast/recast_imapmesh.h"

class CMapMesh : public IMapMesh
{
public:
	CMapMesh();
	~CMapMesh();

	bool Load();
	void Clear();

	const float *GetVerts();
	int GetNumVerts();
	const int *GetTris();
	int GetNumTris();
	const float *GetNorms();
	const rcChunkyTriMesh *GetChunkyMesh();

private:
	bool IsTriangleInValidArea( const Vector *vTriangle );
	void AddCollisionModelToMesh( const matrix3x4_t &transform, CPhysCollide const *pCollisionModel, 
			CUtlVector<float> &verts, CUtlVector<int> &triangles, int filterContents = CONTENTS_EMPTY );

	virtual bool GenerateDispVertsAndTris( void *fileContent, CUtlVector<float> &verts, CUtlVector<int> &triangles );
	virtual bool GenerateStaticPropData( void *fileContent, CUtlVector<float> &verts, CUtlVector<int> &triangles );
	virtual bool GenerateDynamicPropData( CUtlVector<float> &verts, CUtlVector<int> &triangles );
	virtual bool GenerateBrushData( void *fileContent, CUtlVector<float> &verts, CUtlVector<int> &triangles );

private:
	CUtlVector< float > m_Vertices;
	CUtlVector< int > m_Triangles; // Indices into m_Vertices
	CUtlVector< float > m_Normals;

	rcChunkyTriMesh* m_chunkyMesh;
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