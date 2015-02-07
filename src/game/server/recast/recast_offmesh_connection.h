//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_OFFMESH_CONNECTION_H
#define RECAST_OFFMESH_CONNECTION_H

#ifdef _WIN32
#pragma once
#endif

#define SF_OFFMESHCONN_JUMPEDGE				0x000001

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class COffMeshConnection : public CServerOnlyPointEntity
{
public:
	DECLARE_CLASS( COffMeshConnection, CServerOnlyPointEntity );
	DECLARE_DATADESC();
};

#endif // RECAST_OFFMESH_CONNECTION_H