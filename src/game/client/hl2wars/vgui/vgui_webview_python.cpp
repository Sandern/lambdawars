//====== Copyright © 2011, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "vgui_webview_python.h"

#ifndef DISABLE_PYTHON
	#include "src_python.h"
#endif // DISABLE_PYTHON

#include <Awesomium/WebCore.h>

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

#ifndef DISABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose: PyJSObject
//-----------------------------------------------------------------------------
unsigned int PyJSObject::remote_id() const
{
	return m_JSObject.remote_id();
}

bp::object PyJSObject::Invoke(const char *name, bp::list args)
{
	Awesomium::WebString wname = Awesomium::WebString::CreateFromUTF8( name, Q_strlen(name) );
	return ConvertJSValue( m_JSObject.Invoke( wname, ConvertListToJSArguments( args ) ) );
}

void PyJSObject::SetCustomMethod( const char *name, bool has_return_value )
{
	// FIXME: Awesomium sometimes throws an exception and then custom calls without return value fail...
	has_return_value = true;

	Awesomium::WebString wname = Awesomium::WebString::CreateFromUTF8( name, Q_strlen(name) );
	m_JSObject.SetCustomMethod( wname, has_return_value );
}

//-----------------------------------------------------------------------------
// Purpose: To WebString conversion
//-----------------------------------------------------------------------------
Awesomium::WebString StringToWebString( const char *pscript )
{
	if( pscript )
	{
		return Awesomium::WebString::CreateFromUTF8( pscript, Q_strlen(pscript) );
	}
	return Awesomium::WebString();
}

Awesomium::WebString PyObjectToWebString( bp::object str )
{
	char *pscript = PyString_AsString(str.ptr());
	if( pscript )
	{
		return Awesomium::WebString::CreateFromUTF8( pscript, Q_strlen(pscript) );
	}
	return Awesomium::WebString();
}

//-----------------------------------------------------------------------------
// Purpose: Convert functions
//-----------------------------------------------------------------------------
Awesomium::JSArray ConvertListToJSArguments( bp::list pyargs )
{
	Awesomium::JSArray args;

	boost::python::ssize_t n = boost::python::len(pyargs);
	for( int i = 0; i < n; i++ )
	{
		args.Push(ConvertObjectToJSValue(pyargs[i]));
	}

	return args;
}

Awesomium::JSValue ConvertObjectToJSValue( bp::object data )
{
	bp::object type = __builtin__.attr("type")( data );

	// FIXME: Crash when using the bp::object compare operator
	if( data.ptr() == Py_None )
	{
		//Msg("None return type\n");
		return Awesomium::JSValue();
	}
	else if( type.ptr() == bp::object(types.attr("IntType")).ptr() )
	{
		//Msg("Int return type\n");
		return Awesomium::JSValue(boost::python::extract<int>(data));
	}
	else if( type.ptr() == bp::object(types.attr("FloatType")).ptr() )
	{
		//Msg("float return type\n");
		return Awesomium::JSValue(boost::python::extract<float>(data));
	}
	else if( type.ptr() == bp::object(types.attr("StringType")).ptr() )
	{
		//Msg("String return type\n");
		const char *pStr = boost::python::extract<const char *>(data);
		return Awesomium::JSValue( StringToWebString( pStr ) );
	}
	else if( type.ptr() == bp::object(types.attr("BooleanType")).ptr() )
	{
		//Msg("Bool return type\n");
		return Awesomium::JSValue(boost::python::extract<bool>(data));
	}
	else if( type.ptr() == bp::object(types.attr("ListType")).ptr() )
	{
		return ConvertListToJSArguments( bp::list(data) );
	}

	//Msg("Dunno return type\n");
	return Awesomium::JSValue();
}

bp::object ConverToPyString( const Awesomium::WebString &str )
{
	PyObject *pValue = PyUnicode_DecodeUTF16( (const char *)str.data(), str.length()*2, NULL, NULL ); // Assuming length is the string length, then size is 2*length (in bytes)
	boost::python::handle<> h(boost::python::borrowed((PyObject*)pValue));
	return bp::object( h );
}

bp::object ConvertJSValue( const Awesomium::JSValue &value )
{
	if( value.IsBoolean() )
		return bp::object( value.ToBoolean() );
	else if( value.IsInteger() )
		return bp::object( value.ToInteger() );
	else if( value.IsDouble() )
		return bp::object( value.ToDouble() );
	else if( value.IsObject() )
		return bp::object( PyJSObject( value.ToObject() ) );
	else if( value.IsUndefined() || value.IsNull()  )
		return bp::object();

	// Default to string
	//else if( value.isString() || value.isNumber() )
	return ConverToPyString( value.ToString() );
}

bp::list ConvertJSArguments( const Awesomium::JSArray &arguments )
{
	bp::list l;
	for( size_t i = 0 ; i < arguments.size(); i++ )
	{
		l.append( ConvertJSValue( arguments[i] ) );
	}
	return l;
}
#endif // DISABLE_PYTHON