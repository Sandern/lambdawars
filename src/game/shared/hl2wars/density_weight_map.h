//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
// $NoKeywords: $
//=============================================================================//

#ifndef DENSITY_WEIGHT_MAP_H
#define DENSITY_WEIGHT_MAP_H
#pragma once

enum density_type_t
{
	DENSITY_GAUSSIAN = 0, // Use a 2d gaussian function. Sigma based on BoundingRadius.
	DENSITY_GAUSSIANECLIPSE, // Uses mins/maxs, for rectangle shaped objects (buildings).
	DENSITY_NONE,
};

class DensityWeightsMap
{
public:
	DensityWeightsMap();
	~DensityWeightsMap();

	void Init( CBaseEntity *pOuter );
	void Destroy();
	void SetType( int iType );
	int GetType();
	void OnCollisionSizeChanged();
	float Get( const Vector &vPos );
	int GetSizeX() { return m_iSizeX; }
	int GetSizeY() { return m_iSizeY; }

	void RecalculateWeights( const Vector &vMins, const Vector &vMaxs );
	void FillGaussian();
	void FillGaussianEclipse();

	void DebugPrintWeights();

private:
	CBaseEntity *m_pOuter;
	Vector m_vMins, m_vMaxs;
	int m_iType;

	float **m_pWeights;
	int m_iSizeX, m_iSizeY;
	int m_iHalfSizeX, m_iHalfSizeY;
	float m_fXOffset, m_fYOffset;
};

inline int DensityWeightsMap::GetType()
{
	return m_iType;
}

#endif // DENSITY_WEIGHT_MAP_H