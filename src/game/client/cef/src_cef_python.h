//====== Copyright © Sandern Corporation, All rights reserved. ===========//
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
class CefListValue;
class CefDictionaryValue;

// Helper functions
CefRefPtr<CefListValue> PyToCefValueList( boost::python::list l );
boost::python::list CefValueListToPy( CefRefPtr<CefListValue> l );

boost::python::dict CefDictionaryValueToPy( CefRefPtr<CefDictionaryValue> d );
CefRefPtr<CefDictionaryValue> PyToCefDictionaryValue( boost::python::dict d );

// JS object
class PyJSObject 
{
public:
	PyJSObject( CefRefPtr<JSObject> object );

	CefRefPtr<JSObject> GetJSObject();

	boost::python::object GetIdentifier();
	boost::python::object GetName();

private:
	CefRefPtr<JSObject> m_Object;
};

// Frame
class PyCefFrame
{
public:
	PyCefFrame( CefRefPtr<CefFrame> frame );

	bool operator ==( const PyCefFrame &other ) const;
	bool operator !=( const PyCefFrame &other ) const;

private:
	CefRefPtr<CefFrame> m_Frame;
};

#endif // SRC_CEF_PYTHON_H