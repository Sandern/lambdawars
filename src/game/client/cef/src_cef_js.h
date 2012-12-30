//====== Copyright 20xx, Sander Corporation, All rights reserved. =======
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

class JSObject
{
public:
	JSObject( const char *pName );

	int GetIdentifier();
	CefString GetName();

private:
	int m_iIdentifier;
	CefString m_Name;

	IMPLEMENT_REFCOUNTING( JSObject );
};

inline int JSObject::GetIdentifier()
{
	return m_iIdentifier;
}

inline CefString JSObject::GetName()
{
	return m_Name;
}

#endif // SRC_CEF_JS_H