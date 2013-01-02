//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose:		
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef WARS_BASEMAPBOUNDARY_H
#define WARS_BASEMAPBOUNDARY_H
#pragma once

//=============================================================================
//	>> CBaseFuncMapBoundary
//=============================================================================
#if defined( CLIENT_DLL )
#define CBaseFuncMapBoundary C_BaseFuncMapBoundary
#endif

class CBaseFuncMapBoundary : public CBaseEntity
{
	DECLARE_CLASS( CBaseFuncMapBoundary, CBaseEntity );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CBaseFuncMapBoundary();
	~CBaseFuncMapBoundary();

	void GetMapBoundary( Vector &mins, Vector &maxs );

	bool IsWithinMapBoundary( const Vector &vPoint, bool bIgnoreZ = false );
	static CBaseFuncMapBoundary *IsWithinAnyMapBoundary( const Vector &vPoint, bool bIgnoreZ = false );
	static void SnapToNearestBoundary( Vector &vPoint, bool bUseMaxZ=false );

	float GetBloat() { return m_fBloat; }

	#if defined( GAME_DLL )
	int UpdateTransmitState()
	{
		// transmit if in PVS for clientside prediction
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
	#endif

public:
	CBaseFuncMapBoundary			*m_pNext;

private:
	float m_fBloat;
};

CBaseFuncMapBoundary *GetMapBoundaryList();



#endif // WARS_BASEMAPBOUNDARY_H