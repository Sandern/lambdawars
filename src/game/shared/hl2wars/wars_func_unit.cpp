//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose:		Shared functions for the hl2wars player
//
//=============================================================================//

#include "cbase.h"
#include "wars_func_unit.h"
#include "unit_base_shared.h"
#include "iunit.h"

#include "gamestringpool.h"

#ifdef CLIENT_DLL
	#include "c_hl2wars_player.h"
	#include "model_types.h"
	#include "wars_vgui_screen.h"
#else
	#include "hl2wars_player.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Func unit list
CFuncUnitArray g_FuncUnitList;

// Network Table
IMPLEMENT_NETWORKCLASS_ALIASED( FuncUnit, DT_FuncUnit )

BEGIN_NETWORK_TABLE(CFuncUnit, DT_FuncUnit)
#if defined( CLIENT_DLL )
	RecvPropString(  RECVINFO( m_NetworkedUnitType ) ),

	RecvPropInt		(RECVINFO(m_iHealth)),
	RecvPropInt		(RECVINFO(m_iMaxHealth)),
	RecvPropInt		(RECVINFO( m_lifeState)),

	RecvPropEHandle		( RECVINFO( m_hSquadUnit ) ),
	RecvPropEHandle		( RECVINFO( m_hCommander ) ),

	RecvPropInt		(RECVINFO(m_iEnergy)),
	RecvPropInt		(RECVINFO(m_iMaxEnergy)),
	RecvPropInt		(RECVINFO(m_iKills)),
#else
	SendPropString( SENDINFO( m_NetworkedUnitType ) ),

	SendPropInt		(SENDINFO(m_iHealth), 15, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_iMaxHealth), 15, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO( m_lifeState ), 3, SPROP_UNSIGNED ),

	SendPropEHandle		( SENDINFO( m_hSquadUnit ) ),
	SendPropEHandle		( SENDINFO( m_hCommander ) ),

	SendPropInt		(SENDINFO(m_iEnergy), 15, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_iMaxEnergy), 15, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_iKills), 9, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()


// This class is exposed in python and networkable
#ifdef CLIENT_DLL
	IMPLEMENT_PYCLIENTCLASS( C_FuncUnit, PN_FUNCUNIT )
#else
	IMPLEMENT_PYSERVERCLASS( CFuncUnit, PN_FUNCUNIT );
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncUnit::CFuncUnit() : m_bCanBeSeen(true)
{
	g_FuncUnitList.AddToTail(this);
#ifndef CLIENT_DLL
	DensityMap()->SetType( DENSITY_GAUSSIANECLIPSE );
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncUnit::~CFuncUnit()
{
	int i = g_FuncUnitList.Find( this );
	if ( i != -1 )
		g_FuncUnitList.FastRemove( i );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncUnit::Spawn()
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CFuncUnit::GetUnitType()
{
	return STRING(m_UnitType);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHL2WarsPlayer* CFuncUnit::GetCommander() const
{
	return dynamic_cast<CHL2WarsPlayer*>(m_hCommander.Get());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncUnit::UpdateOnRemove( void )
{
	int i;

	i = g_FuncUnitList.Find( this );
	if ( i != -1 )
		g_FuncUnitList.FastRemove( i );
	
	for( i=0; i < m_SelectedByPlayers.Count(); i++ )
	{
		if( m_SelectedByPlayers[i] )
			m_SelectedByPlayers[i]->RemoveUnit( this );
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncUnit::Select( CHL2WarsPlayer *pPlayer, bool bTriggerSel )
{
	if( !pPlayer )
		return;
	pPlayer->AddUnit(this, bTriggerSel);
}

void CFuncUnit::OnSelected( CHL2WarsPlayer *pPlayer )
{
	m_SelectedByPlayers.AddToTail( pPlayer );
}

void CFuncUnit::OnDeSelected( CHL2WarsPlayer *pPlayer )
{
	m_SelectedByPlayers.FindAndRemove( pPlayer );
}

#ifdef CLIENT_DLL
extern void DrawHealthBar( CBaseEntity *pUnit, int offsetx=0, int offsety=0 );
//-----------------------------------------------------------------------------
// Purpose: Default on hover paints the health bar
//-----------------------------------------------------------------------------
void CFuncUnit::OnHoverPaint()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void CFuncUnit::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	// Check if the player's faction changed ( Might want to add a string table )
	if( m_UnitType == NULL_STRING || Q_strncmp( STRING(m_UnitType), m_NetworkedUnitType, MAX_PATH ) != 0 ) 
	{
		const char *pOldUnitType = STRING(m_UnitType);
		m_UnitType = AllocPooledString(m_NetworkedUnitType);
		OnUnitTypeChanged(pOldUnitType);
	}

	// Check change commander
	if( m_hOldCommander != m_hCommander )
	{
		UpdateVisibility();
		m_hOldCommander = m_hCommander;
	}
}

int CFuncUnit::DrawModel( int flags, const RenderableInstance_t &instance )
{
	if( m_bIsBlinking )
	{
		flags |= STUDIO_ITEM_BLINK;
		if( m_fBlinkTimeOut < gpGlobals->curtime )
			m_bIsBlinking = false;
	}

	return BaseClass::DrawModel( flags, instance );
}

void CFuncUnit::Blink( float blink_time )
{
	m_bIsBlinking = true;
	m_fBlinkTimeOut = blink_time != -1 ? gpGlobals->curtime + blink_time : -1;
}
#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncUnit::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "unittype" ) )
	{
		SetUnitType( szValue );
		return true;
	}
	return BaseClass::KeyValue( szKeyName, szValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncUnit::SetUnitType( const char *unit_type )
{
	Q_strcpy( m_NetworkedUnitType.GetForModify(), unit_type );
	const char *pOldUnitType = STRING(m_UnitType);
	m_UnitType = AllocPooledString(unit_type);
	OnUnitTypeChanged(pOldUnitType);
}

//-----------------------------------------------------------------------------
// Purpose: sets which player commands this unit
//-----------------------------------------------------------------------------
void CFuncUnit::SetCommander( CHL2WarsPlayer *player )
{
	if ( m_hCommander.Get() == player )
	{
		return;
	}

	m_hCommander = player;
}

#endif // CLIENT_DLL


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Disposition_t CFuncUnit::IRelationType( CBaseEntity *pTarget )
{
	if ( pTarget )
	{
#if 0
		// First check for specific relationship with this edict
		for (int i=0;i<m_Relationship.Count();i++) 
		{
			if (pTarget == m_Relationship[i].entity) 
			{
				return m_Relationship[i].disposition;
			}
		}
#endif // 0

		// Global relationships between teams
		return GetPlayerRelationShip( GetOwnerNumber(), pTarget->GetOwnerNumber() );
	}
	return D_ER;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CFuncUnit::IRelationPriority( CBaseEntity *pTarget )
{
	return 0;
}