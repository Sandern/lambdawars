//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_HL2Wars_PLAYER_H
#define C_HL2Wars_PLAYER_H
#ifdef _WIN32
#pragma once
#endif


#include "c_baseplayer.h"
#include "hl2wars_player_shared.h"

extern ConVar cl_strategic_cam_speed;
extern ConVar cl_strategic_cam_accelerate;
extern ConVar cl_strategic_cam_stopspeed;
extern ConVar cl_strategic_cam_friction;

class C_HL2WarsPlayer : public C_BasePlayer
{
public:
	friend class HL2WarsViewport; // Calls internal mouse press functions.

	DECLARE_CLASS( C_HL2WarsPlayer, C_BasePlayer );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();
	DECLARE_PYCLIENTCLASS( C_HL2WarsPlayer, PN_HL2WARSPLAYER );

	C_HL2WarsPlayer();
	~C_HL2WarsPlayer();
	
	static C_HL2WarsPlayer*			GetLocalHL2WarsPlayer(int nSlot = -1);

	virtual void					Spawn();

	virtual void					ClientThink();
	virtual bool					CreateMove( float flInputSampleTime, CUserCmd *pCmd );
	virtual void					PostDataUpdate( DataUpdateType_t updateType );
	virtual void					OnDataChanged( DataUpdateType_t updateType );
	virtual void					UpdateOnRemove( void );

	virtual bool					ShouldRegenerateOriginFromCellBits() const;

	virtual Vector					EyePosition(void);
	virtual void					ThirdPersonSwitch( bool bThirdperson );
	virtual void					UpdateCameraMode();

	void							SetScrollTimeOut(bool forward);

	// Shared functions
	//virtual const Vector		GetPlayerMins( void ) const;
	//virtual const Vector		GetPlayerMaxs( void ) const;

	bool							IsStrategicModeOn() const { return GetMoveType() == MOVETYPE_STRATEGIC; }
	const Vector &					GetMouseAim() const { return m_vMouseAim; }
	const MouseTraceData_t &		GetMouseData() const { return m_MouseData; }
	virtual void					CalculateMouseData( const Vector &vMouseAim, const Vector &vPos, const Vector &vCamOffset, MouseTraceData_t &mousedata ) const;
	virtual void					UpdateMouseData( const Vector &vMouseAim );
	virtual void					UpdateButtonState( int nUserCmdButtonMask );

	const MouseTraceData_t &		GetMouseDataLeftPressed() const { return m_MouseDataLeftPressed; }
	const MouseTraceData_t &		GetMouseDataLeftDoublePressed() const { return m_MouseDataLeftDoublePressed; }
	const MouseTraceData_t &		GetMouseDataLeftReleased() const { return m_MouseDataLeftReleased; }
	const MouseTraceData_t &		GetMouseDataRightPressed() const { return m_MouseDataRightPressed; }
	const MouseTraceData_t &		GetMouseDataRightDoublePressed() const { return m_MouseDataRightDoublePressed; }
	const MouseTraceData_t &		GetMouseDataRightReleased() const { return m_MouseDataRightReleased; }

	bool							IsLeftPressed();
	bool							IsLeftDoublePressed();
	bool							WasLeftDoublePressed();
	bool							IsRightPressed();
	bool							IsRightDoublePressed();
	bool							WasRightDoublePressed();

	void							ClearMouse();
	void							SetMouseCapture( CBaseEntity *pEntity );
	CBaseEntity *					GetMouseCapture();
	void							SetForceShowMouse( bool bForce );
	bool							ForceShowMouse();

	virtual bool					IsMouseHoveringEntity( CBaseEntity *pEnt );

	inline const char *				GetFaction();
	virtual void					ChangeFaction( const char *faction );

	// selection management
	void							UpdateSelection( void );
#ifdef ENABLE_PYTHON
	boost::python::list				GetSelection( void );
#endif // ENABLE_PYTHON
	CBaseEntity*					GetUnit( int idx );
	void							AddUnit( CBaseEntity *pUnit, bool bTriggerOnSel=true );
	int								FindUnit( CBaseEntity *pUnit );
	void							RemoveUnit( int idx, bool bTriggerOnSel=true );
	void							RemoveUnit( CBaseEntity *pUnit, bool bTriggerOnSel=true );
	int								CountUnits();
	void							ClearSelection( bool bTriggerOnSel=true );
	void							OnSelectionChanged();
	void							OrderUnits();
	void							ScheduleSelectionChangedSignal();

