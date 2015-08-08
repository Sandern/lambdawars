//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Python wrappers for querying the navigation mesh
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_NAVMESH_H
#define SRCPY_NAVMESH_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	class C_UnitBase;
	#define CUnitBase C_UnitBase
#else
	class CUnitBase;
#endif // CLIENT_DLL

// Generic Nav mesh functions
bool NavMeshAvailable();
//bool NavMeshTestHasArea( Vector &pos, float beneathLimt = 120.0f );
float NavMeshGetPathDistance( Vector &start, Vector &goal, float maxdist = 10000.0f, CBaseEntity *unit = NULL, float beneathlimit = 120.0f );
Vector NavMeshGetPositionNearestNavArea( const Vector &pos, float beneathlimit=120.0f, float maxradius=256.0f, CBaseEntity *unit = NULL );

Vector RandomNavAreaPosition( CBaseEntity *unit = NULL );
Vector RandomNavAreaPositionWithin( const Vector &mins, const Vector &maxs, CBaseEntity *unit = NULL );

// Nav mesh editing
void DestroyAllNavAreas();

int GetNavAreaAt( const Vector &pos, float beneathlimit = 120.0f );

// Nav mesh testing
bool NavIsAreaFlat( const Vector &mins, const Vector &maxs, float flattol = 0.7f, CUnitBase *unit = NULL );

// Hiding/cover spot functions
boost::python::list GetHidingSpotsInRadius( const Vector &pos, float radius, CUnitBase *unit = NULL, bool sort = true, const Vector *sortpos = NULL );

int CreateHidingSpot( const Vector &pos, bool notsaved = false, bool checkground = true );
bool DestroyHidingSpot( const Vector &pos, float tolerance, int num = 1, unsigned int excludeFlags = 0 );
bool DestroyHidingSpotByID( const Vector &pos, unsigned int hidespotid );

#endif // SRCPY_NAVMESH_H
