#include "cbase.h"
#include "clientmode_wars.h"
#include "vgui_int.h"
#include "ienginevgui.h"
#include "cdll_client_int.h"
#include "engine/IEngineSound.h"

#include "vgui/hl2warsviewport.h"
#include "wars_loading_panel.h"
#include "wars_logo_panel.h"
#include "ivmodemanager.h"
#include "panelmetaclassmgr.h"
#include "nb_header_footer.h"
#include "glow_outline_effect.h"
#include "c_hl2wars_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar default_fov( "default_fov", "75", FCVAR_CHEAT );
ConVar fov_desired( "fov_desired", "75", FCVAR_USERINFO, "Sets the base field-of-view.", true, 1.0, true, 75.0 );

vgui::HScheme g_hVGuiCombineScheme = 0;

vgui::DHANDLE<CSDK_Logo_Panel> g_hLogoPanel;

static IClientMode *g_pClientMode[ MAX_SPLITSCREEN_PLAYERS ];
IClientMode *GetClientMode()
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return g_pClientMode[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}

// --------------------------------------------------------------------------------- //
// CASWModeManager.
// --------------------------------------------------------------------------------- //

class CSDKModeManager : public IVModeManager
{
public:
	virtual void	Init();
	virtual void	SwitchMode( bool commander, bool force ) {}
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual void	ActivateMouse( bool isactive ) {}
};

static CSDKModeManager g_ModeManager;
IVModeManager *modemanager = ( IVModeManager * )&g_ModeManager;



// --------------------------------------------------------------------------------- //
// CASWModeManager implementation.
// --------------------------------------------------------------------------------- //

#define SCREEN_FILE		"scripts/vgui_screens.txt"

void CSDKModeManager::Init()
{
	for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		g_pClientMode[ i ] = GetClientModeNormal();
	}

	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );

	//GetClientVoiceMgr()->SetHeadLabelOffset( 40 );
}

void CSDKModeManager::LevelInit( const char *newmap )
{
	for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		GetClientMode()->LevelInit( newmap );
	}
}

void CSDKModeManager::LevelShutdown( void )
{
	for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		GetClientMode()->LevelShutdown();
	}
}

ClientModeSDK g_ClientModeNormal[ MAX_SPLITSCREEN_PLAYERS ];
IClientMode *GetClientModeNormal()
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return &g_ClientModeNormal[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}

ClientModeSDK* GetClientModeSDK()
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return &g_ClientModeNormal[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}

// these vgui panels will be closed at various times (e.g. when the level ends/starts)
static char const *s_CloseWindowNames[]={
	"InfoMessageWindow",
	"SkipIntro",
};
#if 0
//-----------------------------------------------------------------------------
// Purpose: this is the viewport that contains all the hud elements
//-----------------------------------------------------------------------------
class CHudViewport : public CBaseViewport
{
private:
	DECLARE_CLASS_SIMPLE( CHudViewport, CBaseViewport );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		GetHud().InitColors( pScheme );

		SetPaintBackgroundEnabled( false );
	}

	virtual void CreateDefaultPanels( void ) { /* don't create any panels yet*/ };
};
#endif // 0

//--------------------------------------------------------------------------------------------------------

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule the dll and that we don't call Sys_LoadModule 
//  over and over again.
static CDllDemandLoader g_GameUI( "gameui" );

class FullscreenSDKViewport : public HL2WarsViewport
{
private:
	DECLARE_CLASS_SIMPLE( FullscreenSDKViewport, HL2WarsViewport );

private:
	virtual void InitViewportSingletons( void )
	{
		SetAsFullscreenViewportInterface();
	}
};

class ClientModeSDKFullscreen : public	ClientModeSDK
{
	DECLARE_CLASS_SIMPLE( ClientModeSDKFullscreen, ClientModeSDK );
public:
	virtual void InitViewport()
	{
		// Skip over BaseClass!!!
		BaseClass::BaseClass::InitViewport();
		m_pViewport = new FullscreenSDKViewport();
		m_pViewport->Start( gameuifuncs, gameeventmanager );
	}
	virtual void Init()
	{
		CreateInterfaceFn gameUIFactory = g_GameUI.GetFactory();
		if ( gameUIFactory )
		{
			IGameUI *pGameUI = (IGameUI *) gameUIFactory(GAMEUI_INTERFACE_VERSION, NULL );
			if ( NULL != pGameUI )
			{
				// insert stats summary panel as the loading background dialog
				CSDK_Loading_Panel *pPanel = GSDKLoadingPanel();
				pPanel->InvalidateLayout( false, true );
				pPanel->SetVisible( false );
				pPanel->MakePopup( false );
				pGameUI->SetLoadingBackgroundDialog( pPanel->GetVPanel() );

				// add ASI logo to main menu
				CSDK_Logo_Panel *pLogo = new CSDK_Logo_Panel( NULL, "ASILogo" );
				vgui::VPANEL GameUIRoot = enginevgui->GetPanel( PANEL_GAMEUIDLL );
				pLogo->SetParent( GameUIRoot );
				g_hLogoPanel = pLogo;
			}		
		}

		// 
		//CASW_VGUI_Debug_Panel *pDebugPanel = new CASW_VGUI_Debug_Panel( GetViewport(), "ASW Debug Panel" );
		//g_hDebugPanel = pDebugPanel;

		// Skip over BaseClass!!!
		BaseClass::BaseClass::Init();

		// Load up the combine control panel scheme
		if ( !g_hVGuiCombineScheme )
		{
			g_hVGuiCombineScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), IsXbox() ? "resource/ClientScheme.res" : "resource/CombinePanelScheme.res", "CombineScheme" );
			if (!g_hVGuiCombineScheme)
			{
				Warning( "Couldn't load combine panel scheme!\n" );
			}
		}
	}
	void Shutdown()
	{
		DestroySDKLoadingPanel();
		if (g_hLogoPanel.Get())
		{
			delete g_hLogoPanel.Get();
		}
	}
};

