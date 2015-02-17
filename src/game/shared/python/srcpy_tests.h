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
bool SrcPyTest_EntityArg( CBaseEntity *pEntity );

bool SrcPyTest_ExtractEntityArg( boost::python::object entity );

#endif // SRCPYTHON_TESTS_H