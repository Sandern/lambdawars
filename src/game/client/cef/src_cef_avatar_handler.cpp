//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef_avatar_handler.h"
#include "src_cef_vtf_handler.h"

#include "steam/steam_api.h"

// CefStreamResourceHandler is part of the libcef_dll_wrapper project.
#include "include/wrapper/cef_stream_resource_handler.h"
#include "include/cef_parser.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

AvatarSchemeHandlerFactory::AvatarSchemeHandlerFactory( AvatarType avatarType ) : m_AvatarType(avatarType)
{

}

CefRefPtr<CefResourceHandler> AvatarSchemeHandlerFactory::Create(CefRefPtr<CefBrowser> browser,
											CefRefPtr<CefFrame> frame,
											const CefString& scheme_name,
											CefRefPtr<CefRequest> request)
{
	CefRefPtr<CefResourceHandler> pResourceHandler = NULL;

	// Parse the request url into components so we can retrieve the requested steam id
	CefURLParts parts;
	CefParseURL(request->GetURL(), parts);

	// Parse the steam id from the path. The path component looks like "/1234...", so the first character should be stripped.
	std::string strSteamID = CefString(&parts.path);
	CSteamID steamID( static_cast<unsigned __int64>( strSteamID.length() > 1 ? _atoi64(strSteamID.c_str() + 1 ) : 0 ) );
	if( !steamID.IsValid() ) {
		DevMsg( "AvatarSchemeHandlerFactory::Create: INVALID STEAM ID %s\n", strSteamID.c_str() );
		return NULL; 
	}

	if ( steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
	{
		// TODO: what if not available yet? Write custom resource handler instead?
		if ( !steamapicontext->SteamFriends()->RequestUserInformation( steamID, false ) )
		{
			int iAvatar = 0;
			switch( m_AvatarType )
			{
				case k_AvatarTypeSmall: 
					iAvatar = steamapicontext->SteamFriends()->GetSmallFriendAvatar( steamID );
					break;
				case k_AvatarTypeMedium: 
					iAvatar = steamapicontext->SteamFriends()->GetMediumFriendAvatar( steamID );
					break;
				case k_AvatarTypeLarge: 
					iAvatar = steamapicontext->SteamFriends()->GetLargeFriendAvatar( steamID );
					break;
			}

			if ( iAvatar > 0 ) // if its zero, user doesn't have an avatar.  If -1, Steam is telling us that it's fetching it
			{
				uint32 wide = 0, tall = 0;
				if ( steamapicontext->SteamUtils()->GetImageSize( iAvatar, &wide, &tall ) && wide > 0 && tall > 0 )
				{
					int destBufferSize = wide * tall * 4;
					byte *rgbaDest = (byte*)stackalloc( destBufferSize );
					if ( steamapicontext->SteamUtils()->GetImageRGBA( iAvatar, rgbaDest, destBufferSize ) )
					{
						int destRGBBufferSize = wide * tall * 3;
						byte *rgbDest = (byte*)stackalloc( destRGBBufferSize );

						uint32 x, y;
						for( y = 0; y < tall; y++ ) 
						{
							for( x = 0; x < wide; x++ ) 
							{
								rgbDest[y*wide*3+x*3] = rgbaDest[y*wide*4+x*4];
								rgbDest[y*wide*3+x*3 + 1] = rgbaDest[y*wide*4+x*4 + 1];
								rgbDest[y*wide*3+x*3 + 2] = rgbaDest[y*wide*4+x*4 + 2];
							}
						}

						CUtlBuffer buf;
						VTFHandler_ConvertImageToJPG( buf, rgbDest, wide, tall );

						CefRefPtr<CefStreamReader> stream =
							CefStreamReader::CreateForData( static_cast<void*>(buf.Base()), buf.Size() );
						if( stream )
						{
							CefResponse::HeaderMap header_map;
							header_map.insert( std::pair<CefString,CefString>( CefString("Cache-Control"), CefString("no-cache") ) ); 
							pResourceHandler = new CefStreamResourceHandler(200, "OK", "image/jpeg", header_map, stream);
						}

						// CefStreamReader creates a copy of the data, so we can safely release it
						stackfree( rgbDest );
					}
					
					stackfree( rgbaDest );
				}
			}
		}
	}

	return pResourceHandler;
}