//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef_js.h"

// CEF
#include "include/cef_v8.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

static int s_NextJSObjectID = 0;

JSObject::JSObject( const char *pName )
{
	m_iIdentifier = s_NextJSObjectID++;
	m_Name = pName;
}