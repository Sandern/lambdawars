//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_MGR_H
#define RECAST_MGR_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utldict.h"

class CRecastMesh;

class CRecastMgr
{
public:
	CRecastMgr();
	~CRecastMgr();

	// Load methods
	virtual bool Load();
	virtual void Reset();
	
	// Accessors for Units
	CRecastMesh *GetMesh( int index );
	int FindMeshIndex( const char *name );

#ifndef CLIENT_DLL
	// Generation methods
	virtual bool Build();
	virtual bool Save();
#endif // CLIENT_DLL

	// Debug
#ifdef CLIENT_DLL
	void DebugRender();
#endif // CLIENT_DLL

protected:
#ifndef CLIENT_DLL
	const char *GetFilename( void ) const;
#endif // CLIENT_DLL

private:
	CUtlDict< CRecastMesh *, int > m_Meshes;
};

CRecastMgr &RecastMgr();

#endif // RECAST_MGR_H