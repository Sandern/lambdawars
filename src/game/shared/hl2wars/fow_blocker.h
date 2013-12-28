//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef FOW_BLOCKER_H
#define FOW_BLOCKER_H

#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CFoWBlocker C_FoWBlocker

#include "c_triggers.h"
#define CBaseTrigger C_BaseTrigger
#else
#include "triggers.h"
#endif // CLIENT_DLL

class CFoWBlocker : public CBaseTrigger
{
public:
	DECLARE_CLASS( CFoWBlocker, CBaseTrigger );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();

	CFoWBlocker();
	~CFoWBlocker();

	virtual void Spawn();
	virtual void Activate();
	virtual void UpdateOnRemove();

	virtual void UpdateTileHeights();

	bool IsEnabled() { return !m_bDisabled; }
	bool IsDisabled() { return m_bDisabled; }

#ifndef CLIENT_DLL
	int  UpdateTransmitState(void);
#else
	void OnDataChanged( DataUpdateType_t updateType );
#endif // CLIENT_DLL

public:
	CFoWBlocker			*m_pNext;

private:
	bool m_bDisabled;
};

CFoWBlocker *GetFoWBlockerList();

#endif // FOW_BLOCKER_H