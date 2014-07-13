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
#include "editorwarsmapmgr.h"

#ifdef CLIENT_DLL
#include "c_hl2wars_player.h"
#include "vgui/MouseCode.h"
#else
#include "hl2wars_player.h"
#endif // CLIENT_DLL

class CWarsFlora;

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

	bool IsActive();

	// Map managing
	void ClearLoadedMap();
	void LoadCurrentVmf();
#ifndef CLIENT_DLL
	void SaveCurrentVmf();
#endif // CLIENT_DLL
	const char *GetCurrentVmfPath();
	bool IsMapLoaded();
	const char *GetLastMapError();

	// Listener
#ifndef CLIENT_DLL
	void OnEntityDeleted( CBaseEntity *pEntity );
#endif // CLIENT_DLL

	// Selection
	void ClearSelection();
	void DoSelect( CHL2WarsPlayer *pPlayer );
	void DeleteSelection();
	void SetSelectionCenterLocked( bool locked );

	int GetNumSelected();
	CBaseEntity *GetSelected( int idx );
	bool IsAnythingSelected();
	Vector GetSelectionCenter();
	QAngle GetSelectionOrientation();

#ifndef CLIENT_DLL
	void ClearCopyCommands();
	void CopySelection();
	void PasteSelection();
#endif // CLIENT_DLL

	// Editor modes
	enum EditorInteractionMode_t
	{
		EDITORINTERACTION_NONE = 0,
		EDITORINTERACTION_SELECT,
		EDITORINTERACTION_ADD,
		EDITORINTERACTION_TRANSLATE,
		EDITORINTERACTION_ROTATE,

		EDITORINTERACTION_COUNT,
	};
	void SetEditorMode( EditorInteractionMode_t mode ) { m_EditorMode = mode; }
	EditorInteractionMode_t GetEditorMode() { return m_EditorMode; }

#ifdef CLIENT_DLL
	void OnMousePressed( vgui::MouseCode code );
	void OnMouseReleased( vgui::MouseCode code );
#endif // CLIENT_DLL

	// Commands processor
	bool ProcessCommand( KeyValues *pCommand );
	bool ProcessCreateCommand( KeyValues *pCommand );
	bool ProcessDeleteFloraCommand( KeyValues *pCommand );
	bool ProcessSelectCommand( KeyValues *pCommand );

	void QueueCommand( KeyValues *pCommand );
	KeyValues *CreateFloraCreateCommand( CWarsFlora *pFlora, const Vector *vOffset = NULL );
	KeyValues *CreateClearSelectionCommand();

private:
	// Selection
	bool IsSelected( CBaseEntity *pEntity );
	void Deselect( CBaseEntity *pEntity );
	void Select( CBaseEntity *pEntity );

	// Axis interaction for translation/rotation only exists on client
	// On drag end, the result is synced to server
#ifdef CLIENT_DLL
	Vector GetViewOrigin();
	QAngle GetViewAngles();
	Vector GetPickingRay( int x, int y );

	enum EditiorSelectedAxis_t
	{
		EDITORAXIS_NONE = 0,
		EDITORAXIS_X,
		EDITORAXIS_Y,
		EDITORAXIS_Z,
		EDITORAXIS_SCREEN,
		EDITORAXIS_PLANE_XY,
		EDITORAXIS_PLANE_XZ,
		EDITORAXIS_PLANE_YZ,

		EDITORAXIS_COUNT,
		EDITORAXIS_FIRST = EDITORAXIS_X,
		EDITORAXIS_FIRST_PLANE = EDITORAXIS_PLANE_XY,
	};

	EditiorSelectedAxis_t GetCurrentSelectedAxis();
	void SetCurrentSelectedAxis( EditiorSelectedAxis_t axis );
	void UpdateCurrentSelectedAxis( int x, int y );
	void UpdateEditorInteraction();
#endif // CLIENT_DLL

#ifdef CLIENT_DLL
	void RenderSelection();
	void RenderHelpers();
	void RenderTranslate();
	void RenderRotate();

	void StartDrag( int mode );
	void StopDrag();
	void GetDragBounds( int *pi2_min, int *pi2_max );
	Vector GetDragWorldPos( int x, int y );

	void MoveSelection( const Vector &delta );
	void RotateSelection( const VMatrix &matRotate );

	virtual void Update( float frametime );
	virtual void PostRender();
#else
	virtual void FrameUpdatePostEntityThink();
#endif // CLIENT_DLL

private:
	CEditorMapMgr m_MapManager;
	CEditorWarsMapMgr m_WarsMapManager;

	// Selection
	CUtlVector<EHANDLE> m_hSelectedEntities;
	bool m_bSelectionCenterLocked;
	Vector m_vecSelectionCenterCache;

	CUtlVector<KeyValues *> m_SelectionCopyCommands;

#ifdef CLIENT_DLL
	CMaterialReference m_matSelect;
	CMaterialReference m_matHelper;
#endif // CLIENT_DLL

	EditorInteractionMode_t m_EditorMode;
#ifdef CLIENT_DLL
	enum
	{
		EDITORDRAG_NONE = 0,
		EDITORDRAG_VIEW,
		EDITORDRAG_TRANSLATE,
		EDITORDRAG_ROTATE,
		EDITORDRAG_SELECT,
	};

	EditiorSelectedAxis_t m_iCurrentSelectedAxis;
	int m_iLastMouse_x, m_iLastMouse_y;

	int m_iDragMode;
	int m_iDragStartPos[2];
	bool m_bDragSelectArmed;
	Vector m_vecLastDragWorldPos;
#endif // CLIENT_DLL
};


inline bool CEditorSystem::IsActive()
{
	return IsMapLoaded();
}

// Map managing
inline void CEditorSystem::ClearLoadedMap()
{
	m_MapManager.ClearLoadedMap();
	m_WarsMapManager.ClearLoadedMap();
}

inline void CEditorSystem::LoadCurrentVmf()
{
	m_MapManager.LoadCurrentVmf();
	m_WarsMapManager.LoadCurrentMap();
}
#ifndef CLIENT_DLL
inline void CEditorSystem::SaveCurrentVmf()
{
	m_MapManager.SaveCurrentVmf();
	m_WarsMapManager.SaveCurrentMap();
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
inline const char *CEditorSystem::GetLastMapError()
{
	return m_MapManager.GetLastMapError();
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

#ifdef CLIENT_DLL
inline CEditorSystem::EditiorSelectedAxis_t CEditorSystem::GetCurrentSelectedAxis()
{
	return m_iCurrentSelectedAxis;
}

inline void CEditorSystem::SetCurrentSelectedAxis( CEditorSystem::EditiorSelectedAxis_t axis )
{
	m_iCurrentSelectedAxis = axis;
}
#endif // CLIENT_DLL

CEditorSystem *EditorSystem();

#endif // EDITORSYSTEM_H