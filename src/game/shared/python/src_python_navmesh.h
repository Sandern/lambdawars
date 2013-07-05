//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Python wrappers for querying the navigation mesh
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRC_PYTHON_NAVMESH_H
#define SRC_PYTHON_NAVMESH_H
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
bool NavMeshTestHasArea( Vector &pos, float beneathLimt = 120.0f );
float NavMeshGetPathDistance( Vector &start, Vector &goal, bool anyz = false, float maxdist = 10000.0f, bool notolerance = false, CUnitBase *unit = NULL );
Vector NavMeshGetPositionNearestNavArea( const Vector &pos, float beneathlimit=120.0f, bool checkblocked=true );

Vector RandomNavAreaPosition( float minimumarea = 0, int maxtries = -1 );
Vector RandomNavAreaPositionWithin( const Vector &mins, const Vector &maxs, float minimumarea = 0, int maxtries = -1 );

// Nav mesh editing
int CreateNavArea( const Vector &corner, const Vector &otherCorner );
int CreateNavAreaByCorners( const Vector &nwcorner, const Vector &necorner, const Vector &secorner, const Vector &swcorner );
void DestroyNavArea( unsigned int id );
void DestroyAllNavAreas();

int GetActiveNavMesh();
Vector GetEditingCursor();

int GetNavAreaAt( const Vector &pos, float beneathlimit = 120.0f );
bp::list GetNavAreasAtBB( const Vector &mins, const Vector &maxs );
void SplitAreasAtBB( const Vector &mins, const Vector &maxs );
void SetAreasBlocked( bp::list areas, bool blocked );
bool TryMergeSurrounding( int id, float tolerance = FLT_EPSILON );

// Nav mesh testing
bool IsBBCoveredByNavAreas( const Vector &mins, const Vector &maxs, float tolerance = 0.1f, bool requireisflat = true, float flattol = 0.7f );

// Hiding/cover spot functions
bp::list GetHidingSpotsInRadius( const Vector &pos, float radius, CUnitBase *unit = NULL, bool sort = true );

int CreateHidingSpot( const Vector &pos, int &navareaid, bool notsaved = false, bool checkground = true );
bool DestroyHidingSpot( const Vector &pos, float tolerance );
bool DestroyHidingSpotByID( unsigned int navareaid, unsigned int hidespotid );

#endif // SRC_PYTHON_NAVMESH_H
