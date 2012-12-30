// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef RENDER_BROWSER_H
#define RENDER_BROWSER_H
#ifdef _WIN32
#pragma once
#endif

#include "include/cef_app.h"
#include "include/cef_v8.h"

class RenderBrowser;
class ClientApp;

class FunctionV8Handler : public CefV8Handler
{
public:
	FunctionV8Handler( CefRefPtr<RenderBrowser> renderBrowser );

	virtual void SetFunc( CefRefPtr<CefV8Value> func );

	virtual bool Execute(const CefString& name,
						CefRefPtr<CefV8Value> object,
						const CefV8ValueList& arguments,
						CefRefPtr<CefV8Value>& retval,
						CefString& exception) OVERRIDE;

protected:
	CefRefPtr<RenderBrowser> m_RenderBrowser;
	CefRefPtr<CefV8Value> m_Func;

	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(DefaultV8Handler);
};

class FunctionWithCallbackV8Handler : public FunctionV8Handler
{
public:
	FunctionWithCallbackV8Handler( CefRefPtr<RenderBrowser> renderBrowser ) : FunctionV8Handler( renderBrowser ) {}

	virtual bool Execute(const CefString& name,
						CefRefPtr<CefV8Value> object,
						const CefV8ValueList& arguments,
						CefRefPtr<CefV8Value>& retval,
						CefString& exception) OVERRIDE;
};

// Browsers representation on render process (maintains js objects)
class RenderBrowser
{
public:
	RenderBrowser( CefRefPtr<CefBrowser> browser, CefRefPtr<ClientApp> clientApp );

	CefRefPtr<CefBrowser> GetBrowser();

	void SetV8Context( CefRefPtr<CefV8Context> context );
	CefRefPtr<CefV8Context> GetV8Context();

	// Creating new objects
	bool RegisterObject( int iIdentifier, CefRefPtr<CefV8Value> object );

	bool CreateGlobalObject( int iIdentifier, CefString name );

	bool CreateFunction( int iIdentifier, CefString name, int iParentIdentifier = -1, bool bCallback = false );

	// Function handlers
	bool CallFunction(	CefRefPtr<CefV8Value> object, 
						const CefV8ValueList& arguments,
						CefRefPtr<CefV8Value>& retval,
						CefRefPtr<CefV8Value> callback = NULL );

	bool DoCallback( int iCallbackID, CefRefPtr<CefListValue> methodargs );

private:
	CefRefPtr<CefBrowser> m_Browser;
	CefRefPtr<ClientApp> m_ClientApp;
	CefRefPtr<CefV8Context> m_Context;

	std::map< int, CefRefPtr<CefV8Value> > m_Objects;
	std::map< CefString, CefRefPtr<CefV8Value> > m_GlobalObjects;

	typedef struct jscallback_t {
		int callbackid;
		CefRefPtr<CefV8Value> callback;
		CefRefPtr<CefV8Value> thisobject;
	} jscallback_t;
	std::vector< jscallback_t > m_Callbacks;

	IMPLEMENT_REFCOUNTING( RenderBrowser );
};

inline CefRefPtr<CefBrowser> RenderBrowser::GetBrowser()
{
	return m_Browser;
}

inline CefRefPtr<CefV8Context> RenderBrowser::GetV8Context()
{
	return m_Context;
}

#endif // RENDER_BROWSER_H