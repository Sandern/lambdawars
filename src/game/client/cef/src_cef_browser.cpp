//====== Copyright © 20xx, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef_browser.h"
#include "src_cef.h"

// CEF
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_runnable.h"
#include "include/cef_client.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Cef browser internal implementation
//-----------------------------------------------------------------------------
class CefClientHandler : public CefClient,
                      public CefContextMenuHandler,
                      public CefDisplayHandler,
                      public CefDownloadHandler,
                      public CefGeolocationHandler,
                      public CefKeyboardHandler,
                      public CefLifeSpanHandler,
                      public CefLoadHandler,
                      public CefRequestHandler
{
public:
	CefClientHandler( SrcCefBrowser *pSrcBrowser );

	CefRefPtr<CefBrowser> GetBrowser() { return m_Browser; }
	int GetBrowserId() { return m_BrowserId; }

	// CefClient methods
	virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() {
		return this;
	}
	virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() {
		return this;
	}
	virtual CefRefPtr<CefDownloadHandler> GetDownloadHandler() {
		return this;
	}
	virtual CefRefPtr<CefGeolocationHandler> GetGeolocationHandler() {
		return this;
	}
	virtual CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() {
		return this;
	}
	virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() {
		return this;
	}
	virtual CefRefPtr<CefLoadHandler> GetLoadHandler() {
		return this;
	}
	virtual CefRefPtr<CefRequestHandler> GetRequestHandler() {
		return this;
	}

	// CefDisplayHandler methods
	virtual bool OnConsoleMessage(CefRefPtr<CefBrowser> browser,
								const CefString& message,
								const CefString& source,
								int line);

	// CefDownloadHandler methods
	virtual void OnBeforeDownload(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefDownloadItem> download_item,
		const CefString& suggested_name,
		CefRefPtr<CefBeforeDownloadCallback> callback) {}

	// CefLifeSpanHandler methods
	virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser);

	// CefLoadHandler methods
	virtual void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame);
	virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode);
	virtual void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode,
							const CefString& errorText, const CefString& failedUrl);

