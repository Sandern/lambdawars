//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Utility code. Merge with src_python_wrappers_util.h?
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "src_python_util.h"
#include "src_python.h"
#include "ipredictionsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Sets the entity size
//-----------------------------------------------------------------------------
static void PySetMinMaxSize (CBaseEntity *pEnt, const Vector& mins, const Vector& maxs )
{
	for ( int i=0 ; i<3 ; i++ )
	{
		if ( mins[i] > maxs[i] )
		{
			char buf[256];
			Q_snprintf(buf, 256, "%s: backwards mins/maxs", ( pEnt ) ? pEnt->GetDebugName() : "<NULL>");
			PyErr_SetString(PyExc_ValueError, buf );
			throw boost::python::error_already_set(); 
			return;
		}
	}

	Assert( pEnt );

	pEnt->SetCollisionBounds( mins, maxs );
}

//-----------------------------------------------------------------------------
// Sets the model size
//-----------------------------------------------------------------------------
void UTIL_PySetSize( CBaseEntity *pEnt, const Vector &vecMin, const Vector &vecMax )
{
	PySetMinMaxSize (pEnt, vecMin, vecMax);
}

//-----------------------------------------------------------------------------
// Sets the model to be associated with an entity
//-----------------------------------------------------------------------------
void UTIL_PySetModel( CBaseEntity *pEntity, const char *pModelName )
{
	// check to see if model was properly precached
	int i = modelinfo->GetModelIndex( pModelName );
	if ( i < 0 )
	{
		char buf[256];
		Q_snprintf(buf, 256, "%i/%s - %s:  UTIL_SetModel:  not precached: %s\n", pEntity->entindex(),
			STRING( pEntity->GetEntityName() ),
			pEntity->GetClassname(), pModelName );
		PyErr_SetString(PyExc_ValueError, buf );
		throw boost::python::error_already_set(); 
		return;
	}

	pEntity->SetModelIndex( i ) ;
	pEntity->SetModelName( AllocPooledString( pModelName ) );

	// brush model
	const model_t *mod = modelinfo->GetModel( i );
	if ( mod )
	{
		Vector mins, maxs;
		modelinfo->GetModelBounds( mod, mins, maxs );
		PySetMinMaxSize (pEntity, mins, maxs);
	}
	else
	{
		PySetMinMaxSize (pEntity, vec3_origin, vec3_origin);
	}

	CBaseAnimating *pAnimating = pEntity->GetBaseAnimating();
	if ( pAnimating )
	{
		pAnimating->m_nForceBone = 0;
	}
}

#else


#endif // CLIENT_DLL

int UTIL_GetModuleIndex( const char *module )
{
	return SrcPySystem()->GetModuleIndex(module);
}

const char *UTIL_GetModuleNameFromIndex( int index )
{
	return SrcPySystem()->GetModuleNameFromIndex(index);
}

boost::python::list UTIL_ListDir( const char *pPath, const char *pPathID, const char *pWildCard )
{
	if( !pPath || !pWildCard )
		return boost::python::list();

	const char *pFileName;
	char wildcard[MAX_PATH];
	FileFindHandle_t fh;
	boost::python::list result;

	// TODO: Do we need to add a slash in case there is no slash?
	Q_snprintf(wildcard, MAX_PATH, "%s%s", pPath, pWildCard);

	pFileName = filesystem->FindFirstEx(wildcard, pPathID, &fh);
	while( pFileName )
	{
		result.append( boost::python::object( pFileName ) );
		pFileName = filesystem->FindNext(fh);
	}
	filesystem->FindClose(fh);
	return result;
}

// Python fixed up versions
#ifdef CLIENT_DLL
boost::python::object UTIL_PyEntitiesInBox( int listMax, const Vector &mins, const Vector &maxs, 
					   int flagMask, int partitionMask )
{
	int i, n;
	C_BaseEntity **pList;
	boost::python::list pylist = boost::python::list();

	pList = (C_BaseEntity **)malloc(listMax*sizeof(C_BaseEntity *));
	n = UTIL_EntitiesInBox(pList, listMax, mins, maxs, flagMask, partitionMask);
	for( i=0; i < n; i++)
		pylist.append(*pList[i]);
	free(pList);
	return pylist;
}
boost::python::object UTIL_PyEntitiesInSphere( int listMax, const Vector &center, float radius, 
						  int flagMask, int partitionMask )
{
	int i, n;
	C_BaseEntity **pList;
	boost::python::list pylist = boost::python::list();

	pList = (C_BaseEntity **)malloc(listMax*sizeof(C_BaseEntity *));
	n = UTIL_EntitiesInSphere(pList, listMax, center, radius, flagMask, partitionMask);
	for( i=0; i < n; i++)
		pylist.append(*pList[i]);
	free(pList);
	return pylist;
}
#else
boost::python::object UTIL_PyEntitiesInBox( int listMax, const Vector &mins, const Vector &maxs, int flagMask )
{
	int i, n;
	CBaseEntity **pList;
	boost::python::list pylist = boost::python::list();

	pList = (CBaseEntity **)malloc(listMax*sizeof(CBaseEntity *));
	n = UTIL_EntitiesInBox(pList, listMax, mins, maxs, flagMask);
	for( i=0; i < n; i++)
		pylist.append(*pList[i]);
	free(pList);
	return pylist;
}

boost::python::object UTIL_PyEntitiesInSphere( int listMax, const Vector &center, float radius, int flagMask )
{
	int i, n;
	CBaseEntity **pList;
	boost::python::list pylist = boost::python::list();

	pList = (CBaseEntity **)malloc(listMax*sizeof(CBaseEntity *));
	n = UTIL_EntitiesInSphere(pList, listMax, center, radius, flagMask);
	for( i=0; i < n; i++)
		pylist.append(*pList[i]);
	free(pList);
	return pylist;
}

boost::python::object UTIL_PyEntitiesAlongRay( int listMax, const PyRay_t &ray, int flagMask )
{
	int i, n;
	CBaseEntity **pList;
	boost::python::list pylist = boost::python::list();

	pList = (CBaseEntity **)malloc(listMax*sizeof(CBaseEntity *));
	//n = UTIL_EntitiesAlongRay(pList, listMax, *(ray.ray), flagMask);
	n = UTIL_EntitiesAlongRay(pList, listMax, ray.ToRay(), flagMask);
	for( i=0; i < n; i++)
		pylist.append(*pList[i]);

	free(pList);
	return pylist;
}
#endif 


//-----------------------------------------------------------------------------
// Purpose: Trace filter that only hits Units and the player
//-----------------------------------------------------------------------------
bool CTraceFilterOnlyUnitsAndPlayer::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask ) )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( !pEntity )
			return false;
		return (pEntity->IsUnit() || pEntity->IsPlayer());
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Trace filter that only hits anything but Units and the player
//-----------------------------------------------------------------------------
bool CTraceFilterNoUnitsOrPlayer::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask ) )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( !pEntity )
			return NULL;
		return (!pEntity->IsUnit() && !pEntity->IsPlayer());
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Trace filter that ignores team mates
//-----------------------------------------------------------------------------
bool CTraceFilterIgnoreTeam::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	if ( !pEntity )
		return false;
	if( pEntity->GetOwnerNumber() == m_iOwnerNumber )
		return false;
	return CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask);
}

// Prediction
CBaseEntity const *GetSuppressHost()
{
	IPredictionSystem *sys = IPredictionSystem::g_pPredictionSystems;
	if( sys )
		return sys->GetSuppressHost();
	return NULL;
}
