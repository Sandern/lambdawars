//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "srcpy_srcbuiltins.h"
#include "srcpy.h"
#include "tier1/fmtstr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void RegisterTickMethod( boost::python::object method, float ticksignal, bool looped, bool userealtime ) 
{ 
	SrcPySystem()->RegisterTickMethod(method, ticksignal, looped, userealtime); 
}

void UnregisterTickMethod( boost::python::object method ) 
{ 
	SrcPySystem()->UnregisterTickMethod(method); 
}

boost::python::list GetRegisteredTickMethods() 
{ 
	return SrcPySystem()->GetRegisteredTickMethods(); 
}

bool IsTickMethodRegistered( boost::python::object method ) 
{ 
	return SrcPySystem()->IsTickMethodRegistered( method ); 
}

void RegisterPerFrameMethod( boost::python::object method ) 
{ 
	SrcPySystem()->RegisterPerFrameMethod(method); 
}

void UnregisterPerFrameMethod( boost::python::object method ) 
{ 
	SrcPySystem()->UnregisterPerFrameMethod(method); 
}

boost::python::list GetRegisteredPerFrameMethods() 
{ 
	return SrcPySystem()->GetRegisteredPerFrameMethods(); 
}

bool IsPerFrameMethodRegistered( boost::python::object method ) 
{ 
	return SrcPySystem()->IsPerFrameMethodRegistered( method ); 
}

boost::python::dict PyKeyValuesToDict( const KeyValues *pKV )
{
	boost::python::dict d;

	KeyValues *pNonConstKV = (KeyValues *)pKV;

	for ( KeyValues *pKey = pNonConstKV->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		boost::python::object name(pKey->GetName());

		if( d.has_key( name ) )
		{
			if( fntype(d.get(name)) != builtins.attr("list") )
			{
				boost::python::object tmp = d[name];
				d[name] = boost::python::list();
				d[name].attr("append")( tmp );
			}
			d[name].attr("append")( PyKeyValuesToDict( pKey ) );
		}
		else
		{
			d[name] = PyKeyValuesToDict( pKey );
		}
	}
	for ( KeyValues *pValue = pNonConstKV->GetFirstValue(); pValue; pValue = pValue->GetNextValue() )
	{
		boost::python::object name(pValue->GetName());
		boost::python::object value;

		try
		{
			switch( pValue->GetDataType() )
			{
			case KeyValues::TYPE_STRING:
				value = boost::python::object( pValue->GetString() );
				break;
			case KeyValues::TYPE_INT:
				value = boost::python::object( pValue->GetInt() );
				break;
			case KeyValues::TYPE_FLOAT:
				value = boost::python::object( pValue->GetFloat() );
				break;
			case KeyValues::TYPE_WSTRING:
				value = boost::python::object( pValue->GetWString() );
				break;
			case KeyValues::TYPE_COLOR:
				value = boost::python::object( pValue->GetColor() );
				break;
			case KeyValues::TYPE_UINT64:
				value = boost::python::object( pValue->GetUint64() );
				break;
			default:
				value = boost::python::object( pValue->GetString() );
				break;
			}
		}
		catch( boost::python::error_already_set & ) 
		{
			Warning("PyKeyValuesToDict: failed to convert key %s\n", pValue->GetName());
			PyErr_Print();
			continue;
		}

		if( d.has_key( name ) )
		{
			if( fntype(d.get(name)) != builtins.attr("list") )
			{
				boost::python::object tmp = d[name];
				d[name] = boost::python::list();
				d[name].attr("append")( tmp );
			}
			d[name].attr("append")( value );
		}
		else
		{
			d[name] = value;
		}
	}

	return d;
}

boost::python::object PyKeyValuesToDictFromFile( const char *pFileName )
{
	KeyValues *pData = new KeyValues("data");
	KeyValues::AutoDelete autodelete( pData );
	if( pData->LoadFromFile( filesystem, pFileName ) )
	{
		return PyKeyValuesToDict( pData );
	}
	return boost::python::object();
}

static void PyDictToKeyValuesConvertValue( const char *pKeyName, boost::python::object value, KeyValues *pKV, bool keys_sorted = false )
{
	try
	{
		boost::python::object valuetype = fntype( value );

		if( value == boost::python::object() )
		{
			pKV->SetInt( pKeyName, 0 );
		}
		else if( valuetype == builtins.attr("int") )
		{
			pKV->SetInt( pKeyName, boost::python::extract<int>(value) );
		}
		else if( valuetype == builtins.attr("float") )
		{
			pKV->SetFloat( pKeyName, boost::python::extract<float>(value) );
		}
		else if( valuetype == builtins.attr("str") )
		{
			pKV->SetString( pKeyName, boost::python::extract<const char *>(value) );
		}
		else if( valuetype == builtins.attr("bool") )
		{
			pKV->SetBool( pKeyName, boost::python::extract<bool>(value) );
		}
		else if( valuetype == srcbuiltins.attr("Color") )
		{
			pKV->SetColor( pKeyName, boost::python::extract<Color>(value) );
		}
	#if PY_VERSION_HEX >= 0x03000000
		else if( valuetype == builtins.attr("list") || valuetype == builtins.attr("tuple") || valuetype == builtins.attr("map") )
	#else
		else if( valuetype == builtins.attr("list") || valuetype == builtins.attr("tuple") )
	#endif // PY_VERSION_HEX >= 0x03000000
		{
			int n = boost::python::len( value );
			if( n > 0 )
			{
				bool bIsDict = fntype( boost::python::object( value[0] ) ) == builtins.attr("dict");
				if( bIsDict )
				{
					for( int i = 0; i < n; i++ ) 
					{
						// Assume lists for one key only contain dictionaries
						pKV->AddSubKey( PyDictToKeyValues( value[i], pKeyName, keys_sorted ) );
					}
				}
				else
				{
					KeyValues *pListSubKey = pKV->CreateNewKey();
					pListSubKey->SetName( pKeyName );

					for( int i = 0; i < n; i++ ) 
					{
						PyDictToKeyValuesConvertValue( CNumStr( i ).String(), value[i], pListSubKey, keys_sorted );
					}
				}
			}
			else
			{
				KeyValues *pEmpty = pKV->CreateNewKey();
				pEmpty->SetName( pKeyName );
			}
		}
		else if( valuetype == builtins.attr("dict") || valuetype == collections.attr("defaultdict") )
		{
			pKV->AddSubKey( PyDictToKeyValues( value, pKeyName, keys_sorted ) );
		}
		else
		{
			pKV->SetString( pKeyName, boost::python::extract< const char * >( value ) );
		}
	}
	catch( boost::python::error_already_set & ) 
	{
		Warning( "PyDictToKeyValuesConvertValue: failed to convert key %s\n", pKeyName );
		PyErr_Print();
	}
}

