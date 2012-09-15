//========= Copyright © 2008, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base Unit
//
//=============================================================================//

#ifndef WARS_FUNC_UNIT_H
#define WARS_FUNC_UNIT_H

#ifdef _WIN32
#pragma once
#endif

#include "iunit.h"
#include "networkvar.h"
#ifdef CLIENT_DLL
	#define CFuncUnit C_FuncUnit
	#define FUNCUNIT_DERIVED C_FuncBrush
	#include "c_func_brush.h"
	class WarsVGUIScreen;
#else
	#define FUNCUNIT_DERIVED CFuncBrush
	#include "modelentities.h"
#endif // CLIENT_DLL

// Func unit list
class CFuncUnit;
typedef CUtlVector<CFuncUnit *> CFuncUnitArray;

extern CFuncUnitArray g_FuncUnitList;

// Class
class CFuncUnit : public FUNCUNIT_DERIVED, public IUnit
{
	DECLARE_CLASS( CFuncUnit, FUNCUNIT_DERIVED );
public:
	//---------------------------------
#if !defined( CLIENT_DLL )
	DECLARE_SERVERCLASS();
#else
	DECLARE_CLIENTCLASS();
	//DECLARE_PREDICTABLE();
#endif

#ifdef CLIENT_DLL
	DECLARE_PYCLIENTCLASS( CFuncUnit );
#else
	DECLARE_PYSERVERCLASS( CFuncUnit );
#endif // CLIENT_DLL

	CFuncUnit();
	~CFuncUnit();

	virtual IMouse *GetIMouse()												{ return this; }
	virtual IUnit *GetIUnit()												{ return this; }
	virtual bool IsUnit() { return true; }

	virtual void Spawn();
	virtual void UpdateOnRemove( void );

	// IMouse implementation
	virtual void OnClickLeftPressed( CHL2WarsPlayer *player )				{}
	virtual void OnClickRightPressed( CHL2WarsPlayer *player )				{}
	virtual void OnClickLeftReleased( CHL2WarsPlayer *player )				{}
	virtual void OnClickRightReleased( CHL2WarsPlayer *player )				{}

	virtual void OnClickLeftDoublePressed( CHL2WarsPlayer *player )			{}
	virtual void OnClickRightDoublePressed( CHL2WarsPlayer *player )		{}

	virtual void OnCursorEntered( CHL2WarsPlayer *player )					{}
	virtual void OnCursorExited( CHL2WarsPlayer *player )					{}

#ifdef CLIENT_DLL
	virtual void		OnDataChanged( DataUpdateType_t updateType );

	virtual void OnHoverPaint();
	virtual unsigned long GetCursor() { return 2; }

	virtual int			GetHealth() const { return m_iHealth; }
	virtual int			GetMaxHealth()  const	{ return m_iMaxHealth; }

#ifdef HL2WARS_ASW_DLL
	virtual int				DrawModel( int flags, const RenderableInstance_t &instance );
#else
	virtual int				DrawModel( int flags );
#endif // HL2WARS_ASW_DLL
	void					Blink( float blink_time = 3.0f );
#endif // CLIENT_DLL

	// IUnit
	const char *		GetUnitType();
#ifndef CLIENT_DLL
	virtual bool		KeyValue( const char *szKeyName, const char *szValue );
	void				SetUnitType( const char *unit_type );
#endif // CLIENT_DLL

	virtual void		OnUnitTypeChanged( const char *old_unit_type ) {}

#ifndef DISABLE_PYTHON
	virtual bool IsSelectableByPlayer( CHL2WarsPlayer *pPlayer, boost::python::object target_selection = boost::python::object() ) { return true; }
#endif // DISABLE_PYTHON
	virtual void Select( CHL2WarsPlayer *pPlayer, bool bTriggerOnSel = true );
	virtual void OnSelected( CHL2WarsPlayer *pPlayer );
	virtual void OnDeSelected( CHL2WarsPlayer *pPlayer );
#ifdef CLIENT_DLL
	virtual void OnInSelectionBox( void ) {}
	virtual void OnOutSelectionBox( void ) {}
#endif // CLIENT_DLL
	virtual int GetSelectionPriority();
	virtual void SetSelectionPriority( int priority );
	virtual int GetAttackPriority();
	virtual void SetAttackPriority( int priority );

