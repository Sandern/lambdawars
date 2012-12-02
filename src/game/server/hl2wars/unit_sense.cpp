//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_sense.h"
#include "wars_func_unit.h"
#include "nav_mesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifndef DISABLE_PYTHON
UnitBaseSense::UnitBaseSense( boost::python::object outer ) : 
	UnitComponent(outer), m_fSenseDistance(-1), m_bUseLimitedViewCone(false), 
	m_fSenseRate(0.4f), m_fNextSenseTime(0.0f), m_bTestLOS(false)
{
	m_SeenEnemies.EnsureCapacity(512);
	m_SeenOther.EnsureCapacity(512);
}
#endif // DISABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseSense::PerformSensing()
{
	if( m_fNextSenseTime > gpGlobals->curtime )
		return;

	if( m_fSenseDistance == -1 )
		Look( GetOuter()->GetViewDistance() );
	else
		Look( m_fSenseDistance );

	m_fNextSenseTime = gpGlobals->curtime + m_fSenseRate;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseSense::ForcePerformSensing()
{
	m_fNextSenseTime = 0.0f;
	PerformSensing();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseSense::Look( int iDistance )
{
	LookForUnits(iDistance);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UnitBaseSense::TestEntity( CBaseEntity *pOther )
{
	// Skip myself and skip not alive if an unit
	if( pOther == m_pOuter || (pOther->IsUnit() && !pOther->IsAlive()) )
		return false;

	if( m_bUseLimitedViewCone )
	{
		Vector los = ( pOther->GetAbsOrigin() - GetAbsOrigin() );
		los.z = 0;
		VectorNormalize( los );

		float flDot = DotProduct( los, m_pOuter->BodyDirection2D() );
		if( !( flDot > m_fViewCone ) )
			return false;
	}

	if( m_bTestLOS )
	{
		if( !m_pOuter->HasRangeAttackLOS(pOther->WorldSpaceCenter()) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UnitBaseSense::TestUnit( CUnitBase *pUnit )
{
	if( !pUnit->CanBeSeen( m_pOuter ) || !pUnit->FOWShouldShow( m_pOuter->GetOwnerNumber() ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int UnitBaseSense::LookForUnits( int iDistance )
{
	int i;
	float distSqr, otherDist;
	CUnitBase *pOther;
	CFuncUnit *pFuncOther;
	CBaseEntity *pEntOther;

	float fBestEnemyDist = MAX_COORD_FLOAT*MAX_COORD_FLOAT;
	int iAttackPriority, iBestAttackPriority;

	m_NearestEnemy = NULL;
	iBestAttackPriority = -666;

	m_SeenEnemies.RemoveAll();
	m_SeenOther.RemoveAll();

	const Vector &origin = GetOuter()->GetAbsOrigin();
	distSqr = iDistance * iDistance;

	// First check units
	CUnitBase **ppUnits = g_Unit_Manager.AccessUnits();
	for ( i = 0; i < g_Unit_Manager.NumUnits(); i++ )
	{
		pOther = ppUnits[i];

		otherDist = origin.DistToSqr(pOther->GetAbsOrigin());
		if( otherDist > distSqr )
			continue;

		if( !TestEntity( pOther ) || !TestUnit( pOther ) )
			continue;

		if( m_pOuter->IRelationType( pOther ) == D_HT )
		{
			m_SeenEnemies.AddToTail();
			m_SeenEnemies.Tail().entity = pOther;
			m_SeenEnemies.Tail().distancesqr = otherDist;

			// Test if best nearest enemy
			iAttackPriority = pOther->GetAttackPriority();
			if( iAttackPriority > iBestAttackPriority 
				|| (iAttackPriority == iBestAttackPriority && otherDist < fBestEnemyDist) )
			{
				fBestEnemyDist = otherDist;
				m_NearestEnemy = pOther;
				iBestAttackPriority = iAttackPriority;
			}
		}
		else
		{
			m_SeenOther.AddToTail();
			m_SeenOther.Tail().entity = pOther;
			m_SeenOther.Tail().distancesqr = otherDist;
		}
	}

	// Check for func units
	for( i = 0; i < g_FuncUnitList.Count(); i++ )
	{
		pFuncOther = g_FuncUnitList[i];

		if( !TestEntity( pFuncOther ) )
			continue;

		otherDist = origin.DistToSqr(pFuncOther->GetAbsOrigin());
		if( otherDist > distSqr )
			continue;

		if( m_pOuter->IRelationType( pFuncOther ) == D_HT )
		{
			m_SeenEnemies.AddToTail();
			m_SeenEnemies.Tail().entity = pFuncOther;
			m_SeenEnemies.Tail().distancesqr = otherDist;

			// Test if best nearest enemy
			iAttackPriority = pFuncOther->GetAttackPriority();
			if( iAttackPriority > iBestAttackPriority 
				|| (iAttackPriority == iBestAttackPriority && otherDist < fBestEnemyDist) )
			{
				fBestEnemyDist = otherDist;
				m_NearestEnemy = pFuncOther;
				iBestAttackPriority = iAttackPriority;
			}
		}
		else
		{
			m_SeenOther.AddToTail();
			m_SeenOther.Tail().entity = pFuncOther;
			m_SeenOther.Tail().distancesqr = otherDist;
		}
	}

	// Then check for special relations
	for ( i = GetOuter()->m_Relationship.Count()-1; i >= 0; i-- ) 
	{
		pEntOther = GetOuter()->m_Relationship[i].entity;
		if( pEntOther && !pEntOther->IsUnit() && 
			GetOuter()->m_Relationship[i].disposition == D_HT )
		{
			otherDist = origin.DistToSqr(pEntOther->GetAbsOrigin());
			if( otherDist > distSqr )
				continue;

			if( !TestEntity( pEntOther ) )
				continue;

			if( m_pOuter->IRelationType( pEntOther ) == D_HT )
			{
				m_SeenEnemies.AddToTail();
				m_SeenEnemies.Tail().entity = pEntOther;
				m_SeenEnemies.Tail().distancesqr = otherDist;

				// Test if nearest enemy
				if( otherDist < fBestEnemyDist )
				{
					fBestEnemyDist = otherDist;
					m_NearestEnemy = pEntOther;
					iBestAttackPriority = 666;//iAttackPriority;
				}
			}
			else
			{
				m_SeenOther.AddToTail();
				m_SeenOther.Tail().entity = pEntOther;
				m_SeenOther.Tail().distancesqr = otherDist;
			}
		}
	}

	return CountSeen();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int UnitBaseSense::CountEnemiesInRange( float range )
{
	int count = 0;
	float rangeSqr = range * range;
	for( int i = 0; i < m_SeenEnemies.Count(); i++ )
	{
		if( m_SeenEnemies[i].entity && m_SeenEnemies[i].distancesqr < rangeSqr )
			count++;
	}
	return count;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int UnitBaseSense::CountOthersInRange( float range )
{
	int count = 0;
	float rangeSqr = range * range;
	for( int i = 0; i < m_SeenOther.Count(); i++ )
	{
		if( m_SeenOther[i].entity && m_SeenOther[i].distancesqr < rangeSqr )
			count++;
	}
	return count;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *UnitBaseSense::GetNearestOther()
{
	CBaseEntity *pBest = NULL;
	float fBest = MAX_COORD_FLOAT*MAX_COORD_FLOAT;
	for( int i = 0; i < m_SeenOther.Count(); i++ )
	{
		if( m_SeenOther[i].entity == NULL )
			continue;

		if( m_SeenOther[i].distancesqr < fBest )
		{
			fBest = m_SeenOther[i].distancesqr;
			pBest = m_SeenOther[i].entity;
		}
	}
	return pBest;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifndef DISABLE_PYTHON
bp::list UnitBaseSense::PyGetEnemies( const char *unittype )
{
	bp::list units;
	for( int i = 0; i < m_SeenEnemies.Count(); i++ )
	{
		if( m_SeenEnemies[i].entity )
		{
			if( unittype && m_SeenEnemies[i].entity->IsUnit() &&
					Q_stricmp( unittype , m_SeenEnemies[i].entity->MyUnitPointer()->GetUnitType() ) != 0 )
				continue;
			units.append( m_SeenEnemies[i].entity->GetPyHandle() );
		}
	}
	return units;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::list UnitBaseSense::PyGetOthers( const char *unittype )
{
	bp::list units;
	for( int i = 0; i < m_SeenOther.Count(); i++ )
	{
		if( m_SeenOther[i].entity )
		{
			if( unittype && m_SeenOther[i].entity->IsUnit() &&
					Q_stricmp( unittype , m_SeenOther[i].entity->MyUnitPointer()->GetUnitType() ) != 0 )
				continue;
			units.append( m_SeenOther[i].entity->GetPyHandle() );	
		}
	}
	return units;
}
#endif // DISABLE_PYTHON