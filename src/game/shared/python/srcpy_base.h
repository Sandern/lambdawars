//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_BASE_H
#define SRCPY_BASE_H
#ifdef _WIN32
#pragma once
#endif

#include <KeyValues.h>
#include "filesystem.h"
#include "string_t.h"
#include "cmodel.h"
#include <utlrbtree.h>
#include <utlflags.h>

#include "srcpy_boostpython.h"

#if 0
//-----------------------------------------------------------------------------
// Purpose: 
// TODO: Make this part of filesystem module?
//-----------------------------------------------------------------------------
extern  "C" {
	int SrcPyPathIsInGameFolder( const char *pPath );
}
#endif // 0

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

#endif // SRCPY_BASE_H