//====== Copyright © 20xx, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef_python.h"
#include "src_python.h"
#include "src_cef_js.h"

// CEF
#include "include/cef_v8.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
PyJSObject::PyJSObject( CefRefPtr<JSObject> object ) : m_Object(object)
{

}

#if 0
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CefRefPtr<CefV8Value> PyToCefV8Value( boost::python::object value )
{
	bp::object type = __builtin__.attr("type")( value );

	// FIXME: Crash when using the bp::object compare operator
	if( value.ptr() == Py_None )
	{
		return CefRefPtr<CefV8Value>();
	}
	else if( type.ptr() == bp::object( types.attr("IntType")).ptr() )
	{
		return CefV8Value::CreateInt( boost::python::extract<int>(value) );
	}
	else if( type.ptr() == bp::object(types.attr("FloatType")).ptr() )
	{
		return CefV8Value::CreateDouble( boost::python::extract<float>(value) );
	}
	else if( type.ptr() == bp::object( types.attr("StringType")).ptr() )
	{
		const char *pStr = boost::python::extract<const char *>(value);
		return CefV8Value::CreateString( pStr );
	}
	else if( type.ptr() == bp::object( types.attr("BooleanType")).ptr() )
	{
		return CefV8Value::CreateBool( boost::python::extract<bool>(value) );
	}
	else if( type.ptr() == bp::object( types.attr("ListType")).ptr() )
	{
		return ConvertPyListToCefArray( bp::list( value ) );
	}

	return CefRefPtr<CefV8Value>();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
boost::python::object CefV8ValueToPy( CefRefPtr<CefV8Value> value )
{
	if( value->IsBool() )
		return bp::object( value->GetBoolValue() );
	else if( value->IsInt() )
		return bp::object( value->GetIntValue() );
	else if( value->IsDouble() )
		return bp::object( value->GetDoubleValue() );
	else if( value->IsObject() )
		return bp::object( value );
	else if( value->IsUndefined() || value->IsNull()  )
		return bp::object();

	// Default to string
	//else if( value->IsString() || value->IsNumber() )
	return bp::object( value->GetStringValue() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CefRefPtr<CefV8Value> ConvertPyListToCefArray( boost::python::list value )
{
	int n = boost::python::len( value );

	CefRefPtr<CefV8Value> arr = CefV8Value::CreateArray( n );
	if( !arr )
	{
		PyErr_SetString(PyExc_Exception, "Failed to create cef array (no context?)" );
		throw boost::python::error_already_set(); 
		return arr;
	}

	for( int i = 0; i < n; i++ )
		arr->SetValue( i, PyToCefV8Value( value[i] ) );

	return arr;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
boost::python::list ConvertCefV8ValueListToPy( const CefV8ValueList& ceflist )
{
	boost::python::list l;
	for( CefV8ValueList::const_iterator it = ceflist.begin(); it != ceflist.end(); ++it ) 
	{
		l.append( CefV8ValueToPy( *it ) );
	}
	return l;
}
#endif // 0

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
PyCefFrame::PyCefFrame( CefRefPtr<CefFrame> frame ) : m_Frame(frame)
{
	
}
