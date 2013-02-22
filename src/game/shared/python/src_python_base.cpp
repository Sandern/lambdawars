//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "src_python_base.h"
#include "utlrbtree.h"
#include "src_python.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern const char *COM_GetModDirectory( void );

void PyCOM_TimestampedLog( char const *fmt )
{
	COM_TimestampedLog( fmt );
}

bp::dict PyKeyValuesToDict( const KeyValues *pKV )
{
	bp::dict d;

	KeyValues *pNonConstKV = (KeyValues *)pKV;

	for ( KeyValues *pKey = pNonConstKV->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		bp::object name(pKey->GetName());
		if( d.has_key( name ) == false )
			d[name] = bp::list();

		bp::list l = bp::extract< bp::list >(d[name]);
		l.append( PyKeyValuesToDict( pKey ) );
	}
	for ( KeyValues *pValue = pNonConstKV->GetFirstValue(); pValue; pValue = pValue->GetNextValue() )
	{
		bp::object name(pValue->GetName());
		if( d.has_key( name ) == false )
			d[name] = bp::list();

		bp::list l = bp::extract< bp::list >(d[name]);
		l.append( pValue->GetString() ); // TODO: Convert to right type
	}

	return d;
}

KeyValues *PyDictToKeyValues( bp::dict d )
{
	bp::object type = __builtin__.attr("type");
	bp::object dicttype = types.attr("DictType");

	KeyValues *pKV = new KeyValues("Data");

	bp::object objectKey, objectValue;
	const bp::object objectKeys = d.iterkeys();
	const bp::object objectValues = d.itervalues();
	unsigned long ulCount = bp::len(d);
	for( unsigned long u = 0; u < ulCount; u++ )
	{
		objectKey = objectKeys.attr( "next" )();
		objectValue = objectValues.attr( "next" )();

		bp::list elements = bp::list(objectValue);
		bp::ssize_t n = bp::len(elements);
		for( bp::ssize_t i=0; i < n; i++ ) 
		{
			 if( type(elements[i]) == dicttype )
			 {
				 pKV->AddSubKey( PyDictToKeyValues(bp::dict(elements[i])) );
			 }
			 else
			 {
				 pKV->SetString( bp::extract< const char * >( objectKey ), 
								 bp::extract< const char * >( elements[i] ) );
			 }
		}
	}

	return pKV;
}

#if 0
bp::dict PyKeyValues( bp::object name, bp::object firstKey, bp::object firstValue, bp::object secondKey, bp::object secondValue )
{
	bp::object type = __builtin__.attr("type");
	bp::object dicttype = types.attr("DictType");
	if( type(name) == dicttype )
	{
		return bp::dict(name);
	}

	bp::dict d;
	d[name] = bp::dict();
	if( firstKey != bp::object() )
	{
		bp::list l;
		l.append( firstValue );
		d[name][firstKey] = l;
	}

	if( secondKey != bp::object() )
	{
		if( firstKey == secondKey )
		{
			bp::list(d[name][firstKey]).append( secondValue );
		}
		else
		{
			bp::list l;
			l.append( secondValue );
			d[name][secondKey] = l;
		}
	}
	return d;
}
#endif // 0

#if 1
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

#endif // 1

#ifndef CLIENT_DLL
	static ConVar py_disable_protect_path("py_disable_protect_path", "0", FCVAR_CHEAT);
#endif // CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Almost the same as V_FixupPathName, but does not lowercase
//			Linux/Python fileobject does not like that.
//-----------------------------------------------------------------------------
void SrcPyFixupPathName( char *pOut )
{
	V_FixSlashes( pOut );
	V_RemoveDotSlashes( pOut );
	V_FixDoubleSlashes( pOut );
}

bool IsPathProtected()
{
	const char *pGameDir = COM_GetModDirectory();
	const char *pDevModDir = "hl2wars_asw_dev";
	if( Q_strncmp( pGameDir, pDevModDir, Q_strlen( pDevModDir ) ) != 0 )
		return true;

#ifndef CLIENT_DLL
	return !py_disable_protect_path.GetBool();
#else
	return true;
#endif // CLIENT_DLL
}

