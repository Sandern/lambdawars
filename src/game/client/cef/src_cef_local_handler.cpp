//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef_local_handler.h"

#include <filesystem.h>

// CefStreamResourceHandler is part of the libcef_dll_wrapper project.
#include "include/wrapper/cef_stream_resource_handler.h"
#include "include/cef_url.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

LocalSchemeHandlerFactory::LocalSchemeHandlerFactory()
{

}

CefRefPtr<CefResourceHandler> LocalSchemeHandlerFactory::Create(CefRefPtr<CefBrowser> browser,
											CefRefPtr<CefFrame> frame,
											const CefString& scheme_name,
											CefRefPtr<CefRequest> request)
{
	CefRefPtr<CefResourceHandler> pResourceHandler = NULL;

	CefURLParts parts;
	CefParseURL(request->GetURL(), parts);

	if( CefString(&parts.path).size() < 2 )
	{
		return NULL;
	}

	char path[MAX_PATH];
	V_strncpy( path, CefString(&parts.path).ToString().c_str() + 1, sizeof(path) );

	if( filesystem->IsDirectory( path ) ) 
	{
		V_AppendSlash( path, sizeof( path ) );
		V_strcat( path, "index.html", sizeof(path) );
	}

	if( filesystem->FileExists( path, NULL ) )
	{
		const char *pExtension = V_GetFileExtension( path );

		//Msg( "Path: %s, Extension: %s, Mime Type: %s, modified path: %s, exists: %d\n", 
		//	CefString(&parts.path).ToString().c_str(), pExtension, CefGetMimeType(pExtension).ToString().c_str(), path, filesystem->FileExists( path ) );

		CUtlBuffer buf( 0, filesystem->Size( path, NULL ) );
		if( filesystem->ReadFile( path, NULL, buf ) )
		{
			CefRefPtr<CefStreamReader> stream =
				CefStreamReader::CreateForData( buf.Base(), buf.TellPut() );
			if( stream != NULL ) 
			{
				pResourceHandler = new CefStreamResourceHandler(CefGetMimeType(pExtension), stream);
			}
		}
	}

	return pResourceHandler;
}