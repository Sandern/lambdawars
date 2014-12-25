//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WARS_NETWORK_H
#define WARS_NETWORK_H

#ifdef _WIN32
#pragma once
#endif

void WarsNet_StartEntityUpdate( const CSteamID &steamIDRemote, EHANDLE ent );
void WarsNet_EndEntityUpdate();

#endif // WARS_NETWORK_H