//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_sense.h"
#include "wars_func_unit.h"
#include "vprof.h"

#include "recast/recast_mgr.h"
#include "recast/recast_mesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DEFAULT_SENSE_MESH "human"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef ENABLE_PYTHON
UnitBaseSense::UnitBaseSense( boost::python::object outer ) : 
	UnitComponent(outer), m_fSenseDistance(-1), m_bUseLimitedViewCone(false), 
	m_fSenseRate(0.4f), m_fNextSenseTime(0.0f), m_bTestLOS(false), m_bEnabled(true)
{
	m_SeenEnemies.EnsureCapacity(512);
	m_SeenOther.EnsureCapacity(512);
}
#endif // ENABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseSense::PerformSensing()
{
	VPROF_BUDGET( "UnitBaseSense::PerformSensing", VPROF_BUDGETGROUP_UNITS );

	if( m_fNextSenseTime > gpGlobals->curtime )
	{
		UpdateRememberedSeen();
		return;
	}

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
// Purpose: Trash dead units to prevent from reselecting not alive units
//			This is called when no full update of sensing is done.
//-----------------------------------------------------------------------------
void UnitBaseSense::UpdateRememberedSeen()
{
	// Just trash not alive entities
	for( int i = m_SeenEnemies.Count() - 1; i >= 0; i-- )
	{	
		CBaseEntity *pEnemy = m_SeenEnemies[i].entity;
		if( !pEnemy || (pEnemy->IsUnit() && !pEnemy->IsAlive()) ||
			m_pOuter->IRelationType( pEnemy ) != D_HT )
		{
			m_SeenEnemies.Remove( i );
		}
	}

	for( int i = m_SeenOther.Count() - 1; i >= 0; i-- )
	{
		CBaseEntity *pOther = m_SeenOther[i].entity;
		if( !pOther || (pOther->IsUnit() && !pOther->IsAlive()) ||
			m_pOuter->IRelationType( pOther ) == D_HT )
		{
			m_SeenOther.Remove( i );
		}
	}

	// Nearest is used by enemy selection, so do full test
	if( !m_hNearestEnemy || !TestEntity( m_hNearestEnemy ) || m_pOuter->IRelationType( m_hNearestEnemy ) != D_HT )
	{
		CRecastMesh *pMesh = m_pOuter ? RecastMgr().GetMesh( RecastMgr().FindBestMeshForEntity( m_pOuter ) ) : RecastMgr().GetMesh( DEFAULT_SENSE_MESH );

		int iAttackPriority;
		float fBestEnemyDist = MAX_COORD_FLOAT*MAX_COORD_FLOAT;
		int iBestAttackPriority = -1000;

		for( int i = 0; i < m_SeenEnemies.Count(); i++ )
		{
			CUnitBase *pEnemy = m_SeenEnemies[i].entity->MyUnitPointer();
			if( !pEnemy )
				continue;

			// Test if best nearest enemy
			iAttackPriority = pEnemy->GetAttackPriority();
			if( iAttackPriority > iBestAttackPriority 
				|| (iAttackPriority == iBestAttackPriority && m_SeenEnemies[i].distancesqr < fBestEnemyDist) )
			{
				bool bReachable = !pMesh || pMesh->FindPathDistance( m_pOuter->GetAbsOrigin(), pEnemy->GetAbsOrigin(), pEnemy, 1024.0f, true ) >= 0;
				if( bReachable )
				{
					fBestEnemyDist = m_SeenEnemies[i].distancesqr;
					m_hNearestEnemy = m_SeenEnemies[i].entity;
					iBestAttackPriority = iAttackPriority;
				}
			}
		}
	}

	// Just clear nearest friendly and attacked friendly (less important)
	if( m_hNearestFriendly && !TestEntity( m_hNearestFriendly ) )
		m_hNearestFriendly = NULL;
	if( m_hNearestAttackedFriendly && !TestEntity( m_hNearestAttackedFriendly ) )
		m_hNearestAttackedFriendly = NULL;
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
	// Skip myself and skip if it's a unit, but not alive
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
		if( !m_pOuter->HasRangeAttackLOS( pOther->WorldSpaceCenter(), pOther ) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UnitBaseSense::TestUnit( CUnitBase *pUnit )
{
	if( !pUnit->CanBeSeen( m_pOuter ) || !pUnit->IsSolid() || !pUnit->FOWShouldShow( m_pOuter->GetOwnerNumber() ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UnitBaseSense::TestFuncUnit( CFuncUnit *pUnit )
{
	if( !pUnit->CanBeSeen( m_pOuter ) || !pUnit->IsSolid() || !pUnit->FOWShouldShow( m_pOuter->GetOwnerNumber() ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int UnitBaseSense::LookForUnits( int iDistance )
{
	int i, iAttackPriority, iBestAttackPriority;
	float distSqr, otherDist;
	CUnitBase *pOther;
	CFuncUnit *pFuncOther;
	CBaseEntity *pEntOther;

	float fBestEnemyDist = MAX_COORD_FLOAT*MAX_COORD_FLOAT;
	float fBestFriendlyDist = fBestEnemyDist;
	float fBestAttackedFriendlyDist = fBestEnemyDist;

	m_hNearestEnemy = NULL;
	m_hNearestFriendly = NULL;
	m_hNearestAttackedFriendly = NULL;

	iBestAttackPriority = -1000;

	m_SeenEnemies.RemoveAll();
	m_SeenOther.RemoveAll();

	// Don't do any sensing if we are disabled; just empty the lists
	if( !m_bEnabled )
		return CountSeen();

	CRecastMesh *pMesh = m_pOuter ? RecastMgr().GetMesh( RecastMgr().FindBestMeshForEntity( m_pOuter ) ) : RecastMgr().GetMesh( DEFAULT_SENSE_MESH );

	const Vector &origin = GetOuter()->GetAbsOrigin();
	distSqr = iDistance * iDistance;

	// First check units
	CUnitBase **ppUnits = g_Unit_Manager.AccessUnits();
	for ( i = 0; i < g_Unit_Manager.NumUnits(); i++ )
	{
		pOther = ppUnits[i];

		if( GetOuter()->HasOverridenEntityRelationship( pOther ) )
			continue; // Will be checked later in special relations

		otherDist = origin.AsVector2D().DistToSqr(pOther->GetAbsOrigin().AsVector2D());
		if( otherDist > distSqr )
			continue;

		if( !TestEntity( pOther ) || !TestUnit( pOther ) )
			continue;

		Disposition_t relation = m_pOuter->IRelationType( pOther );
		if( relation == D_HT )
		{
			m_SeenEnemies.AddToTail();
			m_SeenEnemies.Tail().entity = pOther;
			m_SeenEnemies.Tail().distancesqr = otherDist;

			// Test if best nearest enemy
			iAttackPriority = pOther->GetAttackPriority();
			if( iAttackPriority > iBestAttackPriority 
				|| (iAttackPriority == iBestAttackPriority && otherDist < fBestEnemyDist) )
			{
				bool bReachable = !pMesh || pMesh->FindPathDistance( origin, pOther->GetAbsOrigin(), pOther, 1024.0f, true ) >= 0;
				if( bReachable )
				{
					fBestEnemyDist = otherDist;
					m_hNearestEnemy = pOther;
					iBestAttackPriority = iAttackPriority;
				}
			}
		}
		else
		{
			m_SeenOther.AddToTail();
			m_SeenOther.Tail().entity = pOther;
			m_SeenOther.Tail().distancesqr = otherDist;

			// Test if nearest friendly
			if( relation == D_LI )
			{
				if( otherDist < fBestFriendlyDist )
				{
					fBestFriendlyDist = otherDist;
					m_hNearestFriendly = pOther;
				}
				if( (gpGlobals->curtime - pOther->GetLastTakeDamageTime()) < 2.0 && otherDist < fBestAttackedFriendlyDist )
				{
					fBestAttackedFriendlyDist = otherDist;
					m_hNearestAttackedFriendly = pOther;
				}
			}
		}
	}

	// Check for func units
	for( i = 0; i < g_FuncUnitList.Count(); i++ )
	{
		pFuncOther = g_FuncUnitList[i];

		if( GetOuter()->HasOverridenEntityRelationship( pFuncOther ) )
			continue; // Will be checked later in special relations

		if( !TestEntity( pFuncOther ) || !TestFuncUnit( pFuncOther ) )
			continue;

		otherDist = origin.AsVector2D().DistToSqr(pFuncOther->GetAbsOrigin().AsVector2D());
		if( otherDist > distSqr )
			continue;

		Disposition_t relation = m_pOuter->IRelationType( pFuncOther );
		if( relation == D_HT )
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
				m_hNearestEnemy = pFuncOther;
				iBestAttackPriority = iAttackPriority;
			}
		}
		else
		{
			m_SeenOther.AddToTail();
			m_SeenOther.Tail().entity = pFuncOther;
			m_SeenOther.Tail().distancesqr = otherDist;

			// Test if nearest friendly
			if( relation == D_LI )
			{
				if( otherDist < fBestFriendlyDist )
				{
					fBestFriendlyDist = otherDist;
					m_hNearestFriendly = pFuncOther;
				}
			}
		}
	}

	// Then check for special relations with custom priorities
	for ( i = GetOuter()->m_Relationship.Count()-1; i >= 0; i-- ) 
	{
		pEntOther = GetOuter()->m_Relationship[i].entity;
		if( pEntOther )
		{
			otherDist = origin.AsVector2D().DistToSqr( pEntOther->GetAbsOrigin().AsVector2D() );
			if( otherDist > distSqr )
				continue;

			if( !TestEntity( pEntOther ) )
				continue;

			if( GetOuter()->m_Relationship[i].disposition == D_HT )
			{
				m_SeenEnemies.AddToTail();
				m_SeenEnemies.Tail().entity = pEntOther;
				m_SeenEnemies.Tail().distancesqr = otherDist;

				// Test if nearest enemy
				iAttackPriority = GetOuter()->m_Relationship[i].priority;
				if( iAttackPriority > iBestAttackPriority 
					|| (iAttackPriority == iBestAttackPriority && otherDist < fBestEnemyDist) )
				{
					bool bReachable = !pMesh || pMesh->FindPathDistance( origin, pEntOther->GetAbsOrigin(), pEntOther, 120.0f, true ) >= 0;
					if( bReachable )
					{
						fBestEnemyDist = otherDist;
						m_hNearestEnemy = pEntOther;
						iBestAttackPriority = iAttackPriority;
					}
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

	// Execute callback if within enemy range
#ifdef ENABLE_PYTHON
	for( int i = m_Callbacks.Count()-1; i >= 0; i-- )
	{
		RangeCallback_t &callback = m_Callbacks[i];
		if( callback.nextchecktime > gpGlobals->curtime )
			continue;

		if( fBestEnemyDist < (callback.range*callback.range) )
		{
			callback.nextchecktime = gpGlobals->curtime + callback.frequency;
			callback.callback();
		}
	}
#endif // ENABLE_PYTHON

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
#ifdef ENABLE_PYTHON
boost::python::list UnitBaseSense::PyGetEnemies( const char *unittype )
{
	CBaseEntity *pEnemy;
	boost::python::list units;
	for( int i = 0; i < m_SeenEnemies.Count(); i++ )
	{
		pEnemy = m_SeenEnemies[i].entity;
		if( !pEnemy )
			continue;

		if( unittype )
		{
			CUnitBase *pEnemyUnit = pEnemy->MyUnitPointer();
			if( !pEnemyUnit )
				continue;

			if( Q_stricmp( unittype, pEnemyUnit->GetUnitType() ) != 0 )
				continue;
		}

		units.append( pEnemy->GetPyHandle() );
	}
	return units;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
boost::python::list UnitBaseSense::PyGetOthers( const char *unittype )
{
	CBaseEntity *pOther;
	boost::python::list units;
	for( int i = 0; i < m_SeenOther.Count(); i++ )
	{
		pOther = m_SeenOther[i].entity;
		if( !pOther )
			continue;

		if( unittype )
		{
			CUnitBase *pOtherUnit = pOther->MyUnitPointer();
			if( !pOtherUnit )
				continue;

			const char *pOtherUnitType = pOtherUnit->GetUnitType();
			if( pOtherUnitType && V_stricmp( unittype , pOtherUnitType ) != 0 )
				continue;
		}

		units.append( pOther->GetPyHandle() );	
	}
	return units;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UnitBaseSense::AddEnemyInRangeCallback( boost::python::object callback, int range, float frequency )
{
	for( int i = 0; i < m_Callbacks.Count(); i++ )
	{
		if( m_Callbacks[i].callback == callback && m_Callbacks[i].range == range )
			return false;
	}

	m_Callbacks.AddToTail();
	RangeCallback_t &callbackinfo = m_Callbacks.Tail();

	callbackinfo.callback = callback;
	callbackinfo.range = range;
	callbackinfo.frequency = frequency;
	callbackinfo.nextchecktime = gpGlobals->curtime;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UnitBaseSense::RemoveEnemyInRangeCallback( boost::python::object callback, int range )
{
	for( int i = 0; i < m_Callbacks.Count(); i++ )
	{
		if( m_Callbacks[i] == callback && m_Callbacks[i].range == range )
		{
			m_Callbacks.Remove( i );
			return true;
		}
	}
	return false;
}

#endif // ENABLE_PYTHON

