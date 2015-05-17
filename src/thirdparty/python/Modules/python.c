/* Minimal main program -- everything is loaded from the library */

#include "Python.h"
#include <locale.h>

#ifdef __FreeBSD__
#include <floatingpoint.h>
#endif

#ifdef MS_WINDOWS

#include <Windows.h>

typedef int (*SrcPyMain_t)( int argc, wchar_t **argv );

int
wmain(int argc, wchar_t **argv)
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
	return main(argc, argv);
}
#else

int
main(int argc, char **argv)
{
    wchar_t **argv_copy;
    /* We need a second copy, as Python might modify the first one. */
    wchar_t **argv_copy2;
    int i, res;
    char *oldloc;
#ifdef __FreeBSD__
    fp_except_t m;
#endif

    argv_copy = (wchar_t **)PyMem_RawMalloc(sizeof(wchar_t*) * (argc+1));
    argv_copy2 = (wchar_t **)PyMem_RawMalloc(sizeof(wchar_t*) * (argc+1));
    if (!argv_copy || !argv_copy2) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    /* 754 requires that FP exceptions run in "no stop" mode by default,
     * and until C vendors implement C99's ways to control FP exceptions,
     * Python requires non-stop mode.  Alas, some platforms enable FP
     * exceptions by default.  Here we disable them.
     */
#ifdef __FreeBSD__
    m = fpgetmask();
    fpsetmask(m & ~FP_X_OFL);
#endif

    oldloc = _PyMem_RawStrdup(setlocale(LC_ALL, NULL));
    if (!oldloc) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    setlocale(LC_ALL, "");
    for (i = 0; i < argc; i++) {
        argv_copy[i] = _Py_char2wchar(argv[i], NULL);
        if (!argv_copy[i]) {
            PyMem_RawFree(oldloc);
            fprintf(stderr, "Fatal Python error: "
                            "unable to decode the command line argument #%i\n",
                            i + 1);
            return 1;
        }
        argv_copy2[i] = argv_copy[i];
    }
    argv_copy2[argc] = argv_copy[argc] = NULL;

    setlocale(LC_ALL, oldloc);
    PyMem_RawFree(oldloc);
    res = Py_Main(argc, argv_copy);
    for (i = 0; i < argc; i++) {
        PyMem_RawFree(argv_copy2[i]);
    }
    PyMem_RawFree(argv_copy);
    PyMem_RawFree(argv_copy2);
    return res;
}
#endif
