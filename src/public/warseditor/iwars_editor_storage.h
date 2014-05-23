#ifndef MISSION_CHOOSER_INT_H
#define MISSION_CHOOSER_INT_H
#ifdef _WIN32
#pragma once
#endif

#include "appframework/IAppSystem.h"
#include "utlvector.h"
//#include "basehandle.h"

class KeyValues;

//-----------------------------------------------------------------------------
// Purpose: Interface exposed from the wars editor storage .dll (to the game dlls)
//-----------------------------------------------------------------------------
abstract_class IWarsEditorStorage : public IAppSystem
{
public:
	virtual void ClearData() = 0;

	// Methods for syncing commands between server and client through
	// a direct bridge.
	virtual void QueueClientCommand( KeyValues *pCommand ) = 0;
	virtual void QueueServerCommand( KeyValues *pCommand ) = 0;
	virtual KeyValues *PopClientCommandQueue() = 0;
	virtual KeyValues *PopServerCommandQueue() = 0;
};

#define WARS_EDITOR_STORAGE_VERSION		"VWarsEditorStorage001"

#endif // MISSION_CHOOSER_INT_H
