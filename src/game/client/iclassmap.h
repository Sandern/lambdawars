//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ICLASSMAP_H
#define ICLASSMAP_H
#ifdef _WIN32
#pragma once
#endif

class C_BaseEntity;
typedef C_BaseEntity* (*DISPATCHFUNCTION)( void );

class PyEntityFactory;

abstract_class IClassMap
{
public:
	virtual					~IClassMap() {}

	virtual void			Add( const char *mapname, const char *classname, int size, DISPATCHFUNCTION factory = 0 ) = 0;
	virtual char const		*Lookup( const char *classname ) = 0;
	virtual C_BaseEntity	*CreateEntity( const char *mapname ) = 0;
	virtual int				GetClassSize( const char *classname ) = 0;

#ifndef DISABLE_PYTHON
	virtual void			PyAdd( const char *mapname, const char *classname, int size, PyEntityFactory *factory ) = 0;
	virtual void			PyRemove( const char *classname ) = 0;
	virtual PyEntityFactory* PyGetFactory( const char *classname ) = 0;
	virtual PyEntityFactory* PyGetFactoryByMapName( const char *classname ) = 0;
#endif // DISABLE_PYTHON
};

extern IClassMap& GetClassMap();


#endif // ICLASSMAP_H
