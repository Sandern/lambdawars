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

#if 0
//-----------------------------------------------------------------------------
// Purpose: Almost the same as V_FixupPathName, but does not lowercase
//			Linux/Python fileobject does not like that.
//-----------------------------------------------------------------------------
static void SrcPyFixupPathName( char *pOut )
{
	V_FixSlashes( pOut );
	V_RemoveDotSlashes( pOut );
	V_FixDoubleSlashes( pOut );
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
#endif // 0

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