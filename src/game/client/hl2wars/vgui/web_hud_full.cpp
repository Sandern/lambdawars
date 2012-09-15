#include "cbase.h"
#include "hud.h"
#include "web_hud_full.h"
#include "iclientmode.h"
#include "view.h"
#include "vgui_controls/controls.h"
#include "vgui/ISurface.h"
#include "IVRenderView.h"
#include "KeyValues.h"
#include "FileSystem.h"
#include "tier1/UtlBuffer.h"

#include "webview_texgen.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INIT_HEALTH -1

#define URL	"http://tympanus.net/Tutorials/AnimatedContentMenu/"

#define PIXEL_BYTES 4 // RGBA (never changes)

ConVar web_ui_hud( "web_ui_hud", "0", FCVAR_CHEAT );

using namespace vgui;

DECLARE_HUDELEMENT( CWebHudFull );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWebHudFull::CWebHudFull( const char *pElementName ) :
  CHudElement( pElementName ), BaseClass( NULL, "WebHUDFull" )
{
	vgui::Panel *pParent = GetClientMode()->GetViewport();
	SetParent( pParent );

	m_iTextureID = ( -1 );

	// Health
	m_iHealth = INIT_HEALTH;

	m_pWebViewSurface = new SrcWebViewSurface();

	// Load a certain URL into our WebView instance
	//webView->loadURL(URL);

	// Load a certain html dir
	//g_pWebCore->setBaseDirectory("../../../steamapps/sourcemods/f-stop/special/webui/ingamemenu");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWebHudFull::~CWebHudFull( void )
{
	delete m_pWebViewSurface;

	if( webView )
	{
		RemoveWebview( webView );
		webView->Destroy();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWebHudFull::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	SetPaintBackgroundEnabled( false );

#if 0
	if ( webView != NULL )
		webView->destroy();
#endif // 0

	// Setting panel size
	GetHudSize( viewWidth, viewHeight );
	SetBounds(0, 0, viewWidth, viewHeight);

	SetWebViewSize( viewWidth, viewHeight );
	if( webView )
		webView->SetTransparent( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWebHudFull::ShouldDraw( void )
{
	if ( !web_ui_hud.GetBool() )
		return false;

	return ( CHudElement::ShouldDraw() && !engine->IsDrawingLoadingImage() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWebHudFull::OnThink()
{
	// Health
	Health();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWebHudFull::Health( void )
{
	int newHealth = 0;
	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	if ( local )
	{
		// Never below zero
		newHealth = MAX( local->GetHealth(), 0 );
	}

	// Only update the fade if we've changed health
	if ( newHealth == m_iHealth )
	{
		return;
	}

	m_iHealth = newHealth;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWebHudFull::HealthParse( void )
{
	if ( !webView )
		return;

	/*
	// Load the default text list
	CUtlBuffer buf( 1024, 0, CUtlBuffer::TEXT_BUFFER );
	if ( !g_pFullFileSystem->ReadFile( "special/webui/hud.html", NULL, buf ) )
		return;

	const char *data = (const char*)buf.Base();
	char finishedHtml[1024];

	// Replacing string in default text list
	Q_StrSubst( data, "{health}", VarArgs( "%i", m_iHealth ), finishedHtml, sizeof(finishedHtml));

	// Updating HTML file with new text
	//webView->loadHTML(finishedHtml);
	*/


	char fullpath[MAX_PATH];
	filesystem->RelativePathToFullPath( "ui/mainmenu.html", "MOD", fullpath, MAX_PATH );
	Msg("Loading html file: %s\n", fullpath);

	Awesomium::WebString awebfilename = Awesomium::WebString::CreateFromUTF8( "file:///", 8);
	Awesomium::WebString afilename = Awesomium::WebString::CreateFromUTF8(fullpath, Q_strlen(fullpath));
	awebfilename.Append( afilename );

	webView->LoadURL( Awesomium::WebURL( awebfilename ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWebHudFull::RenderWebPanel( void )
{
	if ( !webView )
		return;

	if( m_bRebuildPending )
	{
		RebuildWebView();
	}

	m_pWebViewSurface->Draw( GetWide(), GetTall() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWebHudFull::Paint( void )
{
	// Health
	HealthParse();
	
	// Rendering
	RenderWebPanel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWebHudFull::SetWebViewSize( int newWide, int newTall )
{
	if( !g_pWebCore )
		return;

	if( newWide == m_iWVWide && m_iWVTall == newTall )
		return;

	if( newWide < 2 ) newWide = 2;
	if( newTall < 2 ) newTall = 2;

	m_iWVWide = newWide;
	m_iWVTall = newTall;

	m_bRebuildPending = true;

	if( !webView )
	{
		webView = g_pWebCore->CreateWebView(m_iWVWide, m_iWVTall);
		AddWebview( webView, m_pWebViewSurface );
		//m_pWebView->set_js_method_handler( this );
		SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWebHudFull::RebuildWebView()
{
	webView->Resize(m_iWVWide, m_iWVTall);
	OnSizeChanged( m_iWVWide, m_iWVTall );
	m_bRebuildPending = false;
}