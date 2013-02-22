//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "../EventLog.h"
#include "KeyValues.h"

#include "hl2wars_player.h"

class CHL2WarsEventLog : public CEventLog
{
private:
	typedef CEventLog BaseClass;

public:
	virtual ~CHL2WarsEventLog() {};

public:
	bool PrintEvent( IGameEvent * event )	// override virtual function
	{
		if ( !PrintHL2WarsEvent( event ) )
		{
			return BaseClass::PrintEvent( event );
		}
		else
		{
			return true;
		}

		return false;
	}
	bool Init()
	{
		BaseClass::Init();

		ListenForGameEvent( "player_death" );
		ListenForGameEvent( "player_hurt" );

		return true;
	}
protected:

	bool PrintHL2WarsEvent( IGameEvent * event )	// print Mod specific logs
	{
		const char *eventName = event->GetName();
	
		if ( !Q_strncmp( eventName, "server_", strlen("server_")) )
		{
			return false; // ignore server_ messages, always.
		}

		return false;
	}

};

CHL2WarsEventLog g_HL2WarsEventLog;

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
IGameSystem* GameLogSystem()
{
	return &g_HL2WarsEventLog;
}

