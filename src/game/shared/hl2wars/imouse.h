//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: If an entity implements this interface, it can receive mouse clicks
//
// $NoKeywords: $
//=============================================================================//

#ifndef IMOUSE_H
#define IMOUSE_H

#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	#define CHL2WarsPlayer C_HL2WarsPlayer
#endif // CLIENT_DLL

class CHL2WarsPlayer;
class CBaseEntity;

abstract_class IMouse 
{
public:
	virtual IMouse*					GetIMouse()								{ return this; }

	// Shared
	virtual void OnClickLeftPressed( CHL2WarsPlayer *player )				= 0;
	virtual void OnClickRightPressed( CHL2WarsPlayer *player )				= 0;
	virtual void OnClickLeftReleased( CHL2WarsPlayer *player )				= 0;
	virtual void OnClickRightReleased( CHL2WarsPlayer *player )				= 0;

	virtual void OnClickLeftDoublePressed( CHL2WarsPlayer *player )			= 0;
	virtual void OnClickRightDoublePressed( CHL2WarsPlayer *player )		= 0;

	virtual void OnCursorEntered( CHL2WarsPlayer *player )					= 0;
	virtual void OnCursorExited( CHL2WarsPlayer *player )					= 0;

	// CLIENT
#ifdef CLIENT_DLL
	virtual void OnHoverPaint()												= 0;
	virtual unsigned long GetCursor()										= 0;
#endif // CLIENT_DLL
};

// Present this to python as IMouse. Don't want stupid runtime errors.
class PyMouse : public IMouse
{
public:
	// Shared
	virtual void OnClickLeftPressed( CHL2WarsPlayer *player )				{}
	virtual void OnClickRightPressed( CHL2WarsPlayer *player )				{}
	virtual void OnClickLeftReleased( CHL2WarsPlayer *player )				{}
	virtual void OnClickRightReleased( CHL2WarsPlayer *player )				{}

	virtual void OnClickLeftDoublePressed( CHL2WarsPlayer *player )			{}
	virtual void OnClickRightDoublePressed( CHL2WarsPlayer *player )		{}

	virtual void OnCursorEntered( CHL2WarsPlayer *player )					{}
	virtual void OnCursorExited( CHL2WarsPlayer *player )					{}

	// CLIENT
#ifdef CLIENT_DLL
	virtual void OnHoverPaint()												{}
	virtual unsigned long GetCursor()										{ return 2; }
#endif // CLIENT_DLL
};

#endif // IMOUSE_H