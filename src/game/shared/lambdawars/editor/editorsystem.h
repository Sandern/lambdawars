//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#ifndef EDITORSYSTEM_H
#define EDITORSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "c_hl2wars_player.h"
#else
#include "hl2wars_player.h"
#endif // CLIENT_DLL

#ifndef CLIENT_DLL
class CEditorSystem : public CBaseGameSystemPerFrame, public IEntityListener
#else
class CEditorSystem : public CBaseGameSystemPerFrame
#endif // CLIENT_DLL
{
public:
	virtual bool Init();
	virtual void Shutdown();

	virtual void LevelShutdownPreEntity();

	// VMF managing
	void ClearLoadedMap();
	void LoadCurrentVmf();
#ifndef CLIENT_DLL
	void SaveCurrentVmf();
#endif // CLIENT_DLL
	const char *GetCurrentVmfPath();
	bool IsMapLoaded();

	KeyValues *VmfToKeyValues( const char *pszVmf );
	void KeyValuesToVmf( KeyValues *pKV, CUtlBuffer &vmf );

	void LoadVmf( const char *pszVmf );

#ifndef CLIENT_DLL
	void OnEntityDeleted( CBaseEntity *pEntity );
#endif // CLIENT_DLL

	void DoSelect( CHL2WarsPlayer *pPlayer );

private:
	bool ParseVmfFile( KeyValues *pKeyValues );
	
	void BuildVmfPath( char *pszOut, int maxlen, bool bMakeRelative = true );
	bool BuildCurrentVmfPath( char *pszOut, int maxlen );

#ifndef CLIENT_DLL
	void CollectNewAndUpdatedEntities( CUtlMap< int, CBaseEntity * > &updatedEntities, CUtlVector< CBaseEntity * > &newEntities );
	void ApplyChangesToVmfFile();
	void FillEntityEntry( KeyValues *pEntityKey, CBaseEntity *pEnt, int iTargetID, int iHammerID );
#endif // CLIENT_DLL

	// Selection
	bool IsSelected( CBaseEntity *pEntity );
	void Deselect( CBaseEntity *pEntity );
	void Select( CBaseEntity *pEntity );

#ifdef CLIENT_DLL
	void RenderSelection();

	virtual void PostRender();
#endif // CLIENT_DLL

private:
	// Path to current VMF map file being edited
	char m_szCurrentVmf[MAX_PATH];
	// Data of VMF map file being edited
	KeyValues *m_pKVVmf;
	// Deleted Hammer Ids
	CUtlVector<int> m_DeletedHammerIDs;

	// Selection
	CUtlVector<EHANDLE> m_hSelectedEntities;

#ifdef CLIENT_DLL
	CMaterialReference m_matSelect;
#endif // CLIENT_DLL
};

inline bool CEditorSystem::IsSelected( CBaseEntity *pEntity )
{
	return m_hSelectedEntities.HasElement( pEntity );
}

inline void CEditorSystem::Deselect( CBaseEntity *pEntity )
{
	m_hSelectedEntities.FindAndRemove( pEntity );
}

inline void CEditorSystem::Select( CBaseEntity *pEntity )
{
	if( !m_hSelectedEntities.HasElement( pEntity ) )
		m_hSelectedEntities.AddToTail( pEntity );
}

CEditorSystem *EditorSystem();

#endif // EDITORSYSTEM_H