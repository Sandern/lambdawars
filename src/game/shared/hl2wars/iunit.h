//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Any entity that implements this interface is controlable by the player
//
// $NoKeywords: $
//=============================================================================//

#ifndef IUNIT_H
#define IUNIT_H

#ifdef _WIN32
#pragma once
#endif

#include "imouse.h"

#ifdef ENABLE_PYTHON
#include <boost/python.hpp>
#endif // ENABLE_PYTHON

class CHL2WarsPlayer;

abstract_class IUnit : public IMouse
{
public:
	virtual IUnit *GetIUnit() { return this; }

	// Type
	virtual const char *GetUnitType()														= 0;
#ifndef CLIENT_DLL
	virtual void		SetUnitType( const char *unit_type )								= 0;
#endif 

	// Selection
#ifdef ENABLE_PYTHON
	virtual bool IsSelectableByPlayer( CHL2WarsPlayer *pPlayer, boost::python::object targetselection = boost::python::object() ) = 0;
#endif // ENABLE_PYTHON
	virtual void Select( CHL2WarsPlayer *pPlayer, bool bTriggerOnSel = true )				= 0;
	virtual void OnSelected( CHL2WarsPlayer *pPlayer )										= 0;
	virtual void OnDeSelected( CHL2WarsPlayer *pPlayer )									= 0;
#ifdef CLIENT_DLL
	virtual void OnInSelectionBox( void )													= 0;
	virtual void OnOutSelectionBox( void )													= 0;
#endif // CLIENT_DLL
	virtual int GetSelectionPriority( void )												= 0;
	virtual int GetAttackPriority( void )													= 0;

	// Action
	virtual void Order( CHL2WarsPlayer *pPlayer )											= 0;

	// Squads
	virtual  CBaseEntity *GetSquad()														= 0;
#ifndef CLIENT_DLL
	virtual void SetSquad( CBaseEntity *pUnit )												= 0;
#endif // CLIENT_DLL

	// Unit is player controlled and player wants to move
	// Translate the usercmd into the unit movement
	virtual void UserCmd( CUserCmd *pCmd )													= 0;
	virtual void OnUserControl( CHL2WarsPlayer *pPlayer )									= 0;
	virtual void OnUserLeftControl( CHL2WarsPlayer *pPlayer )								= 0;
	virtual bool CanUserControl( CHL2WarsPlayer *pPlayer )									= 0;
	virtual void OnButtonsChanged( int buttonsMask, int buttonsChanged )					= 0;

#ifdef CLIENT_DLL
	virtual bool SelectSlot( int slot )														= 0;
#else
	virtual bool ClientCommand( const CCommand &args )										= 0;
#endif // CLIENT_DLL

#ifndef CLIENT_DLL
	virtual void SetCommander(CHL2WarsPlayer *player)										= 0;
#endif // CLIENT_DLL
	virtual CHL2WarsPlayer* GetCommander() const											= 0;

	// Relationships
	virtual Disposition_t		IRelationType( CBaseEntity *pTarget )						= 0;
	virtual int					IRelationPriority( CBaseEntity *pTarget )					= 0;

	// Main function for testing if this unit blocks LOS or whether an attack can pass this unit
	virtual bool				AreAttacksPassable( CBaseEntity *pTarget )					= 0;

	// Special code for moving to target buildings
	virtual bool				HasEnterOffset( void )										= 0;
	virtual const Vector &		GetEnterOffset( void )										= 0;
};

#endif // IUNIT_H
