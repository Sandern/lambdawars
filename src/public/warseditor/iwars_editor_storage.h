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

	// Server/Client only side entities
	// Created at server side in editor, then synced to client through this bridge
	virtual void AddEntityToQueue( KeyValues *pEntity ) = 0;
	virtual KeyValues *PopEntityFromQueue() = 0;
};

#define WARS_EDITOR_STORAGE_VERSION		"VWarsEditorStorage001"

#endif // MISSION_CHOOSER_INT_H