private:
	// The child browser window
	CefRefPtr<CefBrowser> m_Browser;

	// The child browser id
	int m_BrowserId;

	// Internal
	HWND m_hostWindow;
	SrcCefBrowser *m_pSrcBrowser;

	IMPLEMENT_REFCOUNTING( CefClientHandler );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CefClientHandler::CefClientHandler( SrcCefBrowser *pSrcBrowser ) : m_BrowserId(0), m_pSrcBrowser( pSrcBrowser )
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CefClientHandler::OnConsoleMessage(CefRefPtr<CefBrowser> browser,
							const CefString& message,
							const CefString& source,
							int line)
{
	Warning("%d %ls: %ls\n", line, source.c_str(), message.c_str() );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CefClientHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) 
{
	if( !m_Browser.get() ) 
	{
		// We need to keep the main child window, but not popup windows
		m_Browser = browser;
		m_BrowserId = browser->GetIdentifier();
	}

	//HWND hWnd = browser->GetHost()->GetWindowHandle();

    RECT rect;
    GetClientRect( browser->GetHost()->GetWindowHandle(), &rect );

	m_hostWindow = CEFSystem().GetMainWindow();

	m_pSrcBrowser->OnAfterCreated();

	//::BringWindowToTop( hWnd );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CefClientHandler::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame)
{
	m_pSrcBrowser->OnLoadStart();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CefClientHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
	m_pSrcBrowser->OnLoadEnd( httpStatusCode );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CefClientHandler::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode,
						const CefString& errorText, const CefString& failedUrl)
{
	m_pSrcBrowser->OnLoadError( errorCode, errorText.c_str(), failedUrl.c_str() );
}

//-----------------------------------------------------------------------------
// Purpose: Cef browser
//-----------------------------------------------------------------------------
SrcCefBrowser::SrcCefBrowser( const char *pURL ) : m_bPerformLayout(true), m_bVisible(false)
{
	CEFSystem().AddBrowser( this );

	m_URL = pURL;
	m_CefClientHandler = new CefClientHandler( this );

    RECT mainrect;
	GetWindowRect( CEFSystem().GetMainWindow(), &mainrect );

	//Msg( "Creating SrcCefBrowser, window: %d. rect: %d %d %d %d\n", CEFSystem().GetMainWindow(), rect.left, rect.right, rect.top, rect.bottom );

    CefWindowInfo info;
	//info.SetAsChild( CEFSystem().GetMainWindow(), mainrect );
	info.SetAsPopup( CEFSystem().GetMainWindow(), "Popup" );
	info.style = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	info.ex_style = WS_EX_TOOLWINDOW | WS_EX_TOPMOST; //WS_EX_TRANSPARENT;
	//info.parent_window = 0;

	// Make sure the initial pos is within the main window
	info.x = mainrect.left;
	info.y = mainrect.top;
	info.width = 1;
	info.height = 1;
	info.SetTransparentPainting( TRUE ); // AeroGlass
	
	// Browser settings
    CefBrowserSettings settings;
	settings.web_security_disabled = 1;

    // Creat the new child browser window
    bool success = CefBrowserHost::CreateBrowser(info, m_CefClientHandler.get(),
        m_URL, settings);

	//Msg("Created SrcCefBrowser success: %d, window: %d\n", success, info.window);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
SrcCefBrowser::~SrcCefBrowser()
{
	// Remove ourself from the list
	CEFSystem().RemoveBrowser( this );

	Destroy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefBrowser::Destroy( void )
{
	if( !IsValid() )
		return;

	// Close browser
	if( m_CefClientHandler )
	{
		if( m_CefClientHandler->GetBrowser() )
		{
			m_CefClientHandler->GetBrowser()->GetHost()->CloseBrowser();
		}
	}

	// Delete the handler
	m_CefClientHandler = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool SrcCefBrowser::IsValid( void )
{
	return m_CefClientHandler.get() && m_CefClientHandler->GetBrowser();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefBrowser::Think( void )
{
	if( m_bPerformLayout )
	{
		PerformLayout();

		m_bPerformLayout = false;
	}

	OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefBrowser::OnAfterCreated( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefBrowser::PerformLayout( void )
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefBrowser::InvalidateLayout( void )
{
	m_bPerformLayout = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CefRefPtr< CefClientHandler > SrcCefBrowser::GetClientHandler( void )
{
	return m_CefClientHandler;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
HWND SrcCefBrowser::GetWindow( void )
{
	if( !IsValid() )
		return NULL;

	return m_CefClientHandler->GetBrowser()->GetHost()->GetWindowHandle();
}

// Usage functions
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefBrowser::SetSize( int wide, int tall )
{
	if( !IsValid() )
		return;

	::SetWindowPos( GetWindow(), NULL, -1, -1, wide, tall, SWP_NOMOVE|SWP_NOZORDER );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefBrowser::SetPos( int x, int y )
{
	RECT rect;
	GetWindowRect( CEFSystem().GetMainWindow(), &rect );

	::SetWindowPos( GetWindow(), NULL, rect.left + x, rect.top + y, -1, -1, SWP_NOSIZE|SWP_NOZORDER );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefBrowser::SetZPos( int z )
{
	if( !IsValid() )
		return;


}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefBrowser::SetVisible(bool state)
{
	if( !IsValid() )
		return;

	if( m_bVisible == state )
		return;

	m_bVisible = state;
	::ShowWindow( GetWindow(), state );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool SrcCefBrowser::IsVisible()
{
	if( !IsValid() )
		return false;

	return m_bVisible; //::IsWindowVisible( GetWindow() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool SrcCefBrowser::IsLoading( void )
{
	if( !IsValid() )
		return false;

	return m_CefClientHandler->GetBrowser()->IsLoading();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefBrowser::Reload( void )
{
	if( !IsValid() )
		return;

	m_CefClientHandler->GetBrowser()->Reload();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefBrowser::StopLoad( void )
{
	if( !IsValid() )
		return;

	m_CefClientHandler->GetBrowser()->StopLoad();
}

//-----------------------------------------------------------------------------
// Purpose: Load an URL
//-----------------------------------------------------------------------------
void SrcCefBrowser::LoadURL( const char *url )
{
	if( !IsValid() )
		return;

	CefRefPtr<CefFrame> mainFrame = m_CefClientHandler->GetBrowser()->GetMainFrame();
	if( !mainFrame )
		return;

	mainFrame->LoadURL( CefString( url ) );
}

//-----------------------------------------------------------------------------
// Purpose: Execute javascript code
//-----------------------------------------------------------------------------
void SrcCefBrowser::ExecuteJavaScript( const char *code, const char *script_url, int start_line )
{
	if( !IsValid() )
		return;

	CefRefPtr<CefFrame> mainFrame = m_CefClientHandler->GetBrowser()->GetMainFrame();
	if( !mainFrame )
		return;

	mainFrame->ExecuteJavaScript( code, script_url, start_line );
}