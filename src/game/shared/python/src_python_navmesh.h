//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
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

bool NavMeshAvailable();
bool NavMeshTestHasArea( Vector &pos, float beneathLimt = 120.0f );
float NavMeshGetPathDistance( Vector &vStart, Vector &vGoal, bool anyz = false, float maxdist = 10000.0f, bool notolerance = false, CUnitBase *unit = NULL );
Vector NavMeshGetPositionNearestNavArea( const Vector &pos, float beneathlimit=120.0f );
void DestroyAllNavAreas();

int CreateNavArea( const Vector &corner, const Vector &otherCorner );
int CreateNavAreaByCorners( const Vector &nwCorner, const Vector &neCorner, const Vector &seCorner, const Vector &swCorner );
void DestroyNavArea( int id );
Vector RandomNavAreaPosition( );

int GetNavAreaAt( const Vector &pos, float beneathlimit = 120.0f );
bp::list GetNavAreasAtBB( const Vector &mins, const Vector &maxs );
void SplitAreasAtBB( const Vector &mins, const Vector &maxs );
void SetAreasBlocked( bp::list areas, bool blocked );

bool IsBBCoveredByNavAreas( const Vector &mins, const Vector &maxs, float tolerance = 0.1f, bool bRequireIsFlat = true, float fFlatTol = 0.7f );

bool TryMergeSurrounding( int id, float tolerance = FLT_EPSILON );

bp::list GetHidingSpotsInRadius( const Vector &pos, float radius );

#endif // SRC_PYTHON_NAVMESH_H