	void							GetBoxSelection( int iXMin, int iYMin, int iXMax, int iYMax,  CUtlVector< EHANDLE > &selection, const char *pUnitType = NULL );
	void							SelectBox( int xmin, int ymin, int xmax, int ymax );
	void							SelectAllUnitsOfTypeInScreen( const char *pUnitType );
	void							SimulateOrderUnits( const MouseTraceData_t &mousedata );
	void							MinimapClick( const MouseTraceData_t &mousedata );
	void							MakeSelection( CUtlVector< EHANDLE > &selection );
#ifdef ENABLE_PYTHON
	void							PyMakeSelection( boost::python::list selection );
#endif // ENABLE_PYTHON

	// Selected unit type. Used by the hud and hotkeys to determine which abilities should be shown
	const char *					GetSelectedUnitType( void );
	void							SetSelectedUnitType( const char *pUnitType );
	void							GetSelectedUnitTypeRange(int &iMin, int &iMax);
	void							UpdateSelectedUnitType( void );

	// Group management
	void							ClearGroup( int group );
	void							AddToGroup( int group, CBaseEntity *pUnit );
	void							MakeCurrentSelectionGroup( int group, bool bClearGroup );
	void							SelectGroup( int group );
	int								GetGroupNumber( CBaseEntity *pUnit );
	int								CountGroup( int group );
	CBaseEntity *					GetGroupUnit( int group, int index );
	void							CleanupGroups( void );
	const CUtlVector< EHANDLE > &	GetGroup( int group );

#ifdef ENABLE_PYTHON
	// Abilities
	void							AddActiveAbility( boost::python::object ability );
	void							RemoveActiveAbility( boost::python::object ability );
	bool							IsActiveAbility( boost::python::object ability );
	void							ClearActiveAbilities();
	void							SetSingleActiveAbility( boost::python::object ability );
	boost::python::object			GetSingleActiveAbility();
#endif // ENABLE_PYTHON

	CBaseEntity *					GetControlledUnit() const;

	// Mouse button checking
	virtual void					OnLeftMouseButtonPressed( const MouseTraceData_t &data );
	virtual void					OnLeftMouseButtonDoublePressed( const MouseTraceData_t &data );
	virtual void					OnLeftMouseButtonReleased( const MouseTraceData_t &data );
	virtual void					OnRightMouseButtonPressed( const MouseTraceData_t &data );
	virtual void					OnRightMouseButtonDoublePressed( const MouseTraceData_t &data );
	virtual void					OnRightMouseButtonReleased( const MouseTraceData_t &data );

	// Camera settings
	void							SetCameraOffset( Vector &offs );
	const Vector &					GetCameraOffset() const;
	float							GetCamSpeed() { return cl_strategic_cam_speed.GetFloat(); }
	float							GetCamAcceleration() { return cl_strategic_cam_accelerate.GetFloat(); }
	float							GetCamStopSpeed() { return cl_strategic_cam_stopspeed.GetFloat(); }
	float							GetCamFriction() { return cl_strategic_cam_friction.GetFloat(); }
	const Vector &					GetCamLimits() { return m_vCameraLimits; }
	void							SetCamLimits( const Vector &limits ) { m_vCameraLimits = limits; }

	virtual void					CalculateHeight( const Vector &vPosition );
	float							GetCamMaxHeight() { return m_fMaxHeight; }
	bool							IsCamValid() { return m_bCamValid; }
	const Vector &					GetCamGroundPos() { return m_vCamGroundPos; }

	// Direct move
	virtual void					SetDirectMove( const Vector &vPos );
	virtual void					StopDirectMove();
	virtual void					SnapCameraTo( const Vector &vPos ); // Positions camera correctly, uses SetDirectMove
	
	virtual void					CamFollowEntity( CBaseEntity *pEnt, bool forced = false );
	virtual void					CamFollowGroup( const CUtlVector< EHANDLE > &m_Entities, bool forced = false );
	virtual void					PyCamFollowGroup( boost::python::list entities, bool forced = false );
	virtual void					CamFollowRelease( bool forced = false );
	virtual Vector					CamCalculateGroupOrigin();

	// Inventory
	virtual C_BaseCombatWeapon *	GetActiveWeapon( void ) const;
	virtual C_BaseCombatWeapon *	GetWeapon( int i ) const;

