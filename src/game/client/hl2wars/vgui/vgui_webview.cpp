//====== Copyright © 20xx, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/IInput.h>
#include <vgui/ISurface.h>
#include "materialsystem/IMaterialVar.h"
#include "vgui_webview.h"
#include "input.h"
#include <ConVar.h>
#include "FileSystem.h"
#include "clientmode_shared.h"

#ifndef DISABLE_PYTHON
	#include "src_python.h"
#endif // DISABLE_PYTHON

#include <Awesomium/WebCore.h>

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

ConVar g_debug_webview_methodcalls("g_debug_webview_methodcalls", "0", FCVAR_CHEAT);

extern ConVar g_debug_webview;

#define WEBVIEW_VALID if( !m_pWebView )										\
	{																		\
		PyErr_SetString(PyExc_ValueError, "Webview not initialized yet");	\
		throw boost::python::error_already_set();							\
	}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class WebViewPanel : public vgui::Panel 
{
public:
	DECLARE_CLASS_SIMPLE(WebViewPanel, vgui::Panel);

	WebViewPanel( WebView *pController, vgui::Panel *pParent );
	~WebViewPanel();

	void Paint();
	virtual void OnSizeChanged(int newWide, int newTall);
	virtual void PerformLayout();

	virtual void OnCursorMoved(int x,int y);
	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void OnMouseDoublePressed(vgui::MouseCode code);
	virtual void OnMouseReleased(vgui::MouseCode code);
	virtual void OnMouseWheeled(int delta);

	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void OnKeyCodeTyped(vgui::KeyCode code);
	virtual void OnKeyTyped(wchar_t unichar);
	virtual void OnKeyCodeReleased(vgui::KeyCode code);
	int	GetKeyModifiers();

	virtual vgui::HCursor WebViewPanel::GetCursor();

	void SetWebView( Awesomium::WebView *pWebView ) { m_pWebView = pWebView; }

private:
	WebView *m_pController;
	Awesomium::WebView *m_pWebView;
	int m_iMouseX, m_iMouseY;
};

WebViewPanel::WebViewPanel( WebView *pController, vgui::Panel *pParent ) : Panel( NULL, "WebViewPanel" ), m_pController(pController), m_pWebView(NULL)
{
	SetPaintBackgroundEnabled( false );

	SetParent( pParent ? pParent : GetClientMode()->GetViewport() );
}

