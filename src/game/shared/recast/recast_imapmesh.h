//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_IMAPMESH_H
#define RECAST_IMAPMESH_H

#ifdef _WIN32
#pragma once
#endif

struct rcChunkyTriMesh;

abstract_class IMapMesh
{
public:
	virtual const float *GetVerts() = 0;
	virtual int GetNumVerts() = 0;
	virtual const int *GetTris() = 0;
	virtual int GetNumTris() = 0;
	virtual const float *GetNorms() = 0;
	virtual const rcChunkyTriMesh *GetChunkyMesh() = 0;
};

#endif // RECAST_IMAPMESH_H