	// Action
	virtual void Order( CHL2WarsPlayer *pPlayer )										{}

	// Squads
	virtual  CBaseEntity *GetSquad();
#ifndef CLIENT_DLL
	virtual void SetSquad( CBaseEntity *pUnit ) { m_hSquadUnit = pUnit; }
#endif // CLIENT_DLL

	// Player control
	virtual void UserCmd( CUserCmd *pCmd )												{}
	virtual void OnUserControl( CHL2WarsPlayer *pPlayer )								{}
	virtual void OnUserLeftControl( CHL2WarsPlayer *pPlayer )							{}
	virtual bool CanUserControl( CHL2WarsPlayer *pPlayer )								{ return true; }
	virtual void OnButtonsChanged( int buttonsMask, int buttonsChanged )				{}
#ifdef CLIENT_DLL
	virtual bool SelectSlot( int slot )													{ return false; }
#else
	virtual bool ClientCommand( const CCommand &args )									{ return false; }
#endif // CLIENT_DLL

#ifndef CLIENT_DLL
	void SetCommander(CHL2WarsPlayer *player);		// sets which player commands this unit
#endif // CLIENT_DLL
	CHL2WarsPlayer* GetCommander() const;

	// Energy
	virtual int			GetEnergy() const { return m_iEnergy; }
	virtual int			GetMaxEnergy()  const	{ return m_iMaxEnergy; }
#ifndef CLIENT_DLL
	virtual void		SetEnergy( int iEnergy ) { m_iEnergy = iEnergy; }
	virtual void		SetMaxEnergy( int iMaxEnergy ) { m_iMaxEnergy = iMaxEnergy; }
#endif // CLIENT_DLL

public:
	int m_iMaxHealth;

private:
	// Players that have this unit selected
	CUtlVector< CHandle< CHL2WarsPlayer > > m_SelectedByPlayers;

	int m_iSelectionPriority;
	int m_iAttackPriority;

#ifndef CLIENT_DLL
	string_t						m_UnitType;
	CNetworkString(	m_NetworkedUnitType, MAX_PATH );

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iHealth );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iMaxHealth );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_lifeState );

	// Energy
	CNetworkVar(int, m_iEnergy );
	CNetworkVar(int, m_iMaxEnergy );
#else
	string_t m_UnitType;
	char m_NetworkedUnitType[MAX_PATH];
	CHandle<CHL2WarsPlayer> m_hOldCommander;
	// Target unit/produced new unit: blink
	bool					m_bIsBlinking;
	float					m_fBlinkTimeOut;

	// Energy
	int						m_iEnergy;
	int						m_iMaxEnergy;
#endif // CLIENT_DLL

	CNetworkHandle( CBaseEntity, m_hSquadUnit );
	CNetworkHandle (CHL2WarsPlayer, m_hCommander); 	// the player in charge of this unit
};

inline CBaseEntity *CFuncUnit::GetSquad() 
{ 
	return m_hSquadUnit.Get(); 
}

inline int CFuncUnit::GetSelectionPriority()
{
	return m_iSelectionPriority;
}

inline void CFuncUnit::SetSelectionPriority( int priority )
{
	m_iSelectionPriority = priority;
}

inline int CFuncUnit::GetAttackPriority()
{
	return m_iAttackPriority;
}

inline void CFuncUnit::SetAttackPriority( int priority )
{
	m_iAttackPriority = priority;
}

#endif // WARS_FUNC_UNIT_H