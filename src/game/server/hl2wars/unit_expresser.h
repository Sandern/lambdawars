//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef UNIT_EXPRESSER_H
#define UNIT_EXPRESSER_H

#ifdef _WIN32
#pragma once
#endif

#include "unit_component.h"

class UnitExpresser : public UnitComponent, public CAI_Expresser
{
public:
#ifdef ENABLE_PYTHON
	UnitExpresser( boost::python::object outer );
#endif // ENABLE_PYTHON
};

#endif // UNIT_EXPRESSER_H