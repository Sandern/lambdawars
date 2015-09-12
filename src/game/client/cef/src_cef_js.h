//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#ifndef SRC_CEF_JS_H
#define SRC_CEF_JS_H
#ifdef _WIN32
#pragma once
#endif

#include "include/cef_base.h"

// Forward declarations
class CefFrame;

class JSObject : public CefBase
{
public:
	JSObject( const char *pName = "", const char *pUUID = NULL );
	~JSObject();

	CefString GetIdentifier();
	CefString GetName();

private:
	CefString m_Name;
	CefString m_UUID;

	IMPLEMENT_REFCOUNTING( JSObject );
};

inline CefString JSObject::GetIdentifier()
{
	return m_UUID;
}

inline CefString JSObject::GetName()
{
	return m_Name;
}

#endif // SRC_CEF_JS_H