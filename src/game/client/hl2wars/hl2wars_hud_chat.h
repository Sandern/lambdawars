//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HUD_HL2WARS_CHAT_H
#define HUD_HL2WARS_CHAT_H
#ifdef _WIN32
#pragma once
#endif

#include <hud_basechat.h>

class CHudChat : public CBaseHudChat
{
	DECLARE_CLASS_SIMPLE( CHudChat, CBaseHudChat );

public:
	CHudChat( const char *pElementName );

	virtual void	Init( void );

	void			MsgFunc_SayText(bf_read &msg);
	void			MsgFunc_SayText2( bf_read &msg );
	//void			MsgFunc_TextMsg(bf_read &msg);

	virtual Color	GetClientColor( int clientIndex );
	virtual Color	GetDefaultTextColor( void );
};

#endif	//HUD_HL2WARS_CHAT_H