WebViewPanel::~WebViewPanel()
{
	m_pWebView = NULL;
	m_pController = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebViewPanel::Paint()
{
	if( !g_pWebCore || !m_pController )
		return;

	if( !m_pWebView )
		return;

	if( m_pController->m_bRebuildPending )
	{
		m_pController->RebuildWebView();
	}

	m_pController->m_pWebViewSurface->Draw( GetWide(), GetTall() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebViewPanel::OnSizeChanged(int newWide, int newTall)
{
	BaseClass::OnSizeChanged( newWide, newTall );

	// Don't really care about this one
#if 0
	if( !g_pWebCore || !m_pController )
		return;

	if( !m_pWebView )
		return;

	m_pController->OnSizeChanged( newWide, newTall );
#endif // 0
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebViewPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	if( !g_pWebCore || !m_pController )
		return;

	if( !m_pWebView )
		return;

	m_pController->PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebViewPanel::OnCursorMoved( int x,int y )
{
	if( !m_pWebView )
		return;

	m_iMouseX = x;
	m_iMouseY = y;
	if( m_pController->GetPassMouseTruIfAlphaZero() && m_pController->IsAlphaZeroAt( x, y ) )
	{
		if( g_debug_webview.GetInt() > 2 )
			DevMsg("WebView: passed cursor move %d %d to parent (alpha zero)\n", x, y);

		CallParentFunction(new KeyValues("OnCursorMoved", "x", x, "y", y));
		//return;
	}

	m_pWebView->InjectMouseMove( x, y );

	if( g_debug_webview.GetInt() > 2 )
		DevMsg("WebView: injected cursor move %d %d\n", x, y);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebViewPanel::OnMousePressed(vgui::MouseCode code)
{
	if( !m_pWebView )
		return;

	if( m_pController->GetPassMouseTruIfAlphaZero() && m_pController->IsAlphaZeroAt( m_iMouseX, m_iMouseY ) )
	{
		if( g_debug_webview.GetInt() > 0 )
			DevMsg("WebView: passed mouse pressed %d %d to parent\n", m_iMouseX, m_iMouseY);

		CallParentFunction(new KeyValues("MousePressed", "code", code));
		return;
	}
	
	switch( code )
	{
	case MOUSE_LEFT:
		m_pWebView->InjectMouseDown(Awesomium::kMouseButton_Left);
		break;
	case MOUSE_RIGHT:
		m_pWebView->InjectMouseDown(Awesomium::kMouseButton_Right);
		break;
	case MOUSE_MIDDLE:
		m_pWebView->InjectMouseDown(Awesomium::kMouseButton_Middle);
		break;
	}

	if( m_pController->m_bUseMouseCapture )
	{
		// Make sure released is called on this panel
		vgui::input()->SetMouseCaptureEx(GetVPanel(), code);
	}

	if( g_debug_webview.GetInt() > 0 )
		DevMsg("WebView: injected mouse pressed %d %d (mouse capture: %d)\n", m_iMouseX, m_iMouseY, (int)(m_pController->m_bUseMouseCapture));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebViewPanel::OnMouseDoublePressed(vgui::MouseCode code)
{
	if( !m_pWebView )
		return;

	if( m_pController->GetPassMouseTruIfAlphaZero() && m_pController->IsAlphaZeroAt( m_iMouseX, m_iMouseY ) )
	{
		if( g_debug_webview.GetInt() > 0 )
			DevMsg("WebView: passed mouse double pressed %d %d to parent\n", m_iMouseX, m_iMouseY);

		CallParentFunction(new KeyValues("MouseDoublePressed", "code", code));
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebViewPanel::OnMouseReleased(vgui::MouseCode code)
{
	if( !m_pWebView )
		return;

	if( vgui::input()->GetMouseCapture() == GetVPanel() )
	{
		vgui::input()->SetMouseCaptureEx(0, code);
	}

	if( m_pController->GetPassMouseTruIfAlphaZero() && m_pController->IsAlphaZeroAt( m_iMouseX, m_iMouseY ) )
	{
		if( g_debug_webview.GetInt() > 0 )
			DevMsg("WebView: passed mouse released %d %d to parent\n", m_iMouseX, m_iMouseY);

		CallParentFunction(new KeyValues("MouseReleased", "code", code));
		return;
	}

	switch( code )
	{
	case MOUSE_LEFT:
		m_pWebView->InjectMouseUp(Awesomium::kMouseButton_Left);
		break;
	case MOUSE_RIGHT:
		m_pWebView->InjectMouseUp(Awesomium::kMouseButton_Right);
		break;
	case MOUSE_MIDDLE:
		m_pWebView->InjectMouseUp(Awesomium::kMouseButton_Middle);
		break;
	}

	if( g_debug_webview.GetInt() > 0 )
		DevMsg("WebView: injected mouse released %d %d\n", m_iMouseX, m_iMouseY);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebViewPanel::OnMouseWheeled( int delta )
{
	if( !m_pWebView )
		return;

	if( m_pController->GetPassMouseTruIfAlphaZero() && m_pController->IsAlphaZeroAt( m_iMouseX, m_iMouseY ) )
	{
		CallParentFunction(new KeyValues("MouseWheeled", "delta", delta));
		return;
	}

	// VGUI just gives -1 or +1. injectMouseWheel expects
	// the number of pixels to shift.
	// TODO: Scale/accelerate?
	m_pWebView->InjectMouseWheel( delta * 20, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	WebViewPanel::GetKeyModifiers()
{
	int modifiers = 0;
	if (vgui::input()->IsKeyDown(KEY_LSHIFT) || vgui::input()->IsKeyDown(KEY_RSHIFT)) 
		modifiers |= Awesomium::WebKeyboardEvent::kModShiftKey;
	if (vgui::input()->IsKeyDown(KEY_LCONTROL) || vgui::input()->IsKeyDown(KEY_RCONTROL))
		modifiers |= Awesomium::WebKeyboardEvent::kModControlKey;
	if (vgui::input()->IsKeyDown(KEY_LALT) || vgui::input()->IsKeyDown(KEY_RALT))
		modifiers |= Awesomium::WebKeyboardEvent::kModAltKey;
	if (vgui::input()->IsKeyDown(KEY_LWIN) || vgui::input()->IsKeyDown(KEY_RWIN))
		modifiers |= Awesomium::WebKeyboardEvent::kModMetaKey;
	return modifiers;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebViewPanel::OnKeyCodePressed(vgui::KeyCode code)
{
	if( !m_pWebView )
		return;

	Awesomium::WebKeyboardEvent keyEvent;
	keyEvent.modifiers = GetKeyModifiers();
	keyEvent.native_key_code = 0;

	int idx = m_AwesomiumSrcKeyMap.Find(code);
	if( m_AwesomiumSrcKeyMap.IsValidIndex( idx ) )
	{
		keyEvent.virtual_key_code = m_AwesomiumSrcKeyMap[idx];
	}
	else
	{
		keyEvent.virtual_key_code = Awesomium::KeyCodes::AK_UNKNOWN;
		Warning("WebView::OnKeyCodeReleased: Unknown key code %d\n", code);
	}

	char *buf = new char[20];
	Awesomium::GetKeyIdentifierFromVirtualKeyCode(keyEvent.virtual_key_code, &buf);
	Q_strncpy(keyEvent.key_identifier, buf, 20);
	delete[] buf;

	keyEvent.type = Awesomium::WebKeyboardEvent::kTypeKeyDown;

	m_pWebView->InjectKeyboardEvent( keyEvent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebViewPanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	if( !m_pWebView )
		return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebViewPanel::OnKeyTyped(wchar_t unichar)
{
	if( !m_pWebView )
		return;

	Awesomium::WebKeyboardEvent keyEvent;
	keyEvent.modifiers = GetKeyModifiers();;
	keyEvent.text[0] = unichar;
	keyEvent.type = Awesomium::WebKeyboardEvent::kTypeChar;

	m_pWebView->InjectKeyboardEvent( keyEvent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebViewPanel::OnKeyCodeReleased(vgui::KeyCode code)
{
	if( !m_pWebView )
		return;

	Awesomium::WebKeyboardEvent keyEvent;
	keyEvent.modifiers = GetKeyModifiers();
	keyEvent.native_key_code = 0;

	int idx = m_AwesomiumSrcKeyMap.Find(code);
	if( m_AwesomiumSrcKeyMap.IsValidIndex( idx ) )
	{
		keyEvent.virtual_key_code = m_AwesomiumSrcKeyMap[idx];
	}
	else
	{
		keyEvent.virtual_key_code = Awesomium::KeyCodes::AK_UNKNOWN;
		Warning("WebView::OnKeyCodeReleased: Unknown key code %d\n", code);
	}

	char *buf = new char[20];
	Awesomium::GetKeyIdentifierFromVirtualKeyCode(keyEvent.virtual_key_code, &buf);
	Q_strncpy(keyEvent.key_identifier, buf, 20);
	delete[] buf;

	keyEvent.type = Awesomium::WebKeyboardEvent::kTypeKeyUp;

	m_pWebView->InjectKeyboardEvent( keyEvent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
vgui::HCursor WebViewPanel::GetCursor()
{
	if( !m_pWebView )
	{
		if( GetParent() )
			return GetParent()->GetCursor();
		return BaseClass::GetCursor();
	}

	if( m_pController->GetPassMouseTruIfAlphaZero() && m_pController->IsAlphaZeroAt( m_iMouseX, m_iMouseY ) )
	{
		if( GetParent() )
			return GetParent()->GetCursor();
	}

	return BaseClass::GetCursor();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
WebView::WebView( bool bCustomViewSize, vgui::Panel *pParent ) :
	m_pWebView(NULL),  
	m_bCustomViewSize(bCustomViewSize),  
	m_bRebuildPending(false),
	m_bInGame(false), 
	m_bPassMouseTruIfAlphaZero(false), 
	m_bUseMouseCapture(true)
{
	m_pPanel = new WebViewPanel( this, pParent );

	m_pWebViewSurface = new SrcWebViewSurface();

	m_iWVWide = m_iWVTall = 0;

	SetWebViewSize(2, 2);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
WebView::~WebView( void )
{
	delete m_pPanel;
	m_pPanel = NULL;

	Clear();
}

void WebView::Clear()
{
#ifndef DISABLE_PYTHON
	if( SrcPySystem()->IsPythonRunning() )
	{
		SetViewListener( bp::object() );
		SetLoadListener( bp::object() );
		SetProcessListener( bp::object() );
	}
#endif // DISABLE_PYTHON

	if( m_pPanel )
	{
		m_pPanel->SetWebView( NULL );
		m_pPanel->SetVisible( false );
	}

	// NOTE: Only need to destroy the view if web core is still active.
	//		 Otherwise it is already destroyed by webcore. 
	//		 This is needed because the vgui elements are destroyed after
	//		 the shutdown function of the gamesystem above.
	if( m_pWebView )
	{
		RemoveWebview( m_pWebView );
		if( g_pWebCore )
			m_pWebView->Destroy();
		m_pWebView = NULL;
	}

	if( m_pWebViewSurface )
	{
		delete m_pWebViewSurface;
		m_pWebViewSurface = NULL;
	}
}

bool WebView::IsValid()
{
	return g_pWebCore && m_pWebView && m_pWebView->process_id() != 0;
}

void WebView::SetSize( int wide, int tall )
{
	m_pPanel->SetSize( wide, tall );
	if( !m_bCustomViewSize )
		SetWebViewSize( wide, tall );
}

void WebView::SetPos( int x, int y )
{
	m_pPanel->SetPos( x, y );
}

void WebView::SetZPos( int z )
{
	m_pPanel->SetZPos( z );
}

void WebView::SetVisible(bool state)
{
	m_pPanel->SetVisible( state );
}

bool WebView::IsVisible()
{
	return m_pPanel->IsVisible( );
}

void WebView::SetMouseInputEnabled( bool state )
{
	m_pPanel->SetMouseInputEnabled( state );
}

void WebView::SetKeyBoardInputEnabled( bool state )
{
	m_pPanel->SetKeyBoardInputEnabled( state );
}

bool WebView::IsMouseInputEnabled()
{
	return m_pPanel->IsMouseInputEnabled( );
}

bool WebView::IsKeyBoardInputEnabled()
{
	return m_pPanel->IsKeyBoardInputEnabled( );
}

void WebView::SetCursor(vgui::HCursor cursor)
{
	return m_pPanel->SetCursor( cursor );
}

vgui::HCursor WebView::GetCursor()
{
	return m_pPanel->GetCursor( );
}

#if 0
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebView::OnSizeChanged(int newWide, int newTall)
{
	BaseClass::OnSizeChanged( newWide, newTall );

	if( !m_bCustomViewSize )
		SetWebViewSize( newWide, newTall );
}
#endif // 0

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebView::LoadHTML( const char *html )
{
#ifdef ENABLE_AWESOMIUM
	WEBVIEW_VALID;

	//m_pWebView->LoadHTML( std::string(html) ); // TODO
#endif // DISABLE_AWESOMIUM
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebView::LoadURL( const char *url )
{
#ifdef ENABLE_AWESOMIUM
	WEBVIEW_VALID;

	Awesomium::WebString aurl = Awesomium::WebString::CreateFromUTF8(url, Q_strlen(url));
	m_pWebView->LoadURL( Awesomium::WebURL( aurl ) );
#endif // DISABLE_AWESOMIUM
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebView::LoadFile( const char *filename )
{
#ifdef ENABLE_AWESOMIUM
	WEBVIEW_VALID;

	char fullpath[MAX_PATH];
	filesystem->RelativePathToFullPath( filename, "MOD", fullpath, MAX_PATH );
	//Msg("Loading html file: %s\n", fullpath);

	Awesomium::WebString awebfilename = Awesomium::WebString::CreateFromUTF8( "file:///", 8);
	Awesomium::WebString afilename = Awesomium::WebString::CreateFromUTF8(fullpath, Q_strlen(fullpath));
	awebfilename.Append( afilename );

	m_pWebView->LoadURL( Awesomium::WebURL( awebfilename ) );
#endif // DISABLE_AWESOMIUM
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebView::Reload( bool ignore_cache )
{
#ifdef ENABLE_AWESOMIUM
	WEBVIEW_VALID;

	m_pWebView->Reload( ignore_cache );
#endif // DISABLE_AWESOMIUM
}

#ifndef DISABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebView::ExecuteJavascript( bp::str script, bp::str frame_xpath )
{
#ifdef ENABLE_AWESOMIUM
	WEBVIEW_VALID;

	Awesomium::WebString wscript = PyObjectToWebString( script );
	Awesomium::WebString wframe_xpath = PyObjectToWebString( frame_xpath );

	m_pWebView->ExecuteJavascript( wscript, wframe_xpath );
#endif // DISABLE_AWESOMIUM
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::object WebView::ExecuteJavascriptWithResult(bp::str script, bp::str frame_xpath)
{
#ifdef ENABLE_AWESOMIUM
	WEBVIEW_VALID;

	Awesomium::WebString wscript = PyObjectToWebString( script );
	Awesomium::WebString wframe_xpath = PyObjectToWebString( frame_xpath );

	Awesomium::JSValue result = m_pWebView->ExecuteJavascriptWithResult( wscript, wframe_xpath );
	if( GetWebView()->last_error() != Awesomium::kError_None )
	{
		PyErr_SetString(PyExc_ValueError, "An error occurred during ExecuteJavascriptWithResult");	\
		throw boost::python::error_already_set();													\
	}

	return ConvertJSValue( result );
#else
	return bp::object();
#endif // DISABLE_AWESOMIUM
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::object WebView::CreateGlobalJavascriptObject( bp::object name )
{
#ifdef ENABLE_AWESOMIUM
	WEBVIEW_VALID;

	Awesomium::WebString wname = PyObjectToWebString( name );

	Awesomium::JSValue result = m_pWebView->CreateGlobalJavascriptObject( wname );
	if( GetWebView()->last_error() != Awesomium::kError_None )
	{
		PyErr_SetString(PyExc_ValueError, "An error occurred during CreateGlobalJavascriptObject");	\
		throw boost::python::error_already_set();													\
	}
	return ConvertJSValue( result );
#else
	return bp::object();
#endif // DISABLE_AWESOMIUM
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebView::OnMethodCall(Awesomium::WebView* caller,
						unsigned int remote_object_id, 
						const Awesomium::WebString& method_name,
						const Awesomium::JSArray& args)
{
	int size = method_name.ToUTF8(NULL, 0);
	char *pMethodName = new char[size+1];
	method_name.ToUTF8(pMethodName, size);
	pMethodName[size] = '\0';

	if( g_debug_webview_methodcalls.GetBool() )
		Msg("OnMethodCall: %s\n", pMethodName );

	try {
		bp::str pymethodname( (const char *)pMethodName );

		bp::list pargs = ConvertJSArguments( args );
		PyOnMethodCall( remote_object_id, pymethodname, pargs );
	} catch( bp::error_already_set & ) {
		PyErr_Print();
	}

	delete pMethodName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Awesomium::JSValue WebView::OnMethodCallWithReturnValue(Awesomium::WebView* caller,
											unsigned int remote_object_id,
											const Awesomium::WebString& method_name,
											const Awesomium::JSArray& args)
{
    Assert( SrcPySystem()->IsPythonRunning() );
    Assert( GetCurrentThreadId() == g_hPythonThreadID );

	Awesomium::JSValue rs;

	int size = method_name.ToUTF8(NULL, 0);
	char *pMethodName = new char[size+1];
	method_name.ToUTF8(pMethodName, size);
	pMethodName[size] = '\0';

	if( g_debug_webview_methodcalls.GetBool() )
		Msg("OnMethodCallWithReturnValue: %s\n", pMethodName );

	try {
		bp::str pymethodname( (const char *)pMethodName );

		bp::list pargs = ConvertJSArguments( args );
		bp::object prv = PyOnMethodCall( remote_object_id, pymethodname, pargs );
		rs = ConvertObjectToJSValue( prv );
	} catch( bp::error_already_set & ) {
		PyErr_Print();
	}

	delete pMethodName;

	return rs;
}
#endif // DISABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebView::Focus()
{
#ifdef ENABLE_AWESOMIUM
	WEBVIEW_VALID;
	m_pWebView->Focus();
#endif // DISABLE_AWESOMIUM
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebView::Unfocus()
{
#ifdef ENABLE_AWESOMIUM
	WEBVIEW_VALID;
	m_pWebView->Unfocus();
#endif // DISABLE_AWESOMIUM
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebView::SetTransparent( bool istransparent )
{
#ifdef ENABLE_AWESOMIUM
	WEBVIEW_VALID;
	m_pWebView->SetTransparent( istransparent );
#endif // DISABLE_AWESOMIUM
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int WebView::GetAlphaAt( int x, int y )
{
	if( x < 0 || y < 0 || x >= m_pWebViewSurface->GetClipWidth() || y >= m_pWebViewSurface->GetClipTall() )
		return 0;
	unsigned char *pImageData = m_pWebViewSurface->GetImageData();
	if( pImageData )
	{
		int channels = m_pWebViewSurface->GetChannels();
		return pImageData[(y * m_pWebViewSurface->GetWidth() * channels) + (x * channels)+ 3];
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebView::SetWebViewSize( int newWide, int newTall )
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

	if( !m_pWebView )
	{
		m_pWebView = g_pWebCore->CreateWebView(m_iWVWide, m_iWVTall, WebCoreSystem().GetSession());
		AddWebview( m_pWebView, m_pWebViewSurface );
		m_pWebView->set_js_method_handler( this );
		m_pPanel->SetWebView( m_pWebView );
		m_pPanel->SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebView::RebuildWebView()
{
	m_pWebView->Resize(m_iWVWide, m_iWVTall);
	OnSizeChanged( m_iWVWide, m_iWVTall );
	m_bRebuildPending = false;
}

#ifndef DISABLE_PYTHON
void WebView::SetViewListener( bp::object listener )
{
	if( listener.ptr() == Py_None )
	{
		m_refViewListener = bp::object();
		if( m_pWebView && g_pWebCore )
			m_pWebView->set_view_listener( NULL );
		return;
	}

	m_refViewListener = listener;
	if( m_pWebView && g_pWebCore )
		m_pWebView->set_view_listener( (Awesomium::WebViewListener::View *)this );
}

bp::object WebView::GetViewListener()
{
	return m_refViewListener;
}

void WebView::SetLoadListener( bp::object listener )
{
	if( listener.ptr() == Py_None )
	{
		m_refLoadListener = bp::object();
		if( m_pWebView && g_pWebCore )
			m_pWebView->set_load_listener( NULL );
		return;
	}

	m_refLoadListener = listener;
	if( m_pWebView && g_pWebCore )
		m_pWebView->set_load_listener( (Awesomium::WebViewListener::Load *)this );
}

bp::object WebView::GetLoadListener()
{
	return m_refLoadListener;
}

void WebView::SetProcessListener( bp::object listener )
{
	if( listener.ptr() == Py_None )
	{
		m_refProcessListener = bp::object();
		if( m_pWebView && g_pWebCore )
			m_pWebView->set_process_listener( NULL );
		return;
	}

	m_refProcessListener = listener;
	if( m_pWebView && g_pWebCore )
		m_pWebView->set_process_listener( (Awesomium::WebViewListener::Process *)this );
}

bp::object WebView::GetProcessListener()
{
	return m_refProcessListener;
}
#endif // DISABLE_PYTHON

// VIEW functions
void WebView::OnChangeTitle(Awesomium::WebView* caller,
							const Awesomium::WebString& title)
{
#ifndef DISABLE_PYTHON
	if( m_refViewListener.ptr() != Py_None )
	{
		try {
			m_refViewListener.attr("OnChangeTitle")( ConverToPyString( title ) );
		} catch( bp::error_already_set & ) {
			PyErr_Print();
			return;
		}
	}
#endif // DISABLE_PYTHON
}	

void WebView::OnChangeAddressBar(Awesomium::WebView* caller,
								const Awesomium::WebURL& url)
{
#ifndef DISABLE_PYTHON
	if( m_refViewListener.ptr() != Py_None )
	{
		try {
			m_refViewListener.attr("OnChangeAddressBar")( ConverToPyString( url.spec() ) );
		} catch( bp::error_already_set & ) {
			PyErr_Print();
			return;
		}
	}
#endif // DISABLE_PYTHON
}

void WebView::OnChangeTooltip(Awesomium::WebView* caller,
							const Awesomium::WebString& tooltip)
{
#ifndef DISABLE_PYTHON
	if( m_refViewListener.ptr() != Py_None )
	{
		try {
			m_refViewListener.attr("OnChangeTooltip")( ConverToPyString( tooltip ) );
		} catch( bp::error_already_set & ) {
			PyErr_Print();
			return;
		}
	}

#endif // DISABLE_PYTHON
}

void WebView::OnChangeTargetURL(Awesomium::WebView* caller,
								const Awesomium::WebURL& url)
{
#ifndef DISABLE_PYTHON
	if( m_refViewListener.ptr() != Py_None )
	{
		try {
			m_refViewListener.attr("OnChangeTargetURL")( ConverToPyString( url.spec() ) );
		} catch( bp::error_already_set & ) {
			PyErr_Print();
			return;
		}
	}
#endif // DISABLE_PYTHON
}

void WebView::OnChangeCursor(Awesomium::WebView* caller,
							Awesomium::Cursor cursor)
{
#ifndef DISABLE_PYTHON
	if( m_refViewListener.ptr() != Py_None )
	{
		try {
			m_refViewListener.attr("OnChangeCursor")( cursor );
		} catch( bp::error_already_set & ) {
			PyErr_Print();
			return;
		}
	}
	else
#endif // DISABLE_PYTHON
	{
		switch( cursor )
		{
		case Awesomium::kCursor_Hand:
			SetCursor( dc_hand );
			break;
		case Awesomium::kCursor_Cross:
			SetCursor( dc_crosshair );
			break;
		case Awesomium::kCursor_IBeam:
			SetCursor( dc_ibeam );
			break;
		case Awesomium::kCursor_Wait:
			SetCursor( dc_hourglass );
			break;
		case Awesomium::kCursor_Pointer:
		default:
			SetCursor( dc_arrow );
			break;
		}
	}
}

void WebView::OnChangeFocus(Awesomium::WebView* caller,
							Awesomium::FocusedElementType focused_type)
{
#ifndef DISABLE_PYTHON
	if( m_refViewListener.ptr() != Py_None )
	{
		try {
			m_refViewListener.attr("OnChangeFocus")( focused_type );
		} catch( bp::error_already_set & ) {
			PyErr_Print();
			return;
		}
	}
#endif // DISABLE_PYTHON
}

void WebView::OnShowCreatedWebView(Awesomium::WebView* caller,
								Awesomium::WebView* new_view,
								const Awesomium::WebURL& opener_url,
								const Awesomium::WebURL& target_url,
								const Awesomium::Rect& initial_pos,
								bool is_popup)
{
#ifndef DISABLE_PYTHON
	if( m_refViewListener.ptr() != Py_None )
	{
		try {
			m_refViewListener.attr("OnShowCreatedWebView")( ConverToPyString( opener_url.spec() ), ConverToPyString( target_url.spec() ), is_popup );
		} catch( bp::error_already_set & ) {
			PyErr_Print();
			return;
		}
	}
#endif // DISABLE_PYTHON
}

// Load functions
void WebView::OnBeginLoadingFrame(Awesomium::WebView* caller,
								int64 frame_id,
								bool is_main_frame,
								const Awesomium::WebURL& url,
								bool is_error_page)
{
#ifndef DISABLE_PYTHON
	if( m_refLoadListener.ptr() != Py_None )
	{
		try {
			m_refLoadListener.attr("OnBeginLoadingFrame")( (long long)frame_id, is_main_frame, ConverToPyString( url.spec() ), is_error_page );
		} catch( bp::error_already_set & ) {
			PyErr_Print();
			return;
		}
	}
#endif // DISABLE_PYTHON
}

void WebView::OnFailLoadingFrame(Awesomium::WebView* caller,
								int64 frame_id,
								bool is_main_frame,
								const Awesomium::WebURL& url,
								int error_code,
								const Awesomium::WebString& error_desc)
{
#ifndef DISABLE_PYTHON
	if( m_refLoadListener.ptr() != Py_None )
	{
		try {
			m_refLoadListener.attr("OnFailLoadingFrame")( (long long)frame_id, is_main_frame, ConverToPyString( url.spec() ), error_code, ConverToPyString( error_desc ) );
		} catch( bp::error_already_set & ) {
			PyErr_Print();
			return;
		}
	}
#endif // DISABLE_PYTHON
}

void WebView::OnFinishLoadingFrame(Awesomium::WebView* caller,
								int64 frame_id,
								bool is_main_frame,
								const Awesomium::WebURL& url)
{
#ifndef DISABLE_PYTHON
	if( m_refLoadListener.ptr() != Py_None )
	{
		try {
			m_refLoadListener.attr("OnFinishLoadingFrame")( (long long)frame_id, is_main_frame, ConverToPyString( url.spec() ) );
		} catch( bp::error_already_set & ) {
			PyErr_Print();
			return;
		}
	}
#endif // DISABLE_PYTHON
}

void WebView::OnDocumentReady(Awesomium::WebView* caller,
							const Awesomium::WebURL& url)
{
#ifndef DISABLE_PYTHON
	if( m_refLoadListener.ptr() != Py_None )
	{
		try {
			m_refLoadListener.attr("OnDocumentReady")( ConverToPyString( url.spec() )  );
		} catch( bp::error_already_set & ) {
			PyErr_Print();
			return;
		}
	}
#endif // DISABLE_PYTHON
}

// Process functions
void WebView::OnUnresponsive(Awesomium::WebView* caller)
{
#ifndef DISABLE_PYTHON
	if( m_refProcessListener.ptr() != Py_None )
	{
		try {
			m_refProcessListener.attr("OnUnresponsive")();
		} catch( bp::error_already_set & ) {
			PyErr_Print();
			return;
		}
	}
#endif // DISABLE_PYTHON
}

void WebView::OnResponsive(Awesomium::WebView* caller)
{
#ifndef DISABLE_PYTHON
	if( m_refProcessListener.ptr() != Py_None )
	{
		try {
			m_refProcessListener.attr("OnResponsive")( );
		} catch( bp::error_already_set & ) {
			PyErr_Print();
			return;
		}
	}
#endif // DISABLE_PYTHON
}

void WebView::OnCrashed(Awesomium::WebView* caller, Awesomium::TerminationStatus status)
{
#ifndef DISABLE_PYTHON
	if( m_refProcessListener.ptr() != Py_None )
	{
		try {
			m_refProcessListener.attr("OnCrashed")( status );
		} catch( bp::error_already_set & ) {
			PyErr_Print();
			return;
		}
	}
	else
	{
		Warning("WebView crashed\n");
	}
#endif // DISABLE_PYTHON
}

vgui::Panel *WebView::GetPanel()
{
	return m_pPanel;
}