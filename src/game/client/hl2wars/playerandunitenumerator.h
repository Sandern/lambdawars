//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYERANDUNITENUMERATOR_H
#define PLAYERANDUNITENUMERATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "UtlVector.h"
#include "ehandle.h"
#include "ISpatialPartition.h"

class C_BaseEntity;

class CUnitEnumerator : public IPartitionEnumerator
{
	DECLARE_CLASS_NOBASE( CUnitEnumerator );
public:
	//Forced constructor
	CUnitEnumerator( float radius, Vector vecOrigin )
	{
		m_flRadiusSquared = radius * radius;
		m_vecOrigin = vecOrigin;
		m_Objects.RemoveAll();
	}

	int	GetObjectCount() { return m_Objects.Count(); }

	C_BaseEntity *GetObject( int index )
	{
		if ( index < 0 || index >= GetObjectCount() )
			return NULL;

		return m_Objects[ index ];
	}

	//Actual work code
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( pHandleEntity->GetRefEHandle() );
		if ( pEnt == NULL )
			return ITERATION_CONTINUE;

		if ( !pEnt->IsUnit() )
			return ITERATION_CONTINUE;

		Vector	deltaPos = pEnt->GetAbsOrigin() - m_vecOrigin;
		if ( deltaPos.LengthSqr() > m_flRadiusSquared )
			return ITERATION_CONTINUE;

		CHandle< C_BaseEntity > h;
		h = pEnt;
		m_Objects.AddToTail( h );

		return ITERATION_CONTINUE;
	}

public:
	//Data members
	float	m_flRadiusSquared;
	Vector m_vecOrigin;

	CUtlVector< CHandle< C_BaseEntity > > m_Objects;
};

class CPlayerAndUnitEnumerator : public IPartitionEnumerator
{
	DECLARE_CLASS_NOBASE( CPlayerAndUnitEnumerator );
public:
	//Forced constructor
	CPlayerAndUnitEnumerator( float radius, Vector vecOrigin )
	{
		m_flRadiusSquared = radius * radius;
		m_vecOrigin = vecOrigin;
		m_Objects.RemoveAll();
	}

	int	GetObjectCount() { return m_Objects.Count(); }

	C_BaseEntity *GetObject( int index )
	{
		if ( index < 0 || index >= GetObjectCount() )
			return NULL;

		return m_Objects[ index ];
	}

	//Actual work code
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( pHandleEntity->GetRefEHandle() );
		if ( pEnt == NULL )
			return ITERATION_CONTINUE;

		if ( !pEnt->IsPlayer() && !pEnt->IsUnit() )
			return ITERATION_CONTINUE;

		Vector	deltaPos = pEnt->GetAbsOrigin() - m_vecOrigin;
		if ( deltaPos.LengthSqr() > m_flRadiusSquared )
			return ITERATION_CONTINUE;

		CHandle< C_BaseEntity > h;
		h = pEnt;
		m_Objects.AddToTail( h );

		return ITERATION_CONTINUE;
	}

public:
	//Data members
	float	m_flRadiusSquared;
	Vector m_vecOrigin;

	CUtlVector< CHandle< C_BaseEntity > > m_Objects;
};

#endif // PLAYERANDUNITENUMERATOR_H