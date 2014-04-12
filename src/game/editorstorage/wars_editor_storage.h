#ifndef _INCLUDED_WARS_EDITOR_STORAGE_H
#define _INCLUDED_WARS_EDITOR_STORAGE_H
#ifdef _WIN32
#pragma once
#endif

#include "warseditor/iwars_editor_storage.h"
#include "tier3/tier3dm.h"

class IEngineVGui;

class CWarsEditorStorage : public CTier3AppSystem< IWarsEditorStorage > 
{
	typedef CTier3AppSystem< IWarsEditorStorage > BaseClass;
public:

	// Methods of IAppSystem
	virtual bool Connect( CreateInterfaceFn factory );
	virtual void Disconnect();
	virtual void *QueryInterface( const char *pInterfaceName );
	virtual InitReturnVal_t Init();
	
	// IWarsEditorStorage
	void ClearData();
	void AddEntityToQueue( KeyValues *pEntity );
	KeyValues *PopEntityFromQueue();

private:
	CUtlVector<KeyValues *> m_hQueuedEntities;
};

#endif // _INCLUDED_WARS_EDITOR_STORAGE_H
