//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's C_BaseCombatCharacter entity
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basecombatcharacter.h"

#include "gamestringpool.h"
#ifdef USE_NAV_MESH
#include "nav_mesh.h"
#endif // USE_NAV_MESH
#include "vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( CBaseCombatCharacter )
#undef CBaseCombatCharacter	
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::C_BaseCombatCharacter()
{
	for ( int i=0; i < m_iAmmo.Count(); i++ )
		m_iAmmo.Set( i, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::~C_BaseCombatCharacter()
{
}

/*
//-----------------------------------------------------------------------------
// Purpose: Returns the amount of ammunition of the specified type the character's carrying
//-----------------------------------------------------------------------------
int	C_BaseCombatCharacter::GetAmmoCount( char *szName ) const
{
	return GetAmmoCount( g_pGameRules->GetAmmoDef()->Index(szName) );
}
*/

//-----------------------------------------------------------------------------
// Purpose: Overload our muzzle flash and send it to any actively held weapon
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::DoMuzzleFlash()
{
	// Our weapon takes our muzzle flash command
	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
	{
		pWeapon->DoMuzzleFlash();
		//NOTENOTE: We do not chain to the base here
	}
	else
	{
		BaseClass::DoMuzzleFlash();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get shoot position of BCC at current position/orientation
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector C_BaseCombatCharacter::Weapon_ShootPosition( )
{
	Vector forward, right, up;

	AngleVectors( GetAbsAngles(), &forward, &right, &up );

	Vector vecSrc = GetAbsOrigin() 
					+ forward * m_HackedGunPos.y 
					+ right * m_HackedGunPos.x 
					+ up * m_HackedGunPos.z;

	return vecSrc;
}

#ifdef USE_NAV_MESH
//-----------------------------------------------------------------------------
// Purpose: Invoke this to update our last known nav area 
// (since there is no think method chained to CBaseCombatCharacter)
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::UpdateLastKnownArea( void )
{
	VPROF_BUDGET( "CBaseCombatCharacter::UpdateLastKnownArea", "NextBot" );

	if ( TheNavMesh->IsGenerating() )
	{
		ClearLastKnownArea();
		return;
	}

	// find the area we are directly standing in
	CNavArea *area = TheNavMesh->GetNearestNavArea( this, GETNAVAREA_CHECK_GROUND | GETNAVAREA_CHECK_LOS, 50.0f );
	if ( !area )
		return;

	// make sure we can actually use this area - if not, consider ourselves off the mesh
	if ( !IsAreaTraversable( area ) )
		return;

	if ( area != m_lastNavArea )
	{
		// player entered a new nav area
		if ( m_lastNavArea )
		{
			m_lastNavArea->DecrementPlayerCount( m_registeredNavTeam, entindex() );
			m_lastNavArea->OnExit( this, area );
		}

		m_registeredNavTeam = GetTeamNumber();
		area->IncrementPlayerCount( m_registeredNavTeam, entindex() );
		area->OnEnter( this, m_lastNavArea );

		OnNavAreaChanged( area, m_lastNavArea );

		m_lastNavArea = area;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Return true if we can use (walk through) the given area 
//-----------------------------------------------------------------------------
bool C_BaseCombatCharacter::IsAreaTraversable( const CNavArea *area ) const
{
	return area ? !area->IsBlocked( GetTeamNumber() ) : false;
}


//-----------------------------------------------------------------------------
// Purpose: Leaving the nav mesh
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::ClearLastKnownArea( void )
{
	OnNavAreaChanged( NULL, m_lastNavArea );

	if ( m_lastNavArea )
	{
		m_lastNavArea->DecrementPlayerCount( m_registeredNavTeam, entindex() );
		m_lastNavArea->OnExit( this, NULL );
		m_lastNavArea = NULL;
		m_registeredNavTeam = TEAM_INVALID;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handling editor removing the area we're standing upon
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::OnNavAreaRemoved( CNavArea *removedArea )
{
	if ( m_lastNavArea == removedArea )
	{
		ClearLastKnownArea();
	}
}
#endif // USE_NAV_MESH

IMPLEMENT_CLIENTCLASS(C_BaseCombatCharacter, DT_BaseCombatCharacter, CBaseCombatCharacter);

// Only send active weapon index to local player
BEGIN_RECV_TABLE_NOBASE( C_BaseCombatCharacter, DT_BCCLocalPlayerExclusive )
	RecvPropTime( RECVINFO( m_flNextAttack ) ),


	RecvPropArray3( RECVINFO_ARRAY(m_hMyWeapons), RecvPropEHandle( RECVINFO( m_hMyWeapons[0] ) ) ),


END_RECV_TABLE();


BEGIN_RECV_TABLE(C_BaseCombatCharacter, DT_BaseCombatCharacter)
	RecvPropDataTable( "bcc_localdata", 0, 0, &REFERENCE_RECV_TABLE(DT_BCCLocalPlayerExclusive) ),
	RecvPropEHandle( RECVINFO( m_hActiveWeapon ) ),





END_RECV_TABLE()


BEGIN_PREDICTION_DATA( C_BaseCombatCharacter )

	DEFINE_PRED_ARRAY( m_iAmmo, FIELD_INTEGER,  MAX_AMMO_TYPES, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flNextAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hActiveWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_ARRAY( m_hMyWeapons, FIELD_EHANDLE, MAX_WEAPONS, FTYPEDESC_INSENDTABLE ),

END_PREDICTION_DATA()
