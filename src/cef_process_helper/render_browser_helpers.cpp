#include "render_browser_helpers.h"


//-----------------------------------------------------------------------------
// Purpose: Helper for converting a V8ValueList to CefList
//-----------------------------------------------------------------------------
void V8ValueListToListValue( const CefV8ValueList& arguments, CefRefPtr<CefListValue> args )
{
	int idx = 0;

	// Add arguments
	CefV8ValueList::const_iterator i2 = arguments.begin();
	for( ; i2 != arguments.end(); ++i2 )
	{
		CefRefPtr<CefV8Value> value = (*i2);
		if( value->IsBool() )
		{
			args->SetBool( idx, value->GetBoolValue() );
		}
		else if( value->IsInt() )
		{
			args->SetInt( idx, value->GetIntValue() );
		}
		else if( value->IsDouble() )
		{
			args->SetDouble( idx, value->GetDoubleValue() );
		}
		else if( value->IsString() )
		{
			args->SetString( idx, value->GetStringValue() );
		}
		else if( value->IsObject() )
		{
			args->SetString( idx, value->GetStringValue() );
		}
		else
		{
			args->SetNull( idx );
		}

		idx++;
	}
}

CefRefPtr<CefV8Value> DictionaryValueToV8Value( const CefRefPtr<CefDictionaryValue> args, const CefString &key )
{
	CefRefPtr<CefV8Value> ret = NULL;

	// Keep in sync with ListValueToV8Value!
	switch( args->GetType( key ) )
	{
		case VTYPE_INT:
		{
			ret = CefV8Value::CreateInt( args->GetInt( key ) );
			break;
		}
		case VTYPE_DOUBLE:
		{
			ret = CefV8Value::CreateDouble( args->GetDouble( key ) );
			break;
		}
		case VTYPE_BOOL:
		{
			ret = CefV8Value::CreateBool( args->GetBool( key ) );
			break;
		}
		case VTYPE_STRING:
		{
			ret = CefV8Value::CreateString( args->GetString( key ) );
			break;
		}
		case VTYPE_LIST:
		{
			CefRefPtr<CefListValue> src = args->GetList( key );
			CefRefPtr<CefV8Value> v = CefV8Value::CreateArray( src->GetSize() );
			if( v )
			{
				for( int i = 0; i < v->GetArrayLength(); i++ )
					v->SetValue( i, ListValueToV8Value( src, i ) );
				ret = v;
			}
			break;
		}
		case VTYPE_DICTIONARY:
		{
			CefRefPtr<CefDictionaryValue> src = args->GetDictionary( key );
			CefDictionaryValue::KeyList keys;
			src->GetKeys( keys );

			CefRefPtr<CefV8Value> v = CefV8Value::CreateObject( NULL );
			if( v )
			{
				for( size_t i = 0; i < keys.size(); i++ )
				{
					v->SetValue( keys[i], DictionaryValueToV8Value( src, keys[i] ), V8_PROPERTY_ATTRIBUTE_NONE );
				}
			}
			ret = v;
			break;
		}
		default:
		{
			ret = CefV8Value::CreateString( args->GetString( key ) );
			break;
		}
	}

	return ret ? ret : CefV8Value::CreateString( "Error" );
}

CefRefPtr<CefV8Value> ListValueToV8Value( const CefRefPtr<CefListValue> args, int idx )
{
	CefRefPtr<CefV8Value> ret = NULL;

	// Keep in sync with DictionaryValueToV8Value!
	switch( args->GetType( idx ) )
	{
		case VTYPE_INT:
		{
			ret = CefV8Value::CreateInt( args->GetInt( idx ) );
			break;
		}
		case VTYPE_DOUBLE:
		{
			ret = CefV8Value::CreateDouble( args->GetDouble( idx ) );
			break;
		}
		case VTYPE_BOOL:
		{
			ret = CefV8Value::CreateBool( args->GetBool( idx ) );
			break;
		}
		case VTYPE_STRING:
		{
			ret = CefV8Value::CreateString( args->GetString( idx ) );
			break;
		}
		case VTYPE_LIST:
		{
			CefRefPtr<CefListValue> src = args->GetList( idx );
			CefRefPtr<CefV8Value> v = CefV8Value::CreateArray( src->GetSize() );
			if( v )
			{
				for( int i = 0; i < v->GetArrayLength(); i++ )
					v->SetValue( i, ListValueToV8Value( src, i ) );
				ret = v;
			}
			break;
		}
		case VTYPE_DICTIONARY:
		{
			CefRefPtr<CefDictionaryValue> src = args->GetDictionary( idx );
			CefDictionaryValue::KeyList keys;
			src->GetKeys( keys );

			CefRefPtr<CefV8Value> v = CefV8Value::CreateObject( NULL );
			if( v )
			{
				for( size_t i = 0; i < keys.size(); i++ )
				{
					v->SetValue( keys[i], DictionaryValueToV8Value( src, keys[i] ), V8_PROPERTY_ATTRIBUTE_NONE );
				}
			}
			ret = v;
			break;
		}
		default:
		{
			ret = CefV8Value::CreateString( args->GetString( idx ) );
			break;
		}
	}

	return ret ? ret : CefV8Value::CreateString( "Error" );
}

void ListValueToV8ValueList( const CefRefPtr<CefListValue> args, CefV8ValueList& arguments )
{
	int idx = 0;

	arguments.clear();

	size_t n = args->GetSize();

	for( size_t i = 0; i < n; i++ )
		arguments.push_back( ListValueToV8Value( args, i ) );
}
