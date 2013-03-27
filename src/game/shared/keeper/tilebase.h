//===== Copyright © 2013, Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#ifndef TILEBASE_H
#define TILEBASE_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	#include "c_baseanimating.h"
	#define CBaseAnimating C_BaseAnimating
#else
	#include "baseanimating.h"
#endif // CLIENT_DLL

class CTileBase : public CBaseAnimating
{
public:
	DECLARE_CLASS( CTileBase, CBaseAnimating );

	CTileBase( void );

	virtual void Spawn( void );

	void UpdateModel( void );
};

#endif // TILEBASE_H