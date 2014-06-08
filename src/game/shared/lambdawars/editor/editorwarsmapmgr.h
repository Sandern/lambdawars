//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#ifndef EDITORWARSMAPMGR_H
#define EDITORWARSMAPMGR_H
#ifdef _WIN32
#pragma once
#endif

#include "editorbasemapmgr.h"

class CEditorWarsMapMgr : public CEditorBaseMapMgr
{
public:
	CEditorWarsMapMgr();

	virtual bool IsMapLoaded();
	virtual bool LoadCurrentMap();
	virtual void ClearLoadedMap();
	virtual void SaveCurrentMap();

	void LoadVmf( const char *pszVmf );

	void AddDeletedUUID( const char *pUUID );

	static bool BuildCurrentWarsPath( char *pszOut, int maxlen );

private:
	bool ParseVmfFile( KeyValues *pKeyValues );
	
#ifndef CLIENT_DLL
	void CollectNewAndUpdatedEntities( CUtlMap< const char*, CBaseEntity * > &updatedEntities, CUtlVector< CBaseEntity * > &newEntities );
	bool WriteChangesToWarsFile();
	void FillEntityEntry( KeyValues *pEntityKey, CBaseEntity *pEnt, int iTargetID, int iHammerID );
#endif // CLIENT_DLL

private:
	// Path to current VMF map file being edited
	char m_szCurrentMap[MAX_PATH];
	// Data of Wars map file being edited
	KeyValues *m_pKVWars;
	// Deleted Hammer Ids
	CUtlVector<const char *> m_DeletedUUIDS;
};

inline void CEditorWarsMapMgr::AddDeletedUUID( const char *pUUID )
{
	m_DeletedUUIDS.AddToTail( pUUID );
}

#endif // EDITORWARSMAPMGR_H