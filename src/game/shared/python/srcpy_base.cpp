//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "srcpy_base.h"
#include "utlrbtree.h"
#include "srcpy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern const char *COM_GetModDirectory( void );

boost::python::dict PyKeyValuesToDict( const KeyValues *pKV )
{
	boost::python::dict d;

	KeyValues *pNonConstKV = (KeyValues *)pKV;

	for ( KeyValues *pKey = pNonConstKV->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		boost::python::object name(pKey->GetName());
		if( d.has_key( name ) == false )
			d[name] = boost::python::list();

		boost::python::list l = boost::python::extract< boost::python::list >(d[name]);
		l.append( PyKeyValuesToDict( pKey ) );
	}
	for ( KeyValues *pValue = pNonConstKV->GetFirstValue(); pValue; pValue = pValue->GetNextValue() )
	{
		boost::python::object name(pValue->GetName());
		if( d.has_key( name ) == false )
			d[name] = boost::python::list();

		boost::python::list l = boost::python::extract< boost::python::list >(d[name]);

		switch( pValue->GetDataType() )
		{
		case KeyValues::TYPE_STRING:
			l.append( pValue->GetString() );
			break;
		case KeyValues::TYPE_INT:
			l.append( pValue->GetInt() );
			break;
		case KeyValues::TYPE_FLOAT:
			l.append( pValue->GetFloat() );
			break;
		case KeyValues::TYPE_WSTRING:
			l.append( pValue->GetWString() );
			break;
		case KeyValues::TYPE_COLOR:
			l.append( pValue->GetColor() );
			break;
		case KeyValues::TYPE_UINT64:
			l.append( pValue->GetUint64() );
			break;
		default:
			l.append( pValue->GetString() );
			break;
		}
	}

	return d;
}

KeyValues *PyDictToKeyValues( boost::python::dict d )
{
	boost::python::object type = builtins.attr("type");
	boost::python::object dicttype = types.attr("DictType");

	KeyValues *pKV = new KeyValues("Data");

	boost::python::object items = d.attr("items")();
	boost::python::object iterator = items.attr("__iter__")();
	unsigned long length = boost::python::len(items); 

	for( unsigned long u = 0; u < length; u++ )
	{
		boost::python::object item = iterator.attr( PY_NEXT_METHODNAME )();

		boost::python::list elements = boost::python::list(item[1]);
		boost::python::ssize_t n = boost::python::len(elements);
		for( boost::python::ssize_t i=0; i < n; i++ ) 
		{
			 if( type(elements[i]) == dicttype )
			 {
				 pKV->AddSubKey( PyDictToKeyValues(boost::python::dict(elements[i])) );
			 }
			 else
			 {
				 pKV->SetString( boost::python::extract< const char * >( item[0] ), 
								 boost::python::extract< const char * >( elements[i] ) );
			 }
		}
	}

	return pKV;
}

#if 0
boost::python::dict PyKeyValues( boost::python::object name, boost::python::object firstKey, boost::python::object firstValue, boost::python::object secondKey, boost::python::object secondValue )
{
	boost::python::object type = builtins.attr("type");
	boost::python::object dicttype = types.attr("DictType");
	if( type(name) == dicttype )
	{
		return boost::python::dict(name);
	}

	boost::python::dict d;
	d[name] = boost::python::dict();
	if( firstKey != boost::python::object() )
	{
		boost::python::list l;
		l.append( firstValue );
		d[name][firstKey] = l;
	}

	if( secondKey != boost::python::object() )
	{
		if( firstKey == secondKey )
		{
			boost::python::list(d[name][firstKey]).append( secondValue );
		}
		else
		{
			boost::python::list l;
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
	return SrcPySystem()->IsPathProtected();
}

extern  "C" {

int SrcPyPathIsInGameFolder( const char *pPath )
{
	if( SrcPySystem()->IsPathProtected() )
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

		if( SrcPySystem()->IsPathProtected() )
		{
			if( Q_strnicmp(pFullPath, searchPaths, Q_strlen(searchPaths)) != 0 )
			{
				return 0;
			}
		}
	}
	else
	{
		if( SrcPySystem()->IsPathProtected() )
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
		else
		{
			filesystem->RelativePathToFullPath(pAssumedRelativePath, "MOD", pFullPath, size);
		}
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