extern  "C" {

int SrcPyPathIsInGameFolder( const char *pPath )
{
#ifndef CLIENT_DLL
	if( IsPathProtected() )
#endif // CLIENT_DLL
	{
		// Verify the file is in the gamefolder
		char searchPaths[_MAX_PATH];
		filesystem->GetSearchPath( "MOD", true, searchPaths, sizeof( searchPaths ) );
		V_StripTrailingSlash( searchPaths );

		if( Q_IsAbsolutePath(pPath) )
		{
			if( Q_strnicmp(pPath, searchPaths, Q_strlen(searchPaths)) != 0 ) 
				return 0;
		}
		else
		{
			char pFullPath[_MAX_PATH];
			filesystem->RelativePathToFullPath(pPath, "MOD", pFullPath, _MAX_PATH);
			if( Q_strnicmp(pFullPath, searchPaths, Q_strlen(searchPaths)) != 0 ) 
				return 0;
		}
	}
	return 1;
}

int SrcPyIsClient( void )
{
#ifdef CLIENT_DLL
	return 1;
#else
	return 0;
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Helper. The not silent version throws exception if the path is outside the game folder
//-----------------------------------------------------------------------------
int SrcPyGetFullPathSilent( const char *pAssumedRelativePath, char *pFullPath, int size )
{
	// Verify the file is in the gamefolder
	char searchPaths[_MAX_PATH];
	filesystem->GetSearchPath( "MOD", true, searchPaths, sizeof( searchPaths ) );

	if( Q_IsAbsolutePath(pAssumedRelativePath) )
	{
		char temp[_MAX_PATH];
		Q_strncpy(temp, pAssumedRelativePath, _MAX_PATH);

		SrcPyFixupPathName( temp );
		V_StrSubst( temp, "\\", "//", pFullPath, size ); 
		Q_RemoveDotSlashes( pFullPath );
		V_StripTrailingSlash( searchPaths );

#ifndef CLIENT_DLL
		if( IsPathProtected() )
#endif // CLIENT_DLL
		{
			if( Q_strnicmp(pFullPath, searchPaths, Q_strlen(searchPaths)) != 0 )
			{
				return 0;
			}
		}
	}
	else
	{
#ifndef CLIENT_DLL
		if( IsPathProtected() )
#endif // CLIENT_DLL
		{
			// We know the path we want is relative, so just concate with searchPaths
			char temp[_MAX_PATH];
			Q_snprintf(temp, size, "%s/%s", searchPaths, pAssumedRelativePath);

			SrcPyFixupPathName( temp );
			V_StrSubst(temp, "\\", "//", pFullPath, size ); 
			Q_RemoveDotSlashes(pFullPath);

			if( Q_strnicmp(pFullPath, searchPaths, Q_strlen(searchPaths)) != 0 )
			{
				return 0;
			}
		}
#ifndef CLIENT_DLL
		else
		{
			filesystem->RelativePathToFullPath(pAssumedRelativePath, "MOD", pFullPath, size);
		}
#endif // CLIENT_DLL
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int SrcPyGetFullPath( const char *pAssumedRelativePath, char *pFullPath, int size )
{
	if( SrcPyGetFullPathSilent(pAssumedRelativePath, pFullPath, size) == 0 )
	{
		char buf[512];
		Q_snprintf(buf, 512, "File must be in the mod folder! (%s)", pFullPath);
		PyErr_SetString(PyExc_ValueError, buf);
		throw boost::python::error_already_set(); 
		return 0;
	}
	return 1;
}
}

//-----------------------------------------------------------------------------
// Purpose: Used by the python library
//-----------------------------------------------------------------------------
extern  "C" {
int getmoddir( const char *path, int len )
{
	return g_pFullFileSystem->GetSearchPath("MOD", false, (char *)path, len);
}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
boost::python::object AbsToRel( const char *path )
{
	char temp[_MAX_PATH];
	filesystem->FullPathToRelativePath(path, temp, _MAX_PATH);
	return boost::python::object(temp);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
boost::python::object RelToAbs( const char *path )
{
	char temp[_MAX_PATH];
	filesystem->RelativePathToFullPath(path, "MOD", temp, _MAX_PATH);
	return boost::python::object(temp);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void RegisterTickMethod( boost::python::object method, float ticksignal, bool looped ) { SrcPySystem()->RegisterTickMethod(method, ticksignal, looped); }
void UnregisterTickMethod( boost::python::object method ) { SrcPySystem()->UnregisterTickMethod(method); }
boost::python::list GetRegisteredTickMethods() { return SrcPySystem()->GetRegisteredTickMethods(); }

void RegisterPerFrameMethod( boost::python::object method ) { SrcPySystem()->RegisterPerFrameMethod(method); }
void UnregisterPerFrameMethod( boost::python::object method ) { SrcPySystem()->UnregisterPerFrameMethod(method); }
boost::python::list GetRegisteredPerFrameMethods() { return SrcPySystem()->GetRegisteredPerFrameMethods(); }

//-----------------------------------------------------------------------------
// Purpose: PyUtlRBTree
//-----------------------------------------------------------------------------
PyUtlRBTree::PyUtlRBTree( int growSize, int initSize0, boost::python::object lessfunc )
: CUtlRBTree<boost::python::object, int>(growSize, initSize0, NULL ) // 
{
	m_pyLessFunc = lessfunc;
}

PyUtlRBTree::PyUtlRBTree( boost::python::object lessfunc )
	: CUtlRBTree<boost::python::object, int>( NULL )
{
	m_pyLessFunc = lessfunc;
}

PyUtlRBTree& PyUtlRBTree::operator=( const PyUtlRBTree &other )
{
	return (PyUtlRBTree &)*this;
}

PyUtlRBTree::PyUtlRBTree( PyUtlRBTree const &tree )
{
	CopyFrom(tree);
}

void PyUtlRBTree::SetLessFunc( boost::python::object func )
{
	m_pyLessFunc = func;
}

//-----------------------------------------------------------------------------
// Insert a node into the tree
//-----------------------------------------------------------------------------
int PyUtlRBTree::InsertAt( int parent, bool leftchild )
{
	int i = NewNode();
	LinkToParent( i, parent, leftchild );
	++m_NumElements;

	Assert(IsValid());

	return i;
}

//-----------------------------------------------------------------------------
// inserts a node into the tree
//-----------------------------------------------------------------------------

// Inserts a node into the tree, doesn't copy the data in.
void PyUtlRBTree::FindInsertionPosition( boost::python::object const &insert, int &parent, bool &leftchild )
{
	/* find where node belongs */
	int current = m_Root;
	parent = InvalidIndex();
	leftchild = false;
	while (current != InvalidIndex()) 
	{
		parent = current;
		if (m_pyLessFunc( insert, Element(current) ))
		{
			leftchild = true; current = LeftChild(current);
		}
		else
		{
			leftchild = false; current = RightChild(current);
		}
	}
}

int PyUtlRBTree::Insert( boost::python::object insert )
{
	// use copy constructor to copy it in
	int parent;
	bool leftchild;
	FindInsertionPosition( insert, parent, leftchild );
	int newNode = InsertAt( parent, leftchild );
	m_Elements[newNode].m_Data = insert;
	//CopyConstruct( &Element( newNode ), insert );
	return newNode;
}

int PyUtlRBTree::InsertIfNotFound( boost::python::object insert )
{
	// use copy constructor to copy it in
	int parent;
	bool leftchild;

	int current = m_Root;
	parent = InvalidIndex();
	leftchild = false;
	while (current != InvalidIndex()) 
	{
		parent = current;
		if (m_pyLessFunc( insert, Element(current) ))
		{
			leftchild = true; current = LeftChild(current);
		}
		else if (m_pyLessFunc( Element(current), insert ))
		{
			leftchild = false; current = RightChild(current);
		}
		else
			// Match found, no insertion
			return InvalidIndex();
	}

	int newNode = InsertAt( parent, leftchild );
	m_Elements[newNode].m_Data = insert;
	//CopyConstruct( &Element( newNode ), insert );
	return newNode;
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: PyOutputEvent
//-----------------------------------------------------------------------------
PyOutputEvent::PyOutputEvent()
{
	// Set to NULL! Normally it depends on the the memory allocation function of CBaseEntity
	m_ActionList = NULL; 

	// Default
	m_Value.Set( FIELD_VOID, NULL );
}

void PyOutputEvent::Set( variant_t value )
{
	m_Value = value;
}

// void Firing, no parameter
void PyOutputEvent::FireOutput( CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay )
{
	CBaseEntityOutput::FireOutput(m_Value, pActivator, pCaller, fDelay);
}
#endif // CLIENT_DLL