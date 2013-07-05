//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRC_PYTHON_ENTITES_H
#define SRC_PYTHON_ENTITES_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"
#include "ehandle.h"
#ifndef CLIENT_DLL
	#include "physics_bone_follower.h"
	#include "src_python_physics.h"

	class CBaseAnimating;
	class CRagdollProp;
#endif // CLIENT_DLL

#include <boost/python.hpp>

namespace bp = boost::python;


//-----------------------------------------------------------------------------
// Purpose: Python entity handle
//-----------------------------------------------------------------------------
template< class T >
class CEPyHandle : public CHandle<T>
{
public:
	CEPyHandle() : CHandle<T>() {}
	CEPyHandle( T *pVal ) : CHandle<T>(pVal) {}
	CEPyHandle( int iEntry, int iSerialNumber ) : CHandle<T>(iEntry, iSerialNumber) {}

	bp::object GetAttr( const char *name );

	int Cmp( bp::object other );
	bool NonZero();
};

template< class T >
inline bp::object CEPyHandle<T>::GetAttr( const char *name )
{
	return bp::object(bp::ptr(Get())).attr(name);
}

// ? Is using memcmp on a PyObject correct ?
template< class T >
int CEPyHandle<T>::Cmp( bp::object other )
{
	// The thing to which we compare is NULL
	PyObject *pPyObject = other.ptr();
	if( pPyObject == Py_None ) {
		return Get() != NULL;
	}

	// We are NULL
	if( Get() == NULL )
	{
		return pPyObject != NULL;
	}

	// Check if it is directly a pointer to an entity
#ifdef CLIENT_DLL
	if( PyObject_IsInstance(pPyObject, bp::object(_entities.attr("C_BaseEntity")).ptr()) )
#else
	if( PyObject_IsInstance(pPyObject, bp::object(_entities.attr("CBaseEntity")).ptr()) )
#endif // CLIENT_DLL
	{
		CBaseEntity *pSelf = Get();
		CBaseEntity *pOther = bp::extract<CBaseEntity *>(other);
		if( pOther == pSelf )
		{
			return 0;
		}
		else if( pOther->entindex() > pSelf->entindex() )
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}

	// Must be a handle
	CBaseHandle *pHandle = bp::extract<CBaseHandle *>( other );
	if( pHandle )
	{
		if( pHandle->ToInt() == ToInt() )
			return 0;
		else if( pHandle->GetEntryIndex() > GetEntryIndex() )
			return 1;
		else
			return -1;
	}

	return -1;
}

template< class T >
inline bool CEPyHandle<T>::NonZero()
{
	return Get() != NULL;
}

//----------------------------------------------------------------------------
// Purpose: Python entity handle 2, python entities only
//			This is a bit annoying, since CHandle template assumes T is a pointer 
//			to the entity, so we can't fill in "bp::object"
//-----------------------------------------------------------------------------
class PyHandle : public CBaseHandle
{
public:
	PyHandle(bp::object ent);
	PyHandle( int iEntry, int iSerialNumber ) : CBaseHandle(iEntry, iSerialNumber) {}

public:
	CBaseEntity *Get() const;
	bp::object PyGet() const;
	void Set( bp::object ent );

	bool	operator==( bp::object val ) const;
	bool	operator!=( bp::object val ) const;
	bool	operator==( const PyHandle &val ) const;
	bool	operator!=( const PyHandle &val ) const;
	const PyHandle& operator=( bp::object val );

	bp::object GetAttr( const char *name );
	bp::object GetAttribute( const char *name );
	void SetAttr( const char *name, bp::object v );

	int Cmp( bp::object other );
	bool NonZero() { return PyGet().ptr() != Py_None; }

	virtual PyObject *GetPySelf() { return NULL; }

	boost::python::object Str();
};

inline int PyHandle::Cmp( bp::object other )
{
	// The thing to which we compare is NULL
	PyObject *pPyObject = other.ptr();
	if( pPyObject == Py_None ) {
		return Get() != NULL;
	}

	// We are NULL
	if( Get() == NULL )
	{
		return pPyObject != NULL;
	}

	// Check if it is directly a pointer to an entity
#ifdef CLIENT_DLL
	if( PyObject_IsInstance(pPyObject, bp::object(_entities.attr("C_BaseEntity")).ptr()) )
#else
	if( PyObject_IsInstance(pPyObject, bp::object(_entities.attr("CBaseEntity")).ptr()) )
#endif // CLIENT_DLL
	{
		CBaseEntity *pSelf = Get();
#ifdef PYPP_GENERATION // FIXME: Generation compiler doesn't likes this...
		CBaseEntity *pOther = NULL;
#else
		CBaseEntity *pOther = boost::python::extract< CBaseEntity * >(other);
#endif // PYPP_GENERATION
		if( pOther == pSelf )
		{
			return 0;
		}
		else if( pOther->entindex() > pSelf->entindex() )
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}

	// Must be a handle
#ifdef PYPP_GENERATION // FIXME: Generation compiler doesn't likes this...
	CBaseHandle *pHandle = NULL;
#else
	CBaseHandle *pHandle = bp::extract< CBaseHandle * >( other );
#endif // PYPP_GENERATION
	if( pHandle )
	{
		if( pHandle->ToInt() == ToInt() )
			return 0;
		else if( pHandle->GetEntryIndex() > GetEntryIndex() )
			return 1;
		else
			return -1;
	}

	return -1;

	return 0;
}

