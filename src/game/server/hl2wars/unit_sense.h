//========= Copyright © 2008, Valve Corporation, All rights reserved. ============//
//
// Purpose:	
//
//=============================================================================//

#ifndef UNITSENSE_H
#define UNITSENSE_H

#ifdef _WIN32
#pragma once
#endif

#include "unit_component.h"

// Sensing class
class UnitBaseSense : public UnitComponent
{
public:
	friend class CUnitBase;

#ifndef DISABLE_PYTHON
	UnitBaseSense( boost::python::object outer );
#endif // DISABLE_PYTHON

	void PerformSensing();
	void ForcePerformSensing();
	void Look( int iDistance );

	bool TestEntity( CBaseEntity *pEntity );
	bool TestUnit( CUnitBase *pUnit );

	void SetUseLimitedViewCone( bool bUseCone );
	void SetViewCone( float fCone );

	void SetTestLOS( bool bTestLOS );
	bool GetTestLOS();

	int CountSeen();
	int CountSeenEnemy();
	int CountSeenOther();
	CBaseEntity *GetEnemy( int idx );
	CBaseEntity *GetOther( int idx );

#ifndef DISABLE_PYTHON
	bp::object PyGetEnemy( int idx );
	bp::object PyGetOther( int idx );

	bp::list PyGetEnemies( const char *unittype = NULL );
	bp::list PyGetOthers( const char *unittype = NULL );
#endif // DISABLE_PYTHON

	int CountEnemiesInRange( float range );
	int CountOthersInRange( float range );

	CBaseEntity *GetNearestOther();
	CBaseEntity *GetNearestEnemy();

private:
	int 			LookForUnits( int iDistance );

public:
	float m_fSenseDistance;
	float m_fSenseRate;

private:
	struct SeenObject_t {
		EHANDLE entity;
		float distancesqr;
	};
	CUtlVector<SeenObject_t> m_SeenEnemies;
	CUtlVector<SeenObject_t> m_SeenOther;
	EHANDLE m_NearestEnemy; // Cache nearest enemy while sensing

	bool m_bUseLimitedViewCone;
	float m_fViewCone;
	float m_fNextSenseTime;
	bool m_bTestLOS;
};

// Inlines
inline void UnitBaseSense::SetUseLimitedViewCone( bool bUseCone )
{
	m_bUseLimitedViewCone = bUseCone;
}

inline void UnitBaseSense::SetViewCone( float fCone )
{
	m_fViewCone = fCone;
}

inline void UnitBaseSense::SetTestLOS( bool bTestLOS )
{
	m_bTestLOS = bTestLOS;
}

inline bool UnitBaseSense::GetTestLOS()
{
	return m_bTestLOS;
}


inline int UnitBaseSense::CountSeen()
{
	return m_SeenEnemies.Count() + m_SeenOther.Count();
}

inline int UnitBaseSense::CountSeenEnemy()
{
	return m_SeenEnemies.Count();
}

inline int UnitBaseSense::CountSeenOther()
{
	return m_SeenOther.Count();
}

inline CBaseEntity *UnitBaseSense::GetEnemy( int idx )
{
	if( !m_SeenEnemies.IsValidIndex(idx) )
		return NULL;
	return m_SeenEnemies.Element(idx).entity;
}

inline CBaseEntity *UnitBaseSense::GetNearestEnemy()
{
	return m_NearestEnemy;
}

inline CBaseEntity *UnitBaseSense::GetOther( int idx )
{
	if( !m_SeenOther.IsValidIndex(idx) )
		return NULL;
	return m_SeenOther.Element(idx).entity;
}

#ifndef DISABLE_PYTHON
inline bp::object UnitBaseSense::PyGetEnemy( int idx )
{
	CBaseEntity *pEnt = GetEnemy(idx);
	return pEnt ? pEnt->GetPyHandle() : bp::object();
}

inline bp::object UnitBaseSense::PyGetOther( int idx )
{
	CBaseEntity *pEnt = GetOther(idx);
	return pEnt ? pEnt->GetPyHandle() : bp::object();
}
#endif // DISABLE_PYTHON

#endif // UNITSENSE_H
