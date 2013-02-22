//====== Copyright � 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRC_PYTHON_BASE_H
#define SRC_PYTHON_BASE_H
#ifdef _WIN32
#pragma once
#endif

#include <KeyValues.h>
#include "filesystem.h"
#include "string_t.h"
#include "cmodel.h"
#include <utlrbtree.h>
#include <utlflags.h>

#include <boost/python.hpp>

class CBasePlayer;

void PyCOM_TimestampedLog( char const *fmt );

//-----------------------------------------------------------------------------
// Purpose: Safe KeyValues version for Python
//-----------------------------------------------------------------------------
boost::python::dict PyKeyValuesToDict( const KeyValues *pKV );
KeyValues *PyDictToKeyValues( boost::python::dict d );

//boost::python::dict PyKeyValues( boost::python::object name, 
//	boost::python::object firstKey = boost::python::object(), boost::python::object firstValue = boost::python::object(),
//	boost::python::object secondKey = boost::python::object(), boost::python::object secondValue = boost::python::object() );

#if 1
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
	bool  IsEmpty(const char *keyName = NULL) { return m_realKeyValues->IsEmpty(keyName); }

	// Data access
	int   GetInt( int keySymbol, int defaultValue = 0 ) { return m_realKeyValues->GetInt(keySymbol, defaultValue); }
	float GetFloat( int keySymbol, float defaultValue = 0.0f ) { return m_realKeyValues->GetFloat(keySymbol, defaultValue); }
	const char *GetString( int keySymbol, const char *defaultValue = "" ) { return m_realKeyValues->GetString(keySymbol, defaultValue); }
	//const wchar_t *GetWString( int keySymbol, const wchar_t *defaultValue = L"" );
	//void *GetPtr( int keySymbol, void *defaultValue = (void*)0 );
	Color GetColor( int keySymbol /* default value is all black */) { return m_realKeyValues->GetColor(keySymbol); }
	bool  IsEmpty( int keySymbol ) { return m_realKeyValues->IsEmpty(keySymbol); }

	// Key writing
	//void SetWString( const char *keyName, const wchar_t *value );
	void SetString( const char *keyName, const char *value ) { m_realKeyValues->SetString(keyName, value); }
	void SetInt( const char *keyName, int value ) { m_realKeyValues->SetInt(keyName, value); }
	void SetUint64( const char *keyName, uint64 value ) { m_realKeyValues->SetUint64(keyName, value); }
	void SetFloat( const char *keyName, float value ) { m_realKeyValues->SetFloat(keyName, value); }
	//void SetPtr( const char *keyName, void *value );
	void SetColor( const char *keyName, Color value) { m_realKeyValues->SetColor(keyName, value); }

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
#endif // 0

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
extern  "C" {
	int SrcPyPathIsInGameFolder( const char *pPath );
}
boost::python::object AbsToRel( const char *path );
boost::python::object RelToAbs( const char *path );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RegisterTickMethod( boost::python::object method, float ticksignal, bool looped = true );
void UnregisterTickMethod( boost::python::object method );
boost::python::list GetRegisteredTickMethods();

void RegisterPerFrameMethod( boost::python::object method );
void UnregisterPerFrameMethod( boost::python::object method );
boost::python::list GetRegisteredPerFrameMethods();

//-----------------------------------------------------------------------------
// Purpose: PyUtlRBTree
//-----------------------------------------------------------------------------
#define PyUtlRBTreeTEMPLATES boost::python::object, int
class PyUtlRBTree : CUtlRBTree< PyUtlRBTreeTEMPLATES >
{
public:
	PyUtlRBTree( int growSize = 0, int initSize = 0, boost::python::object lessfunc = boost::python::object() );
	PyUtlRBTree( boost::python::object lessfunc );

