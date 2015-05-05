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
	PySaveHelper( CSave *save, const char *pFieldName ) : m_pSave( save ), m_pFieldName( pFieldName ) {}

	void WriteShort( short value );
	void WriteInteger( int value );
	void WriteFloat( float value );
	void WriteBoolean( bool value );
	void WriteString( const char *value );
	void WriteVector( Vector &value );
	void WriteEHandle( CBaseHandle &h );
	void WriteFields( boost::python::object instance );

#ifndef CLIENT_DLL
	void WriteOutputEvent( PyOutputEvent &ev );
#endif // CLIENT_DLL

private:
	CSave *m_pSave;
	const char *m_pFieldName;
};

class PyRestoreHelper
{
public:
	PyRestoreHelper( CRestore *restore ) : m_pRestore( restore ) {}

	void SetActiveField( SaveRestoreRecordHeader_t *pActiveField ) { m_pActiveHeader = pActiveField; }

	boost::python::object ReadShort();
	boost::python::object ReadInteger();
	boost::python::object ReadFloat();
	boost::python::object ReadBoolean();
	boost::python::object ReadString();
	boost::python::object ReadVector();
	boost::python::object ReadEHandle();
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