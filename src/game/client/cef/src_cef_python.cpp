//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef_python.h"
#include "srcpy.h"
#include "src_cef_js.h"
#include "warscef/wars_cef_shared.h"

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
boost::python::object PyJSObject::GetIdentifier()
{
	return boost::python::object( m_Object->GetIdentifier().ToString().c_str() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
boost::python::object PyJSObject::GetName()
{
	std::string name = m_Object->GetName().ToString();
	return boost::python::object(name.c_str());
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void PySingleToCefValueList( boost::python::object value, CefListValue *result, int i )
{
	boost::python::object jsobject = _cef.attr("JSObject");
	boost::python::object valuetype = fntype( value );

	if( value == boost::python::object() )
	{
		result->SetNull( i );
	}
	else if( valuetype == builtins.attr("int") )
	{
		result->SetInt( i, boost::python::extract<int>(value) );
	}
	else if( valuetype == builtins.attr("float") )
	{
		result->SetDouble( i, boost::python::extract<float>(value) );
	}
	else if( valuetype == builtins.attr("str") )
	{
		const char *pStr = boost::python::extract<const char *>(value);
		result->SetString( i, pStr );
	}
#if PY_VERSION_HEX < 0x03000000
	else if( valuetype == builtins.attr("unicode") )
	{
		const wchar_t *pStr = PyUnicode_AS_UNICODE( value.ptr() );
		if( !pStr )
		{
			PyErr_SetString(PyExc_ValueError, "PyToCefValueList: Invalid unicode object in message list" );
			throw boost::python::error_already_set(); 
		}
		result->SetString( i, pStr );
	}
#endif // PY_VERSION_HEX < 0x03000000
	else if( valuetype == builtins.attr("bool") )
	{
		result->SetBool( i, boost::python::extract<bool>(value) );
	}
#if PY_VERSION_HEX >= 0x03000000
	else if( valuetype == builtins.attr("list") || valuetype == builtins.attr("tuple") || valuetype == builtins.attr("set") || valuetype == builtins.attr("map") )
#else
	else if( valuetype == builtins.attr("list") || valuetype == builtins.attr("tuple") || valuetype == builtins.attr("set") )
#endif // PY_VERSION_HEX >= 0x03000000
	{
		result->SetList( i, PyToCefValueList( value ) );
	}
	else if( valuetype == builtins.attr("dict") || valuetype == collections.attr("defaultdict") )
	{
		result->SetDictionary( i, PyToCefDictionaryValue( value ) );
	}
	else if( valuetype == srcbuiltins.attr("Color") )
	{
		Color c = boost::python::extract<Color>(value);
		result->SetString( i, UTIL_VarArgs("rgba(%d, %d, %d, %.2f)", c.r(), c.g(), c.b(), c.a() / 255.0f) );
	}
	else if( valuetype == jsobject )
	{
		PyJSObject *pJSObject = boost::python::extract< PyJSObject * >( value );
		WarsCefJSObject_t warsCefJSObject;
		V_strncpy( warsCefJSObject.uuid, pJSObject->GetJSObject()->GetIdentifier().ToString().c_str(), sizeof( warsCefJSObject.uuid ) );
			
		CefRefPtr<CefBinaryValue> pRefData = CefBinaryValue::Create( &warsCefJSObject, sizeof( warsCefJSObject ) );
		result->SetBinary( i, pRefData );
	}
	else
	{
		const char *pObjectTypeStr = boost::python::extract<const char *>( boost::python::str( valuetype ) );
		const char *pObjectStr = boost::python::extract<const char *>( boost::python::str( value ) );
		char buf[512];
		V_snprintf( buf, sizeof(buf), "PyToCefValueList: Unsupported type \"%s\" for object \"%s\" in message list", pObjectTypeStr, pObjectStr );
		PyErr_SetString( PyExc_ValueError, buf );
		throw boost::python::error_already_set(); 
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CefRefPtr<CefListValue> PyToCefValueList( boost::python::object l )
{
	int n = boost::python::len( l );

	CefRefPtr<CefListValue> result = CefListValue::Create();
	result->SetSize( n );

	boost::python::object iterator = l.attr("__iter__")();
	boost::python::ssize_t length = boost::python::len(l); 
	for( boost::python::ssize_t i = 0; i < length; i++ )
	{
		boost::python::object value = iterator.attr( PY_NEXT_METHODNAME )();
		PySingleToCefValueList( value, result, i );
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
			case VTYPE_BINARY:
			{
				WarsCefData_t warsCefData;
				l->GetBinary( i )->GetData( &warsCefData, sizeof( warsCefData ), 0 );
				if( warsCefData.type == WARSCEF_TYPE_JSOBJECT )
				{
					WarsCefJSObject_t warsCefJSObject;
					l->GetBinary( i )->GetData( &warsCefJSObject, sizeof( warsCefJSObject ), 0 );

					CefRefPtr<JSObject> jsResultObject = new JSObject( "", warsCefJSObject.uuid );
					result.append( boost::python::object( PyJSObject( jsResultObject ) ) );
				}
				else
				{
					result.append( boost::python::object() );
				}
				break;
			}
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
CefRefPtr<CefDictionaryValue> PyToCefDictionaryValue( boost::python::object d )
{
	CefRefPtr<CefDictionaryValue> result = CefDictionaryValue::Create();

	boost::python::object items = d.attr("items")();
	boost::python::object iterator = items.attr("__iter__")();
	boost::python::ssize_t length = boost::python::len(items); 
	for( boost::python::ssize_t u = 0; u < length; u++ )
	{
		boost::python::object item = iterator.attr( PY_NEXT_METHODNAME )();
		boost::python::object value = item[1];

		boost::python::object valuetype = fntype( value );

		CefString cefkey = boost::python::extract< const char * >( boost::python::str( item[0] ) );

		if( value == boost::python::object() )
		{
			result->SetNull( cefkey );
		}
		else if( valuetype == builtins.attr("int") )
		{
			result->SetInt( cefkey, boost::python::extract<int>(value) );
		}
		else if( valuetype == builtins.attr("float") )
		{
			result->SetDouble( cefkey, boost::python::extract<float>(value) );
		}
		else if( valuetype == builtins.attr("str") )
		{
			const char *pStr = boost::python::extract<const char *>(value);
			result->SetString( cefkey, pStr );
		}
#if PY_VERSION_HEX < 0x03000000
		else if( valuetype == builtins.attr("unicode") )
		{
			const wchar_t *pStr = PyUnicode_AS_UNICODE( value.ptr() );
			if( !pStr )
			{
				PyErr_SetString(PyExc_ValueError, "PyToCefDictionaryValue: Invalid unicode object in message list" );
				throw boost::python::error_already_set(); 
			}
			result->SetString( cefkey, pStr );
		}
#endif // PY_VERSION_HEX < 0x03000000
		else if( valuetype == builtins.attr("bool") )
		{
			result->SetBool( cefkey, boost::python::extract<bool>(value) );
		}
#if PY_VERSION_HEX >= 0x03000000
		else if( valuetype == builtins.attr("list") || valuetype == builtins.attr("tuple") || valuetype == builtins.attr("set") || valuetype == builtins.attr("map") )
#else
		else if( valuetype == builtins.attr("list") || valuetype == builtins.attr("tuple") || valuetype == builtins.attr("set") )
#endif // PY_VERSION_HEX >= 0x03000000
		{
			result->SetList( cefkey, PyToCefValueList( value ) );
		}
		else if( valuetype == builtins.attr("dict") || valuetype == collections.attr("defaultdict") )
		{
			result->SetDictionary( cefkey, PyToCefDictionaryValue( value ) );
		}
		else if( valuetype == srcbuiltins.attr("Color") )
		{
			Color c = boost::python::extract<Color>(value);
			result->SetString( cefkey, UTIL_VarArgs("rgba(%d, %d, %d, %.2f)", c.r(), c.g(), c.b(), c.a() / 255.0f) );
		}
		else
		{
			const char *pObjectTypeStr = boost::python::extract<const char *>( boost::python::str( valuetype ) );
			const char *pObjectStr = boost::python::extract<const char *>( boost::python::str( value ) );
			char buf[512];
			V_snprintf( buf, sizeof(buf), "PyToCefDictionaryValue: Unsupported type \"%s\" for object \"%s\" in message list", pObjectTypeStr, pObjectStr );
			PyErr_SetString(PyExc_ValueError, buf );
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
			case VTYPE_BINARY:
			{
				WarsCefData_t warsCefData;
				CefRefPtr<CefBinaryValue> binaryData = d->GetBinary( keys[i] );
				binaryData->GetData( &warsCefData, sizeof( warsCefData ), 0 );
				if( warsCefData.type == WARSCEF_TYPE_JSOBJECT )
				{
					WarsCefJSObject_t warsCefJSObject;
					binaryData->GetData( &warsCefJSObject, sizeof( warsCefJSObject ), 0 );

					CefRefPtr<JSObject> jsResultObject = new JSObject( "", warsCefJSObject.uuid );
					result[k.c_str()] = boost::python::object( PyJSObject( jsResultObject ) );
				}
				else
				{
					result[k.c_str()] = boost::python::object();
				}
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

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
PyCefFrame::PyCefFrame( CefRefPtr<CefFrame> frame ) : m_Frame(frame)
{
	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool PyCefFrame::operator ==( const PyCefFrame &other ) const
{
	if( !m_Frame || !other.m_Frame )
		return false;
	return m_Frame->GetIdentifier() == other.m_Frame->GetIdentifier();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool PyCefFrame::operator !=( const PyCefFrame &other ) const
{
	if( !m_Frame || !other.m_Frame )
		return false;
	return m_Frame->GetIdentifier() != other.m_Frame->GetIdentifier();
}