bp::object CreatePyHandle( int iEntry, int iSerialNumber );

#ifndef CLIENT_DLL
bp::object PyGetWorldEntity();
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Dead python entity. The __class__ object of a removed entity gets 
//			rebinded to this. This way you can't accidently access (potential)
//			dangerous methods.
//-----------------------------------------------------------------------------
class DeadEntity 
{
public:
	bool NonZero() { return false; }
};

#ifdef CLIENT_DLL
// ----------- Linking python classes to entities
class PyEntityFactory 
{
public:
	PyEntityFactory( const char *pClassName, bp::object PyClass );
	~PyEntityFactory();

	C_BaseEntity *Create();

	bp::object GetClass() { return m_PyClass; }

private:
	char m_ClassName[128];
	bp::object m_PyClass;
};

#else
// ----------- Linking python classes to entities
class PyEntityFactory : public IEntityFactory
{
public:
	PyEntityFactory( const char *pClassName, bp::object PyClass );
	~PyEntityFactory();

	IServerNetworkable *Create( const char *pClassName );

	void Destroy( IServerNetworkable *pNetworkable );

	virtual size_t GetEntitySize();

	void InitPyClass();
	virtual bool IsPyFactory() { return true; }

	bp::object GetClass() { return m_PyClass; }
	const char *GetClassname() { return m_ClassName; }

private:
	void CheckEntities();

public:
	PyEntityFactory *m_pPyNext;

private:
	char m_ClassName[128];
	bp::object m_PyClass;
};
void InitAllPythonEntities();
#endif

bp::object PyGetClassByClassname( const char *class_name );
bp::list PyGetAllClassnames();

// ----------- Sending events to client
#ifndef CLIENT_DLL
void PySendEvent( IRecipientFilter &filter, EHANDLE ent, int event, int data);
#endif // CLIENT_DLL


#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Spawn a player
//-----------------------------------------------------------------------------
bp::object PyRespawnPlayer( CBasePlayer *pPlayer, const char *classname );
#endif // CLIENT_DLL


#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Bone follow stuff (mainly for the strider)
//-----------------------------------------------------------------------------
Vector GetAttachmentPositionInSpaceOfBone( CStudioHdr *pStudioHdr, const char *pAttachmentName, int outputBoneIndex );

typedef struct pyphysfollower_t
{
	int boneindex;
	bp::object follower;
} pyphysfollower_t;

class PyBoneFollowerManager : public CBoneFollowerManager
{
public:
	// Use either of these to create the bone followers in your entity's CreateVPhysics()
	void InitBoneFollowers( CBaseAnimating *pParentEntity, bp::list followerbonenames );
	void AddBoneFollower( CBaseAnimating *pParentEntity, const char *pFollowerBoneName, solid_t *pSolid = NULL );	// Adds a single bone follower

	// Call this after you move your bones
	void UpdateBoneFollowers( CBaseAnimating *pParentEntity );

	// Call this when your entity's removed
	void DestroyBoneFollowers( void );

	pyphysfollower_t GetBoneFollower( int iFollowerIndex );
	int				GetBoneFollowerIndex( CBoneFollower *pFollower );
	int				GetNumBoneFollowers( void ) const { return CBoneFollowerManager::GetNumBoneFollowers(); }
};

Vector GetAttachmentPositionInSpaceOfBone( CStudioHdr *pStudioHdr, const char *pAttachmentName, int outputBoneIndex );

CRagdollProp *PyCreateServerRagdollAttached( CBaseAnimating *pAnimating, const Vector &vecForce, int forceBone, int collisionGroup, PyPhysicsObject &pyAttached, CBaseAnimating *pParentEntity, int boneAttach, const Vector &originAttached, int parentBoneAttach, const Vector &boneOrigin );

#endif // CLIENT_DLL

#endif // SRC_PYTHON_ENTITES_H