//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_SRCBUILTINS_H
#define SRCPY_SRCBUILTINS_H
#ifdef _WIN32
#pragma once
#endif

#include <tier0/dbg.h>

#if PY_VERSION_HEX < 0x03000000
#include "srcpy_boostpython.h"
#endif // PY_VERSION_HEX

#include <KeyValues.h>
#include "filesystem.h"

#if PY_VERSION_HEX < 0x03000000
// These classes redirect input to Msg and Warning respectively
class SrcPyStdOut 
{
public:
	void write( boost::python::object msg )
	{
		if( PyUnicode_Check( msg.ptr() ) )
		{
			const wchar_t *pUniMsg = PyUnicode_AS_UNICODE( msg.ptr() );
			if( pUniMsg )
			{
				Msg( "%ls", pUniMsg );
			}
		}
		else
		{
			char* pMsg = PyString_AsString( msg.ptr() );
			if( pMsg == NULL )
				return;
			Msg( "%s", pMsg );
		}
	}

	void flush() {}
};

class SrcPyStdErr 
{
public:
	void write( boost::python::object msg )
	{
		if( PyUnicode_Check( msg.ptr() ) )
		{
			const wchar_t *pUniMsg = PyUnicode_AS_UNICODE( msg.ptr() );
			if( pUniMsg )
			{
				Warning( "%ls", pUniMsg );
			}
		}
		else
		{
			char* pMsg = PyString_AsString( msg.ptr() );
			if( pMsg )
			{
				Warning( "%s", pMsg );
			}
		}
	}

	void flush() {}
};

// Wrappers for Msg, Warning and DevMsg (Python does not use VarArgs)
inline void SrcPyMsg( boost::python::object msg ) 
{ 
	if( PyUnicode_Check( msg.ptr() ) )
	{
		const wchar_t *pUniMsg = PyUnicode_AS_UNICODE( msg.ptr() );
		if( pUniMsg )
		{
			Msg( "%ls", pUniMsg );
		}
	}
	else
	{
		char* pMsg = PyString_AsString( msg.ptr() );
		if( pMsg )
		{
			Msg( "%s", pMsg );
		}
	}
}

inline void SrcPyWarning( boost::python::object msg ) 
{ 
	if( PyUnicode_Check( msg.ptr() ) )
	{
		const wchar_t *pUniMsg = PyUnicode_AS_UNICODE( msg.ptr() );
		if( pUniMsg )
		{
			Warning( "%ls", pUniMsg );
		}
	}
	else
	{
		char* pMsg = PyString_AsString( msg.ptr() );
		if( pMsg )
		{
			Warning( "%s", pMsg );
		}
	}
}

