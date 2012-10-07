//====== Copyright 20xx, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef VGUI_WEBVIEW_H
#define VGUI_WEBVIEW_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui/vgui.h>

#include <Awesomium/WebURL.h>
#include <Awesomium/WebViewListener.h>
#include <Awesomium/JSObject.h>

#include "webview_texgen.h"

//#define DISABLE_AWESOMIUM

#ifndef DISABLE_PYTHON
	#include "vgui_webview_python.h"
#endif // DISABLE_PYTHON

class WebViewPanel;

Awesomium::WebString CreateWebString();

#define CREATE_WEBSTRING( str ) Awesomium::WebString::CreateFromUTF8( str, Q_strlen(str) );

class WebView : public Awesomium::WebViewListener::View, 
	public Awesomium::WebViewListener::Load, public Awesomium::WebViewListener::Process,
	public Awesomium::JSMethodHandler 
{
public:
	friend class SrcWebViewSurfaceFactory;
	friend class CWebViewTextureRegen;
	friend class WebViewPanel;

	WebView( bool bCustomViewSize = false, vgui::Panel *pParent = NULL );
	~WebView( void );

	void Clear();
	bool IsValid(); // True if both webcore and webview are valid

	void SetSize( int wide, int tall );
	void SetPos( int x, int y );
	void SetZPos( int z );
	void SetVisible(bool state);
	bool IsVisible();
	void SetMouseInputEnabled( bool state );
	void SetKeyBoardInputEnabled( bool state );
	bool IsMouseInputEnabled();
	bool IsKeyBoardInputEnabled();
	virtual void SetCursor(vgui::HCursor cursor);
	virtual vgui::HCursor GetCursor();

	void LoadHTML( const char *html );
	void LoadURL( const char *url );
	void LoadFile( const char *filename );
	void Reload( bool ignore_cache );

	void OnMethodCall(Awesomium::WebView* caller,
							unsigned int remote_object_id, 
							const Awesomium::WebString& method_name,
							const Awesomium::JSArray& args);
	
	Awesomium::JSValue OnMethodCallWithReturnValue(Awesomium::WebView* caller,
												unsigned int remote_object_id,
												const Awesomium::WebString& method_name,
												const Awesomium::JSArray& args);

#ifndef DISABLE_PYTHON
	void ExecuteJavascript( boost::python::str script, boost::python::str frame_xpath );
	boost::python::object ExecuteJavascriptWithResult(boost::python::str script, boost::python::str frame_xpath);
	boost::python::object CreateGlobalJavascriptObject( boost::python::object name );

	virtual boost::python::object PyOnMethodCall( unsigned int remote_object_id, boost::python::object, boost::python::object args ) { return boost::python::object(); }
#endif // DISABLE_PYTHON

	void Focus();
	void Unfocus();
	void SetTransparent( bool istransparent );

	virtual void OnSizeChanged(int newWide, int newTall) {}
	virtual void PerformLayout() {}

	//void SetWebViewColor( Color &color ) { m_Color = color; }
	//Color GetWebViewColor( void ) { return m_Color; }

	void SetUseMouseCapture( bool usemousecapture ) { m_bUseMouseCapture = usemousecapture; }
	bool GetUseMouseCapture( ) { return m_bUseMouseCapture; }
	void SetPassMouseTruIfAlphaZero( bool passtruifzero ) { m_bPassMouseTruIfAlphaZero = passtruifzero; }
	bool GetPassMouseTruIfAlphaZero( void ) { return m_bPassMouseTruIfAlphaZero; }
	bool IsAlphaZeroAt( int x, int y );
	int GetAlphaAt( int x, int y );

	void SetWebViewSize( int newWide, int newTall );
	void RebuildWebView();

#ifndef DISABLE_PYTHON
	void SetViewListener( boost::python::object listener );
	boost::python::object GetViewListener();
	void SetLoadListener( boost::python::object listener );
	boost::python::object GetLoadListener();
	void SetProcessListener( boost::python::object listener );
	boost::python::object GetProcessListener();
#endif // DISABLE_PYTHON

	Awesomium::WebView *GetWebViewInternal() { return m_pWebView; }

	// View functions
	virtual void OnChangeTitle(Awesomium::WebView* caller,
								const Awesomium::WebString& title);
	virtual void OnChangeAddressBar(Awesomium::WebView* caller,
									const Awesomium::WebURL& url);
	virtual void OnChangeTooltip(Awesomium::WebView* caller,
								const Awesomium::WebString& tooltip);
	virtual void OnChangeTargetURL(Awesomium::WebView* caller,
									const Awesomium::WebURL& url);
	virtual void OnChangeCursor(Awesomium::WebView* caller,
								Awesomium::Cursor cursor);
	virtual void OnChangeFocus(Awesomium::WebView* caller,
								Awesomium::FocusedElementType focused_type);
	virtual void OnShowCreatedWebView(Awesomium::WebView* caller,
									Awesomium::WebView* new_view,
									const Awesomium::WebURL& opener_url,
									const Awesomium::WebURL& target_url,
									const Awesomium::Rect& initial_pos,
									bool is_popup);

	// Load functions
	virtual void OnBeginLoadingFrame(Awesomium::WebView* caller,
									int64 frame_id,
									bool is_main_frame,
									const Awesomium::WebURL& url,
									bool is_error_page);
	virtual void OnFailLoadingFrame(Awesomium::WebView* caller,
									int64 frame_id,
									bool is_main_frame,
									const Awesomium::WebURL& url,
									int error_code,
									const Awesomium::WebString& error_desc);
	virtual void OnFinishLoadingFrame(Awesomium::WebView* caller,
									int64 frame_id,
									bool is_main_frame,
									const Awesomium::WebURL& url);
	virtual void OnDocumentReady(Awesomium::WebView* caller,
								const Awesomium::WebURL& url);

	// Process
	virtual void OnUnresponsive(Awesomium::WebView* caller);
	virtual void OnResponsive(Awesomium::WebView* caller);
	virtual void OnCrashed(Awesomium::WebView* caller,
							Awesomium::TerminationStatus status);

protected:
	Awesomium::WebView *GetWebView() { return m_pWebView; }
	vgui::Panel *GetPanel();

private:
	void Paint();
	
private:
	WebViewPanel *m_pPanel;

	int m_iWVWide, m_iWVTall;
	bool m_bCustomViewSize;
	bool m_bRebuildPending;
	bool m_bInGame;
	bool m_bPassMouseTruIfAlphaZero;
	bool m_bUseMouseCapture;

	SrcWebViewSurface *m_pWebViewSurface;

	Awesomium::WebView *m_pWebView;

#ifndef DISABLE_PYTHON
	boost::python::object m_refViewListener;
	boost::python::object m_refProcessListener;
	boost::python::object m_refLoadListener;
#endif // DISABLE_PYTHON
};

inline bool WebView::IsAlphaZeroAt( int x, int y )
{
	return GetAlphaAt( x, y ) == 0;
}

#endif // VGUI_WEBVIEW_H