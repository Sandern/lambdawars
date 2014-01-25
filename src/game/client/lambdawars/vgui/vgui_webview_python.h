//====== Copyright 1996-2005, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef VGUI_WEBVIEW_PYTHON_H
#define VGUI_WEBVIEW_PYTHON_H
#ifdef _WIN32
#pragma once
#endif

#include <Awesomium/WebURL.h>
#include <Awesomium/JSObject.h>

class PyJSObject
{
public:
	PyJSObject( const Awesomium::JSObject &object ) : m_JSObject(object) {}

	unsigned int remote_id() const;

	boost::python::object Invoke(const char *name, boost::python::list args);
	void SetCustomMethod(const char *name, bool has_return_value);

private:
	Awesomium::JSObject m_JSObject;
};


Awesomium::JSArray ConvertListToJSArguments( boost::python::list pyargs );
Awesomium::JSValue ConvertObjectToJSValue( boost::python::object data );

Awesomium::WebString StringToWebString( const char *pscript );
Awesomium::WebString PyObjectToWebString( boost::python::object str );
#ifdef WIN32
	std::wstring ConvertPyStrToWString( boost::python::object object );
#endif // WIN32
boost::python::object ConverToPyString( const Awesomium::WebString &str );
boost::python::object ConvertJSValue( const Awesomium::JSValue &value );
boost::python::list ConvertJSArguments( const Awesomium::JSArray &arguments );

#endif // VGUI_WEBVIEW_PYTHON_H