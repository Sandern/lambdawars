//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#ifndef SRCPY_SAVERESTORE_H
#define SRCPY_SAVERESTORE_H

#ifdef _WIN32
#pragma once
#endif

ISaveRestoreBlockHandler *GetPythonSaveRestoreBlockHandler();

#endif // SRCPY_SAVERESTORE_H