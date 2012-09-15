//#include <dbg.h>
#include "srcwrap.h"
#include <filesystem.h>
#include "Python.h"

#include "osdefs.h"

typedef char tchar;
#ifdef MS_WINDOWS
extern "C" __declspec( dllimport ) void Msg( const tchar* pMsg, ... );
extern "C" __declspec( dllimport ) void Warning( const tchar* pMsg, ... );
extern "C" __declspec( dllimport ) void DevMsg( int level, const tchar* pMsg, ... );
#else
extern "C" void Msg( const tchar* pMsg, ... );
extern "C" void Warning( const tchar* pMsg, ... );
extern "C" void DevMsg( int level, const tchar* pMsg, ... );
#endif

#if 0
template<byte count>
struct SVaPassNext{
	SVaPassNext<count-1> big;
	//DWORD dw;
	unsigned long dw;
};
template<> struct SVaPassNext<0>{};
//SVaPassNext - is generator of structure of any size at compile time.


class CVaPassNext{
public:
	SVaPassNext<50> svapassnext;
	CVaPassNext(va_list & args){
		try{//to avoid access violation

			memcpy(&svapassnext, args, sizeof(svapassnext));
		} catch (...) {}
	}
};
#define va_pass(valist) CVaPassNext(valist).svapassnext


extern "C" void SrcMsg( const char *pMsg, ... )
{
	va_list args;
	va_start(args, pMsg);
	Msg(pMsg, va_pass(args));
	va_end(args);
}

extern "C" void SrcWarning( const char *pMsg, ... )
{
	va_list args;
	va_start(args, pMsg);
	Warning(pMsg, va_pass(args));
	va_end(args);
}
#endif // 0

extern "C" void SrcMsg( const char *pMsg, ... )
{
	va_list argptr;
	va_start(argptr,pMsg);
	Msg(pMsg, argptr);
	va_end(argptr);
}

extern "C" void SrcWarning( const char *pMsg, ... )
{
	va_list argptr;
	va_start(argptr,pMsg);
	Warning(pMsg, argptr);
	va_end(argptr);
}

extern "C" void SrcDevMsg( int level, const char *pMsg, ... )
{
	va_list argptr;
	va_start(argptr,pMsg);
	DevMsg(level, pMsg, argptr);
	va_end(argptr);
}

// Filesystem
extern "C" int SrcPyGetFullPath( const char *pAssumedRelativePath, char *pFullPath, int size );
#undef GetCurrentDirectory

int SrcFileSystem_GetCurrentDirectory( char* pDirectory, int maxlen )
{
	return g_pFullFileSystem->GetCurrentDirectory(pDirectory, maxlen);
}

void SrcFileSystem_CreateDirHierarchy( const char *path, const char *pathID = 0 )
{
	g_pFullFileSystem->CreateDirHierarchy(path, pathID);
}

void SrcFileSystem_RemoveFile( char const* pRelativePath, const char *pathID = 0 )
{
	g_pFullFileSystem->RemoveFile(pRelativePath, pathID);
}

extern "C" PyObject *SrcFileSystem_ListDir(PyObject *self, PyObject *args)
{
#if defined(MS_WINDOWS) 
	PyObject *d, *v;
	FileFindHandle_t hFindFile;
	char namebuf[MAX_PATH+5]; /* Overallocate for \\*.*\0 */
	char *bufptr = namebuf;
	Py_ssize_t len = sizeof(namebuf)-5; /* only claim to have space for MAX_PATH */
	char fullPath[_MAX_PATH];

	if (!PyArg_ParseTuple(args, "et#:listdir",
		Py_FileSystemDefaultEncoding, &bufptr, &len))
		return NULL;

	if (len > 0) {
		char ch = namebuf[len-1];
		if (ch != SEP && ch != ALTSEP && ch != ':')
			namebuf[len++] = '/';
	}
	strcpy(namebuf + len, "*.*");

	// Verify if the directory is relative to our mod folder
	if( SrcPyGetFullPath(namebuf, fullPath, _MAX_PATH) == 0)
		return NULL;

	if ((d = PyList_New(0)) == NULL)
		return NULL;

	// Loop files
	char const *fn = g_pFullFileSystem->FindFirstEx( namebuf, "GAME", &hFindFile );
	if( fn == NULL )
	{
		// TODO: Find out the type of error. 
		// If no file is found we should return the list, which we will do here
		return d;
	}
	do
	{
		/* Skip over . and .. */
		if (strcmp(fn, ".") != 0 &&
			strcmp(fn, "..") != 0) {
				v = PyString_FromString(fn);
				if (v == NULL) {
					Py_DECREF(d);
					d = NULL;
					break;
				}
				if (PyList_Append(d, v) != 0) {
					Py_DECREF(v);
					Py_DECREF(d);
					d = NULL;
					break;
				}
				Py_DECREF(v);
		}
		Py_BEGIN_ALLOW_THREADS
		fn = g_pFullFileSystem->FindNext( hFindFile );
		Py_END_ALLOW_THREADS
	} while( fn );
	g_pFullFileSystem->FindClose( hFindFile );
	return d;

#else
	PyObject *d, *v;
	FileFindHandle_t hFindFile;
	char namebuf[MAX_PATH+5]; /* Overallocate for \\*.*\0 */
	char *bufptr = namebuf;
	Py_ssize_t len = sizeof(namebuf)-5; /* only claim to have space for MAX_PATH */
	char fullPath[_MAX_PATH];

	if (!PyArg_ParseTuple(args, "et#:listdir",
		Py_FileSystemDefaultEncoding, &bufptr, &len))
		return NULL;

	if (len > 0) {
		char ch = namebuf[len-1];
		if (ch != SEP && ch != ':')
			namebuf[len++] = '/';
	}
	strcpy(namebuf + len, "*.*");

	// Verify if the directory is relative to our mod folder
	if( SrcPyGetFullPath(namebuf, fullPath, _MAX_PATH) == 0)
		return NULL;

	if ((d = PyList_New(0)) == NULL)
		return NULL;

	// Loop files
	char const *fn = g_pFullFileSystem->FindFirstEx( namebuf, "GAME", &hFindFile );
	if( fn == NULL )
	{
		// TODO: Find out the type of error. 
		// If no file is found we should return the list, which we will do here
		return d;
	}
	do
	{
		/* Skip over . and .. */
		if (strcmp(fn, ".") != 0 &&
			strcmp(fn, "..") != 0) {
				v = PyString_FromString(fn);
				if (v == NULL) {
					Py_DECREF(d);
					d = NULL;
					break;
				}
				if (PyList_Append(d, v) != 0) {
					Py_DECREF(v);
					Py_DECREF(d);
					d = NULL;
					break;
				}
				Py_DECREF(v);
		}
		Py_BEGIN_ALLOW_THREADS
		fn = g_pFullFileSystem->FindNext( hFindFile );
		Py_END_ALLOW_THREADS
	} while( fn );
	g_pFullFileSystem->FindClose( hFindFile );
	return d;
#endif // LINUX
}