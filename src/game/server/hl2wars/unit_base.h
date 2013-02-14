//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose:		Base Unit
//
//=============================================================================//

#ifndef UNIT_BASE_H
#define UNIT_BASE_H

#ifdef _WIN32
#pragma once
#endif

#include "npcevent.h"

class CUnitBase;

Vector VecCheckThrowTolerance( CBaseEntity *pEdict, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flTolerance, int iCollisionGroup );

// Anim Event handlers
class BaseAnimEventHandler
{
public:
	virtual void HandleEvent(CUnitBase *pUnit, animevent_t *event) {}
};

class EmitSoundAnimEventHandler : public BaseAnimEventHandler
{
public:
	EmitSoundAnimEventHandler(const char *soundscript);

	void HandleEvent(CUnitBase *pUnit, animevent_t *event);

private:
	char m_SoundScript[ 256 ];
};


class TossGrenadeAnimEventHandler : public BaseAnimEventHandler
{
public:
	TossGrenadeAnimEventHandler(const char *pEntityName, float fSpeed);

	virtual void HandleEvent(CUnitBase *pUnit, animevent_t *event) {}

	bool GetTossVector(CUnitBase *pUnit, const Vector &vecStartPos, const Vector &vecTarget, int iCollisionGroup, Vector *vecOut );
	CBaseEntity *TossGrenade(CUnitBase *pUnit, Vector &vecStartPos, Vector &vecTarget, int iCollisionGroup);

private:
	char m_EntityName[ 40 ];
	float m_fSpeed;
};


// Managing the map
struct animeventhandler_t
{
#ifdef ENABLE_PYTHON
	boost::python::object m_pyInstance;
#endif // ENABLE_PYTHON
	BaseAnimEventHandler *m_pHandler;
};

class AnimEventMap
{
public:
	friend class CUnitBase;

	AnimEventMap();
	AnimEventMap( AnimEventMap &animeventmap );
#ifdef ENABLE_PYTHON
	AnimEventMap( AnimEventMap &animeventmap, boost::python::dict d );
	AnimEventMap( boost::python::dict d );

	void AddAnimEventHandlers( boost::python::dict d );
	void SetAnimEventHandler( int event, boost::python::object handler );
#endif // ENABLE_PYTHON

private:
	CUtlMap<int, animeventhandler_t> m_AnimEventMap;
};

#endif // UNIT_BASE_H