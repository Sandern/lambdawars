//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#ifndef SRC_CEF_LOCAL_HANDLER_H
#define SRC_CEF_LOCAL_HANDLER_H
#ifdef _WIN32
#pragma once
#endif

#include "include/cef_scheme.h"

class LocalSchemeHandlerFactory : public CefSchemeHandlerFactory 
{
public:
	LocalSchemeHandlerFactory();

	virtual CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser,
												CefRefPtr<CefFrame> frame,
												const CefString& scheme_name,
												CefRefPtr<CefRequest> request)
												OVERRIDE;

	IMPLEMENT_REFCOUNTING(LocalSchemeHandlerFactory);
};

#endif // SRC_CEF_LOCAL_HANDLER_H