	// boost::python requires these
	PyUtlRBTree& operator=( const PyUtlRBTree &other );
	PyUtlRBTree( PyUtlRBTree const &tree );

public:
	// Functions we want exposed
	inline boost::python::object Element( int i ) { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::Element(i); }
	boost::python::object operator[]( int i ) { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::Element(i); }

	// Gets the root
	inline int  Root() const { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::Root(); }

	// Num elements
	inline unsigned int Count() const { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::Count(); }

	// Sets the less func
	void SetLessFunc( boost::python::object func );

	// Insert method (inserts in order)
	int  Insert( boost::python::object insert );
	int  InsertIfNotFound( boost::python::object insert );

	// Inserts a node into the tree, doesn't copy the data in.
	void FindInsertionPosition( boost::python::object const &insert, int &parent, bool &leftchild );

	// Insertion, removal
	int  InsertAt( int parent, bool leftchild );

	// Find method
	int  Find( boost::python::object search ) const { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::Find(search); }

	// Remove methods
	inline void     RemoveAt( int i ) { CUtlRBTree<PyUtlRBTreeTEMPLATES>::RemoveAt(i); }
	inline bool     Remove( boost::python::object remove ) { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::Remove(remove); }
	inline void     RemoveAll( ) { CUtlRBTree<PyUtlRBTreeTEMPLATES>::RemoveAll(); }
	inline void		Purge() { CUtlRBTree<PyUtlRBTreeTEMPLATES>::Purge(); }	

	// Iteration
	inline int  FirstInorder() const { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::FirstInorder(); }	
	inline int  NextInorder( int i ) const { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::NextInorder(i); }
	inline int  PrevInorder( int i ) const { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::PrevInorder(i); }
	inline int  LastInorder() const { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::LastInorder(); }

	inline int  FirstPreorder() const { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::FirstPreorder(); }
	inline int  NextPreorder( int i ) const { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::NextPreorder(i); }
	inline int  PrevPreorder( int i ) const { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::PrevPreorder(i); }
	inline int  LastPreorder( ) const { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::LastPreorder(); }

	inline int  FirstPostorder() const { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::FirstPostorder(); }
	inline int  NextPostorder( int i ) const { return CUtlRBTree<PyUtlRBTreeTEMPLATES>::NextPostorder(i); }

private:
	boost::python::object m_pyLessFunc;
};

//-----------------------------------------------------------------------------
// Purpose: PyUtlFlags
//-----------------------------------------------------------------------------
class PyUtlFlags : public CUtlFlags<unsigned short>
{
public:
	PyUtlFlags( int nInitialFlags = 0 ) : CUtlFlags<unsigned short>(nInitialFlags) {}

	// Flag setting
	inline void SetFlag( int nFlagMask ) { CUtlFlags<unsigned short>::SetFlag(nFlagMask); }
	inline void SetFlag( int nFlagMask, bool bEnable ) { CUtlFlags<unsigned short>::SetFlag(nFlagMask, bEnable); }

	// Flag clearing
	inline void ClearFlag( int nFlagMask ) { CUtlFlags<unsigned short>::ClearFlag(nFlagMask); }
	inline void ClearAllFlags() { CUtlFlags<unsigned short>::ClearAllFlags(); }
	inline bool IsFlagSet( int nFlagMask ) const { return CUtlFlags<unsigned short>::IsFlagSet(nFlagMask); }

	// Is any flag set?
	inline bool IsAnyFlagSet() const { return CUtlFlags<unsigned short>::IsAnyFlagSet(); }
};

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: PyOutputEvent
//-----------------------------------------------------------------------------
class PyOutputEvent : public CBaseEntityOutput
{
public:
	PyOutputEvent();

	void Set( variant_t value );

	// void Firing, no parameter
	void FireOutput( CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay = 0 );
	void FireOutput( variant_t Value, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay = 0 ) 
	{ CBaseEntityOutput::FireOutput( Value, pActivator, pCaller, fDelay ); }
};
#endif // CLIENT_DLL

#endif // SRC_PYTHON_BASE_H