	// Owner number
	virtual void					OnChangeOwnerNumber( int old_owner_number );

private:
    void					OnLeftMouseButtonPressedInternal( const MouseTraceData_t &data );
	void					OnLeftMouseButtonDoublePressedInternal( const MouseTraceData_t &data );
	void					OnLeftMouseButtonReleasedInternal( const MouseTraceData_t &data );
	void					OnRightMouseButtonPressedInternal( const MouseTraceData_t &data );
	void					OnRightMouseButtonDoublePressedInternal( const MouseTraceData_t &data );
	void					OnRightMouseButtonReleasedInternal( const MouseTraceData_t &data );	

private:
	bool m_bOldIsStrategicModeOn;
	EHANDLE				m_hOldControlledUnit;
	EHANDLE				m_hOldViewEntity;

	// Shared Mouse data
	Vector				m_vMouseAim;
	MouseTraceData_t	m_MouseData;
	Vector				m_vCameraOffset;
	Vector				m_vCameraLimits;

	MouseTraceData_t	m_MouseDataLeftPressed;
	MouseTraceData_t	m_MouseDataLeftReleased;
	MouseTraceData_t	m_MouseDataRightPressed;
	MouseTraceData_t	m_MouseDataRightReleased;
	MouseTraceData_t	m_MouseDataLeftDoublePressed;
	MouseTraceData_t	m_MouseDataRightDoublePressed;
	bool				m_bIsLeftPressed;
	bool				m_bIsLeftDoublePressed;
	bool				m_bWasLeftDoublePressed;
	bool				m_bIsRightPressed;
	bool				m_bIsRightDoublePressed;
	bool				m_bWasRightDoublePressed;
	bool				m_bIsMouseCleared;

	EHANDLE				m_hMouseCaptureEntity;

	bool				m_bForceShowMouse;

	// Direct move
	bool				m_bDirectMoveActive;
	bool				m_bDisableDirectMove;
	Vector				m_vDirectMove;
	float				m_fDirectMoveTimeOut;

	float				m_fMaxHeight;
	bool				m_bCamValid;
	Vector				m_vCamGroundPos;

	EHANDLE				m_hCamFollowEntity;
	CUtlVector< EHANDLE > m_CamFollowEntities;
	bool				m_bForcedFollowEntity;

	// Player data
	string_t			m_FactionName;
	char				m_NetworkedFactionName[MAX_PATH];

	// Selection data
	CUtlVector< EHANDLE >		m_hSelectedUnits;
	bool				m_bSelectionChangedSignalScheduled;
	CNetworkHandle( C_BaseEntity, m_hControlledUnit );	
	bool				m_bRebuildPySelection;
#ifdef ENABLE_PYTHON
	boost::python::list			m_pySelection;
#endif // ENABLE_PYTHON

	// Selected unittype data
	string_t			m_pSelectedUnitType;
	int					m_iSelectedUnitTypeMin;
	int					m_iSelectedUnitTypeMax;

	// Group data
	unitgroup_t m_Groups[PLAYER_MAX_GROUPS];
	float m_fLastSelectGroupTime;
	int m_iLastSelectedGroup;

	// Ability
#ifdef ENABLE_PYTHON
	CUtlVector<boost::python::object> m_vecActiveAbilities;
#endif // ENABLE_PYTHON
};

// Inlines
inline bool CHL2WarsPlayer::IsLeftPressed() 
{ 
	return m_bIsLeftPressed; 
}

inline bool CHL2WarsPlayer::IsLeftDoublePressed() 
{ 
	return m_bIsLeftDoublePressed; 
}

inline bool CHL2WarsPlayer::WasLeftDoublePressed() 
{ 
	return m_bWasLeftDoublePressed; 
}

inline bool CHL2WarsPlayer::IsRightPressed()
{ 
	return m_bIsRightPressed; 
}

inline bool CHL2WarsPlayer::IsRightDoublePressed() 
{ 
	return m_bIsRightDoublePressed; 
}

inline bool CHL2WarsPlayer::WasRightDoublePressed() 
{ 
	return m_bWasRightDoublePressed; 
}

inline void C_HL2WarsPlayer::SetForceShowMouse( bool bForce )
{
	m_bForceShowMouse = bForce;
}

inline bool C_HL2WarsPlayer::ForceShowMouse()
{
	return m_bForceShowMouse;
}

inline const char *	C_HL2WarsPlayer::GetFaction()
{
	return STRING(m_FactionName);
}

inline C_HL2WarsPlayer* ToHL2WarsPlayer( CBaseEntity *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsPlayer() )
		return NULL;

	return static_cast< C_HL2WarsPlayer* >( pPlayer );
}

#endif // C_HL2Wars_PLAYER_H
