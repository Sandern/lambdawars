//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#include "cbase.h"
#include "hud.h"
#include "clientmode_hl2wars.h"
#include "cdll_client_int.h"
#include "iinput.h"
#include "vgui/isurface.h"
#include "vgui/ipanel.h"
#include <vgui_controls/AnimationController.h>
#include "ivmodemanager.h"
#include "BuyMenu.h"
#include "filesystem.h"
#include "vgui/ivgui.h"
#include "hud_basechat.h"
#include "view_shared.h"
#include "view.h"
#include "ivrenderview.h"
#include "model_types.h"
#include "iefx.h"
#include "dlight.h"
#include <imapoverview.h>
#include "c_playerresource.h"
#include <keyvalues.h>
#include "text_message.h"
#include "panelmetaclassmgr.h"
#include "c_hl2wars_player.h"
#include "c_weapon__stubs.h"		//Tony; add stubs
#include "glow_outline_effect.h"

#include "../common/GameUI/IGameUI.h"

class CHudChat;

ConVar default_fov( "default_fov", "90", FCVAR_CHEAT );

IClientMode *g_pClientMode = NULL;

IClientMode *GetClientMode()
{
	return g_pClientMode;
}

//Tony; add stubs for cycler weapon and cubemap.
STUB_WEAPON_CLASS( cycler_weapon,   WeaponCycler,   C_BaseCombatWeapon );
STUB_WEAPON_CLASS( weapon_cubemap,  WeaponCubemap,  C_BaseCombatWeapon );

// See interface.h/.cpp for specifics:  basically this ensures that we actually 
// Sys_UnloadModule the dll and that we don't call Sys_LoadModule
//  over and over again.
static CDllDemandLoader g_GameUI( "gameui" );

//-----------------------------------------------------------------------------
// HACK: the detail sway convars are archive, and default to 0.  Existing CS:S players thus have no detail
// prop sway.  We'll force them to DoD's default values for now.  What we really need in the long run is
// a system to apply changes to archived convars' defaults to existing players.
/*extern ConVar cl_detail_max_sway;
extern ConVar cl_detail_avoid_radius;
extern ConVar cl_detail_avoid_force;
extern ConVar cl_detail_avoid_recover_speed;*/
// --------------------------------------------------------------------------------- //
// CHL2WarsModeManager.
// --------------------------------------------------------------------------------- //

class CHL2WarsModeManager : public IVModeManager
{
public:
	virtual void	Init();
	virtual void	SwitchMode( bool commander, bool force ) {}
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual void	ActivateMouse( bool isactive ) {}
};

static CHL2WarsModeManager g_ModeManager;
IVModeManager *modemanager = ( IVModeManager * )&g_ModeManager;

// --------------------------------------------------------------------------------- //
// CHL2WarsModeManager implementation.
// --------------------------------------------------------------------------------- //

#define SCREEN_FILE		"scripts/vgui_screens.txt"

void CHL2WarsModeManager::Init()
{
	g_pClientMode = GetClientModeNormal();
	
	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );
	
}

void CHL2WarsModeManager::LevelInit( const char *newmap )
{
	g_pClientMode->LevelInit( newmap );
	// HACK: the detail sway convars are archive, and default to 0.  Existing CS:S players thus have no detail
	// prop sway.  We'll force them to DoD's default values for now.
	/*if ( !cl_detail_max_sway.GetFloat() &&
		!cl_detail_avoid_radius.GetFloat() &&
		!cl_detail_avoid_force.GetFloat() &&
		!cl_detail_avoid_recover_speed.GetFloat() )
	{
		cl_detail_max_sway.SetValue( "5" );
		cl_detail_avoid_radius.SetValue( "64" );
		cl_detail_avoid_force.SetValue( "0.4" );
		cl_detail_avoid_recover_speed.SetValue( "0.25" );
	}*/

	g_GlowObjectManager.Init();
}

void CHL2WarsModeManager::LevelShutdown( void )
{
	g_pClientMode->LevelShutdown();

	g_GlowObjectManager.Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeHL2WarsNormal::ClientModeHL2WarsNormal()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ClientModeHL2WarsNormal::~ClientModeHL2WarsNormal()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ClientModeHL2WarsNormal::Init()
{
	BaseClass::Init();

	CreateInterfaceFn gameUIFactory = g_GameUI.GetFactory();
	if ( gameUIFactory )
	{
		m_pGameUI = (IGameUI *) gameUIFactory(GAMEUI_INTERFACE_VERSION, 
			NULL );
		/*if ( NULL != m_pGameUI )
		{
			// insert custom loading panel for the loading dialog
			CLoadingPanel *pPanel = GetLoadingPanel();
			pPanel->InvalidateLayout( false, true );
			pPanel->SetVisible( false );
			pPanel->MakePopup( false );
			m_pGameUI->SetLoadingBackgroundDialog( 
				pPanel->GetVPanel() );
		}*/
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ClientModeHL2WarsNormal::SetLoadingBackgroundDialog( boost::python::object panel )
{
	if( !m_pGameUI ) {
		m_refLoadingDialog = bp::object();
		return;
	}

	if( panel.ptr() == Py_None )
	{
		m_pGameUI->SetLoadingBackgroundDialog( 
			0 );
		m_refLoadingDialog = panel;
		return;
	}
	Panel *pPanel = boost::python::extract<Panel *>(panel);
	if( !pPanel )
	{
		Warning("SetLoadingBackgroundDialog: panel is not an instance of the class Panel.");
		return;
	}
	m_refLoadingDialog = panel;
	m_pGameUI->SetLoadingBackgroundDialog( 
		pPanel->GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ClientModeHL2WarsNormal::InitViewport()
{
	m_pViewport = new HL2WarsViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ClientModeHL2WarsNormal::Shutdown()
{
	BaseClass::Shutdown();

	if( m_pGameUI )
		m_pGameUI->SetLoadingBackgroundDialog(0);
	m_refLoadingDialog = bp::object();
}

int ClientModeHL2WarsNormal::KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	return BaseClass::KeyInput( down, keynum, pszCurrentBinding );
}

ClientModeHL2WarsNormal g_ClientModeNormal;

IClientMode *GetClientModeNormal()
{
	return &g_ClientModeNormal;
}


ClientModeHL2WarsNormal* GetClientModeHL2WarsNormal()
{
	Assert( dynamic_cast< ClientModeHL2WarsNormal* >( GetClientModeNormal() ) );

	return static_cast< ClientModeHL2WarsNormal* >( GetClientModeNormal() );
}

int ClientModeHL2WarsNormal::GetDeathMessageStartHeight( void )
{
	return m_pViewport->GetDeathMessageStartHeight();
}

void ClientModeHL2WarsNormal::PostRenderVGui()
{
}

bool ClientModeHL2WarsNormal::CanRecordDemo( char *errorMsg, int length ) const
{
	C_HL2WarsPlayer *player = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
	if ( !player )
	{
		return true;
	}

	if ( !player->IsAlive() )
	{
		return true;
	}

	return true;
}

void ClientModeHL2WarsNormal::DoPostScreenSpaceEffects( const CViewSetup *pSetup )
{
	CMatRenderContextPtr pRenderContext( materials );

	g_GlowObjectManager.RenderGlowEffects( pSetup, /* GetSplitScreenPlayerSlot() */ -1 );
}