//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2wars_player.h"
#include "in_buttons.h"
#include "hl2wars_gamerules.h"
#include "hl2wars_shareddefs.h"
#include "unit_base_shared.h"
#include "predicted_viewmodel.h"
#include "world.h"

#ifdef ENABLE_PYTHON
	#include "srcpy.h"
#endif // ENABLE_PYTHON

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// -------------------------------------------------------------------------------- //
// player spawn 
// -------------------------------------------------------------------------------- //
class CStrategicStart : public CPointEntity
{
public:
	DECLARE_CLASS( CStrategicStart, CPointEntity );

	DECLARE_DATADESC();

	int m_iOwner;
private:
};

BEGIN_DATADESC( CStrategicStart )
	DEFINE_KEYFIELD( m_iOwner, FIELD_INTEGER, "owner" ),
END_DATADESC()

// These are the new entry points to entities. 
LINK_ENTITY_TO_CLASS(info_player_strategic,CStrategicStart);


// -------------------------------------------------------------------------------- //
// Tables.
// -------------------------------------------------------------------------------- //

LINK_ENTITY_TO_CLASS( player, CHL2WarsPlayer );
PRECACHE_REGISTER(player);

// CHL2WarsPlayerShared Data Tables
//=============================

extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

// specific to the local player
BEGIN_SEND_TABLE_NOBASE( CHL2WarsPlayer, DT_HL2WarsLocalPlayerExclusive )
	SendPropString( SENDINFO( m_NetworkedFactionName ) ),
	SendPropEHandle		( SENDINFO( m_hControlledUnit ) ),
	SendPropExclude( "DT_HL2WarsPlayer", "m_vMouseAim" ),

	// send a hi-res origin to the local player for use in prediction
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CHL2WarsPlayer, DT_HL2WarsNonLocalPlayerExclusive )
	SendPropVector	( SENDINFO(m_vMouseAim), -1, SPROP_COORD ),

	// send a lo-res origin to other players
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
END_SEND_TABLE()

