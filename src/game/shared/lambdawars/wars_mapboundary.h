//====== Copyright � Sandern Corporation, All rights reserved. ===========//
//
// Purpose:		
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
#ifdef CLIENT_DLL
	DECLARE_PYCLIENTCLASS( CBaseFuncMapBoundary );
#else
	DECLARE_PYSERVERCLASS( CBaseFuncMapBoundary );
#endif // CLIENT_DLL

	CBaseFuncMapBoundary();
	~CBaseFuncMapBoundary();

	void GetMapBoundary( Vector &mins, Vector &maxs );

	bool IsWithinMapBoundary( const Vector &vPoint, const Vector &vMins, const Vector &vMaxs, bool bIgnoreZ = false );
	static CBaseFuncMapBoundary *IsWithinAnyMapBoundary( const Vector &vPoint, const Vector &vMins, const Vector &vMaxs, bool bIgnoreZ = false );
	static void SnapToNearestBoundary( Vector &vPoint, const Vector &vMins, const Vector &vMaxs, bool bUseMaxZ=false );
	static bool	DidHitMapBoundary( CBaseEntity *pHitEnt );

	float GetBloat() { return m_fBloat; }

#if defined( GAME_DLL )
	int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
#endif

public:
	CBaseFuncMapBoundary			*m_pNext;
	CBaseFuncMapBoundary			*GetNext() { return m_pNext; } // For python

private:
	float m_fBloat;
};

CBaseFuncMapBoundary *GetMapBoundaryList();

#endif // WARS_BASEMAPBOUNDARY_H