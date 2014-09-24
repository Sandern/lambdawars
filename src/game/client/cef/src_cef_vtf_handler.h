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

class CUtlBuffer;

void VTFHandler_ConvertImageToJPG( CUtlBuffer &buf, unsigned char *imageData, uint32 width, uint32 height );

class VTFSchemeHandlerFactory : public CefSchemeHandlerFactory 
{
public:
	VTFSchemeHandlerFactory();

	virtual CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser,
												CefRefPtr<CefFrame> frame,
												const CefString& scheme_name,
												CefRefPtr<CefRequest> request)
												OVERRIDE;

	IMPLEMENT_REFCOUNTING(VTFSchemeHandlerFactory);
};

#endif // SRC_CEF_VTF_HANDLER_H