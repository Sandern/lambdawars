//====== Copyright 20xx, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef SRC_CEF_PYTHON_H
#define SRC_CEF_PYTHON_H
#ifdef _WIN32
#pragma once
#endif

#include "include/internal/cef_ptr.h"

// Forward declarations
class CefFrame;
class JSObject;

// JS object
class PyJSObject 
{
public:
	PyJSObject( CefRefPtr<JSObject> object );

private:
	CefRefPtr<JSObject> m_Object;
};

// Frame
class PyCefFrame
{
public:
	PyCefFrame( CefRefPtr<CefFrame> frame );

private:
	CefRefPtr<CefFrame> m_Frame;
};

#endif // SRC_CEF_PYTHON_H