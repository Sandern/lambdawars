//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPYTHON_TESTS_H
#define SRCPYTHON_TESTS_H

#ifdef _WIN32
#pragma once
#endif

class CBaseEntity;

// Entity Converter related function tests
void SrcPyTest_EntityArg( CBaseEntity *pEntity );

// Temp test
void SrcPyTest_NCrossProducts( int n, Vector &a, Vector &b );

#endif // SRCPYTHON_TESTS_H