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

class CSave;
class CRestore;
class PyOutputEvent;

//-----------------------------------------------------------------------------
// Helper interface exposed to fields in Python
//-----------------------------------------------------------------------------
class PySaveHelper
{
public:
	PySaveHelper( CSave *save ) : m_pSave( save ) {}

	void WriteString( const char *fieldname, const char *fieldvalue );
	void WriteFields( const char *fieldname, boost::python::object instance );

#ifndef CLIENT_DLL
	void WriteOutputEvent( const char *fieldname, PyOutputEvent &ev );
#endif // CLIENT_DLL

private:
	CSave *m_pSave;
};

class PyRestoreHelper
{
public:
	PyRestoreHelper( CRestore *restore ) : m_pRestore( restore ) {}

	void SetActiveField( SaveRestoreRecordHeader_t *pActiveField ) { m_pActiveHeader = pActiveField; }

	boost::python::object ReadString();
	void ReadFields( boost::python::object instance );

#ifndef CLIENT_DLL
	void ReadOutputEvent( PyOutputEvent &ev );
#endif // CLIENT_DLL

private:
	CRestore *m_pRestore;
	SaveRestoreRecordHeader_t *m_pActiveHeader;
};

//-----------------------------------------------------------------------------
// Block handler for data not directly related to entities in Python
//-----------------------------------------------------------------------------
ISaveRestoreBlockHandler *GetPythonSaveRestoreBlockHandler();

#endif // SRCPY_SAVERESTORE_H