//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "triggers.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CFoWBlocker : public CBaseTrigger
{
public:
	DECLARE_CLASS( CFoWBlocker, CBaseTrigger );
	DECLARE_DATADESC();

	virtual void Spawn();
};

LINK_ENTITY_TO_CLASS( fow_blocker, CFoWBlocker );

BEGIN_DATADESC( CFoWBlocker )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CFoWBlocker::Spawn()
{
	BaseClass::Spawn();

	InitTrigger();
}