// main table
IMPLEMENT_SERVERCLASS_ST( CHL2WarsPlayer, DT_HL2WarsPlayer )
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseAnimating", "m_nNewSequenceParity" ),
	SendPropExclude( "DT_BaseAnimating", "m_nResetEventsParity" ),
	//SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	// Data that only gets sent to the local player.
	SendPropDataTable( "hl2warslocaldata", 0, &REFERENCE_SEND_TABLE(DT_HL2WarsLocalPlayerExclusive), SendProxy_SendLocalDataTable ),

	// Data that gets sent to all other players
	SendPropDataTable( "hl2warsnonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_HL2WarsNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable ),

END_SEND_TABLE()

// DATADESC
BEGIN_DATADESC( CHL2WarsPlayer )
	//DEFINE_AUTO_ARRAY( m_NetworkedFactionName,    FIELD_CHARACTER ),
END_DATADESC()

CHL2WarsPlayer::CHL2WarsPlayer() : m_nMouseButtonsPressed(0)
{
	SetViewDistance(1500.0f);
}


CHL2WarsPlayer::~CHL2WarsPlayer()
{

}

void CHL2WarsPlayer::UpdateOnRemove()
{
	SetControlledUnit(NULL);
	ClearSelection(); // Ensure selection changed signal is send

	BaseClass::UpdateOnRemove();
}


CHL2WarsPlayer *CHL2WarsPlayer::CreatePlayer( const char *className, edict_t *ed )
{
	CHL2WarsPlayer::s_PlayerEdict = ed;
	return (CHL2WarsPlayer*)CreateEntityByName( className );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::Precache()
{
	PrecacheModel("models/error.mdl");

	PrecacheParticleSystem("clicked_default");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Called everytime the player respawns
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::Spawn( void )
{
	SetModel( "models/error.mdl" );

	// Default faction if we have no faction selected yet
	if( m_FactionName == NULL_STRING )
	{
		const char *pFaction = GetDefaultFaction() ? GetDefaultFaction() : "rebels" ;
		ChangeFaction(pFaction);
	}

	BaseClass::Spawn();

	UpdateCameraSettings();

	m_Local.m_iHideHUD |= HIDEHUD_UNIT;
	SetStrategicMode( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CHL2WarsPlayer::UpdateTransmitState()
{
	if ( GetControlledUnit() )
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	return BaseClass::UpdateTransmitState();
}

//-----------------------------------------------------------------------------
// Purpose: Retrieve camera settings
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::UpdateCameraSettings()
{
	m_fCamSpeed = atof( engine->GetClientConVarValue( entindex(), "cl_strategic_cam_speed" ) );
	m_fCamAcceleration = atof( engine->GetClientConVarValue( entindex(), "cl_strategic_cam_accelerate" ) );
	m_fCamStopSpeed = atof( engine->GetClientConVarValue( entindex(), "cl_strategic_cam_stopspeed" ) );
	m_fCamFriction = atof( engine->GetClientConVarValue( entindex(), "cl_strategic_cam_friction" ) );

	SetMaxSpeed( m_fCamSpeed );

	//Msg("Camera settings: %f %f %f %f\n", m_fCamSpeed, m_fCamAcceleration, m_fCamStopSpeed, m_fCamFriction );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::PreThink( void )
{
	BaseClass::PreThink();

	//NDebugOverlay::Box( GetAbsOrigin(), Vector(-16, -16, -16), Vector(16, 24, 16), 0, 255, 0, 255, 1.0f);
	//Msg("Mouse aim is: %f %f %f\n", GetMouseAim().x, GetMouseAim().y, GetMouseAim().z );
	//DebugDrawLine( Weapon_ShootPosition()+GetCameraOffset(),  Weapon_ShootPosition()+GetCameraOffset()+GetMouseAim()*4096, 0, 255, 0, true, 5.0f );

	// The mouse actions usually involve some abilities or units that are not predicted by the player
	// They result in effects, and we want to see the effects :)
	IPredictionSystem::SuppressHostEvents( NULL );

	// Update mouse actions
	UpdateSelection();
	CleanupGroups();
	UpdateControl();

	// Restore 
	IPredictionSystem::SuppressHostEvents( this );
}

CBaseEntity *GetEntityFromMouseCommand( int idx )
{
	if( idx == -1 || idx >= MAX_EDICTS ) 
		return NULL;
	if( idx == 0 )
		return GetWorldEntity();
	
	return UTIL_EntityByIndex( idx );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CHL2WarsPlayer::ClientCommand( const CCommand &args )
{
	// FIXME: Most commands do not check the number of args.
	//        (Although this doesn't really matter since an arg out of range is just an empty string...).
	if( GetControlledUnit() && GetControlledUnit()->GetIUnit() )
	{
		if( GetControlledUnit()->GetIUnit()->ClientCommand( args ) )
			return true;
	}

	if( !Q_stricmp( args[0], "player_clearselection" ) )
	{
		ClearSelection();
		return true;
	}
	else if( !Q_stricmp( args[0], "player_addunit" ) )
	{
		for( int i = 1; i < args.ArgC(); i++ )
		{
#ifdef CLIENTSENDEHANDLE
			long iEncodedEHandle = atol(args[i]);
			int iSerialNum = (iEncodedEHandle >> MAX_EDICT_BITS);
			int iEntryIndex = iEncodedEHandle & ~(iSerialNum << MAX_EDICT_BITS);
			EHANDLE pEnt( iEntryIndex, iSerialNum );
#else
			CBaseEntity *pEnt = UTIL_EntityByIndex( atoi(args[i]) );
#endif // CLIENTSENDEHANDLE
			//DevMsg( "player_addunit: selecting ent #%d\n",  atoi(args[i]) );
			if( pEnt && pEnt->IsAlive() && pEnt->GetIUnit() )
				pEnt->GetIUnit()->Select(this);
			else
				DevMsg( "player_addunit: tried to select an invalid unit (#%d)\n",  atoi(args[i]) );
		}
		return true;
	}
	else if( !Q_stricmp( args[0], "player_removeunit" ) )
	{
		if( args.ArgC() < 2 )
		{
			Warning( "player_removeunit: not enough arguments (only %d provided, while 2 needed)\n", args.ArgC() );
			return true;
		}

#ifdef CLIENTSENDEHANDLE
		long iEncodedEHandle = atol(args[1]);
		int iSerialNum = (iEncodedEHandle >> MAX_EDICT_BITS);
		int iEntryIndex = iEncodedEHandle & ~(iSerialNum << MAX_EDICT_BITS);
		EHANDLE pEnt( iEntryIndex, iSerialNum );
#else
		CBaseEntity *pEnt = UTIL_EntityByIndex( atoi(args[1]) );
#endif // CLIENTSENDEHANDLE
		
		if( pEnt && pEnt->IsAlive() && pEnt->GetIUnit() )
		{
			int idx = FindUnit(pEnt);
			if( idx != -1 )
			{
				RemoveUnit(idx);
			}
		}
		return true;
	}
	else if( !Q_stricmp( args[0], "make_group" ) )
	{
		if( args.ArgC() < 3 )
		{
			Warning( "select_group: not enough arguments (only %d provided, while 3 needed)\n", args.ArgC() );
			return true;
		}
		MakeCurrentSelectionGroup(atoi(args[1]), (bool)atoi(args[2]) );
		return true;
	}
	else if( !Q_stricmp( args[0], "select_group" ) )
	{
		if( args.ArgC() < 2 )
		{
			Warning( "select_group: not enough arguments (only %d provided, while 2 needed)\n", args.ArgC() );
			return true;
		}
		SelectGroup(atoi(args[1]));
		return true;
	}
	else if( !Q_stricmp( args[0], "player_orderunits" ) )
	{
		if( args.ArgC() < 7 )
		{
			Warning( "player_orderunits: not enough arguments (only %d provided, while 7 needed)\n", args.ArgC() );
			return true;
		}

		MouseTraceData_t mousedata;
		mousedata.m_vStartPos = Vector(atof(args[1]), atof(args[2]), atof(args[3]));
		mousedata.m_vEndPos = Vector(atof(args[4]), atof(args[5]), atof(args[6]));
		mousedata.m_vWorldOnlyEndPos = mousedata.m_vEndPos;

		int idx = atol(args[7]);
		mousedata.m_hEnt = GetEntityFromMouseCommand( idx );

		m_MouseData = mousedata;
		m_MouseDataRightPressed = mousedata;
		m_MouseDataRightReleased = mousedata;
		
		OrderUnits();

		return true;
	}
	else if( !Q_stricmp( args[0], "minimap_lm" ) )
	{
		if( args.ArgC() < 7 )
		{
			Warning( "minimap_lm: not enough arguments (only %d provided, while 7 needed)\n", args.ArgC() );
			return true;
		}

		MouseTraceData_t mousedata;
		mousedata.m_vStartPos = Vector(atof(args[1]), atof(args[2]), atof(args[3]));
		mousedata.m_vEndPos = Vector(atof(args[4]), atof(args[5]), atof(args[6]));
		mousedata.m_vWorldOnlyEndPos = mousedata.m_vEndPos;

		int idx = atol(args[7]);
		mousedata.m_hEnt = GetEntityFromMouseCommand( idx );

		m_MouseData = mousedata;
		m_MouseDataRightPressed = mousedata;
		m_MouseDataRightReleased = mousedata;
		m_MouseDataLeftPressed = mousedata;
		m_MouseDataLeftReleased = mousedata;

#ifdef ENABLE_PYTHON
		CUtlVector<bp::object> activeAbilities;
		activeAbilities = m_vecActiveAbilities;
		for(int i=0; i< activeAbilities.Count(); i++)
		{
			SrcPySystem()->RunT<bool, MouseTraceData_t>( SrcPySystem()->Get("OnMinimapClick", activeAbilities[i]), false, mousedata );
		}
#endif // ENABLE_PYTHON

		return true;
	}
	else if( !Q_stricmp( args[0], "player_lmp" ) )
	{
		if( args.ArgC() < 2 )
		{
			Warning( "player_lmp: not enough arguments (only %d provided, while 2 needed)\n", args.ArgC() );
			return true;
		}

		MouseTraceData_t mousedata = GetMouseData();

		int idx = atol(args[1]);
		mousedata.m_hEnt = GetEntityFromMouseCommand( idx );

		OnLeftMouseButtonPressedInternal( mousedata );
		return true;
	}
	else if( !Q_stricmp( args[0], "player_lmdp" ) )
	{
		if( args.ArgC() < 2 )
		{
			Warning( "player_lmdp: not enough arguments (only %d provided, while 2 needed)\n", args.ArgC() );
			return true;
		}

		MouseTraceData_t mousedata = GetMouseData();

		int idx = atol(args[1]);
		mousedata.m_hEnt = GetEntityFromMouseCommand( idx );

		OnLeftMouseButtonDoublePressedInternal( mousedata );
		return true;
	}
	else if( !Q_stricmp( args[0], "player_lmr" ) )
	{
		if( args.ArgC() < 2 )
		{
			Warning( "player_lmr: not enough arguments (only %d provided, while 2 needed)\n", args.ArgC() );
			return true;
		}

		MouseTraceData_t mousedata = GetMouseData();

		int idx = atol(args[1]);
		mousedata.m_hEnt = GetEntityFromMouseCommand( idx );

		OnLeftMouseButtonReleasedInternal( mousedata );
		return true;
	}
	else if( !Q_stricmp( args[0], "player_rmp" ) )
	{
		if( args.ArgC() < 2 )
		{
			Warning( "player_rmp: not enough arguments (only %d provided, while 2 needed)\n", args.ArgC() );
			return true;
		}

		MouseTraceData_t mousedata = GetMouseData();

		int idx = atol(args[1]);
		mousedata.m_hEnt = GetEntityFromMouseCommand( idx );

		OnRightMouseButtonPressedInternal( mousedata );
		return true;
	}
	else if( !Q_stricmp( args[0], "player_rmdp" ) )
	{
		if( args.ArgC() < 2 )
		{
			Warning( "player_rmdp: not enough arguments (only %d provided, while 2 needed)\n", args.ArgC() );
			return true;
		}

		MouseTraceData_t mousedata = GetMouseData();

		int idx = atol(args[1]);
		mousedata.m_hEnt = GetEntityFromMouseCommand( idx );

		OnRightMouseButtonDoublePressedInternal( mousedata );
		return true;
	}
	else if( !Q_stricmp( args[0], "player_rmr" ) )
	{
		if( args.ArgC() < 2 )
		{
			Warning( "player_rmr: not enough arguments (only %d provided, while 2 needed)\n", args.ArgC() );
			return true;
		}

		MouseTraceData_t mousedata = GetMouseData();

		int idx = atol(args[1]);
		mousedata.m_hEnt = GetEntityFromMouseCommand( idx );

		OnRightMouseButtonReleasedInternal( mousedata );
		return true;
	}
	else if( !Q_stricmp( args[0], "player_cm" ) )
	{
		ClearMouse();
		return true;
	}
	else if( !Q_stricmp( args[0], "player_camerasettings" ) )
	{
		UpdateCameraSettings();
		return true;
	}
	else if( !Q_stricmp( args[0], "player_release_control_unit" ) )
	{
		SetControlledUnit( NULL );
		return true;
	}
	else if( !Q_stricmp( args[0], "spectate" ) )
	{
		if ( GetTeamNumber() == TEAM_SPECTATOR )
			return true;

		ChangeTeam( TEAM_SPECTATOR );
		SetOwnerNumber( 0 );
		//SetObserverMode( OBS_MODE_ROAMING );
		//m_afPhysicsFlags |= PFLAG_OBSERVER;
		//SetMoveType( MOVETYPE_STRATEGIC );

		return true;
	}

	return BaseClass::ClientCommand(args);
}

//-----------------------------------------------------------------------------
// Purpose: Check the mouse buttons and make actions
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::UpdateControl( void )
{
	// Check left mouse button
	if( ( m_nButtons & IN_MOUSELEFT) && !( m_nMouseButtonsPressed & IN_MOUSELEFT ) ) 
	{
		m_nMouseButtonsPressed |= IN_MOUSELEFT;

		//OnLeftMouseButtonPressed( m_MouseData );
	}
	else if( (m_nButtons & IN_MOUSELEFTDOUBLE) && !( m_nMouseButtonsPressed & IN_MOUSELEFTDOUBLE ) ) 
	{
		//OnLeftMouseButtonDoublePressed( m_MouseData );

		m_nMouseButtonsPressed |= IN_MOUSELEFTDOUBLE;
	}
	else if( !(m_nButtons & IN_MOUSELEFT) && ( m_nMouseButtonsPressed & IN_MOUSELEFT ) ) 
	{	
		//OnLeftMouseButtonReleased( m_MouseData );

		m_nMouseButtonsPressed &= ~IN_MOUSELEFT;
	}
	else if( !(m_nButtons & IN_MOUSELEFTDOUBLE) && ( m_nMouseButtonsPressed & IN_MOUSELEFTDOUBLE ) )
	{	
		//OnLeftMouseButtonReleased( m_MouseData );

		m_nMouseButtonsPressed &= ~IN_MOUSELEFTDOUBLE;
	}
	// Check right mouse button
	if( (m_nButtons & IN_MOUSERIGHT) && !(m_nMouseButtonsPressed & IN_MOUSERIGHT)) 
	{
		m_nMouseButtonsPressed |= IN_MOUSERIGHT;

		//OnRightMouseButtonPressed( m_MouseData );
	}
	else if( (m_nButtons & IN_MOUSELEFTDOUBLE) && !( m_nMouseButtonsPressed & IN_MOUSELEFTDOUBLE ) ) 
	{
		//OnRightMouseButtonDoublePressed( m_MouseData );

		m_nMouseButtonsPressed |= IN_MOUSERIGHTDOUBLE;
	}
	else if(!(m_nButtons & IN_MOUSERIGHT) && (m_nMouseButtonsPressed & IN_MOUSERIGHT) ) 
	{
		//OnRightMouseButtonReleased( m_MouseData );

		m_nMouseButtonsPressed &= ~IN_MOUSERIGHT;
	}
	else if(!(m_nButtons & IN_MOUSERIGHTDOUBLE) && (m_nMouseButtonsPressed & IN_MOUSERIGHTDOUBLE) ) 
	{
		//OnRightMouseButtonReleased( m_MouseData );

		m_nMouseButtonsPressed &= ~IN_MOUSERIGHTDOUBLE;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::SetStrategicMode( bool state )
{
	// Tell old unit we left control
	if( m_hControlledUnit.Get() )
	{
		m_hControlledUnit->GetIUnit()->OnUserLeftControl( this );
		m_hControlledUnit->GetIUnit()->SetCommander(NULL);
		m_hControlledUnit = NULL;
		m_Local.m_iHideHUD |= HIDEHUD_UNIT;
	}

	if( state )
	{
		SetMoveType( MOVETYPE_STRATEGIC );
		AddEffects( EF_NODRAW );
		m_Local.m_iHideHUD &= ~HIDEHUD_STRATEGIC;
		m_Local.m_iHideHUD |= HIDEHUD_CROSSHAIR;
		SetGroundEntity( (CBaseEntity *)NULL );
		AddFlag( FL_FLY );
		m_takedamage = DAMAGE_NO;					// take no damage

		// holster weapon
		if ( GetActiveWeapon() )
			GetActiveWeapon()->Holster();

		//m_afPhysicsFlags |= PFLAG_OBSERVER;
		RemoveFlag( FL_DUCKING );
		AddSolidFlags( FSOLID_NOT_SOLID );
		RemoveFOWFlags(FOWFLAG_ALL_MASK);
	}
	else
	{
		SetMoveType( MOVETYPE_WALK );
		RemoveEffects( EF_NODRAW );
		m_Local.m_iHideHUD &= ~HIDEHUD_CROSSHAIR;
		m_Local.m_iHideHUD |= HIDEHUD_STRATEGIC;
		RemoveFlag( FL_FLY );						// nope, we don't fly.
		m_takedamage = DAMAGE_YES;					// become a mortal person again.	
		SetCollisionGroup( COLLISION_GROUP_PLAYER );// normal collision for player
		RemoveSolidFlags( FSOLID_NOT_SOLID );
		//m_afPhysicsFlags &= ~PFLAG_OBSERVER;
		AddFOWFlags(FOWFLAG_UNITS_MASK);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::SetControlledUnit( CBaseEntity *pUnit )
{
	IUnit *pIUnit = NULL;
	if( pUnit ) {
		// Verify that if we have an unit that it is an unit.
		pIUnit = pUnit->GetIUnit();
		if( !pIUnit )
		{
			Warning("SetControlledUnit: invalid unit\n");
			return;
		}
	}

	// Test if something changed
	if( pUnit && m_hControlledUnit.Get() == pUnit )
		return;

#ifdef ENABLE_PYTHON
	// Setup dict for sending a signal
	bp::dict kwargs;
	kwargs["sender"] = bp::object();
	kwargs["player"] = GetPyHandle();
#endif // ENABLE_PYTHON

	// Tell old unit we left control
	if( m_hControlledUnit.Get() )
	{
		m_hControlledUnit->GetIUnit()->OnUserLeftControl( this );
		m_hControlledUnit->GetIUnit()->SetCommander(NULL);
		m_Local.m_iHideHUD |= HIDEHUD_UNIT;
		HideViewModels();

#ifdef ENABLE_PYTHON
		kwargs["unit"] = m_hControlledUnit->GetPyHandle();
		bp::object signal = SrcPySystem()->Get( "playerleftcontrolunit", "core.signals", true );
		SrcPySystem()->CallSignal( signal, kwargs );
#endif // ENABLE_PYTHON

		// Dispatch event
		IGameEvent * event = gameeventmanager->CreateEvent( "player_leftcontrol_unit" );
		if ( event )
		{
			event->SetInt( "userid", GetUserID() );
			event->SetInt( "entindex", m_hControlledUnit ? m_hControlledUnit->entindex() : -1 );
			gameeventmanager->FireEvent( event );
		}

		m_hControlledUnit = NULL;
	}

	// Check if the new unit allows us to control
	if( pIUnit && pIUnit->CanUserControl(this) )
	{
		SetStrategicMode( true );			// Makes m_hControlledUnit NULL
		SetMoveType( MOVETYPE_WALK );

		// Set unit and setup player
		m_hControlledUnit = pUnit;


		if( m_hControlledUnit )
		{
			CUnitBase *pUnit = m_hControlledUnit->MyUnitPointer();
			if( pUnit->GetActiveWeapon() )
			{
				pUnit->GetActiveWeapon()->Deploy();
				//pUnit->GetActiveWeapon()->SetViewModel();
				pUnit->GetActiveWeapon()->SetModel( pUnit->GetActiveWeapon()->GetViewModel() );
			}
		}

		// Tell new unit we gained control
		pIUnit->OnUserControl( this );
		pIUnit->SetCommander( this );
		m_Local.m_iHideHUD |= HIDEHUD_STRATEGIC;
		m_Local.m_iHideHUD &= ~HIDEHUD_UNIT;
		m_Local.m_iHideHUD &= ~HIDEHUD_CROSSHAIR;

#ifdef ENABLE_PYTHON
		// Dispatch signal
		kwargs["unit"] = m_hControlledUnit->GetPyHandle();
		bp::object signal = SrcPySystem()->Get( "playercontrolunit", "core.signals", true );
		SrcPySystem()->CallSignal( signal, kwargs );
#endif // ENABLE_PYTHON

		// Dispatch event
		IGameEvent * event = gameeventmanager->CreateEvent( "player_control_unit" );
		if ( event )
		{
			event->SetInt( "userid", GetUserID() );
			event->SetInt( "entindex", m_hControlledUnit ? m_hControlledUnit->entindex() : -1 );
			gameeventmanager->FireEvent( event );
		}
	}
	else
	{
		SetStrategicMode( true );
	}

	DispatchUpdateTransmitState();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::InputSetCameraFollowEntity( inputdata_t &inputdata )
{

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::VPhysicsShadowUpdate( IPhysicsObject *pPhysics )
{
	// No need for this in strategic mode and would require a special version.
	if( IsStrategicModeOn() )
		return;

	BaseClass::VPhysicsShadowUpdate( pPhysics );
}

void CHL2WarsPlayer::CreateViewModel( int index /*=0*/ )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel( index ) )
		return;

	CPredictedViewModel *vm = ( CPredictedViewModel * )CreateEntityByName( "predicted_viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( index, vm );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
extern ConVar VisForce;
void CHL2WarsPlayer::SetupVisibility( CBaseEntity *pViewEntity, unsigned char *pvs, int pvssize )
{
	if( !IsStrategicModeOn() )
	{
		BaseClass::SetupVisibility( pViewEntity, pvs, pvssize );
		return;
	}

	Vector org;

	// If we have a view entity, we don't add the player's origin.
	if ( !pViewEntity || VisForce.GetBool() )
	{
		org = Weapon_ShootPosition() + GetCameraOffset();
		engine->AddOriginToPVS( org );
	}

	// Merge in PVS from split screen players
	for ( int i = 1; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		CHL2WarsPlayer *pl = (CHL2WarsPlayer *)ToHL2WarsPlayer( GetContainingEntity( engine->GetSplitScreenPlayerForEdict( entindex(), i ) ) );
		if ( !pl )
			continue;
		org = pl->Weapon_ShootPosition() + pl->GetCameraOffset();
		engine->AddOriginToPVS( org );
	}
}

//================================================================================
// TEAM HANDLING
//================================================================================
//-----------------------------------------------------------------------------
// Purpose: Put the player in the specified team
//-----------------------------------------------------------------------------
#if 0
void CHL2WarsPlayer::ChangeTeam( int iTeamNum, bool bAutoTeam, bool bSilent)
{
	// if this is our current team, just abort
	if ( iTeamNum == GetTeamNumber() )
	{
		return;
	}

	// Immediately tell all clients that he's changing team. This has to be done
	// first, so that all user messages that follow as a result of the team change
	// come after this one, allowing the client to be prepared for them.
	// FIXME: Do not send. Requires CTeam instance. Decide wheter we use it or not.
	/*
	IGameEvent * event = gameeventmanager->CreateEvent( "player_team" );
	if ( event )
	{
		event->SetInt("userid", GetUserID() );
		event->SetInt("team", iTeamNum );
		event->SetInt("oldteam", GetTeamNumber() );
		event->SetInt("disconnect", IsDisconnecting());
		event->SetInt("autoteam", bAutoTeam );
		event->SetInt("silent", bSilent );
		event->SetString("name", GetPlayerName() );

		gameeventmanager->FireEvent( event );
	}
	*/

	CBaseCombatCharacter::ChangeTeam( iTeamNum );
}
#endif // 0