inline void SrcPyDevMsg( int level, boost::python::object msg ) 
{ 
	if( PyUnicode_Check( msg.ptr() ) )
	{
		const wchar_t *pUniMsg = PyUnicode_AS_UNICODE( msg.ptr() );
		if( pUniMsg )
		{
			DevMsg( level, "%ls", pUniMsg );
		}
	}
	else
	{
		char* pMsg = PyString_AsString( msg.ptr() );
		if( pMsg )
		{
			DevMsg( level, "%s", pMsg );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline void PyCOM_TimestampedLog( boost::python::object msg )
{
	if( PyUnicode_Check( msg.ptr() ) )
	{
		const wchar_t *pUniMsg = PyUnicode_AS_UNICODE( msg.ptr() );
		if( pUniMsg )
		{
			COM_TimestampedLog( "%ls", pUniMsg );
		}
	}
	else
	{
		char* pMsg = PyString_AsString( msg.ptr() );
		if( pMsg )
		{
			COM_TimestampedLog( "%s", pMsg );
		}
	}
}
#else
// These classes redirect input to Msg and Warning respectively
class SrcPyStdOut 
{
public:
	void write( const char *msg ) { Msg( "%s", msg ); }
	void flush() {}
	const char *encoding() { return "ansi"; }
};

class SrcPyStdErr 
{
public:
	void write( const char *msg ) { Warning( "%s", msg ); }
	void flush() {}
	const char *encoding() { return "ansi"; }
};

// Wrappers for Msg, Warning and DevMsg (Python does not use VarArgs)
inline void SrcPyMsg( const char *msg ) { Msg( "%s", msg ); }
inline void SrcPyWarning( const char *msg ) { Warning( "%s", msg ); }
inline void SrcPyDevMsg( int level, const char *msg ) { DevMsg( level, "%s", msg ); }

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline void PyCOM_TimestampedLog( const char *msg ) { COM_TimestampedLog( "%s", msg ); }
#endif // 0x03000000

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RegisterTickMethod( boost::python::object method, float ticksignal, bool looped = true, bool userealtime = false );
void UnregisterTickMethod( boost::python::object method );
boost::python::list GetRegisteredTickMethods();
bool IsTickMethodRegistered( boost::python::object method );

void RegisterPerFrameMethod( boost::python::object method );
void UnregisterPerFrameMethod( boost::python::object method );
boost::python::list GetRegisteredPerFrameMethods();
bool IsPerFrameMethodRegistered( boost::python::object method );

//-----------------------------------------------------------------------------
// Purpose: Safe (but inefficient) KeyValues version for Python
//-----------------------------------------------------------------------------
boost::python::dict PyKeyValuesToDict( const KeyValues *pKV );
boost::python::object PyKeyValuesToDictFromFile( const char *pFileName );
KeyValues *PyDictToKeyValues( boost::python::object d );

class PyKeyValues
{
public:
	PyKeyValues( const KeyValues *pKV );

	PyKeyValues(const PyKeyValues& src);
	PyKeyValues& operator=( PyKeyValues& src );

	// Constructors
	PyKeyValues( const char *setName )
	{
		m_realKeyValues = new KeyValues(setName);
	}

	PyKeyValues( const char *setName, const char *firstKey, const char *firstValue )
	{
		m_realKeyValues = new KeyValues(setName, firstKey, firstValue);
	}
	PyKeyValues( const char *setName, const char *firstKey, int firstValue )
	{
		m_realKeyValues = new KeyValues(setName, firstKey, firstValue);
	}
	PyKeyValues( const char *setName, const char *firstKey, const char *firstValue, const char *secondKey, const char *secondValue )
	{
		m_realKeyValues = new KeyValues(setName, firstKey, firstValue, secondKey, secondValue);
	}
	PyKeyValues( const char *setName, const char *firstKey, int firstValue, const char *secondKey, int secondValue )
	{
		m_realKeyValues = new KeyValues(setName, firstKey, firstValue, secondKey, secondValue);
	}

	~PyKeyValues();

	KeyValues *GetRawKeyValues() const { return m_realKeyValues->MakeCopy(); }

	// -- methods
	// Section name
	const char *GetName() const;
	void SetName( const char *setName);

	// gets the name as a unique int
	int GetNameSymbol();

	// File access. Set UsesEscapeSequences true, if resource file/buffer uses Escape Sequences (eg \n, \t)
	//void UsesEscapeSequences(bool state); // default false
	//bool LoadFromFile( IBaseFileSystem *filesystem, const char *resourceName, const char *pathID = NULL );
	//bool SaveToFile( IBaseFileSystem *filesystem, const char *resourceName, const char *pathID = NULL);

	// Read from a buffer...  Note that the buffer must be null terminated
	//bool LoadFromBuffer( char const *resourceName, const char *pBuffer, IBaseFileSystem* pFileSystem = NULL, const char *pPathID = NULL );

	// Read from a utlbuffer...
	//bool LoadFromBuffer( char const *resourceName, CUtlBuffer &buf, IBaseFileSystem* pFileSystem = NULL, const char *pPathID = NULL );

	// Find a keyValue, create it if it is not found.
	// Set bCreate to true to create the key if it doesn't already exist (which ensures a valid pointer will be returned)
	KeyValues *FindKey(const char *keyName, bool bCreate = false);
	KeyValues *FindKey(int keySymbol) const;
	KeyValues *CreateNewKey();		// creates a new key, with an autogenerated name.  name is guaranteed to be an integer, of value 1 higher than the highest other integer key name
	void AddSubKey( KeyValues *pSubkey );	// Adds a subkey. Make sure the subkey isn't a child of some other keyvalues
	void RemoveSubKey(KeyValues *subKey);	// removes a subkey from the list, DOES NOT DELETE IT

	// Key iteration.
	//
	// NOTE: GetFirstSubKey/GetNextKey will iterate keys AND values. Use the functions 
	// below if you want to iterate over just the keys or just the values.
	//
	KeyValues *GetFirstSubKey();	// returns the first subkey in the list
	KeyValues *GetNextKey();		// returns the next subkey
	void SetNextKey( KeyValues * pDat);

	//
	// These functions can be used to treat it like a true key/values tree instead of 
	// confusing values with keys.
	//
	// So if you wanted to iterate all subkeys, then all values, it would look like this:
	//     for ( KeyValues *pKey = pRoot->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	//     {
	//		   Msg( "Key name: %s\n", pKey->GetName() );
	//     }
	//     for ( KeyValues *pValue = pRoot->GetFirstValue(); pKey; pKey = pKey->GetNextValue() )
	//     {
	//         Msg( "Int value: %d\n", pValue->GetInt() );  // Assuming pValue->GetDataType() == TYPE_INT...
	//     }
	KeyValues* GetFirstTrueSubKey();
	KeyValues* GetNextTrueSubKey();

	KeyValues* GetFirstValue();	// When you get a value back, you can use GetX and pass in NULL to get the value.
	KeyValues* GetNextValue();


	// Data access
	int   GetInt( const char *keyName = NULL, int defaultValue = 0 ) { return m_realKeyValues->GetInt(keyName, defaultValue); }
	uint64 GetUint64( const char *keyName = NULL, uint64 defaultValue = 0 ) { return m_realKeyValues->GetUint64(keyName, defaultValue); }
	float GetFloat( const char *keyName = NULL, float defaultValue = 0.0f ) { return m_realKeyValues->GetFloat(keyName, defaultValue); }
	const char *GetString( const char *keyName = NULL, const char *defaultValue = "" ) { return m_realKeyValues->GetString(keyName, defaultValue); }
	//const wchar_t *GetWString( const char *keyName = NULL, const wchar_t *defaultValue = L"" ) { return m_realKeyValues->GetInt(keyName, defaultValue); }
	//void *GetPtr( const char *keyName = NULL, void *defaultValue = (void*)0 ) { return m_realKeyValues->GetInt(keyName, defaultValue); }
	Color GetColor( const char *keyName = NULL /* default value is all black */) { return m_realKeyValues->GetColor(keyName); }
	bool GetBool( const char *keyName = NULL, bool defaultValue = false ) { return GetInt( keyName, defaultValue ? 1 : 0 ) ? true : false; }
	bool  IsEmpty(const char *keyName = NULL) { return m_realKeyValues->IsEmpty(keyName); }

	// Data access
	int   GetInt( int keySymbol, int defaultValue = 0 ) { return m_realKeyValues->GetInt(keySymbol, defaultValue); }
	float GetFloat( int keySymbol, float defaultValue = 0.0f ) { return m_realKeyValues->GetFloat(keySymbol, defaultValue); }
	const char *GetString( int keySymbol, const char *defaultValue = "" ) { return m_realKeyValues->GetString(keySymbol, defaultValue); }
	//const wchar_t *GetWString( int keySymbol, const wchar_t *defaultValue = L"" );
	//void *GetPtr( int keySymbol, void *defaultValue = (void*)0 );
	Color GetColor( int keySymbol /* default value is all black */) { return m_realKeyValues->GetColor(keySymbol); }
	bool GetBool( int keySymbol, bool defaultValue = false ) { return GetInt( keySymbol, defaultValue ? 1 : 0 ) ? true : false; }
	bool  IsEmpty( int keySymbol ) { return m_realKeyValues->IsEmpty(keySymbol); }

	// Key writing
	//void SetWString( const char *keyName, const wchar_t *value );
	void SetString( const char *keyName, const char *value ) { m_realKeyValues->SetString(keyName, value); }
	void SetInt( const char *keyName, int value ) { m_realKeyValues->SetInt(keyName, value); }
	void SetUint64( const char *keyName, uint64 value ) { m_realKeyValues->SetUint64(keyName, value); }
	void SetFloat( const char *keyName, float value ) { m_realKeyValues->SetFloat(keyName, value); }
	//void SetPtr( const char *keyName, void *value );
	void SetColor( const char *keyName, Color value) { m_realKeyValues->SetColor(keyName, value); }
	void SetBool( const char *keyName, bool value ) { SetInt( keyName, value ? 1 : 0 ); }

	//KeyValues& operator=( KeyValues& src );

	// Adds a chain... if we don't find stuff in this keyvalue, we'll look
	// in the one we're chained to.
	void ChainKeyValue( KeyValues* pChain )  { m_realKeyValues->ChainKeyValue(pChain); }

	//void RecursiveSaveToFile( CUtlBuffer& buf, int indentLevel );

	//bool WriteAsBinary( CUtlBuffer &buffer );
	//bool ReadAsBinary( CUtlBuffer &buffer );

	// Allocate & create a new copy of the keys
	//KeyValues *MakeCopy( void ) const;

	// Make a new copy of all subkeys, add them all to the passed-in keyvalues
	//void CopySubkeys( KeyValues *pParent ) const;

	// Clear out all subkeys, and the current value
	void Clear( void ) { m_realKeyValues->Clear(); }

	// Data type
	enum pytypes_t
	{
		TYPE_NONE = 0,
		TYPE_STRING,
		TYPE_INT,
		TYPE_FLOAT,
		TYPE_PTR,
		TYPE_WSTRING,
		TYPE_COLOR,
		TYPE_UINT64,
		TYPE_NUMTYPES, 
	};
	KeyValues::types_t GetDataType(const char *keyName = NULL) { return (KeyValues::types_t)m_realKeyValues->GetDataType(keyName); }

	// Virtual deletion function - ensures that KeyValues object is deleted from correct heap
	void deleteThis() { m_realKeyValues->deleteThis(); m_realKeyValues = NULL; }

	void SetStringValue( char const *strValue ) { m_realKeyValues->SetStringValue(strValue); }

	// unpack a key values list into a structure
	//void UnpackIntoStructure( struct KeyValuesUnpackStructure const *pUnpackTable, void *pDest );

	// Process conditional keys for widescreen support.
	//bool ProcessResolutionKeys( const char *pResString );

	// Assign keyvalues from a string
	static KeyValues * FromString( char const *szName, char const *szStringVal, char const **ppEndOfParse = NULL );

private:
	KeyValues *m_realKeyValues;
};

#endif // SRCPY_SRCBUILTINS_H