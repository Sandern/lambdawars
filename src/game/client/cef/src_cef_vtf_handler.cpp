//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef_vtf_handler.h"
#include <filesystem.h>

// CefStreamResourceHandler is part of the libcef_dll_wrapper project.
#include "include/wrapper/cef_stream_resource_handler.h"
#include "include/cef_url.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

VTFSchemeHandlerFactory::VTFSchemeHandlerFactory()
{
}

CefRefPtr<CefResourceHandler> VTFSchemeHandlerFactory::Create(CefRefPtr<CefBrowser> browser,
											CefRefPtr<CefFrame> frame,
											const CefString& scheme_name,
											CefRefPtr<CefRequest> request)
{
	CefRefPtr<CefResourceHandler> pResourceHandler = NULL;

	CefURLParts parts;
	CefParseURL(request->GetURL(), parts);

	std::string strVtfPath = CefString(&parts.path);

	Msg("VTF request url: %ls, vtf path: %s\n", request->GetURL().c_str(), strVtfPath.c_str());

	char vtfPath[MAX_PATH];
	V_snprintf(vtfPath, sizeof(vtfPath), "materials/%s", strVtfPath.c_str());

	if (!filesystem->FileExists(vtfPath))
	{
		Warning("VTFSchemeHandlerFactory: invalid vtf %s\n", vtfPath);
		return null;
	}

	return null;
	//return pResourceHandler;
}