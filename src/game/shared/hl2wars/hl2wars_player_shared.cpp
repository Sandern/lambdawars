//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose:		Shared functions for the hl2wars player
//
//=============================================================================//

#include "cbase.h"
#include "hl2wars_player_shared.h"
#include "debugoverlay_shared.h"

#ifdef CLIENT_DLL
	#include "hl2wars/c_hl2wars_player.h"
	#include "gamestringpool.h"
	#include "iinput.h"

	#include "vgui_controls/Controls.h"
	#include "vgui/IScheme.h"
#else
	#include "hl2wars_player.h"
	#include "hl2wars_gamerules.h"
#endif // CLIENT_DLL

#include "imouse.h"
#include "in_buttons.h"
#include "hl2wars_shareddefs.h"
#include "unit_base_shared.h"
#include "hl2wars_util_shared.h"
#include "wars_mapboundary.h"

#ifndef DISABLE_PYTHON
	#include "src_python.h"
	namespace bp = boost::python;
#endif // DISABLE_PYTHON

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
// unit group info
BEGIN_DATADESC_NO_BASE( unitgroup_t )
	DEFINE_ARRAY( m_Group,			FIELD_EHANDLE, MAX_EDICTS ),
END_DATADESC()
#endif // CLIENT_DLL

#ifdef CLIENT_DLL
	ConVar cl_mouse_selectionbox_threshold( "cl_mouse_selectionbox_threshold", "5", FCVAR_ARCHIVE );
	ConVar cl_selection_noclear("cl_selection_noclear", "1", FCVAR_ARCHIVE );

	ConVar g_debug_mouse_aim( "cl_g_debug_mouse_aim", "0" );
	ConVar g_debug_mouse_noupdate( "cl_g_debug_mouse_noupdate", "0" );
	ConVar g_debug_mouse_noradiuscheck( "cl_g_debug_mouse_noradiuscheck", "0" );

	ConVar debug_snapcamerapos("cl_debug_snapcamerapos", "0", FCVAR_CHEAT);
#else
	ConVar g_debug_mouse_aim( "g_debug_mouse_aim", "0" );
	ConVar g_debug_mouse_noupdate( "g_debug_mouse_noupdate", "0" );
	ConVar g_debug_mouse_noradiuscheck( "g_debug_mouse_noradiuscheck", "0" );

	ConVar debug_snapcamerapos("debug_snapcamerapos", "0", FCVAR_CHEAT);
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: the player's eyes go above the unit he's spectating/controlling
//-----------------------------------------------------------------------------
Vector CHL2WarsPlayer::EyePosition(void)
{
	Vector vecLastUnitOrigin = vec3_origin;
	CBaseEntity *pUnit = GetControlledUnit();
	if ( pUnit && pUnit->GetHealth() > 0 )
	{
		vecLastUnitOrigin = pUnit->EyePosition();
	}

	if ( vecLastUnitOrigin != vec3_origin )
	{
		return vecLastUnitOrigin;
	}

	return BaseClass::EyePosition();
}

class CMouseCollideList : public IEntityEnumerator
{
public:
	CMouseCollideList( Ray_t *pRay, CBaseEntity* pIgnoreEntity, int nContentsMask ) : 
		m_pIgnoreEntity( pIgnoreEntity ), m_nContentsMask( nContentsMask ), m_pRay(pRay), didhit(false) {}

	virtual bool EnumEntity( IHandleEntity *pHandleEntity )
	{
		// Don't bother with the ignore entity.
		if ( pHandleEntity == m_pIgnoreEntity )
			return true;

		Assert( pHandleEntity );

		trace_t tr;
		enginetrace->ClipRayToEntity( *m_pRay, m_nContentsMask, pHandleEntity, &tr );
		if (( tr.fraction < 1.0f ) || (tr.startsolid) || (tr.allsolid))
		{
#ifdef CLIENT_DLL
			CBaseEntity *pEntity = ClientEntityList().GetBaseEntity( pHandleEntity->GetRefEHandle().GetEntryIndex() );
#else
			CBaseEntity *pEntity = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );
#endif // CLIENT_DLL

			// If hitting a trigger, it must have the IMouse interface
			if( pEntity->GetSolidFlags() & FSOLID_TRIGGER|FSOLID_NOT_SOLID )
			{
				if( !pEntity->GetIMouse() )
					return true;

				resulttr = tr;
				didhit = true;
				return false;
			}
			else
			{
				// Do normal trace
				return false;
			}
		}

		return true;
	}

	trace_t resulttr;
	bool didhit;

private:
	CBaseEntity		*m_pIgnoreEntity;
	int				m_nContentsMask;
	Ray_t			*m_pRay;
};

