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

#define SF_OFFMESHCONN_SMALL				0x000001
#define SF_OFFMESHCONN_HUMAN				0x000002
#define SF_OFFMESHCONN_LARGE				0x000004
#define SF_OFFMESHCONN_AIR					0x000008

#define SF_OFFMESHCONN_JUMPEDGE				0x000010

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