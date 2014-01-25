//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:		Player for HL2Wars Game
//
// $NoKeywords: $
//=============================================================================//

#ifndef HL2WARS_PLAYER_SHARED_H
#define HL2WARS_PLAYER_SHARED_H
#pragma once

#include "networkstringtabledefs.h"

#define INVALID_ABILITYSLOT		0
#define INVALID_ABILITY			INVALID_STRING_INDEX 

#ifdef CLIENT_DLL
#define CHL2WarsPlayer C_HL2WarsPlayer
#endif // CLIENT_DLL

#if defined( CLIENT_DLL )
	#define CUnitBase C_UnitBase
#endif
class CUnitBase;
class CBaseEntity;

#ifdef CLIENT_DLL
	extern ConVar cl_selection_noclear;
#endif // CLIENT_DLL

struct MouseTraceData_t
{
	MouseTraceData_t() : m_hEnt(NULL)
	{
#ifdef CLIENT_DLL
		m_iX = m_iY = 0;
#endif 
	}

	void SetEnt( CBaseEntity *pEnt ) 
	{ 
		m_hEnt = pEnt;
	}

	CBaseEntity *GetEnt() 
	{ 
		return m_hEnt.Get();
	}

	Vector m_vStartPos;

	// Trace data hit ent
	Vector m_vEndPos;
	Vector m_vNormal;
	EHANDLE m_hEnt;
	int m_nHitBox;

	// Trace data world only
	Vector m_vWorldOnlyEndPos;
	Vector m_vWorldOnlyNormal;

	// Client only data
#ifdef CLIENT_DLL
	int m_iX;
	int m_iY;
#endif 
};

typedef struct unitgroup_t
{
#ifndef CLIENT_DLL
	DECLARE_CLASS_NOBASE( unitgroup_t );
	DECLARE_DATADESC();
#endif // CLIENT_DLL

	CUtlVector< EHANDLE > m_Group;
} unitgroup_t;
#define PLAYER_MAX_GROUPS 10

// Helper
// #define CLIENTSENDEHANDLE
inline long EncodeEntity( CBaseEntity *pEntity )
{
	// TODO: Change this to ehandle when the serial between client and server matches
	//		 Due the limited number of networked serial bits it does not
	//		 Should change SERIAL_MASK to NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS
	//		 Side effects?
#ifdef CLIENTSENDEHANDLE
	long iEncodedEHandle;
	
	EHANDLE hEnt = pEntity;

	int iSerialNum = hEnt.GetSerialNumber() & (1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS) - 1;
	iEncodedEHandle = hEnt.GetEntryIndex() | (iSerialNum << MAX_EDICT_BITS);

	return iEncodedEHandle;
#else
	return pEntity->entindex();
#endif // 
}

#endif // HL2WARS_PLAYER_SHARED_H