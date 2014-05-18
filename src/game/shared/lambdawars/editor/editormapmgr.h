//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#ifndef EDITORMAPMGR_H
#define EDITORMAPMGR_H
#ifdef _WIN32
#pragma once
#endif

#include "editorbasemapmgr.h"

class CEditorMapMgr : public CEditorBaseMapMgr
{
public:
	CEditorMapMgr();

	// VMF managing
	void ClearLoadedMap();
	void LoadCurrentVmf();
#ifndef CLIENT_DLL
	void SaveCurrentVmf();
#endif // CLIENT_DLL
	const char *GetCurrentVmfPath();
	bool IsMapLoaded();

	void LoadVmf( const char *pszVmf );

	void AddDeletedHammerID( int iHammerID );

private:
	bool ParseVmfFile( KeyValues *pKeyValues );
	
	bool BuildCurrentVmfPath( char *pszOut, int maxlen );

#ifndef CLIENT_DLL
	void CollectNewAndUpdatedEntities( CUtlMap< int, CBaseEntity * > &updatedEntities, CUtlVector< CBaseEntity * > &newEntities );
	bool ApplyChangesToVmfFile();
	void FillEntityEntry( KeyValues *pEntityKey, CBaseEntity *pEnt, int iTargetID, int iHammerID );
#endif // CLIENT_DLL

private:
	// Path to current VMF map file being edited
	char m_szCurrentVmf[MAX_PATH];
	// Data of VMF map file being edited
	KeyValues *m_pKVVmf;
	// Deleted Hammer Ids
	CUtlVector<int> m_DeletedHammerIDs;
	// Visgroup flora
	int m_iFloraVisGroupId;
};

inline void CEditorMapMgr::AddDeletedHammerID( int iHammerID )
{
	m_DeletedHammerIDs.AddToTail( iHammerID );
}

#endif // EDITORMAPMGR_H