KeyValues *PyDictToKeyValues( boost::python::object d, const char *name, bool keys_sorted )
{
	KeyValues *pKV = new KeyValues( name ? name : "Data" );

	boost::python::object keys = d.attr( "keys" )();
	if( keys_sorted ) 
	{
		keys = builtins.attr( "sorted" )( keys );
	}
	boost::python::object iterator = keys.attr( "__iter__" )();
	unsigned long length = boost::python::len( keys ); 

	for( unsigned long u = 0; u < length; u++ )
	{
		boost::python::object key = iterator.attr( PY_NEXT_METHODNAME )();
		const char *pKeyName = boost::python::extract< const char * >( key );
		boost::python::object value = d[key];

		PyDictToKeyValuesConvertValue( pKeyName, value, pKV, keys_sorted );
	}

	return pKV;
}

bool PyWriteKeyValuesToFile( KeyValues *pKV, const char *filename, const char *pathid )
{
	return pKV->SaveToFile( filesystem, filename, pathid );
}

boost::python::object PyReadKeyValuesFromFile( const char *filename, const char *pathid )
{
	KeyValues *pData = new KeyValues("data");
	KeyValues::AutoDelete autodelete( pData );
	if( pData->LoadFromFile( filesystem, filename, pathid ) )
	{
		return srcbuiltins.attr("KeyValues")( boost::python::ptr( pData ) );
	}
	return boost::python::object();
}

//-----------------------------------------------------------------------------
// Purpose: Safe KeyValues version for Python
//-----------------------------------------------------------------------------
PyKeyValues::PyKeyValues( const KeyValues *pKV )
{
	m_realKeyValues = pKV->MakeCopy();
}

PyKeyValues::PyKeyValues(const PyKeyValues& src)
{
	m_realKeyValues = src.GetRawKeyValues();
}

PyKeyValues& PyKeyValues::operator=( PyKeyValues& src )
{
	if( this != &src )
	{
		if( m_realKeyValues )
			m_realKeyValues->deleteThis();
		m_realKeyValues = src.GetRawKeyValues();
	}
	return *this;
}

PyKeyValues::~PyKeyValues()
{
	m_realKeyValues->deleteThis();
}

const char *PyKeyValues::GetName() const
{
	return m_realKeyValues->GetName();
}

void PyKeyValues::SetName( const char *setName)
{
	return m_realKeyValues->SetName(setName);
}

// gets the name as a unique int
int PyKeyValues::GetNameSymbol()
{
	return m_realKeyValues->GetNameSymbol();
}

// Find a keyValue, create it if it is not found.
// Set bCreate to true to create the key if it doesn't already exist (which ensures a valid pointer will be returned)
KeyValues *PyKeyValues::FindKey(const char *keyName, bool bCreate )
{
	return m_realKeyValues->FindKey(keyName, bCreate);
}

KeyValues *PyKeyValues::FindKey(int keySymbol) const
{
	return m_realKeyValues->FindKey(keySymbol);
}

KeyValues *PyKeyValues::CreateNewKey()
{
	return m_realKeyValues->CreateNewKey();
}

void PyKeyValues::AddSubKey( KeyValues *pSubkey )
{
	return m_realKeyValues->AddSubKey(pSubkey);
}

void PyKeyValues::RemoveSubKey(KeyValues *subKey)
{
	return m_realKeyValues->RemoveSubKey(subKey);
}

KeyValues *PyKeyValues::GetFirstSubKey()
{
	return m_realKeyValues->GetFirstSubKey();
}

KeyValues *PyKeyValues::GetNextKey()
{
	return m_realKeyValues->GetNextKey();
}	

void PyKeyValues::SetNextKey( KeyValues * pDat)
{
	m_realKeyValues->SetNextKey(pDat);
}

KeyValues* PyKeyValues::GetFirstTrueSubKey()
{
	return m_realKeyValues->GetFirstTrueSubKey();
}

KeyValues* PyKeyValues::GetNextTrueSubKey()
{
	return m_realKeyValues->GetNextTrueSubKey();
}

KeyValues* PyKeyValues::GetFirstValue()
{
	return m_realKeyValues->GetFirstValue();
}

KeyValues* PyKeyValues::GetNextValue()
{
	return m_realKeyValues->GetNextValue();
}

// Assign keyvalues from a string
KeyValues * PyKeyValues::FromString( char const *szName, char const *szStringVal, char const **ppEndOfParse )
{
	return KeyValues::FromString( szName, szStringVal, ppEndOfParse );
}