//--------------------------------------------------------------------------------------------------------
static ClientModeSDKFullscreen g_FullscreenClientMode;
IClientMode *GetFullscreenClientMode( void )
{
	return &g_FullscreenClientMode;
}


void ClientModeSDK::Init()
{
	BaseClass::Init();

	gameeventmanager->AddListener( this, "game_newmap", false );
}
void ClientModeSDK::Shutdown()
{
	if ( SDKBackgroundMovie() )
	{
		SDKBackgroundMovie()->ClearCurrentMovie();
	}
	DestroySDKLoadingPanel();
	if (g_hLogoPanel.Get())
	{
		delete g_hLogoPanel.Get();
	}
}

void ClientModeSDK::InitViewport()
{
	m_pViewport = new HL2WarsViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}

void ClientModeSDK::LevelInit( const char *newmap )
{
	// reset ambient light
	static ConVarRef mat_ambient_light_r( "mat_ambient_light_r" );
	static ConVarRef mat_ambient_light_g( "mat_ambient_light_g" );
	static ConVarRef mat_ambient_light_b( "mat_ambient_light_b" );

	if ( mat_ambient_light_r.IsValid() )
	{
		mat_ambient_light_r.SetValue( "0" );
	}

	if ( mat_ambient_light_g.IsValid() )
	{
		mat_ambient_light_g.SetValue( "0" );
	}

	if ( mat_ambient_light_b.IsValid() )
	{
		mat_ambient_light_b.SetValue( "0" );
	}

	BaseClass::LevelInit(newmap);

	// sdk: make sure no windows are left open from before
	SDK_CloseAllWindows();

	// clear any DSP effects
	CLocalPlayerFilter filter;
	enginesound->SetRoomType( filter, 0 );
	enginesound->SetPlayerDSP( filter, 0, true );
}

void ClientModeSDK::LevelShutdown( void )
{
	BaseClass::LevelShutdown();

	// sdk:shutdown all vgui windows
	SDK_CloseAllWindows();
}
void ClientModeSDK::FireGameEvent( IGameEvent *event )
{
	const char *eventname = event->GetName();

	if ( Q_strcmp( "asw_mission_restart", eventname ) == 0 )
	{
		SDK_CloseAllWindows();
	}
	else if ( Q_strcmp( "game_newmap", eventname ) == 0 )
	{
		engine->ClientCmd("exec newmapsettings\n");
	}
	else
	{
		BaseClass::FireGameEvent(event);
	}
}
// Close all ASW specific VGUI windows that the player might have open
void ClientModeSDK::SDK_CloseAllWindows()
{
	SDK_CloseAllWindowsFrom(GetViewport());
}

// recursive search for matching window names
void ClientModeSDK::SDK_CloseAllWindowsFrom(vgui::Panel* pPanel)
{
	if (!pPanel)
		return;

	int num_names = NELEMS(s_CloseWindowNames);

	for (int k=0;k<pPanel->GetChildCount();k++)
	{
		Panel *pChild = pPanel->GetChild(k);
		if (pChild)
		{
			SDK_CloseAllWindowsFrom(pChild);
		}
	}

	// When VGUI is shutting down (i.e. if the player closes the window), GetName() can return NULL
	const char *pPanelName = pPanel->GetName();
	if ( pPanelName != NULL )
	{
		for (int i=0;i<num_names;i++)
		{
			if ( !strcmp( pPanelName, s_CloseWindowNames[i] ) )
			{
				pPanel->SetVisible(false);
				pPanel->MarkForDeletion();
			}
		}
	}
}

int	ClientModeSDK::KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( engine->Con_IsVisible() )
		return 1;

	// annoyingly we can't intercept ESC here, it'll still bring up the gameui stuff
	//  but we'll use it for a couple of things anyway
	if (keynum == KEY_ESCAPE)
	{
		C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
		if( pPlayer && pPlayer->GetControlledUnit() )
		{
			engine->ClientCmd( "player_release_control_unit" );
		}
	}

	return BaseClass::KeyInput(down, keynum, pszCurrentBinding);
}

void ClientModeSDK::DoPostScreenSpaceEffects( const CViewSetup *pSetup )
{
	CMatRenderContextPtr pRenderContext( materials );

	g_GlowObjectManager.RenderGlowEffects( pSetup, 0 /*GetSplitScreenPlayerSlot()*/ );
}

/*
CON_COMMAND(wars_test, "")
{
	Msg("HDR type: %d\n",  g_pMaterialSystemHardwareConfig->GetHDRType());
	Msg("Supports hdr type float: %d\n", materials->SupportsHDRMode( HDR_TYPE_FLOAT ));
	Msg("DX support level: %d\n", g_pMaterialSystemHardwareConfig->GetDXSupportLevel());
}
*/