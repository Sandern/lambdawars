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

#include "editormapmgr.h"
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

	// Map managing
	void ClearLoadedMap();
	void LoadCurrentVmf();
#ifndef CLIENT_DLL
	void SaveCurrentVmf();
#endif // CLIENT_DLL
	const char *GetCurrentVmfPath();
	bool IsMapLoaded();

	// Listener
#ifndef CLIENT_DLL
	void OnEntityDeleted( CBaseEntity *pEntity );
#endif // CLIENT_DLL

	// Selection
	void ClearSelection();
	void DoSelect( CHL2WarsPlayer *pPlayer );

	// Flora
	void RemoveFloraInRadius( const Vector &vPosition, float fRadius );

private:
	// Selection
	bool IsSelected( CBaseEntity *pEntity );
	void Deselect( CBaseEntity *pEntity );
	void Select( CBaseEntity *pEntity );

#ifdef CLIENT_DLL
	void RenderSelection();

	virtual void PostRender();
#endif // CLIENT_DLL

private:
	CEditorMapMgr m_MapManager;

	// Selection
	CUtlVector<EHANDLE> m_hSelectedEntities;

#ifdef CLIENT_DLL
	CMaterialReference m_matSelect;
#endif // CLIENT_DLL
};

// Map managing
inline void CEditorSystem::ClearLoadedMap()
{
	m_MapManager.ClearLoadedMap();
}

inline void CEditorSystem::LoadCurrentVmf()
{
	m_MapManager.LoadCurrentVmf();
}
#ifndef CLIENT_DLL
inline void CEditorSystem::SaveCurrentVmf()
{
	m_MapManager.SaveCurrentVmf();
}
#endif // CLIENT_DLL
inline const char *CEditorSystem::GetCurrentVmfPath()
{
	return m_MapManager.GetCurrentVmfPath();
}
inline bool CEditorSystem::IsMapLoaded()
{
	return m_MapManager.IsMapLoaded();
}
// Selection
inline void CEditorSystem::ClearSelection()
{
	m_hSelectedEntities.Purge();
}

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