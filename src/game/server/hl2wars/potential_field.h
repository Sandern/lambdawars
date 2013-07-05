//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef POTENTIAL_FIELD_H
#define POTENTIAL_FIELD_H

#ifdef _WIN32
#pragma once
#endif

#define PF_TILESIZE 8		// tile size in units
#define PF_SIZE 32768
#define PF_ARRAYSIZE (PF_SIZE/PF_TILESIZE)
#define PF_CONSIDER 50

class CPotentialField 
{
public:
	friend class CPotentialFieldMgr;

	void Reset();
	void Update();

private:
	float m_Density[PF_ARRAYSIZE][PF_ARRAYSIZE];
	Vector m_AvgVelocities[PF_ARRAYSIZE][PF_ARRAYSIZE];
};

class CPotentialFieldMgr : public CBaseGameSystemPerFrame
{
public:
	CPotentialFieldMgr();

	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPostEntity();

	virtual bool NeedsUpdate();
	virtual void FrameUpdatePreEntityThink();

	float LookupDensity( const Vector &vPos );
	void LookupAvgVelocity( const Vector &vPos, Vector *pResult );

private:
	CPotentialField *m_pPotentialField;

	bool		m_bActive;
	float		m_fNeedsUpdate;
};

extern CPotentialFieldMgr* PotentialFieldMgr();

// Inlines
inline float CPotentialFieldMgr::LookupDensity( const Vector &vPos )
{
	return m_pPotentialField->m_Density[ (int)((vPos.x + (PF_SIZE / 2)) / (float)PF_TILESIZE) ][ (int)((vPos.y + (PF_SIZE / 2)) / (float)PF_TILESIZE) ];
}

inline void CPotentialFieldMgr::LookupAvgVelocity( const Vector &vPos, Vector *pResult )
{
	*pResult = m_pPotentialField->m_AvgVelocities[ (int)((vPos.x + (PF_SIZE / 2)) / (float)PF_TILESIZE) ][ (int)((vPos.y + (PF_SIZE / 2)) / (float)PF_TILESIZE) ];
}

#endif // POTENTIAL_FIELD_H
