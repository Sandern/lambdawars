//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: A trigger entity to manipulate the fog of war height tile map
//			This entity has two modes: a static and dynamic version.
//			The static version is only used on the server and is used when
//			generating the static tile height map file. Both client and server
//			will load this file.
//			The dynamic version is networked and requests the fog of war manager
//			to update an extent.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "fow_blocker.h"
#include "fowmgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( fow_blocker, CFoWBlocker );

BEGIN_DATADESC( CFoWBlocker )
END_DATADESC()

IMPLEMENT_NETWORKCLASS_ALIASED( FoWBlocker, DT_FoWBlocker);

BEGIN_NETWORK_TABLE( CFoWBlocker, DT_FoWBlocker )
END_NETWORK_TABLE()

// a list of map bounderies to search quickly
#ifndef CLIENT_DLL
CEntityClassList< CFoWBlocker > g_FoWBlockerList;
template< class CFoWBlocker >
CFoWBlocker *CEntityClassList< CFoWBlocker >::m_pClassList = NULL;
#else
C_EntityClassList< CFoWBlocker > g_FoWBlockerList;
template< class CFoWBlocker >
CFoWBlocker *C_EntityClassList< CFoWBlocker >::m_pClassList = NULL;
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFoWBlocker *GetFoWBlockerList()
{
	return g_FoWBlockerList.m_pClassList;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFoWBlocker::CFoWBlocker()
{
	g_FoWBlockerList.Insert( this );
}

CFoWBlocker::~CFoWBlocker()
{
	g_FoWBlockerList.Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CFoWBlocker::Spawn()
{
	BaseClass::Spawn();

#ifndef CLIENT_DLL
	m_bClientSidePredicted = true;

	InitTrigger();
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFoWBlocker::Activate()
{
	BaseClass::Activate();

#ifndef CLIENT_DLL
	if( GetEntityName() != NULL_STRING )
#endif // CLIENT_DLL
	{
		// Update tile height FOW
		UpdateTileHeights();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFoWBlocker::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	m_bDisabled = true;

	// On client it should always be updated (since always dynamic)
	// On server only if entity name is set (otherwise static)
#ifndef CLIENT_DLL
	if( GetEntityName() != NULL_STRING )
#endif // CLIENT_DLL
	{
		// Update tile height FOW
		UpdateTileHeights();
	}
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Override ShouldTransmit to transmit when we are "ON" and have a name
//-----------------------------------------------------------------------------
int CFoWBlocker::UpdateTransmitState()
{
	if( GetEntityName() != NULL_STRING )
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
	else
	{
		return SetTransmitState( FL_EDICT_DONTSEND );
	}
}
#else
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void CFoWBlocker::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Update tile height FOW
		UpdateTileHeights();
	}
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Call whenever dynamic tile heights should be updated
//-----------------------------------------------------------------------------
void CFoWBlocker::UpdateTileHeights()
{
	const Vector &vOrigin = GetAbsOrigin();
	FogOfWarMgr()->UpdateHeightAtExtentDynamic( vOrigin + CollisionProp()->OBBMins(), vOrigin + CollisionProp()->OBBMaxs() );
}