//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef_js.h"
#include "warscef/wars_cef_shared.h"

// CEF
#include "include/cef_v8.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

static int s_NextJSObjectID = 0;

JSObject::JSObject( const char *pName, const char *pUUID )
{
	m_Name = pName;

	if( pUUID == NULL )
	{
		char uuid[37];
		WarsCef_GenerateUUID( uuid );
		m_UUID = uuid;
	}
	else
	{
		m_UUID = pUUID;
	}
}

JSObject::~JSObject()
{
	// TODO: cleanup
}