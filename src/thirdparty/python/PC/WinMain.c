/* Minimal main program -- everything is loaded from the library. */

#include "Python.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef int (*SrcPyMain_t)( int argc, wchar_t **argv );

int WINAPI wWinMain(
    HINSTANCE hInstance,      /* handle to current instance */
    HINSTANCE hPrevInstance,  /* handle to previous instance */
    LPWSTR lpCmdLine,         /* pointer to command line */
    int nCmdShow              /* show state of window */
)
{
	TCHAR fullPath[MAX_PATH];
	TCHAR driveLetter[3];
	TCHAR directory[MAX_PATH];
	TCHAR FinalPath[MAX_PATH];
	GetModuleFileName(NULL, fullPath, MAX_PATH);
	_splitpath(fullPath, driveLetter, directory, NULL, NULL);

	// Must add 'bin' to the path....
	char* pPath = getenv("PATH");

	char szBuffer[MAX_PATH];
	char szFullPath[MAX_PATH];
	char szFullPath2[MAX_PATH];
	_fullpath( szFullPath, "..\\..\\bin\\", sizeof( szFullPath ) );
	_fullpath( szFullPath2, "..\\..\\", sizeof( szFullPath2 ) );

	_snprintf( szBuffer, sizeof( szBuffer ), "PATH=%s%s\\..\\..;%s%s\\..\\..\\bin;%s", driveLetter, directory, driveLetter, directory, pPath );
	szBuffer[sizeof( szBuffer ) - 1] = '\0';
	assert( len < sizeof( szBuffer ) );
	_putenv( szBuffer );

	HINSTANCE server = LoadLibraryEx( "server.dll", NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
	SrcPyMain_t main = (SrcPyMain_t)GetProcAddress( server, "SrcPy_Main" );
	return main(__argc, __wargv);
}
