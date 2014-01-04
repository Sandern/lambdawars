//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef_python.h"
#include "srcpy.h"
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
boost::python::object PyJSObject::GetName()
{
	std::string name = m_Object->GetName().ToString();
	return boost::python::object(name.c_str());
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CefRefPtr<CefListValue> PyToCefValueList( boost::python::list l )
{
	int n = boost::python::len( l );

	CefRefPtr<CefListValue> result = CefListValue::Create();
	result->SetSize( n );

	for( int i = 0; i < n; i++ )
	{
		boost::python::object value = l[i];
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
		else if( valuetype == builtins.attr("bool") )
		{
			result->SetBool( i, boost::python::extract<bool>(value) );
		}
		else if( valuetype == builtins.attr("list") )
		{
			result->SetList( i, PyToCefValueList( boost::python::list( value ) ) );
		}
		else if( valuetype == builtins.attr("dict") )
		{
			result->SetDictionary( i, PyToCefDictionaryValue( boost::python::dict( value ) ) );
		}
		else
		{
			const char *pObjectTypeStr = boost::python::extract<const char *>( boost::python::str( valuetype ) );
			const char *pObjectStr = boost::python::extract<const char *>( boost::python::str( value ) );
			char buf[512];
			V_snprintf( buf, 512, "PyToCefValueList: Unsupported type \"%s\" for object \"%s\" in message list", pObjectTypeStr, pObjectStr );
			PyErr_SetString( PyExc_ValueError, buf );
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

	boost::python::object key, value;
	const boost::python::object objectKeys = d.iterkeys();
	unsigned long ulCount = boost::python::len(d); 
	for( unsigned long i = 0; i < ulCount; i++ )
	{
		key = objectKeys.attr( "next" )();
		value = d[key];
		boost::python::object valuetype = fntype( value );

		CefString cefkey = boost::python::extract< const char * >( key );

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
		else if( valuetype == builtins.attr("bool") )
		{
			result->SetBool( cefkey, boost::python::extract<bool>(value) );
		}
		else if( valuetype == builtins.attr("list") )
		{
			result->SetList( cefkey, PyToCefValueList( boost::python::list( value ) ) );
		}
		else if( valuetype == builtins.attr("dict") )
		{
			result->SetDictionary( cefkey, PyToCefDictionaryValue( boost::python::dict( value ) ) );
		}
		else
		{
			const char *pObjectTypeStr = boost::python::extract<const char *>( boost::python::str( valuetype ) );
			const char *pObjectStr = boost::python::extract<const char *>( boost::python::str( value ) );
			char buf[512];
			V_snprintf( buf, 512, "PyToCefDictionaryValue: Unsupported type \"%s\" for object \"%s\" in message list", pObjectTypeStr, pObjectStr );
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