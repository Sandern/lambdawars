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

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CefRefPtr<JSObject> PyJSObject::GetJSObject()
{
	return m_Object;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int PyJSObject::GetIdentifier()
{
	return m_Object->GetIdentifier();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bp::object PyJSObject::GetName()
{
	std::string name = m_Object->GetName().ToString();
	return bp::object(name.c_str());
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CefRefPtr<CefListValue> PyToCefValueList( boost::python::list l )
{
	int n = bp::len( l );

	CefRefPtr<CefListValue> result = CefListValue::Create();
	result->SetSize( n );

	bp::object type = __builtin__.attr("type");

	for( int i = 0; i < n; i++ )
	{
		bp::object value = l[i];

		if( type(value) == types.attr("NoneType") )
		{
			result->SetNull( i );
		}
		else if( type(value) == types.attr("IntType") )
		{
			result->SetInt( i, boost::python::extract<int>(value) );
		}
		else if( type(value) == types.attr("FloatType") )
		{
			result->SetDouble( i, boost::python::extract<float>(value) );
		}
		else if( type(value) == types.attr("StringType") )
		{
			const char *pStr = boost::python::extract<const char *>(value);
			result->SetString( i, pStr );
		}
		else if( type(value) == types.attr("BooleanType") )
		{
			result->SetBool( i, boost::python::extract<bool>(value) );
		}
		else if( type(value) == types.attr("ListType") )
		{
			result->SetList( i, PyToCefValueList( bp::list( value ) ) );
		}
		else if( type(value) == types.attr("DictType") )
		{
			result->SetDictionary( i, PyToCefDictionaryValue( bp::dict( value ) ) );
		}
		else
		{
			PyErr_SetString(PyExc_ValueError, "Unsupported type in message list" );
			throw boost::python::error_already_set(); 
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
boost::python::list CefValueListToPy( CefRefPtr<CefListValue> l )
{
	boost::python::list result;
	for( size_t i = 0; i < l->GetSize(); i++ )
	{
		switch( l->GetType( i ) )
		{
			case VTYPE_INT:
			{
				result.append( l->GetInt( i ) );
				break;
			}
			case VTYPE_DOUBLE:
			{
				result.append( l->GetDouble( i ) );
				break;
			}
			case VTYPE_BOOL:
			{
				result.append( l->GetBool( i ) );
				break;
			}
			case VTYPE_STRING:
			{
				result.append( l->GetString( i ).ToString().c_str() );
				break;
			}
			/*case VTYPE_BINARY:
			{
				result.append( l->GetBinary( i )->GetData );
				break;
			}*/
			case VTYPE_DICTIONARY:
			{
				result.append( CefDictionaryValueToPy( l->GetDictionary( i ) ) );
				break;
			}
			case VTYPE_LIST:
			{
				result.append( CefValueListToPy( l->GetList( i ) ) );
				break;
			}
			default:
			{
				result.append( l->GetString( i ).ToString().c_str() );
				break;
			}
		}
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CefRefPtr<CefDictionaryValue> PyToCefDictionaryValue( boost::python::dict d )
{
	CefRefPtr<CefDictionaryValue> result = CefDictionaryValue::Create();

	bp::object type = __builtin__.attr("type");

	bp::object key, value;
	const bp::object objectKeys = d.iterkeys();
	unsigned long ulCount = bp::len(d); 
	for( unsigned long i = 0; i < ulCount; i++ )
	{
		key = objectKeys.attr( "next" )();
		value = d[key];

		CefString cefkey = bp::extract< const char * >( key );

		if( type(value) == types.attr("NoneType") )
		{
			result->SetNull( cefkey );
		}
		else if( type(value) == types.attr("IntType") )
		{
			result->SetInt( cefkey, boost::python::extract<int>(value) );
		}
		else if( type(value) == types.attr("FloatType") )
		{
			result->SetDouble( cefkey, boost::python::extract<float>(value) );
		}
		else if( type(value) == types.attr("StringType") )
		{
			const char *pStr = boost::python::extract<const char *>(value);
			result->SetString( cefkey, pStr );
		}
		else if( type(value) == types.attr("BooleanType") )
		{
			result->SetBool( cefkey, boost::python::extract<bool>(value) );
		}
		else if( type(value) == types.attr("ListType") )
		{
			result->SetList( cefkey, PyToCefValueList( bp::list( value ) ) );
		}
		else if( type(value) == types.attr("DictType") )
		{
			result->SetDictionary( cefkey, PyToCefDictionaryValue( bp::dict( value ) ) );
		}
		else
		{
			PyErr_SetString(PyExc_ValueError, "Unsupported type in message list" );
			throw boost::python::error_already_set(); 
		}
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
boost::python::dict CefDictionaryValueToPy( CefRefPtr<CefDictionaryValue> d )
{
	boost::python::dict result;

	CefDictionaryValue::KeyList keys;
	d->GetKeys( keys );

	for( size_t i = 0; i < keys.size(); i++ )
	{
		std::string k = keys[i].ToString();

		switch( d->GetType( keys[i] ) )
		{
			case VTYPE_INT:
			{
				result[k.c_str()] = d->GetInt( keys[i] );
				break;
			}
			case VTYPE_DOUBLE:
			{
				result[k.c_str()] = d->GetDouble( keys[i] );
				break;
			}
			case VTYPE_BOOL:
			{
				result[k.c_str()] = d->GetBool( keys[i] );
				break;
			}
			case VTYPE_STRING:
			{
				result[k.c_str()] = d->GetString( keys[i] ).ToString().c_str();
				break;
			}
			case VTYPE_DICTIONARY:
			{
				result[k.c_str()] = CefDictionaryValueToPy( d->GetDictionary( keys[i] ) );
				break;
			}
			case VTYPE_LIST:
			{
				result[k.c_str()] = CefValueListToPy( d->GetList( keys[i] ) );
				break;
			}
			default:
			{
				result[k.c_str()] = d->GetString( keys[i] ).ToString().c_str();
				break;
			}
		}
	}

	return result;
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
