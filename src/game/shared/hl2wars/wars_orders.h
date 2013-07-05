//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WARS_ORDERS_H
#define WARS_ORDERS_H

#ifdef _WIN32
#pragma once
#endif

// Forward declarations
class Vector;

#ifdef CLIENT_DLL
	#define CUnitBase C_UnitBase
	#define CHL2WarsPlayer C_HL2WarsPlayer
	class CUnitBase;
	class CHL2WarsPlayer;
#else
	class CUnitBase;
	class CHL2WarsPlayer;
#endif // CLIENT_DLL
class GroupMoveOrder
{
public:
	GroupMoveOrder( CHL2WarsPlayer *pPlayer, const Vector &vPosition );
	virtual void Apply( void );

	void AddUnit( CUnitBase *pUnit );

	typedef struct UnitRatio_t
	{
		UnitRatio_t( CUnitBase *pUnit, float fRatio ) : unit(pUnit), ratio(fRatio) {}
		CUnitBase *unit;
		float ratio;
	} UnitRatio_t;

	typedef struct PositionRatio_t
	{
		PositionRatio_t( const Vector &vPosition, float fRatio ) : position(vPosition), ratio(fRatio) {}
		Vector position;
		float ratio;
	} PositionRatio_t;

protected:
	void ComputeSquareFormation( void );
	void ComputeFormationPositions( void );
	void ComputeFormationMidPoint( Vector &vMidPoint );
	void ComputeMidPointUnits( Vector &vMidPoint );
	void Project( const Vector &point1, const Vector &point2, const Vector &toproject, Vector &projected );
	void ProjectAndSortUnits( const Vector &unitmidpoint, const Vector &formationmidpoint, CUtlVector< UnitRatio_t > &result  );
	void ProjectAndSortPositions( const Vector &unitmidpoint, const Vector &formationmidpoint, CUtlVector< PositionRatio_t > &result  );
	void OrderUnit( CUnitBase *pUnit, const Vector &targetposition, const QAngle &angle, bp::list selection );

private:
	CUtlVector< CUnitBase * > m_Units;
	CUtlVector< Vector > m_Positions;

	CHL2WarsPlayer *m_pPlayer;
	Vector m_vPosition;
};

// Inlines
inline void GroupMoveOrder::AddUnit( CUnitBase *pUnit )
{
	m_Units.AddToTail( pUnit );
}

#endif // WARS_ORDERS_H