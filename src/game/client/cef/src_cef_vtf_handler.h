//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#ifndef SRC_CEF_VTF_HANDLER_H
#define SRC_CEF_VTF_HANDLER_H
#ifdef _WIN32
#pragma once
#endif

#include "include/cef_scheme.h"

class VTFSchemeHandlerFactory : public CefSchemeHandlerFactory 
{
public:
	enum AvatarType
	{
		k_AvatarTypeSmall = 0,
		k_AvatarTypeMedium = 1,
		k_AvatarTypeLarge = 2,
	};

	VTFSchemeHandlerFactory();

	virtual CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser,
												CefRefPtr<CefFrame> frame,
												const CefString& scheme_name,
												CefRefPtr<CefRequest> request)
												OVERRIDE;

	IMPLEMENT_REFCOUNTING(VTFSchemeHandlerFactory);
};

#endif // SRC_CEF_VTF_HANDLER_H