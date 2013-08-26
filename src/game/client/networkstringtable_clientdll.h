//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef NETWORKSTRINGTABLE_CLIENTDLL_H
#define NETWORKSTRINGTABLE_CLIENTDLL_H
#ifdef _WIN32
#pragma once
#endif

#include "networkstringtabledefs.h"

extern INetworkStringTableContainer *networkstringtable;

// String tables used by the client DLL	
// (see InstallStringTableCallback for where they're initialized)
extern INetworkStringTable *g_StringTableVguiScreen;
extern INetworkStringTable *g_StringTableEffectDispatch;
extern INetworkStringTable *g_StringTableMaterials;
extern INetworkStringTable *g_pStringTableInfoPanel;
extern INetworkStringTable *g_pStringTableClientSideChoreoScenes;

#ifdef HL2WARS_DLL
extern INetworkStringTable *g_pStringTablePyModules;
extern INetworkStringTable *g_pStringTableGameDBNames;
#endif // HL2WARS_DLL

#endif // NETWORKSTRINGTABLE_CLIENTDLL_H