//-----------------------------------------------------------------------------
// Purpose: Called when we received a new mouse aim vector or when moving around
//			Calculate new mouse data, so we only need to do this once.
//-----------------------------------------------------------------------------
#define MOUSE_TRACE_BOX Vector(8.0, 8.0, 8.0)
void CHL2WarsPlayer::UpdateMouseData( Vector &vMouseAim )
{
	if( g_debug_mouse_noupdate.GetBool() )
		return;

	trace_t tr;
	Vector vStartPos, vEndPos;
	CBaseEntity *pEnt, *pOldEnt;

	// Copy mouse aim
	m_vMouseAim = vMouseAim;

	// Get information
	// Note: Filter controlled unit rather then the player. 
	//		 The player is already not traceable. When activating mouse in direct control we need to ignore the unit.
	vStartPos = Weapon_ShootPosition()+GetCameraOffset();
	vEndPos = vStartPos + (m_vMouseAim *  MAX_TRACE_LENGTH);

#if 0
	// Don't need a trace hull, already doing an extra check below
	UTIL_TraceHull( vStartPos, vEndPos,
			-MOUSE_TRACE_BOX, MOUSE_TRACE_BOX, MASK_SOLID, GetControlledUnit(), COLLISION_GROUP_NONE, &tr );
#else
	// First version takes triggers into account with mouse interface
	Ray_t ray;
	ray.Init( vStartPos, vEndPos, -MOUSE_TRACE_BOX, MOUSE_TRACE_BOX );

	CMouseCollideList collideList( &ray, this, MASK_SOLID );
	enginetrace->EnumerateEntities( ray, true, &collideList );

	if( collideList.didhit )
	{
		tr = collideList.resulttr;
	}
	else
	{
		// Fallback, so it hits the world
		UTIL_TraceHull( vStartPos, vEndPos,
				-MOUSE_TRACE_BOX, MOUSE_TRACE_BOX, MASK_SOLID, GetControlledUnit(), COLLISION_GROUP_NONE, &tr );
	}
#endif // 0

	// Store old
	pOldEnt = m_MouseData.GetEnt();

	// Update
	m_MouseData.m_vStartPos = tr.startpos;
	m_MouseData.m_vEndPos = tr.endpos;
	m_MouseData.m_vNormal = tr.plane.normal;
	m_MouseData.SetEnt( tr.m_pEnt );
	m_MouseData.m_nHitBox = tr.hitbox;

	// Check world only
	CTraceFilterWorldOnly filter;
	UTIL_TraceHull( Weapon_ShootPosition()+GetCameraOffset(), Weapon_ShootPosition()+GetCameraOffset() + (m_vMouseAim *  8192),
		-MOUSE_TRACE_BOX, MOUSE_TRACE_BOX, MASK_SOLID_BRUSHONLY, &filter, &tr ); 
	m_MouseData.m_vWorldOnlyEndPos = tr.endpos;
	m_MouseData.m_vWorldOnlyNormal = tr.plane.normal;
	
	// On Client: Update mouse x and y position
#ifdef CLIENT_DLL
	::input->GetFullscreenMousePos( &(m_MouseData.m_iX), &(m_MouseData.m_iY) );
#endif 

	// Clear ent in case we are not supposed to see it with the mouse
	if( m_MouseData.m_hEnt )
	{
#ifdef CLIENT_DLL
		if( !m_MouseData.m_hEnt->FOWShouldShow() )
#else
		if( !m_MouseData.m_hEnt->FOWShouldShow( this ) )
#endif // CLIENT_DLL
			m_MouseData.SetEnt( NULL );
	}

	// If we hit the world or nothing, do a larger scan for entities
#define MOUSE_ENT_TOLERANCE 64.0f 
	if( (!m_MouseData.GetEnt() || m_MouseData.GetEnt()->GetRefEHandle().GetEntryIndex() == 0) && !g_debug_mouse_noradiuscheck.GetBool() )
	{
		// Do not use UTIL_EntitiesInSphere around corners. It causes about infinitive cpu load
		if( fabs(m_MouseData.m_vWorldOnlyEndPos.x)+513 < MAX_COORD_FLOAT &&
			fabs(m_MouseData.m_vWorldOnlyEndPos.y)+513 < MAX_COORD_FLOAT &&
			fabs(m_MouseData.m_vWorldOnlyEndPos.z)+513 < MAX_COORD_FLOAT)
		{
			CBaseEntity *pList[512];
			int i, n;
			float fSpeed, fDist, fBestDist;
			CBaseEntity *pEnt, *pBest;
			pBest = NULL;
			fBestDist = 512.0f;
			n = UTIL_EntitiesInSphere(pList, 512, m_MouseData.m_vWorldOnlyEndPos, 512.0, 0);
			for(i=0; i <n; i++)
			{
				pEnt = pList[i];
				if( !pEnt || pEnt->GetRefEHandle().GetEntryIndex() == 0 || pEnt->IsPointSized() || !pEnt->IsSolid() || pEnt->IsDormant() )
					continue;

				// Dont grab entities that should not be shown
#ifdef CLIENT_DLL
				if( !pEnt->FOWShouldShow() )
					continue;
#else
				if( !pEnt->FOWShouldShow( this ) )
					continue;
#endif // CLIENT_DLL

#ifdef CLIENT_DLL
				Vector vVel;
				pEnt->EstimateAbsVelocity(vVel);
				fSpeed = vVel.Length2D();
#else
				fSpeed = pEnt->GetSmoothedVelocity().Length();
#endif // CLIENT_DLL
				fDist = (tr.endpos - pEnt->GetAbsOrigin()).Length();
				if( fDist < MOUSE_ENT_TOLERANCE * MAX(1.0, (fSpeed/125.0)) )
				{
					if( pBest )
					{
						if( fDist < fBestDist )
						{
							fBestDist = fDist;
							pBest = pEnt;
						}
					}
					else
					{
						fBestDist = fDist;
						pBest = pEnt;
					}
					break;
				}
			}
			if( pBest )
				m_MouseData.SetEnt( pBest );
		}
	}

	if( g_debug_mouse_aim.GetBool() )
	{
		NDebugOverlay::SweptBox(vStartPos,  vStartPos + (m_vMouseAim *  8192), 
				-MOUSE_TRACE_BOX, MOUSE_TRACE_BOX, QAngle(), 255, 0, 0, 200, 0.05f);
		if( m_MouseData.GetEnt() )
		{
#ifndef CLIENT_DLL
			NDebugOverlay::BoxAngles(m_MouseData.GetEnt()->GetAbsOrigin(), m_MouseData.GetEnt()->WorldAlignMins(),
				m_MouseData.GetEnt()->WorldAlignMaxs(), m_MouseData.GetEnt()->GetAbsAngles(), 0, 0, 255, 255, 0.05f);
#else
			NDebugOverlay::BoxAngles(m_MouseData.GetEnt()->GetAbsOrigin(), m_MouseData.GetEnt()->WorldAlignMins(),
				m_MouseData.GetEnt()->WorldAlignMaxs(), m_MouseData.GetEnt()->GetAbsAngles(), 0, 255, 0, 255, 0.05f);
#endif // CLIENT_DLL
		}
	}

	// Check if entity is a dummy
	pEnt = m_MouseData.GetEnt();
	if( pEnt && pEnt->IsUnit() &&pEnt->GetMousePassEntity() )
	{
		m_MouseData.SetEnt( pEnt->GetMousePassEntity() );
	}

	// Check entity change. Call OnCursorEntered/Exited if the entity is an IMouse entity.
	pEnt = m_MouseData.GetEnt();
	if( pEnt != pOldEnt )
	{
		if( pOldEnt && pOldEnt->GetIMouse() )
			pOldEnt->GetIMouse()->OnCursorExited( this );
		if( pEnt && pEnt->GetIMouse() )
			pEnt->GetIMouse()->OnCursorEntered( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::SetCameraOffset( Vector &offs )
{
	m_vCameraOffset = offs;
}
const Vector &CHL2WarsPlayer::GetCameraOffset() const
{
	return m_vCameraOffset;
}

//-----------------------------------------------------------------------------
// Purpose: Called in PostThink, which is called after UpdateButtonState
//			Call signals in python to when the state of a button changed.
//			This makes it easy to hookup player input anywhere.
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::UpdateButtonState( int nUserCmdButtonMask )
{
	BaseClass::UpdateButtonState( nUserCmdButtonMask );

	// FIXME: Don't use m_nButtons and m_nButtonsLast
	// There seems to be a bug with that code on the client
	// It doesn't always store m_nButtons correctly 
	// (or it's cleared somewhere, but I can't find where)
	static int m_nCorrectButtons;
	static int m_CorrectButtonsLast;

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_CorrectButtonsLast = m_nCorrectButtons;

	// Get button states
	m_nCorrectButtons = nUserCmdButtonMask;
	int buttonsChanged = m_CorrectButtonsLast ^ m_nCorrectButtons;

	if( buttonsChanged == 0 )
		return;
	
#ifndef DISABLE_PYTHON
	if( SrcPySystem()->IsPythonRunning() )
	{
		bp::dict kwargs;
		kwargs["sender"] = bp::object();
		kwargs["player"] = GetPyHandle();

		if( (buttonsChanged & IN_SPEED) != 0 )
		{
			kwargs["state"] = (bool)(m_nCorrectButtons &IN_SPEED)!= 0;
			bp::object signal = SrcPySystem()->Get("keyspeed", "core.signals", true);
			SrcPySystem()->CallSignal( signal, kwargs );
		}
		else if( (buttonsChanged & IN_DUCK) != 0 )
		{
			kwargs["state"] = (bool)(m_nCorrectButtons &IN_DUCK)!= 0;
			bp::object signal = SrcPySystem()->Get("keyduck", "core.signals", true);
			SrcPySystem()->CallSignal( signal, kwargs );
		}
	}
#endif // DISABLE_PYTHON

	if( GetControlledUnit() && GetControlledUnit()->GetIUnit() )
	{
		GetControlledUnit()->GetIUnit()->OnButtonsChanged( m_nCorrectButtons, buttonsChanged );
	}
}

//-----------------------------------------------------------------------------
// Purpose: The following are the internal functions and update the state of mouse
//			pressing/releasing.
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::OnLeftMouseButtonPressedInternal( const MouseTraceData_t &data )
{
	m_bIsMouseCleared = false;
	m_MouseDataLeftPressed = data;
	m_bIsLeftPressed = true;

	OnLeftMouseButtonPressed( data );

#ifdef CLIENT_DLL
	engine->ServerCmd( VarArgs("player_lmp %lu", m_MouseDataLeftPressed.m_hEnt.Get() ? m_MouseDataLeftPressed.m_hEnt.GetEntryIndex() : -1) );
#endif // CLIENT_DLL
}

void CHL2WarsPlayer::OnLeftMouseButtonDoublePressedInternal( const MouseTraceData_t &data )
{
	if( m_bIsMouseCleared )
	{
#ifdef CLIENT_DLL
		engine->ServerCmd( VarArgs("player_lmdp %lu", data.m_hEnt.Get() ? data.m_hEnt.GetEntryIndex() : -1) );
#endif // CLIENT_DLL
		return;
	}

	m_MouseDataLeftDoublePressed = data;
	m_bIsLeftDoublePressed = true;

	OnLeftMouseButtonDoublePressed( data );

#ifdef CLIENT_DLL
	engine->ServerCmd( VarArgs("player_lmdp %lu", m_MouseDataLeftDoublePressed.m_hEnt.Get() ? m_MouseDataLeftDoublePressed.m_hEnt.GetEntryIndex() : -1) );
#endif // CLIENT_DLL
}

void CHL2WarsPlayer::OnLeftMouseButtonReleasedInternal( const MouseTraceData_t &data )
{
	if( m_bIsMouseCleared )
	{
#ifdef CLIENT_DLL
		engine->ServerCmd( VarArgs("player_lmr %lu", data.m_hEnt.Get() ? data.m_hEnt.GetEntryIndex() : -1) );
#endif // CLIENT_DLL
		return;
	}

	m_MouseDataLeftReleased = data;
	m_bWasLeftDoublePressed = m_bIsLeftDoublePressed;
	m_bIsLeftPressed = false;
	m_bIsLeftDoublePressed = false;

	OnLeftMouseButtonReleased( data );


#ifdef CLIENT_DLL
	engine->ServerCmd( VarArgs("player_lmr %lu", m_MouseDataLeftReleased.m_hEnt.Get() ? m_MouseDataLeftReleased.m_hEnt.GetEntryIndex() : -1) );
#endif // CLIENT_DLL
}

void CHL2WarsPlayer::OnRightMouseButtonPressedInternal( const MouseTraceData_t &data )
{
	m_bIsMouseCleared = false;
	m_MouseDataRightPressed = data;
	m_bIsRightPressed = true;

	OnRightMouseButtonPressed( data );

#ifdef CLIENT_DLL
	engine->ServerCmd( VarArgs("player_rmp %lu", m_MouseDataRightPressed.m_hEnt.Get() ? m_MouseDataRightPressed.m_hEnt.GetEntryIndex() : -1) );
#endif // CLIENT_DLL
}

void CHL2WarsPlayer::OnRightMouseButtonDoublePressedInternal( const MouseTraceData_t &data )
{
	if( m_bIsMouseCleared )
	{
#ifdef CLIENT_DLL
		engine->ServerCmd( VarArgs("player_rmdp %lu", data.m_hEnt.Get() ? data.m_hEnt.GetEntryIndex() : -1) );
#endif // CLIENT_DLL
		return;
	}

	m_MouseDataRightDoublePressed = data;
	m_bIsRightDoublePressed = true;

	OnRightMouseButtonDoublePressed( data );

#ifdef CLIENT_DLL
	engine->ServerCmd( VarArgs("player_rmdp %lu", m_MouseDataRightDoublePressed.m_hEnt.Get() ? m_MouseDataRightDoublePressed.m_hEnt.GetEntryIndex() : -1) );
#endif // CLIENT_DLL
}

void CHL2WarsPlayer::OnRightMouseButtonReleasedInternal( const MouseTraceData_t &data )
{
	if( m_bIsMouseCleared )
	{
#ifdef CLIENT_DLL
		engine->ServerCmd( VarArgs("player_rmr %lu", data.m_hEnt.Get() ? data.m_hEnt.GetEntryIndex() : -1) );
#endif // CLIENT_DLL
		return;
	}

	m_MouseDataRightReleased = data;	
	m_bIsRightPressed = false;
	m_bWasRightDoublePressed = m_bIsRightDoublePressed;
	m_bIsRightDoublePressed = false;

	OnRightMouseButtonReleased( data );

#ifdef CLIENT_DLL
	engine->ServerCmd( VarArgs("player_rmr %lu", m_MouseDataRightReleased.m_hEnt.Get() ? m_MouseDataRightReleased.m_hEnt.GetEntryIndex() : -1) );
#endif // CLIENT_DLL
}	

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::OnLeftMouseButtonPressed( const MouseTraceData_t &data )
{
	// If we have an active ability, it overrides our mouse actions
	bool bEatMouse = false;
#ifndef DISABLE_PYTHON
	CUtlVector<bp::object> activeAbilities;
	activeAbilities = m_vecActiveAbilities;
	for(int i=0; i< activeAbilities.Count(); i++)
	{
		bEatMouse = SrcPySystem()->RunT<bool>( SrcPySystem()->Get("OnLeftMouseButtonPressed", activeAbilities[i]), false ) || bEatMouse;
	}
#endif // DISABLE_PYTHON

	if( bEatMouse )
		return;

	// if we hit an entity...
	CBaseEntity *pMouseEnt = ( GetMouseCapture() != NULL ) ? GetMouseCapture() : m_MouseDataLeftPressed.m_hEnt.Get();
	if( pMouseEnt != NULL && !pMouseEnt->IsWorld() )
	{
		// If mouse interface call the approciate function
		if( pMouseEnt->GetIMouse() )
			pMouseEnt->GetIMouse()->OnClickLeftPressed(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::OnLeftMouseButtonDoublePressed( const MouseTraceData_t &data )
{
	// If we have an active ability, it overrides our mouse actions
	bool bEatMouse = false;
#ifndef DISABLE_PYTHON
	CUtlVector<bp::object> activeAbilities;
	activeAbilities = m_vecActiveAbilities;
	for(int i=0; i< activeAbilities.Count(); i++)
	{
		bEatMouse = SrcPySystem()->RunT<bool>( SrcPySystem()->Get("OnLeftMouseButtonDoublePressed", activeAbilities[i]), false ) || bEatMouse;
	}
#endif // DISABLE_PYTHON
	if( bEatMouse )
		return;

	// if we hit an entity...
	CBaseEntity *pMouseEnt = ( GetMouseCapture() != NULL ) ? GetMouseCapture() : m_MouseDataLeftDoublePressed.m_hEnt.Get();
	if( pMouseEnt != NULL && !pMouseEnt->IsWorld() )
	{
		// If mouse interface call the approciate function
		if( pMouseEnt->GetIMouse() )
			pMouseEnt->GetIMouse()->OnClickLeftDoublePressed(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::OnLeftMouseButtonReleased( const MouseTraceData_t &data )
{
	CBaseEntity *pMouseEnt;
#ifdef CLIENT_DLL
	IUnit *pIUnit;
#endif // CLIENT_DLL
	bool bEatMouse;

	// Mouse capture entity takes priority
	if( GetMouseCapture() )
	{
		if( GetMouseCapture()->GetIMouse() )
			GetMouseCapture()->GetIMouse()->OnClickLeftReleased(this);
		SetMouseCapture( NULL );
		return;
	}

	// If we have an active ability it overrides our mouse actions
	bEatMouse = false;
#ifndef DISABLE_PYTHON
	CUtlVector<bp::object> activeAbilities;
	activeAbilities = m_vecActiveAbilities;
	for(int i=0; i< activeAbilities.Count(); i++)
	{
		bEatMouse = SrcPySystem()->RunT<bool>( SrcPySystem()->Get("OnLeftMouseButtonReleased", activeAbilities[i]), false ) || bEatMouse;
	}
#endif // DISABLE_PYTHON

	if( bEatMouse )
		return;

	// If distance between released and pressed is larger than the threshold use the select box
	// NOTE: This is client side, so we can use the pixel coordinates
#ifdef CLIENT_DLL
	if( abs( GetMouseDataLeftPressed().m_iX - GetMouseDataLeftReleased().m_iX ) > XRES( cl_mouse_selectionbox_threshold.GetInt() ) &&
		abs( GetMouseDataLeftPressed().m_iY - GetMouseDataLeftReleased().m_iY ) > YRES( cl_mouse_selectionbox_threshold.GetInt() ) )
	{
		SelectBox(	MIN( GetMouseDataLeftPressed().m_iX, GetMouseDataLeftReleased().m_iX ), 
					MIN( GetMouseDataLeftPressed().m_iY, GetMouseDataLeftReleased().m_iY ),
					MAX( GetMouseDataLeftPressed().m_iX, GetMouseDataLeftReleased().m_iX ), 
					MAX( GetMouseDataLeftPressed().m_iY, GetMouseDataLeftReleased().m_iY ));
		engine->ServerCmd( "player_cm" ); // Just clear mouse
		return;
	}
#endif // CLIENT_DLL

	// if we hit an entity...
	pMouseEnt = m_MouseDataLeftPressed.GetEnt();
	if( pMouseEnt != NULL && !pMouseEnt->IsWorld() )
	{
		// If mouse interface call the approciate function
		if( pMouseEnt->GetIMouse() )
		{
			pMouseEnt->GetIMouse()->OnClickLeftReleased(this);
		}

#ifdef CLIENT_DLL
		pIUnit = pMouseEnt->GetIUnit();
		if( pIUnit )
		{

			if( m_bWasLeftDoublePressed )
			{
				if( pMouseEnt->GetOwnerNumber() == GetOwnerNumber() )
					SelectAllUnitsOfTypeInScreen( pIUnit->GetUnitType() );
			}
			else
			{
				if( m_nButtons & IN_SPEED )
				{
					int idx = FindUnit(pMouseEnt);
					if( idx != -1 )
					{
						RemoveUnit(idx);
						engine->ServerCmd( VarArgs("player_removeunit %ld", EncodeEntity(pMouseEnt)) );
					}
					else
					{	
#ifndef DISABLE_PYTHON
						if( pIUnit->IsSelectableByPlayer(this, GetSelection()) ) 
#endif // DISABLE_PYTHON
						{
							pIUnit->Select(this);
							engine->ServerCmd( VarArgs("player_addunit %ld", EncodeEntity(pMouseEnt)) );
						}
					}
				}
				else
				{
					ClearSelection();
					engine->ServerCmd("player_clearselection");
#ifndef DISABLE_PYTHON
					if( pIUnit->IsSelectableByPlayer(this, GetSelection()) ) 
#endif // DISABLE_PYTHON
					{
						pIUnit->Select(this);
						engine->ServerCmd( VarArgs("player_addunit %ld", EncodeEntity(pMouseEnt)) );
					}
				}
			}

		}
		else
		{
			if( cl_selection_noclear.GetBool() == false )
			{
				ClearSelection();
				engine->ServerCmd("player_clearselection");
			}
		}
#endif // CLIENT_DLL
	}
	else
	{
#ifdef CLIENT_DLL
		if( cl_selection_noclear.GetBool() == false )
		{
			ClearSelection();
			engine->ServerCmd("player_clearselection");
		}
#endif // CLIENT_DLL
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::OnRightMouseButtonPressed( const MouseTraceData_t &data )
{
	// If we have an active ability, it overrides our mouse actions
	bool bEatMouse = false;
#ifndef DISABLE_PYTHON
	CUtlVector<bp::object> activeAbilities;
	activeAbilities = m_vecActiveAbilities;
	for(int i=0; i< activeAbilities.Count(); i++)
	{
		bEatMouse = SrcPySystem()->RunT<bool>( SrcPySystem()->Get("OnRightMouseButtonPressed", activeAbilities[i]), false ) || bEatMouse;
	}
#endif // DISABLE_PYTHON
	if( bEatMouse )
		return;

	// if we hit an entity...
	CBaseEntity *pMouseEnt = ( GetMouseCapture() != NULL ) ? GetMouseCapture() : m_MouseDataRightPressed.m_hEnt.Get();
	if( pMouseEnt != NULL && !pMouseEnt->IsWorld() )
	{
		// If mouse interface call the approciate function
		if( pMouseEnt->GetIMouse() )
			pMouseEnt->GetIMouse()->OnClickRightPressed(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::OnRightMouseButtonDoublePressed( const MouseTraceData_t &data )
{
	// If we have an active ability, it overrides our mouse actions
	bool bEatMouse = false;
#ifndef DISABLE_PYTHON
	CUtlVector<bp::object> activeAbilities;
	activeAbilities = m_vecActiveAbilities;
	for(int i=0; i< activeAbilities.Count(); i++)
	{
		bEatMouse = SrcPySystem()->RunT<bool>( SrcPySystem()->Get("OnRightMouseButtonDoublePressed", activeAbilities[i]), false ) || bEatMouse;	
	}
#endif // DISABLE_PYTHON
	if( bEatMouse )
		return;

	// if we hit an entity...
	CBaseEntity *pMouseEnt = ( GetMouseCapture() != NULL ) ? GetMouseCapture() : m_MouseDataRightDoublePressed.m_hEnt.Get();
	if( pMouseEnt != NULL && !pMouseEnt->IsWorld() )
	{
		// If mouse interface call the approciate function
		if( pMouseEnt->GetIMouse() )
			pMouseEnt->GetIMouse()->OnClickRightDoublePressed(this);
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::OnRightMouseButtonReleased( const MouseTraceData_t &data )
{
	// Mouse capture entity takes priority
	if( GetMouseCapture() )
	{
		if( GetMouseCapture()->GetIMouse() )
			GetMouseCapture()->GetIMouse()->OnClickRightReleased(this);
		SetMouseCapture(NULL);
#ifdef CLIENT_DLL
		engine->ServerCmd( VarArgs("player_rmr %lu", m_MouseDataRightReleased.m_hEnt.Get() ? m_MouseDataRightReleased.m_hEnt.GetEntryIndex() : -1) );
#endif // CLIENT_DLL
		return;
	}

	// If we have an active ability, it overrides our mouse actions
	bool bEatMouse = false;
#ifndef DISABLE_PYTHON
	CUtlVector<bp::object> activeAbilities;
	activeAbilities = m_vecActiveAbilities;
	for(int i=0; i< activeAbilities.Count(); i++)
	{
		bEatMouse = SrcPySystem()->RunT<bool>( SrcPySystem()->Get("OnRightMouseButtonReleased", activeAbilities[i]), false ) || bEatMouse;	
	}
#endif // DISABLE_PYTHON
	if( bEatMouse )
		return;

	// if we hit an entity...
	CBaseEntity *pMouseEnt = m_MouseDataRightReleased.GetEnt();
	if( pMouseEnt != NULL && !pMouseEnt->IsWorld() )
	{
		// If mouse interface call the approciate function
		if( pMouseEnt->GetIMouse() )
			pMouseEnt->GetIMouse()->OnClickRightReleased(this);
	}

	OrderUnits();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::ClearMouse()
{
	m_bIsMouseCleared = true;
	m_bIsLeftPressed = false;
	m_bIsLeftDoublePressed = false;
	m_bWasLeftDoublePressed = false;
	m_bIsRightPressed = false;
	m_bIsRightDoublePressed = false;
	m_bWasRightDoublePressed = false;
	SetMouseCapture(NULL);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::SetMouseCapture( CBaseEntity *pEntity )
{
	if( !pEntity )
	{
		m_hMouseCaptureEntity = NULL;
		return;
	}

	// Must be a valid mouse entity
	if( !pEntity->GetIMouse() )
		return;

	m_hMouseCaptureEntity = pEntity;
}

CBaseEntity *CHL2WarsPlayer::GetMouseCapture()
{
	return m_hMouseCaptureEntity.Get();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::ChangeFaction( const char *faction )
{
	// Check if the player's faction changed ( Might want to add a string table )
	if( m_FactionName != NULL_STRING && Q_strncmp( STRING(m_FactionName), faction, MAX_PATH ) == 0 ) 
		return;

	char* pszOldValue = NULL;
	if( m_FactionName != NULL_STRING ) {
		int len = Q_strlen(STRING(m_FactionName)) + 1;
		pszOldValue = (char*)stackalloc( len );
		memcpy( pszOldValue, STRING(m_FactionName), len );

	}
#ifndef CLIENT_DLL
	Q_strcpy( m_NetworkedFactionName.GetForModify(), faction );
#endif // CLIENT_DLL

	m_FactionName = AllocPooledString(faction);

#ifndef DISABLE_PYTHON
	if( SrcPySystem()->IsPythonRunning() )
	{
		// Dispatch changed faction signal
		try {
			bp::dict kwargs;
			kwargs["sender"] = bp::object();
			kwargs["player"] = GetPyHandle();
			kwargs["oldfaction"] = (pszOldValue ? bp::str((const char *)pszOldValue) : bp::object());
			bp::object signal = SrcPySystem()->Get("playerchangedfaction", "core.signals", true);
			SrcPySystem()->CallSignal( signal, kwargs );
		} catch( bp::error_already_set & ) {
			PyErr_Print();
		}
	}
#endif // DISABLE_PYTHON

	if( pszOldValue )
		stackfree( pszOldValue );
}

//-----------------------------------------------------------------------------
// Purpose: Selection management
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::UpdateSelection( void )
{
	int nCount, i;
	bool bChanged;

	// Check selection
	nCount = CountUnits();
	bChanged = false;
	for ( i = nCount-1; i >= 0; i-- )
	{
		CBaseEntity *pUnit = GetUnit(i);
		if( !pUnit || !pUnit->IsAlive() ||
#ifdef CLIENT_DLL
				!pUnit->FOWShouldShow() )
#else
				!pUnit->FOWShouldShow( this ) )
#endif // CLIENT_DLL
		{
			RemoveUnit( i, false );		
			bChanged = true;
		}
	}

#ifdef CLIENT_DLL
	UpdateSelectedUnitType();
#endif // CLIENT_DLL

	if( bChanged || m_bSelectionChangedSignalScheduled )
		OnSelectionChanged();
}

#ifndef DISABLE_PYTHON
boost::python::list	CHL2WarsPlayer::GetSelection( void )
{
	if( m_bRebuildPySelection )
	{
		m_pySelection = bp::list();

		CBaseEntity *pUnit;
		int nCount = CountUnits();
		for ( int i = 0; i < nCount; i++ )
		{
			pUnit = m_hSelectedUnits[i];
			if( pUnit )
				m_pySelection.append( pUnit->GetPyHandle() );	
		}

		m_bRebuildPySelection = false;
	}

	return m_pySelection;
}
#endif // DISABLE_PYTHON

CBaseEntity *CHL2WarsPlayer::GetUnit( int idx )
{
	if( m_hSelectedUnits.IsValidIndex(idx) == false ) {
		//Warning("CHL2Wars_Player::GetUnit: trying to get an unit with an invalid index\n");
		return NULL;
	}
	return m_hSelectedUnits.Element(idx);
	
}

void CHL2WarsPlayer::AddUnit( CBaseEntity *pUnit, bool bTriggerOnSel )
{
	int iIndex, iMyPriority, iFirstInsertPriority; 
	if( !pUnit || !pUnit->GetIUnit() )
	{
		Warning("CHL2WarsPlayer::AddUnit: Trying to add an invalid unit to the selection array!\n");
		return;
	}

	IUnit *pIUnit = pUnit->GetIUnit();

	// Find out if the unit is already selected
	iIndex = m_hSelectedUnits.Find(pUnit);
	if( m_hSelectedUnits.IsValidIndex(iIndex) )
		return;
	
	// Find our insertion place ( maybe use a priority number, add this to IUnit? )
	iIndex = -1;
	iMyPriority = pIUnit->GetSelectionPriority();
	iFirstInsertPriority = -1;
	for( int i=0; i<m_hSelectedUnits.Count(); i++ )
	{
		if( m_hSelectedUnits[i] == NULL )
			continue;
		if( !Q_stricmp( m_hSelectedUnits[i]->GetIUnit()->GetUnitType(), pIUnit->GetUnitType() ) )
		{
			iIndex = i;
			break;
		}
		else if( m_hSelectedUnits[i]->GetIUnit()->GetSelectionPriority() < iMyPriority )
		{
			iFirstInsertPriority = i;
			break;
		}
	}
	if( iIndex != -1 ) 
	{   // Already units of this type in the list, insert after that unit
		m_hSelectedUnits.InsertAfter(iIndex, pUnit);
	}
	else if( iFirstInsertPriority != -1 ) 
	{   // Not in selection yet, insert by priority
		m_hSelectedUnits.InsertBefore(iFirstInsertPriority, pUnit);
	}
	else 
	{   // Selection is empty
		m_hSelectedUnits.AddToTail(pUnit);
	}

	m_bRebuildPySelection = true;

	pIUnit->OnSelected(this);
	if( bTriggerOnSel )
		ScheduleSelectionChangedSignal();
}

int CHL2WarsPlayer::FindUnit( CBaseEntity *pEnt )
{
	Assert(pEnt);
	return m_hSelectedUnits.Find(pEnt);
}

void CHL2WarsPlayer::RemoveUnit( int idx, bool bTriggerOnSel )
{
	if( m_hSelectedUnits.IsValidIndex(idx) == false ) {
		Warning("CHL2WarsPlayer::RemoveUnit: trying to remove an invalid index\n");
		return;
	}
	if( m_hSelectedUnits.Element(idx) ) {
		m_hSelectedUnits.Element(idx)->GetIUnit()->OnDeSelected(this);
	}
	m_hSelectedUnits.Remove(idx);

	m_bRebuildPySelection = true;

	if( bTriggerOnSel )
		ScheduleSelectionChangedSignal();
}

void CHL2WarsPlayer::RemoveUnit( CBaseEntity *pUnit, bool bTriggerOnSel )
{
	RemoveUnit(FindUnit(pUnit), bTriggerOnSel);
}

int CHL2WarsPlayer::CountUnits()
{
	return m_hSelectedUnits.Count();
}

void CHL2WarsPlayer::ClearSelection( bool bTriggerOnSel )
{
	int i;
	for( i=0; i<m_hSelectedUnits.Count(); i++ )
	{
		if( m_hSelectedUnits.Element(i) )
			m_hSelectedUnits.Element(i)->GetIUnit()->OnDeSelected(this);
	}
	m_hSelectedUnits.RemoveAll();
	m_bRebuildPySelection = true;
	if( bTriggerOnSel )
		ScheduleSelectionChangedSignal();
}

void CHL2WarsPlayer::OnSelectionChanged()
{
#ifndef DISABLE_PYTHON
	if( SrcPySystem()->IsPythonRunning() )
	{
		bp::dict kwargs;
		kwargs["sender"] = bp::object();
		kwargs["player"] = GetPyHandle();
		bp::object signal = SrcPySystem()->Get("selectionchanged", "core.signals", true);
		SrcPySystem()->CallSignal( signal, kwargs );
	}
#endif // DISABLE_PYTHON

	m_bSelectionChangedSignalScheduled = false;
}

void CHL2WarsPlayer::OrderUnits()
{
	int i;

#ifndef DISABLE_PYTHON
	if( SrcPySystem()->IsPythonRunning() )
	{
		bp::dict kwargs;
		kwargs["sender"] = bp::object();
		kwargs["player"] = GetPyHandle();
		bp::object signal = SrcPySystem()->Get("pre_orderunits", "core.signals", true);
		SrcPySystem()->CallSignal( signal, kwargs );
	}
#endif // DISABLE_PYTHON

	for( i=0; i<m_hSelectedUnits.Count(); i++ )
	{
		if( m_hSelectedUnits.Element(i) == NULL )
			continue;

		m_hSelectedUnits.Element(i)->GetIUnit()->Order(this);
	}

#ifndef DISABLE_PYTHON
	if( SrcPySystem()->IsPythonRunning() )
	{
		bp::dict kwargs;
		kwargs["sender"] = bp::object();
		kwargs["player"] = GetPyHandle();
		bp::object signal = SrcPySystem()->Get("post_orderunits", "core.signals", true);
		SrcPySystem()->CallSignal( signal, kwargs );
	}
#endif // DISABLE_PYTHON
}

void CHL2WarsPlayer::ScheduleSelectionChangedSignal()
{
	m_bSelectionChangedSignalScheduled = true;
}

//-----------------------------------------------------------------------------
// Purpose: Group management
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::ClearGroup( int iGroup )
{
	if( iGroup < 0 || iGroup >= PLAYER_MAX_GROUPS )
		return;
	m_Groups[iGroup].m_Group.RemoveAll();
}

void CHL2WarsPlayer::AddToGroup( int iGroup, CBaseEntity *pUnit )
{
	if( iGroup < 0 || iGroup >= PLAYER_MAX_GROUPS || !pUnit )
		return;
	m_Groups[iGroup].m_Group.AddToTail(pUnit);
}

void CHL2WarsPlayer::MakeCurrentSelectionGroup( int iGroup, bool bClearGroup )
{
	if( iGroup < 0 || iGroup >= PLAYER_MAX_GROUPS )
		return;
	if( bClearGroup ) ClearGroup(iGroup);
	for( int i=0; i<m_hSelectedUnits.Count(); i++ )
	{
		if( m_hSelectedUnits.Element(i) == NULL )
			continue;
		m_Groups[iGroup].m_Group.AddToTail(m_hSelectedUnits[i]);
	}

#ifndef DISABLE_PYTHON
	if( SrcPySystem()->IsPythonRunning() )
	{
		bp::dict kwargs;
		kwargs["sender"] = bp::object();
		kwargs["player"] = GetPyHandle();
		kwargs["group"] = iGroup;
		bp::object signal = SrcPySystem()->Get("groupchanged", "core.signals", true);
		SrcPySystem()->CallSignal( signal, kwargs );
	}
#endif // DISABLE_PYTHON
}

void CHL2WarsPlayer::SelectGroup( int iGroup )
{
	if( iGroup < 0 || iGroup >= PLAYER_MAX_GROUPS || m_Groups[iGroup].m_Group.Count() == 0 )
		return;

	if( (m_nButtons & IN_SPEED) == 0 )
		ClearSelection();

	for(int i=0; i<m_Groups[iGroup].m_Group.Count(); i++)
	{
		if( m_Groups[iGroup].m_Group[i] )
			m_Groups[iGroup].m_Group[i]->GetIUnit()->Select(this, false);
	}
	ScheduleSelectionChangedSignal();

	m_iLastSelectedGroup = iGroup;
	m_fLastSelectGroupTime = gpGlobals->curtime;
}

int	CHL2WarsPlayer::GetGroupNumber(CBaseEntity *pUnit)
{
	if( !pUnit )
		return -1;

	int iGroup, idx;
	for(iGroup=0; iGroup<PLAYER_MAX_GROUPS; iGroup++)
	{
		idx = m_Groups[iGroup].m_Group.Find(pUnit);
		if( m_Groups[iGroup].m_Group.IsValidIndex(idx) )
			return iGroup + 1;
	}
	return -1;
}

int CHL2WarsPlayer::CountGroup( int iGroup )
{
	if( iGroup < 0 || iGroup >= PLAYER_MAX_GROUPS )
		return -1;
	return m_Groups[iGroup].m_Group.Count();
}

CBaseEntity *CHL2WarsPlayer::GetGroupUnit( int iGroup, int index )
{
	if( iGroup < 0 || iGroup >= PLAYER_MAX_GROUPS || !(m_Groups[iGroup].m_Group.IsValidIndex(index)) )
		return NULL;

	return m_Groups[iGroup].m_Group[index];
}

void CHL2WarsPlayer::CleanupGroups( void )
{
	int iGroup, i;
	bool bChanged;

	// Check groups
	for( iGroup = 0; iGroup < PLAYER_MAX_GROUPS; iGroup++ )
	{
		bChanged = false;
		for( i= m_Groups[iGroup].m_Group.Count()-1; i >= 0; i-- )
		{
			EHANDLE m_pEntity = m_Groups[iGroup].m_Group[i];
			if( m_pEntity != NULL && m_pEntity->IsAlive() )
				continue;

			m_Groups[iGroup].m_Group.Remove(i);
			bChanged = true;
		}

#ifndef DISABLE_PYTHON
		if( bChanged )
		{
			if( SrcPySystem()->IsPythonRunning() )
			{
				bp::dict kwargs;
				kwargs["sender"] = bp::object();
				kwargs["player"] = GetPyHandle();
				kwargs["group"] = iGroup;
				bp::object signal = SrcPySystem()->Get("groupchanged", "core.signals", true);
				SrcPySystem()->CallSignal( signal, kwargs );
			}
		}
#endif // DISABLE_PYTHON
	}
}

const CUtlVector< EHANDLE > & CHL2WarsPlayer::GetGroup( int iGroup )
{
	static CUtlVector< EHANDLE > emptygroup;
	if( iGroup < 0 || iGroup >= PLAYER_MAX_GROUPS )
		return emptygroup;
	return m_Groups[iGroup].m_Group;
}

#ifndef DISABLE_PYTHON
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::AddActiveAbility( bp::object ability )
{
	if( ability.ptr() == Py_None ) 
		return;
	if( !IsActiveAbility(ability) )
		m_vecActiveAbilities.AddToTail(ability);
}

void CHL2WarsPlayer::RemoveActiveAbility( bp::object ability )
{
	if( ability.ptr() == Py_None ) 
		return;
	if( !m_vecActiveAbilities.FindAndRemove(ability) )
	{
		PyErr_SetString(PyExc_Exception, "Not an active ability" );
		throw boost::python::error_already_set(); 
	}
	SrcPySystem()->Run( SrcPySystem()->Get("OnMouseLost", ability) );
}

bool CHL2WarsPlayer::IsActiveAbility( bp::object ability )
{
	if( ability.ptr() == Py_None ) 
		return false;
	return (m_vecActiveAbilities.Find(ability) != -1);
}

void CHL2WarsPlayer::ClearActiveAbilities()
{
	CUtlVector<bp::object> clearAbilities;
	clearAbilities = m_vecActiveAbilities;
	m_vecActiveAbilities.RemoveAll();
	for(int i=0; i<clearAbilities.Count(); i++)
	{
		SrcPySystem()->Run( SrcPySystem()->Get("OnMouseLost", clearAbilities[i]) );
	}
}

void CHL2WarsPlayer::SetSingleActiveAbility( bp::object ability )
{
	ClearActiveAbilities();
	if( ability.ptr() == Py_None ) 
		return;
	m_vecActiveAbilities.AddToTail(ability);
}

bp::object CHL2WarsPlayer::GetSingleActiveAbility()
{
	if( m_vecActiveAbilities.Count() == 0 )
		return bp::object();
	return m_vecActiveAbilities[0];
}
#endif // DISABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBaseEntity *CHL2WarsPlayer::GetControlledUnit()
{
	return m_hControlledUnit.Get(); 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::OnChangeOwnerNumber( int old_owner_number )
{
	BaseClass::OnChangeOwnerNumber(old_owner_number);

#ifndef DISABLE_PYTHON
#ifdef CLIENT_DLL
	char pLevelName[_MAX_PATH];
	Q_FileBase(engine->GetLevelName(), pLevelName, _MAX_PATH);
#else
	const char *pLevelName = STRING(gpGlobals->mapname);
#endif

	if( SrcPySystem()->IsPythonRunning() )
	{
		// Send clientactive signal
		bp::dict kwargs;
		kwargs["sender"] = bp::object();
		kwargs["player"] = GetPyHandle();
		kwargs["oldownernumber"] = old_owner_number;
		bp::object signal = SrcPySystem()->Get("playerchangedownernumber", "core.signals", true);
		SrcPySystem()->CallSignal( signal, kwargs );

		signal = SrcPySystem()->Get("map_playerchangedownernumber", "core.signals", true)[pLevelName];
		SrcPySystem()->CallSignal( signal, kwargs );
	}
#endif // DISABLE_PYTHON

#ifndef CLIENT_DLL
	// Inform gamerules
	HL2WarsGameRules()->PlayerChangedOwnerNumber(this, old_owner_number, GetOwnerNumber() );
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::CalculateHeight( const Vector &vPosition )
{
	trace_t tr;
	CTraceFilterWars traceFilter( this, COLLISION_GROUP_PLAYER_MOVEMENT );

	Vector vTestStartPos( vPosition );
	vTestStartPos.z += 64.0f; // Should be placed at the ground, so raise a bit.

	// Trace up
	UTIL_TraceHull( vTestStartPos, vTestStartPos + Vector(0, 0, MAX_TRACE_LENGTH), 
		GetPlayerMins(), GetPlayerMaxs(),
		CONTENTS_PLAYERCLIP, &traceFilter, &tr );
	if( !tr.DidHit() )
	{
		m_fMaxHeight = -1;
		m_vCamGroundPos = vTestStartPos;
		return;
	}

	Vector vMaxPos = tr.endpos;

	// Trace down
	UTIL_TraceHull( vTestStartPos, vTestStartPos + Vector(0, 0, -MAX_TRACE_LENGTH), 
		GetPlayerMins(), GetPlayerMaxs(),
		CONTENTS_PLAYERCLIP, &traceFilter, &tr );
	if( !tr.DidHit() )
	{
		m_fMaxHeight = -1;
		m_vCamGroundPos = vTestStartPos;
		return;
	}

	// Update height settings
	m_vCamGroundPos = tr.endpos;
	m_fMaxHeight = (vMaxPos - tr.endpos).Length() - 64.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsPlayer::SnapCameraTo( const Vector &vPos )
{
	// No boundary? free to do whatever you want
	if( !GetMapBoundaryList() )
	{
#ifdef CLIENT_DLL
		SetDirectMove( vPos );
#else
		SetAbsOrigin( vPos );
#endif // CLIENT_DLL
		return;
	}

	CBaseFuncMapBoundary *pBoundary = CBaseFuncMapBoundary::IsWithinAnyMapBoundary( vPos, true );
	if( !pBoundary )
		return;

	Vector vAbsMins, vAbsMaxs;
	pBoundary->GetMapBoundary( vAbsMins, vAbsMaxs );

	QAngle vAngles = GetAbsAngles();
	Vector vForward, vDir, vTest, vCamLookAt;
	AngleVectors(vAngles, &vForward, NULL, NULL);

	vTest.x = vPos.x; vTest.y = vPos.y;
	vTest.z = vAbsMaxs.z - 8.0f - GetPlayerMaxs().z;

	// Figure out position at which we are looking
	trace_t tr;
	CTraceFilterWars traceFilter( this, COLLISION_GROUP_PLAYER_MOVEMENT );
	UTIL_TraceHull( vTest, vTest + Vector(0, 0, -MAX_TRACE_LENGTH), GetPlayerMins(), GetPlayerMaxs(), CONTENTS_PLAYERCLIP, &traceFilter, &tr );
	if( !tr.DidHit() || tr.startsolid )
	{
		Warning("SnapCameraTo: Failed. Increase bloat (%f) setting of func_map_boundary.\n", pBoundary->GetBloat());
		return;
	}
	vCamLookAt = tr.endpos;

	if( debug_snapcamerapos.GetBool() )
	{
		NDebugOverlay::Box( vCamLookAt, -Vector(16, 16, 16), Vector(16, 16, 16), 255, 0, 0, 255, 2.0f );
	}

#if 0
	vDir = -vForward;
	vDir.z = 0.0f;
	VectorNormalize(vDir);

	float fDist;
	float fAngle = acos( DotProduct( -vForward, vDir ) );
	fAngle = sin( fAngle );
	if( fAngle != 0 )
		fDist = m_fCurHeight / fAngle;
	else
		fDist = m_fCurHeight;
	//Msg("Cur: %f, travel: %f, angle: %f\n", m_fCurHeight, fDist, fAngle);

	// Figure out cam position
	//NDebugOverlay::Box( vCamLookAt, -Vector(32, 32, 32), Vector(32, 32, 32), 255, 0, 0, 255, 5 );
	//NDebugOverlay::SweptBox(vCamLookAt,  vCamLookAt + (-vForward *  m_fCurHeight), 
	//	-Vector(32, 32, 32), Vector(32, 32, 32), QAngle(), 0, 255, 0, 200, 5.0);
	UTIL_TraceHull( vCamLookAt, vCamLookAt + -vForward * fDist, GetPlayerMins(), GetPlayerMaxs(), CONTENTS_PLAYERCLIP, &traceFilter, &tr );
	if( tr.startsolid )
	{
		Warning("SnapCameraTo: Failed. Check your map boundary brush!\n");
		return;
	}

	SetDirectMove( tr.endpos );
#else
#ifdef CLIENT_DLL
	SetDirectMove( vCamLookAt );
#else
	SetAbsOrigin( vCamLookAt );
#endif // CLIENT_DLL
#endif // 0
}

#if 0
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const Vector CHL2WarsPlayer::GetPlayerMins( void ) const
{
	if( IsStrategicModeOn() )
	{
		return Vector(-128, -128, 0);
	}
	else
	{
		if ( IsObserver() )
		{
			return VEC_OBS_HULL_MIN;	
		}
		else
		{
			if ( GetFlags() & FL_DUCKING )
			{
				return VEC_DUCK_HULL_MIN;
			}
			else
			{
				return VEC_HULL_MIN;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const Vector CHL2WarsPlayer::GetPlayerMaxs( void ) const
{
	if( IsStrategicModeOn() )
	{
		return Vector(128, 128, 128);
	}
	else
	{
		if ( IsObserver() )
		{
			return VEC_OBS_HULL_MAX;	
		}
		else
		{
			if ( GetFlags() & FL_DUCKING )
			{
				return VEC_DUCK_HULL_MAX;
			}
			else
			{
				return VEC_HULL_MAX;
			}
		}
	}
}
#endif // 0