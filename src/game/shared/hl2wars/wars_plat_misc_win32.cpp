//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "wars_plat_misc.h"

#include <windows.h>

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

void SrcShellExecute( const char *pCommand )
{
	ShellExecute(NULL, "open", pCommand,
					NULL, NULL, SW_SHOWNORMAL);
}

#define TOTALBYTES    8192

bool SrcHasProtocolHandler( const char *pProtocol )
{
    DWORD BufferSize = TOTALBYTES;
    DWORD cbData;
    DWORD dwRet;

    PPERF_DATA_BLOCK PerfData = (PPERF_DATA_BLOCK) malloc( BufferSize );
    cbData = BufferSize;

    dwRet = RegQueryValueEx( HKEY_PERFORMANCE_DATA,
                             TEXT(pProtocol),
                             NULL,
                             NULL,
                             (LPBYTE) PerfData,
                             &cbData );

	return dwRet == ERROR_SUCCESS;
}