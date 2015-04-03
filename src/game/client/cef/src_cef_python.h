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

#ifdef ENABLE_PYTHON

#include "include/internal/cef_ptr.h"

// Forward declarations
class CefFrame;
class JSObject;
class CefListValue;
class CefDictionaryValue;

// Helper functions
void PySingleToCefValueList( boost::python::object value, CefListValue *result, int i );
CefRefPtr<CefListValue> PyToCefValueList( boost::python::object l ); // l must be iterable
boost::python::list CefValueListToPy( CefRefPtr<CefListValue> l );

boost::python::dict CefDictionaryValueToPy( CefRefPtr<CefDictionaryValue> d );
CefRefPtr<CefDictionaryValue> PyToCefDictionaryValue( boost::python::object d );

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

	bool IsMain();
	const char *GetURL();

private:
	CefRefPtr<CefFrame> m_Frame;
};

#endif // ENABLE_PYTHON

#endif // SRC_CEF_PYTHON_H