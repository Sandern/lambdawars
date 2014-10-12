//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Python usermessages
//
//=============================================================================//

#ifndef SRCPY_USERMESSAGE_H
#define SRCPY_USERMESSAGE_H
#ifdef _WIN32
#pragma once
#endif

#ifndef CLIENT_DLL
	typedef struct pywrite {
		pywrite() {}
		pywrite(const pywrite &w) 
		{
			memcpy(this, &w, sizeof(pywrite)-sizeof(CUtlVector< struct pywrite >));
			writelist = w.writelist;
		}

		char			type;
		union
		{
			int				writeint;
			float			writefloat;
			const char *	writestr;
			bool			writebool;
			float			writevector[3];
			byte			writecolor[4];
			uint64			writeuint64;
		};

		EHANDLE			writehandle;
		CUtlVector< struct pywrite > writelist;
	} pywrite;

	void PyFillWriteElement( pywrite &w, boost::python::object data );
	void PyWriteElement( pywrite &w );
	void PyPrintElement( pywrite &w, int indent = 0 );
	void PySendUserMessage( IRecipientFilter& filter, const char *messagename, boost::python::list msg );
#else
	boost::python::object PyReadElement( bf_read &msg );
	void HookPyMessage();
#endif // CLIENT_DLL

#endif // SRCPY_USERMESSAGE_H