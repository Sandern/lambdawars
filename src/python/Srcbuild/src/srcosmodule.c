
/* POSIX module implementation */

/* This file is also used for Windows NT/MS-Win and OS/2.  In that case the
   module actually calls itself 'nt' or 'os2', not 'posix', and a few
   functions are either unimplemented or implemented differently.  The source
   assumes that for Windows NT, the macro 'MS_WINDOWS' is defined independent
   of the compiler used.  Different compilers define their own feature
   test macro, e.g. '__BORLANDC__' or '_MSC_VER'.  For OS/2, the compiler
   independent macro PYOS_OS2 should be defined.  On OS/2 the default
   compiler is assumed to be IBM's VisualAge C++ (VACPP).  PYCC_GCC is used
   as the compiler specific macro for the EMX port of gcc to OS/2. */

/* See also ../Dos/dosmodule.c */

// Source engine filesystem
//#include <filesystem.h>
#include "../Srcbuild/src/srcwrap.h"

// Convert relative path to full path + check things
// Right now it is defined in the game dll's, might want to do this different
extern int SrcPyGetFullPath( const char *pAssumedRelativePath, char *pFullPath, int size );
extern int SrcPyGetFullPathSilent( const char *pAssumedRelativePath, char *pFullPath, int size );
extern int SrcPyPathIsInGameFolder( const char *pPath );

#ifndef WIN32
#ifndef _MAX_PATH
#define _MAX_PATH 255
#endif
#ifndef MAX_PATH
#define MAX_PATH _MAX_PATH
#endif
#endif // WIN32

#ifdef __APPLE__
   /*
    * Step 1 of support for weak-linking a number of symbols existing on 
    * OSX 10.4 and later, see the comment in the #ifdef __APPLE__ block
    * at the end of this file for more information.
    */
#  pragma weak lchown
#  pragma weak statvfs
#  pragma weak fstatvfs

#endif /* __APPLE__ */

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "structseq.h"

#if defined(__VMS)
#    include <unixio.h>
#endif /* defined(__VMS) */

PyDoc_STRVAR(srcos__doc__,
"This module provides access to operating system functionality that is\n\
standardized by the C Standard and the POSIX standard (a thinly\n\
disguised Unix interface).  Refer to the library manual and\n\
corresponding Unix manual entries for more information on calls.");

#ifndef Py_USING_UNICODE
/* This is used in signatures of functions. */
#define Py_UNICODE void
#endif

#if defined(PYOS_OS2)
#define  INCL_DOS
#define  INCL_DOSERRORS
#define  INCL_DOSPROCESS
#define  INCL_NOPMAPI
#include <os2.h>
#if defined(PYCC_GCC)
#include <ctype.h>
#include <io.h>
#include <stdio.h>
#include <process.h>
#endif
#include "osdefs.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>           /* For WNOHANG */
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#ifdef HAVE_GRP_H
#include <grp.h>
#endif

#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#endif /* HAVE_SYSEXITS_H */

#ifdef HAVE_SYS_LOADAVG_H
#include <sys/loadavg.h>
#endif

/* Various compilers have only certain posix functions */
/* XXX Gosh I wish these were all moved into pyconfig.h */
#if defined(PYCC_VACPP) && defined(PYOS_OS2)
#include <process.h>
#else
#if defined(__WATCOMC__) && !defined(__QNX__)           /* Watcom compiler */
#define HAVE_GETCWD     1
#define HAVE_OPENDIR    1
#define HAVE_SYSTEM     1
#if defined(__OS2__)
#define HAVE_EXECV      1
#define HAVE_WAIT       1
#endif
#include <process.h>
#else
#ifdef __BORLANDC__             /* Borland compiler */
#define HAVE_EXECV      1
#define HAVE_GETCWD     1
#define HAVE_OPENDIR    1
#define HAVE_PIPE       1
#define HAVE_POPEN      1
#define HAVE_SYSTEM     1
#define HAVE_WAIT       1
#else
#ifdef _MSC_VER         /* Microsoft compiler */
#define HAVE_GETCWD     1
#define HAVE_SPAWNV     1
#define HAVE_EXECV      1
#define HAVE_PIPE       1
#define HAVE_POPEN      1
#define HAVE_SYSTEM     1
#define HAVE_CWAIT      1
#define HAVE_FSYNC      1
#define fsync _commit
#else
#if defined(PYOS_OS2) && defined(PYCC_GCC) || defined(__VMS)
/* Everything needed is defined in PC/os2emx/pyconfig.h or vms/pyconfig.h */
#else                   /* all other compilers */
/* Unix functions that the configure script doesn't check for */
#define HAVE_EXECV      1
#define HAVE_FORK       1
#if defined(__USLC__) && defined(__SCO_VERSION__)       /* SCO UDK Compiler */
#define HAVE_FORK1      1
#endif
#define HAVE_GETCWD     1
#define HAVE_GETEGID    1
#define HAVE_GETEUID    1
#define HAVE_GETGID     1
#define HAVE_GETPPID    1
#define HAVE_GETUID     1
#define HAVE_KILL       1
#define HAVE_OPENDIR    1
#define HAVE_PIPE       1
#ifndef __rtems__
#define HAVE_POPEN      1
#endif
#define HAVE_SYSTEM     1
#define HAVE_WAIT       1
#define HAVE_TTYNAME    1
#endif  /* PYOS_OS2 && PYCC_GCC && __VMS */
#endif  /* _MSC_VER */
#endif  /* __BORLANDC__ */
#endif  /* ! __WATCOMC__ || __QNX__ */
#endif /* ! __IBMC__ */

#ifndef _MSC_VER

#if defined(__sgi)&&_COMPILER_VERSION>=700
/* declare ctermid_r if compiling with MIPSPro 7.x in ANSI C mode
   (default) */
extern char        *ctermid_r(char *);
#endif

#ifndef HAVE_UNISTD_H
#if defined(PYCC_VACPP)
extern int mkdir(char *);
#else
#if ( defined(__WATCOMC__) || defined(_MSC_VER) ) && !defined(__QNX__)
extern int mkdir(const char *);
#else
extern int mkdir(const char *, mode_t);
#endif
#endif
#if defined(__IBMC__) || defined(__IBMCPP__)
extern int chdir(char *);
extern int rmdir(char *);
#else
extern int chdir(const char *);
extern int rmdir(const char *);
#endif
#ifdef __BORLANDC__
extern int chmod(const char *, int);
#else
extern int chmod(const char *, mode_t);
#endif
/*#ifdef HAVE_FCHMOD
extern int fchmod(int, mode_t);
#endif*/
/*#ifdef HAVE_LCHMOD
extern int lchmod(const char *, mode_t);
#endif*/
extern int chown(const char *, uid_t, gid_t);
extern char *getcwd(char *, int);
extern char *strerror(int);
extern int link(const char *, const char *);
extern int rename(const char *, const char *);
extern int stat(const char *, struct stat *);
extern int unlink(const char *);
extern int pclose(FILE *);
#ifdef HAVE_SYMLINK
extern int symlink(const char *, const char *);
#endif /* HAVE_SYMLINK */
#ifdef HAVE_LSTAT
extern int lstat(const char *, struct stat *);
#endif /* HAVE_LSTAT */
#endif /* !HAVE_UNISTD_H */

#endif /* !_MSC_VER */

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif /* HAVE_UTIME_H */

#ifdef HAVE_SYS_UTIME_H
#include <sys/utime.h>
#define HAVE_UTIME_H /* pretend we do for the rest of this file */
#endif /* HAVE_SYS_UTIME_H */

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif /* HAVE_SYS_TIMES_H */

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif /* HAVE_SYS_UTSNAME_H */

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#if defined(__WATCOMC__) && !defined(__QNX__)
#include <direct.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#endif
#ifdef HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#ifdef HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

#ifdef _MSC_VER
#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_PROCESS_H
#include <process.h>
#endif
#include "osdefs.h"
#include <malloc.h>
#include <windows.h>
#include <shellapi.h>   /* for ShellExecute() */
#define popen   _popen
#define pclose  _pclose
#endif /* _MSC_VER */

#if defined(PYCC_VACPP) && defined(PYOS_OS2)
#include <io.h>
#endif /* OS2 */

#ifndef MAXPATHLEN
#if defined(PATH_MAX) && PATH_MAX > 1024
#define MAXPATHLEN PATH_MAX
#else
#define MAXPATHLEN 1024
#endif
#endif /* MAXPATHLEN */

#ifdef UNION_WAIT
/* Emulate some macros on systems that have a union instead of macros */

#ifndef WIFEXITED
#define WIFEXITED(u_wait) (!(u_wait).w_termsig && !(u_wait).w_coredump)
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(u_wait) (WIFEXITED(u_wait)?((u_wait).w_retcode):-1)
#endif

#ifndef WTERMSIG
#define WTERMSIG(u_wait) ((u_wait).w_termsig)
#endif

#define WAIT_TYPE union wait
#define WAIT_STATUS_INT(s) (s.w_status)

#else /* !UNION_WAIT */
#define WAIT_TYPE int
#define WAIT_STATUS_INT(s) (s)
#endif /* UNION_WAIT */

/* Issue #1983: pid_t can be longer than a C long on some systems */
#if !defined(SIZEOF_PID_T) || SIZEOF_PID_T == SIZEOF_INT
#define PARSE_PID "i"
#define PyLong_FromPid PyInt_FromLong
#define PyLong_AsPid PyInt_AsLong
#elif SIZEOF_PID_T == SIZEOF_LONG
#define PARSE_PID "l"
#define PyLong_FromPid PyInt_FromLong
#define PyLong_AsPid PyInt_AsLong
#elif defined(SIZEOF_LONG_LONG) && SIZEOF_PID_T == SIZEOF_LONG_LONG
#define PARSE_PID "L"
#define PyLong_FromPid PyLong_FromLongLong
#define PyLong_AsPid PyInt_AsLongLong
#else
#error "sizeof(pid_t) is neither sizeof(int), sizeof(long) or sizeof(long long)"
#endif /* SIZEOF_PID_T */

/* Don't use the "_r" form if we don't need it (also, won't have a
   prototype for it, at least on Solaris -- maybe others as well?). */
#if defined(HAVE_CTERMID_R) && defined(WITH_THREAD)
#define USE_CTERMID_R
#endif

#if defined(HAVE_TMPNAM_R) && defined(WITH_THREAD)
#define USE_TMPNAM_R
#endif

/* choose the appropriate stat and fstat functions and return structs */
#undef STAT
#if defined(MS_WIN64) || defined(MS_WINDOWS)
#       define STAT win32_stat
#       define FSTAT win32_fstat
#       define STRUCT_STAT struct win32_stat
#else
#       define STAT stat
#       define FSTAT fstat
#       define STRUCT_STAT struct stat
#endif

#if defined(MAJOR_IN_MKDEV)
#include <sys/mkdev.h>
#else
#if defined(MAJOR_IN_SYSMACROS)
#include <sys/sysmacros.h>
#endif
#if defined(HAVE_MKNOD) && defined(HAVE_SYS_MKDEV_H)
#include <sys/mkdev.h>
#endif
#endif

#if defined _MSC_VER && _MSC_VER >= 1400
/* Microsoft CRT in VS2005 and higher will verify that a filehandle is
 * valid and throw an assertion if it isn't.
 * Normally, an invalid fd is likely to be a C program error and therefore
 * an assertion can be useful, but it does contradict the POSIX standard
 * which for write(2) states:
 *    "Otherwise, -1 shall be returned and errno set to indicate the error."
 *    "[EBADF] The fildes argument is not a valid file descriptor open for
 *     writing."
 * Furthermore, python allows the user to enter any old integer
 * as a fd and should merely raise a python exception on error.
 * The Microsoft CRT doesn't provide an official way to check for the
 * validity of a file descriptor, but we can emulate its internal behaviour
 * by using the exported __pinfo data member and knowledge of the
 * internal structures involved.
 * The structures below must be updated for each version of visual studio
 * according to the file internal.h in the CRT source, until MS comes
 * up with a less hacky way to do this.
 * (all of this is to avoid globally modifying the CRT behaviour using
 * _set_invalid_parameter_handler() and _CrtSetReportMode())
 */
/* The actual size of the structure is determined at runtime.
 * Only the first items must be present.
 */
typedef struct {
    intptr_t osfhnd;
    char osfile;
} my_ioinfo;

//extern __declspec(dllimport) char * __pioinfo[];
extern char * __pioinfo[];
#define IOINFO_L2E 5
#define IOINFO_ARRAY_ELTS   (1 << IOINFO_L2E)
#define IOINFO_ARRAYS 64
#define _NHANDLE_           (IOINFO_ARRAYS * IOINFO_ARRAY_ELTS)
#define FOPEN 0x01
#define _NO_CONSOLE_FILENO (intptr_t)-2

/* This function emulates what the windows CRT does to validate file handles */
int
_PyVerify_fd(int fd)
{
    const int i1 = fd >> IOINFO_L2E;
    const int i2 = fd & ((1 << IOINFO_L2E) - 1);

    static int sizeof_ioinfo = 0;

    /* Determine the actual size of the ioinfo structure,
     * as used by the CRT loaded in memory
     */
    if (sizeof_ioinfo == 0 && __pioinfo[0] != NULL) {
        sizeof_ioinfo = _msize(__pioinfo[0]) / IOINFO_ARRAY_ELTS;
    }
    if (sizeof_ioinfo == 0) {
        /* This should not happen... */
        goto fail;
    }

    /* See that it isn't a special CLEAR fileno */
    if (fd != _NO_CONSOLE_FILENO) {
        /* Microsoft CRT would check that 0<=fd<_nhandle but we can't do that.  Instead
         * we check pointer validity and other info
         */
        if (0 <= i1 && i1 < IOINFO_ARRAYS && __pioinfo[i1] != NULL) {
            /* finally, check that the file is open */
            my_ioinfo* info = (my_ioinfo*)(__pioinfo[i1] + i2 * sizeof_ioinfo);
            if (info->osfile & FOPEN) {
                return 1;
            }
        }
    }
  fail:
    errno = EBADF;
    return 0;
}

/* the special case of checking dup2.  The target fd must be in a sensible range */
static int
_PyVerify_fd_dup2(int fd1, int fd2)
{
    if (!_PyVerify_fd(fd1))
        return 0;
    if (fd2 == _NO_CONSOLE_FILENO)
        return 0;
    if ((unsigned)fd2 < _NHANDLE_)
        return 1;
    else
        return 0;
}
#else
/* dummy version. _PyVerify_fd() is already defined in fileobject.h */
#define _PyVerify_fd_dup2(A, B) (1)
#endif

/* Return a dictionary corresponding to the POSIX environment table */
#ifdef WITH_NEXT_FRAMEWORK
/* On Darwin/MacOSX a shared library or framework has no access to
** environ directly, we must obtain it with _NSGetEnviron().
*/
#include <crt_externs.h>
static char **environ;
#elif !defined(_MSC_VER) && ( !defined(__WATCOMC__) || defined(__QNX__) )
extern char **environ;
#endif /* !_MSC_VER */

// Gives problems with the Source Engine filesystem, so undef
#undef GetCurrentDirectory

static PyObject *
convertenviron(void)
{
    PyObject *d;
    char **e;
#if defined(PYOS_OS2)
    APIRET rc;
    char   buffer[1024]; /* OS/2 Provides a Documented Max of 1024 Chars */
#endif
    d = PyDict_New();
    if (d == NULL)
        return NULL;
#ifdef WITH_NEXT_FRAMEWORK
    if (environ == NULL)
        environ = *_NSGetEnviron();
#endif
    if (environ == NULL)
        return d;
    /* This part ignores errors */
    for (e = environ; *e != NULL; e++) {
        PyObject *k;
        PyObject *v;
        char *p = strchr(*e, '=');
        if (p == NULL)
            continue;
        k = PyString_FromStringAndSize(*e, (int)(p-*e));
        if (k == NULL) {
            PyErr_Clear();
            continue;
        }
        v = PyString_FromString(p+1);
        if (v == NULL) {
            PyErr_Clear();
            Py_DECREF(k);
            continue;
        }
        if (PyDict_GetItem(d, k) == NULL) {
            if (PyDict_SetItem(d, k, v) != 0)
                PyErr_Clear();
        }
        Py_DECREF(k);
        Py_DECREF(v);
    }
#if defined(PYOS_OS2)
    rc = DosQueryExtLIBPATH(buffer, BEGIN_LIBPATH);
    if (rc == NO_ERROR) { /* (not a type, envname is NOT 'BEGIN_LIBPATH') */
        PyObject *v = PyString_FromString(buffer);
            PyDict_SetItemString(d, "BEGINLIBPATH", v);
        Py_DECREF(v);
    }
    rc = DosQueryExtLIBPATH(buffer, END_LIBPATH);
    if (rc == NO_ERROR) { /* (not a typo, envname is NOT 'END_LIBPATH') */
        PyObject *v = PyString_FromString(buffer);
            PyDict_SetItemString(d, "ENDLIBPATH", v);
        Py_DECREF(v);
    }
#endif
	return d;
}


/* Set a POSIX-specific error from errno, and return NULL */

static int
srcos_error_outsidemodfolder(void)
{
	PyErr_SetString(PyExc_OSError,
		"File access outside the mod folder is not allowed");
	return 0;
}

static PyObject *
srcos_error(void)
{
	return PyErr_SetFromErrno(PyExc_OSError);
}
static PyObject *
srcos_error_with_filename(char* name)
{
	return PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);
}

#ifdef MS_WINDOWS
static PyObject *
srcos_error_with_unicode_filename(Py_UNICODE* name)
{
	return PyErr_SetFromErrnoWithUnicodeFilename(PyExc_OSError, name);
}
#endif /* MS_WINDOWS */


static PyObject *
srcos_error_with_allocated_filename(char* name)
{
	PyObject *rc = PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);
	PyMem_Free(name);
	return rc;
}

#ifdef MS_WINDOWS
static PyObject *
win32_error(char* function, char* filename)
{
	/* XXX We should pass the function name along in the future.
	   (_winreg.c also wants to pass the function name.)
	   This would however require an additional param to the
	   Windows error object, which is non-trivial.
	*/
	errno = GetLastError();
	if (filename)
		return PyErr_SetFromWindowsErrWithFilename(errno, filename);
	else
		return PyErr_SetFromWindowsErr(errno);
}

static PyObject *
win32_error_unicode(char* function, Py_UNICODE* filename)
{
	/* XXX - see win32_error for comments on 'function' */
	errno = GetLastError();
	if (filename)
		return PyErr_SetFromWindowsErrWithUnicodeFilename(errno, filename);
	else
		return PyErr_SetFromWindowsErr(errno);
}

static int
convert_to_unicode(PyObject **param)
{
	if (PyUnicode_CheckExact(*param))
		Py_INCREF(*param);
	else if (PyUnicode_Check(*param))
		/* For a Unicode subtype that's not a Unicode object,
		   return a true Unicode object with the same data. */
		*param = PyUnicode_FromUnicode(PyUnicode_AS_UNICODE(*param),
					       PyUnicode_GET_SIZE(*param));
	else
		*param = PyUnicode_FromEncodedObject(*param,
				                     Py_FileSystemDefaultEncoding,
					             "strict");
	return (*param) != NULL;
}

#endif /* MS_WINDOWS */

#if defined(PYOS_OS2)
/**********************************************************************
 *         Helper Function to Trim and Format OS/2 Messages
 **********************************************************************/
    static void
os2_formatmsg(char *msgbuf, int msglen, char *reason)
{
    msgbuf[msglen] = '\0'; /* OS/2 Doesn't Guarantee a Terminator */

    if (strlen(msgbuf) > 0) { /* If Non-Empty Msg, Trim CRLF */
        char *lastc = &msgbuf[ strlen(msgbuf)-1 ];

        while (lastc > msgbuf && isspace(Py_CHARMASK(*lastc)))
            *lastc-- = '\0'; /* Trim Trailing Whitespace (CRLF) */
    }

    /* Add Optional Reason Text */
    if (reason) {
        strcat(msgbuf, " : ");
        strcat(msgbuf, reason);
    }
}

/**********************************************************************
 *             Decode an OS/2 Operating System Error Code
 *
 * A convenience function to lookup an OS/2 error code and return a
 * text message we can use to raise a Python exception.
 *
 * Notes:
 *   The messages for errors returned from the OS/2 kernel reside in
 *   the file OSO001.MSG in the \OS2 directory hierarchy.
 *
 **********************************************************************/
    static char *
os2_strerror(char *msgbuf, int msgbuflen, int errorcode, char *reason)
{
    APIRET rc;
    ULONG  msglen;

    /* Retrieve Kernel-Related Error Message from OSO001.MSG File */
    Py_BEGIN_ALLOW_THREADS
    rc = DosGetMessage(NULL, 0, msgbuf, msgbuflen,
                       errorcode, "oso001.msg", &msglen);
    Py_END_ALLOW_THREADS

    if (rc == NO_ERROR)
        os2_formatmsg(msgbuf, msglen, reason);
    else
        PyOS_snprintf(msgbuf, msgbuflen,
        	      "unknown OS error #%d", errorcode);

    return msgbuf;
}

/* Set an OS/2-specific error and return NULL.  OS/2 kernel
   errors are not in a global variable e.g. 'errno' nor are
   they congruent with posix error numbers. */

static PyObject *
os2_error(int code)
{
    char text[1024];
    PyObject *v;

    os2_strerror(text, sizeof(text), code, "");

    v = Py_BuildValue("(is)", code, text);
    if (v != NULL) {
        PyErr_SetObject(PyExc_OSError, v);
        Py_DECREF(v);
    }
    return NULL; /* Signal to Python that an Exception is Pending */
}

#endif /* OS2 */

/* POSIX generic methods */

static PyObject *
posix_fildes(PyObject *fdobj, int (*func)(int))
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}

static PyObject *
posix_1str(PyObject *args, char *format, int (*func)(const char*))
{
    char *path1 = NULL;
    int res;
    if (!PyArg_ParseTuple(args, format,
                          Py_FileSystemDefaultEncoding, &path1))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = (*func)(path1);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return srcos_error_with_allocated_filename(path1);
    PyMem_Free(path1);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
posix_2str(PyObject *args,
	   char *format,
	   int (*func)(const char *, const char *))
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}

#ifdef MS_WINDOWS
static PyObject*
win32_1str(PyObject* args, char* func,
           char* format, BOOL (__stdcall *funcA)(LPCSTR),
           char* wformat, BOOL (__stdcall *funcW)(LPWSTR))
{
    //PyObject *uni;
    char *ansi;
    BOOL result;
	char fullPath[_MAX_PATH];

#if 0
    if (!PyArg_ParseTuple(args, wformat, &uni))
        PyErr_Clear();
    else {
        Py_BEGIN_ALLOW_THREADS
        result = funcW(PyUnicode_AsUnicode(uni));
        Py_END_ALLOW_THREADS
        if (!result)
            return win32_error_unicode(func, PyUnicode_AsUnicode(uni));
        Py_INCREF(Py_None);
        return Py_None;
    }
#endif // 0
    if (!PyArg_ParseTuple(args, format, &ansi))
        return NULL;

	if( SrcPyGetFullPathSilent(ansi, fullPath, _MAX_PATH) == 0 ) {
		srcos_error_outsidemodfolder();
		return NULL;
	}

    Py_BEGIN_ALLOW_THREADS
    result = funcA(fullPath);
    Py_END_ALLOW_THREADS
    if (!result)
        return win32_error(func, ansi);
    Py_INCREF(Py_None);
    return Py_None;

}

/* This is a reimplementation of the C library's chdir function,
   but one that produces Win32 errors instead of DOS error codes.
   chdir is essentially a wrapper around SetCurrentDirectory; however,
   it also needs to set "magic" environment variables indicating
   the per-drive current directory, which are of the form =<drive>: */
static BOOL __stdcall
win32_chdir(LPCSTR path)
{
    char new_path[MAX_PATH+1];
    int result;
    char env[4] = "=x:";

    if(!SetCurrentDirectoryA(path))
        return FALSE;
    result = GetCurrentDirectoryA(MAX_PATH+1, new_path);
    if (!result)
        return FALSE;
    /* In the ANSI API, there should not be any paths longer
       than MAX_PATH. */
    assert(result <= MAX_PATH+1);
    if (strncmp(new_path, "\\\\", 2) == 0 ||
        strncmp(new_path, "//", 2) == 0)
        /* UNC path, nothing to do. */
        return TRUE;
    env[1] = new_path[0];
    return SetEnvironmentVariableA(env, new_path);
}

/* The Unicode version differs from the ANSI version
   since the current directory might exceed MAX_PATH characters */
static BOOL __stdcall
win32_wchdir(LPWSTR path)
{
    wchar_t _new_path[MAX_PATH+1], *new_path = _new_path;
    int result;
    wchar_t env[4] = L"=x:";

    if(!SetCurrentDirectoryW(path))
        return FALSE;
    result = GetCurrentDirectoryW(MAX_PATH+1, new_path);
    if (!result)
        return FALSE;
    if (result > MAX_PATH+1) {
        new_path = (wchar_t *)malloc(result * sizeof(wchar_t));
        if (!new_path) {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        result = GetCurrentDirectoryW(result, new_path);
        if (!result) {
            free(new_path);
            return FALSE;
        }
    }
    if (wcsncmp(new_path, L"\\\\", 2) == 0 ||
        wcsncmp(new_path, L"//", 2) == 0)
        /* UNC path, nothing to do. */
        return TRUE;
    env[1] = new_path[0];
    result = SetEnvironmentVariableW(env, new_path);
    if (new_path != _new_path)
        free(new_path);
    return result;
}
#endif

#ifdef MS_WINDOWS
/* The CRT of Windows has a number of flaws wrt. its stat() implementation:
   - time stamps are restricted to second resolution
   - file modification times suffer from forth-and-back conversions between
     UTC and local time
   Therefore, we implement our own stat, based on the Win32 API directly.
*/
#define HAVE_STAT_NSEC 1

struct win32_stat{
    int st_dev;
    __int64 st_ino;
    unsigned short st_mode;
    int st_nlink;
    int st_uid;
    int st_gid;
    int st_rdev;
    __int64 st_size;
    int st_atime;
    int st_atime_nsec;
    int st_mtime;
    int st_mtime_nsec;
    int st_ctime;
    int st_ctime_nsec;
};

static __int64 secs_between_epochs = 11644473600; /* Seconds between 1.1.1601 and 1.1.1970 */

static void
FILE_TIME_to_time_t_nsec(FILETIME *in_ptr, int *time_out, int* nsec_out)
{
    /* XXX endianness. Shouldn't matter, as all Windows implementations are little-endian */
    /* Cannot simply cast and dereference in_ptr,
       since it might not be aligned properly */
    __int64 in;
    memcpy(&in, in_ptr, sizeof(in));
    *nsec_out = (int)(in % 10000000) * 100; /* FILETIME is in units of 100 nsec. */
    /* XXX Win32 supports time stamps past 2038; we currently don't */
    *time_out = Py_SAFE_DOWNCAST((in / 10000000) - secs_between_epochs, __int64, int);
}

static void
time_t_to_FILE_TIME(int time_in, int nsec_in, FILETIME *out_ptr)
{
    /* XXX endianness */
    __int64 out;
    out = time_in + secs_between_epochs;
    out = out * 10000000 + nsec_in / 100;
    memcpy(out_ptr, &out, sizeof(out));
}

/* Below, we *know* that ugo+r is 0444 */
#if _S_IREAD != 0400
#error Unsupported C library
#endif
static int
attributes_to_mode(DWORD attr)
{
    int m = 0;
    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        m |= _S_IFDIR | 0111; /* IFEXEC for user,group,other */
    else
        m |= _S_IFREG;
    if (attr & FILE_ATTRIBUTE_READONLY)
        m |= 0444;
    else
        m |= 0666;
    return m;
}

static int
attribute_data_to_stat(WIN32_FILE_ATTRIBUTE_DATA *info, struct win32_stat *result)
{
	memset(result, 0, sizeof(*result));
	result->st_mode = attributes_to_mode(info->dwFileAttributes);
	result->st_size = (((__int64)info->nFileSizeHigh)<<32) + info->nFileSizeLow;
	FILE_TIME_to_time_t_nsec(&info->ftCreationTime, &result->st_ctime, &result->st_ctime_nsec);
	FILE_TIME_to_time_t_nsec(&info->ftLastWriteTime, &result->st_mtime, &result->st_mtime_nsec);
	FILE_TIME_to_time_t_nsec(&info->ftLastAccessTime, &result->st_atime, &result->st_atime_nsec);

    return 0;
}

static BOOL
attributes_from_dir(LPCSTR pszFile, LPWIN32_FILE_ATTRIBUTE_DATA pfad)
{
	HANDLE hFindFile;
	WIN32_FIND_DATAA FileData;
	hFindFile = FindFirstFileA(pszFile, &FileData);
	if (hFindFile == INVALID_HANDLE_VALUE)
		return FALSE;
	FindClose(hFindFile);
	pfad->dwFileAttributes = FileData.dwFileAttributes;
	pfad->ftCreationTime   = FileData.ftCreationTime;
	pfad->ftLastAccessTime = FileData.ftLastAccessTime;
	pfad->ftLastWriteTime  = FileData.ftLastWriteTime;
	pfad->nFileSizeHigh    = FileData.nFileSizeHigh;
	pfad->nFileSizeLow     = FileData.nFileSizeLow;
	return TRUE;
}

static BOOL
attributes_from_dir_w(LPCWSTR pszFile, LPWIN32_FILE_ATTRIBUTE_DATA pfad)
{
	HANDLE hFindFile;
	WIN32_FIND_DATAW FileData;
	hFindFile = FindFirstFileW(pszFile, &FileData);
	if (hFindFile == INVALID_HANDLE_VALUE)
		return FALSE;
	FindClose(hFindFile);
	pfad->dwFileAttributes = FileData.dwFileAttributes;
	pfad->ftCreationTime   = FileData.ftCreationTime;
	pfad->ftLastAccessTime = FileData.ftLastAccessTime;
	pfad->ftLastWriteTime  = FileData.ftLastWriteTime;
	pfad->nFileSizeHigh    = FileData.nFileSizeHigh;
	pfad->nFileSizeLow     = FileData.nFileSizeLow;
	return TRUE;
}

static int
win32_stat(const char* path, struct win32_stat *result)
{
    WIN32_FILE_ATTRIBUTE_DATA info;
    int code;
    char *dot;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &info)) {
        if (GetLastError() != ERROR_SHARING_VIOLATION) {
            /* Protocol violation: we explicitly clear errno, instead of
               setting it to a POSIX error. Callers should use GetLastError. */
            errno = 0;
            return -1;
        } else {
            /* Could not get attributes on open file. Fall back to
               reading the directory. */
            if (!attributes_from_dir(path, &info)) {
                /* Very strange. This should not fail now */
                errno = 0;
                return -1;
            }
        }
    }
    code = attribute_data_to_stat(&info, result);
    if (code != 0)
        return code;
    /* Set S_IFEXEC if it is an .exe, .bat, ... */
	dot = (char *)strrchr(path, '.');
    if (dot) {
        if (stricmp(dot, ".bat") == 0 ||
        stricmp(dot, ".cmd") == 0 ||
        stricmp(dot, ".exe") == 0 ||
        stricmp(dot, ".com") == 0)
            result->st_mode |= 0111;
    }
    return code;
}

static int
win32_wstat(const wchar_t* path, struct win32_stat *result)
{
    int code;
    const wchar_t *dot;
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &info)) {
        if (GetLastError() != ERROR_SHARING_VIOLATION) {
            /* Protocol violation: we explicitly clear errno, instead of
               setting it to a POSIX error. Callers should use GetLastError. */
            errno = 0;
            return -1;
        } else {
            /* Could not get attributes on open file. Fall back to
               reading the directory. */
            if (!attributes_from_dir_w(path, &info)) {
                /* Very strange. This should not fail now */
                errno = 0;
                return -1;
            }
        }
    }
    code = attribute_data_to_stat(&info, result);
    if (code < 0)
        return code;
    /* Set IFEXEC if it is an .exe, .bat, ... */
    dot = wcsrchr(path, '.');
    if (dot) {
        if (_wcsicmp(dot, L".bat") == 0 ||
            _wcsicmp(dot, L".cmd") == 0 ||
            _wcsicmp(dot, L".exe") == 0 ||
            _wcsicmp(dot, L".com") == 0)
            result->st_mode |= 0111;
    }
    return code;
}

static int
win32_fstat(int file_number, struct win32_stat *result)
{
    BY_HANDLE_FILE_INFORMATION info;
    HANDLE h;
    int type;

    h = (HANDLE)_get_osfhandle(file_number);

    /* Protocol violation: we explicitly clear errno, instead of
       setting it to a POSIX error. Callers should use GetLastError. */
    errno = 0;

    if (h == INVALID_HANDLE_VALUE) {
        /* This is really a C library error (invalid file handle).
           We set the Win32 error to the closes one matching. */
        SetLastError(ERROR_INVALID_HANDLE);
        return -1;
    }
    memset(result, 0, sizeof(*result));

    type = GetFileType(h);
    if (type == FILE_TYPE_UNKNOWN) {
        DWORD error = GetLastError();
        if (error != 0) {
        return -1;
        }
        /* else: valid but unknown file */
    }

    if (type != FILE_TYPE_DISK) {
        if (type == FILE_TYPE_CHAR)
            result->st_mode = _S_IFCHR;
        else if (type == FILE_TYPE_PIPE)
            result->st_mode = _S_IFIFO;
        return 0;
    }

    if (!GetFileInformationByHandle(h, &info)) {
        return -1;
    }

    /* similar to stat() */
    result->st_mode = attributes_to_mode(info.dwFileAttributes);
    result->st_size = (((__int64)info.nFileSizeHigh)<<32) + info.nFileSizeLow;
    FILE_TIME_to_time_t_nsec(&info.ftCreationTime, &result->st_ctime, &result->st_ctime_nsec);
    FILE_TIME_to_time_t_nsec(&info.ftLastWriteTime, &result->st_mtime, &result->st_mtime_nsec);
    FILE_TIME_to_time_t_nsec(&info.ftLastAccessTime, &result->st_atime, &result->st_atime_nsec);
    /* specific to fstat() */
    result->st_nlink = info.nNumberOfLinks;
    result->st_ino = (((__int64)info.nFileIndexHigh)<<32) + info.nFileIndexLow;
    return 0;
}

#endif /* MS_WINDOWS */

PyDoc_STRVAR(stat_result__doc__,
"stat_result: Result from stat or lstat.\n\n\
This object may be accessed either as a tuple of\n\
  (mode, ino, dev, nlink, uid, gid, size, atime, mtime, ctime)\n\
or via the attributes st_mode, st_ino, st_dev, st_nlink, st_uid, and so on.\n\
\n\
Posix/windows: If your platform supports st_blksize, st_blocks, st_rdev,\n\
or st_flags, they are available as attributes only.\n\
\n\
See os.stat for more information.");

static PyStructSequence_Field stat_result_fields[] = {
    {"st_mode",    "protection bits"},
    {"st_ino",     "inode"},
    {"st_dev",     "device"},
    {"st_nlink",   "number of hard links"},
    {"st_uid",     "user ID of owner"},
    {"st_gid",     "group ID of owner"},
    {"st_size",    "total size, in bytes"},
    /* The NULL is replaced with PyStructSequence_UnnamedField later. */
    {NULL,   "integer time of last access"},
    {NULL,   "integer time of last modification"},
    {NULL,   "integer time of last change"},
    {"st_atime",   "time of last access"},
    {"st_mtime",   "time of last modification"},
    {"st_ctime",   "time of last change"},
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
    {"st_blksize", "blocksize for filesystem I/O"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    {"st_blocks",  "number of blocks allocated"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
    {"st_rdev",    "device type (if inode device)"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_FLAGS
    {"st_flags",   "user defined flags for file"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_GEN
    {"st_gen",    "generation number"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
    {"st_birthtime",   "time of creation"},
#endif
    {0}
};

#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
#define ST_BLKSIZE_IDX 13
#else
#define ST_BLKSIZE_IDX 12
#endif

#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
#define ST_BLOCKS_IDX (ST_BLKSIZE_IDX+1)
#else
#define ST_BLOCKS_IDX ST_BLKSIZE_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_RDEV
#define ST_RDEV_IDX (ST_BLOCKS_IDX+1)
#else
#define ST_RDEV_IDX ST_BLOCKS_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_FLAGS
#define ST_FLAGS_IDX (ST_RDEV_IDX+1)
#else
#define ST_FLAGS_IDX ST_RDEV_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_GEN
#define ST_GEN_IDX (ST_FLAGS_IDX+1)
#else
#define ST_GEN_IDX ST_FLAGS_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
#define ST_BIRTHTIME_IDX (ST_GEN_IDX+1)
#else
#define ST_BIRTHTIME_IDX ST_GEN_IDX
#endif

static PyStructSequence_Desc stat_result_desc = {
    "stat_result", /* name */
    stat_result__doc__, /* doc */
    stat_result_fields,
    10
};

PyDoc_STRVAR(statvfs_result__doc__,
"statvfs_result: Result from statvfs or fstatvfs.\n\n\
This object may be accessed either as a tuple of\n\
  (bsize, frsize, blocks, bfree, bavail, files, ffree, favail, flag, namemax),\n\
or via the attributes f_bsize, f_frsize, f_blocks, f_bfree, and so on.\n\
\n\
See os.statvfs for more information.");

static PyStructSequence_Field statvfs_result_fields[] = {
    {"f_bsize",  },
    {"f_frsize", },
    {"f_blocks", },
    {"f_bfree",  },
    {"f_bavail", },
    {"f_files",  },
    {"f_ffree",  },
    {"f_favail", },
    {"f_flag",   },
    {"f_namemax",},
    {0}
};

static PyStructSequence_Desc statvfs_result_desc = {
    "statvfs_result", /* name */
    statvfs_result__doc__, /* doc */
    statvfs_result_fields,
    10
};

static int initialized;
static PyTypeObject StatResultType;
static PyTypeObject StatVFSResultType;
static newfunc structseq_new;

static PyObject *
statresult_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyStructSequence *result;
    int i;

    result = (PyStructSequence*)structseq_new(type, args, kwds);
    if (!result)
        return NULL;
    /* If we have been initialized from a tuple,
       st_?time might be set to None. Initialize it
       from the int slots.  */
    for (i = 7; i <= 9; i++) {
        if (result->ob_item[i+3] == Py_None) {
            Py_DECREF(Py_None);
            Py_INCREF(result->ob_item[i]);
            result->ob_item[i+3] = result->ob_item[i];
        }
    }
    return (PyObject*)result;
}



/* If true, st_?time is float. */
static int _stat_float_times = 1;

PyDoc_STRVAR(stat_float_times__doc__,
"stat_float_times([newval]) -> oldval\n\n\
Determine whether os.[lf]stat represents time stamps as float objects.\n\
If newval is True, future calls to stat() return floats, if it is False,\n\
future calls return ints. \n\
If newval is omitted, return the current setting.\n");

static PyObject*
stat_float_times(PyObject* self, PyObject *args)
{
    int newval = -1;
    if (!PyArg_ParseTuple(args, "|i:stat_float_times", &newval))
        return NULL;
    if (newval == -1)
        /* Return old value */
        return PyBool_FromLong(_stat_float_times);
    _stat_float_times = newval;
    Py_INCREF(Py_None);
    return Py_None;
}

static void
fill_time(PyObject *v, int index, time_t sec, unsigned long nsec)
{
    PyObject *fval,*ival;
#if SIZEOF_TIME_T > SIZEOF_LONG
    ival = PyLong_FromLongLong((PY_LONG_LONG)sec);
#else
    ival = PyInt_FromLong((long)sec);
#endif
    if (!ival)
        return;
    if (_stat_float_times) {
        fval = PyFloat_FromDouble(sec + 1e-9*nsec);
    } else {
        fval = ival;
        Py_INCREF(fval);
    }
    PyStructSequence_SET_ITEM(v, index, ival);
    PyStructSequence_SET_ITEM(v, index+3, fval);
}

/* pack a system stat C structure into the Python stat tuple
   (used by posix_stat() and posix_fstat()) */
static PyObject*
_pystat_fromstructstat(STRUCT_STAT *st)
{
    unsigned long ansec, mnsec, cnsec;
    PyObject *v = PyStructSequence_New(&StatResultType);
    if (v == NULL)
        return NULL;

    PyStructSequence_SET_ITEM(v, 0, PyInt_FromLong((long)st->st_mode));
#ifdef HAVE_LARGEFILE_SUPPORT
    PyStructSequence_SET_ITEM(v, 1,
                              PyLong_FromLongLong((PY_LONG_LONG)st->st_ino));
#else
    PyStructSequence_SET_ITEM(v, 1, PyInt_FromLong((long)st->st_ino));
#endif
#if defined(HAVE_LONG_LONG) && !defined(MS_WINDOWS)
    PyStructSequence_SET_ITEM(v, 2,
                              PyLong_FromLongLong((PY_LONG_LONG)st->st_dev));
#else
    PyStructSequence_SET_ITEM(v, 2, PyInt_FromLong((long)st->st_dev));
#endif
    PyStructSequence_SET_ITEM(v, 3, PyInt_FromLong((long)st->st_nlink));
    PyStructSequence_SET_ITEM(v, 4, PyInt_FromLong((long)st->st_uid));
    PyStructSequence_SET_ITEM(v, 5, PyInt_FromLong((long)st->st_gid));
#ifdef HAVE_LARGEFILE_SUPPORT
    PyStructSequence_SET_ITEM(v, 6,
                              PyLong_FromLongLong((PY_LONG_LONG)st->st_size));
#else
    PyStructSequence_SET_ITEM(v, 6, PyInt_FromLong(st->st_size));
#endif

#if defined(HAVE_STAT_TV_NSEC)
    ansec = st->st_atim.tv_nsec;
    mnsec = st->st_mtim.tv_nsec;
    cnsec = st->st_ctim.tv_nsec;
#elif defined(HAVE_STAT_TV_NSEC2)
    ansec = st->st_atimespec.tv_nsec;
    mnsec = st->st_mtimespec.tv_nsec;
    cnsec = st->st_ctimespec.tv_nsec;
#elif defined(HAVE_STAT_NSEC)
    ansec = st->st_atime_nsec;
    mnsec = st->st_mtime_nsec;
    cnsec = st->st_ctime_nsec;
#else
    ansec = mnsec = cnsec = 0;
#endif
    fill_time(v, 7, st->st_atime, ansec);
    fill_time(v, 8, st->st_mtime, mnsec);
    fill_time(v, 9, st->st_ctime, cnsec);

#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
    PyStructSequence_SET_ITEM(v, ST_BLKSIZE_IDX,
                              PyInt_FromLong((long)st->st_blksize));
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    PyStructSequence_SET_ITEM(v, ST_BLOCKS_IDX,
                              PyInt_FromLong((long)st->st_blocks));
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
    PyStructSequence_SET_ITEM(v, ST_RDEV_IDX,
                              PyInt_FromLong((long)st->st_rdev));
#endif
#ifdef HAVE_STRUCT_STAT_ST_GEN
    PyStructSequence_SET_ITEM(v, ST_GEN_IDX,
                              PyInt_FromLong((long)st->st_gen));
#endif
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
    {
      PyObject *val;
      unsigned long bsec,bnsec;
      bsec = (long)st->st_birthtime;
#ifdef HAVE_STAT_TV_NSEC2
      bnsec = st->st_birthtimespec.tv_nsec;
#else
      bnsec = 0;
#endif
      if (_stat_float_times) {
        val = PyFloat_FromDouble(bsec + 1e-9*bnsec);
      } else {
        val = PyInt_FromLong((long)bsec);
      }
      PyStructSequence_SET_ITEM(v, ST_BIRTHTIME_IDX,
                                val);
    }
#endif
#ifdef HAVE_STRUCT_STAT_ST_FLAGS
    PyStructSequence_SET_ITEM(v, ST_FLAGS_IDX,
                              PyInt_FromLong((long)st->st_flags));
#endif

    if (PyErr_Occurred()) {
        Py_DECREF(v);
        return NULL;
    }

    return v;
}

#ifdef MS_WINDOWS

/* IsUNCRoot -- test whether the supplied path is of the form \\SERVER\SHARE\,
   where / can be used in place of \ and the trailing slash is optional.
   Both SERVER and SHARE must have at least one character.
*/

#define ISSLASHA(c) ((c) == '\\' || (c) == '/')
#define ISSLASHW(c) ((c) == L'\\' || (c) == L'/')
#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

static BOOL
IsUNCRootA(char *path, int pathlen)
{
    #define ISSLASH ISSLASHA

    int i, share;

    if (pathlen < 5 || !ISSLASH(path[0]) || !ISSLASH(path[1]))
        /* minimum UNCRoot is \\x\y */
        return FALSE;
    for (i = 2; i < pathlen ; i++)
        if (ISSLASH(path[i])) break;
    if (i == 2 || i == pathlen)
        /* do not allow \\\SHARE or \\SERVER */
        return FALSE;
    share = i+1;
    for (i = share; i < pathlen; i++)
        if (ISSLASH(path[i])) break;
    return (i != share && (i == pathlen || i == pathlen-1));

    #undef ISSLASH
}

static BOOL
IsUNCRootW(Py_UNICODE *path, int pathlen)
{
    #define ISSLASH ISSLASHW

    int i, share;

    if (pathlen < 5 || !ISSLASH(path[0]) || !ISSLASH(path[1]))
        /* minimum UNCRoot is \\x\y */
        return FALSE;
    for (i = 2; i < pathlen ; i++)
        if (ISSLASH(path[i])) break;
    if (i == 2 || i == pathlen)
        /* do not allow \\\SHARE or \\SERVER */
        return FALSE;
    share = i+1;
    for (i = share; i < pathlen; i++)
        if (ISSLASH(path[i])) break;
    return (i != share && (i == pathlen || i == pathlen-1));

    #undef ISSLASH
}
#endif /* MS_WINDOWS */

static PyObject *
srcos_do_stat(PyObject *self, PyObject *args,
              char *format,
#ifdef __VMS
              int (*statfunc)(const char *, STRUCT_STAT *, ...),
#else
              int (*statfunc)(const char *, STRUCT_STAT *),
#endif
              char *wformat,
              int (*wstatfunc)(const Py_UNICODE *, STRUCT_STAT *))
{
    STRUCT_STAT st;
    char *path = NULL;          /* pass this to stat; do not free() it */
    char *pathfree = NULL;  /* this memory must be free'd */
    int res;
    PyObject *result;
	char fullPath[_MAX_PATH];

#if 0
#ifdef MS_WINDOWS
    PyUnicodeObject *po;
    if (PyArg_ParseTuple(args, wformat, &po)) {
        Py_UNICODE *wpath = PyUnicode_AS_UNICODE(po);

        Py_BEGIN_ALLOW_THREADS
            /* PyUnicode_AS_UNICODE result OK without
               thread lock as it is a simple dereference. */
        res = wstatfunc(wpath, &st);
        Py_END_ALLOW_THREADS

        if (res != 0)
            return win32_error_unicode("stat", wpath);
        return _pystat_fromstructstat(&st);
    }
    /* Drop the argument parsing error as narrow strings
       are also valid. */
    PyErr_Clear();
#endif
#endif // 0

    if (!PyArg_ParseTuple(args, format,
                          Py_FileSystemDefaultEncoding, &path))
        return NULL;
    pathfree = path;


	if( SrcPyGetFullPathSilent(path, fullPath, _MAX_PATH) == 0 ) {
		srcos_error_outsidemodfolder();
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
		res = (*statfunc)(fullPath, &st);
	Py_END_ALLOW_THREADS

		if (res != 0) {
#ifdef MS_WINDOWS
			result = win32_error("stat", pathfree);
#else
			result = srcos_error_with_filename(pathfree);
#endif
    }
    else
        result = _pystat_fromstructstat(&st);

    PyMem_Free(pathfree);
    return result;
}

/* POSIX methods */

PyDoc_STRVAR(srcos_access__doc__,
"access(path, mode) -> True if granted, False otherwise\n\n\
Use the real uid/gid to test for access to a path.  Note that most\n\
operations will use the effective uid/gid, therefore this routine can\n\
be used in a suid/sgid environment to test if the invoking user has the\n\
specified access to the path.  The mode argument can be F_OK to test\n\
existence, or the inclusive-OR of R_OK, W_OK, and X_OK.");

static PyObject *
srcos_access(PyObject *self, PyObject *args)
{
    char *path;
    int mode;

#ifdef MS_WINDOWS
    DWORD attr;
    PyUnicodeObject *po;
    if (PyArg_ParseTuple(args, "Ui:access", &po, &mode)) {
        Py_BEGIN_ALLOW_THREADS
        /* PyUnicode_AS_UNICODE OK without thread lock as
           it is a simple dereference. */
        attr = GetFileAttributesW(PyUnicode_AS_UNICODE(po));
        Py_END_ALLOW_THREADS
        goto finish;
    }
    /* Drop the argument parsing error as narrow strings
       are also valid. */
    PyErr_Clear();
    if (!PyArg_ParseTuple(args, "eti:access",
                          Py_FileSystemDefaultEncoding, &path, &mode))
        return NULL;

	// Verify if the directory is relative to our mod folder
	if( SrcPyPathIsInGameFolder(path) == 0 ) {
		return PyBool_FromLong(0);
	}

    Py_BEGIN_ALLOW_THREADS
    attr = GetFileAttributesA(path);
    Py_END_ALLOW_THREADS
    PyMem_Free(path);
finish:
    if (attr == 0xFFFFFFFF)
        /* File does not exist, or cannot read attributes */
        return PyBool_FromLong(0);
    /* Access is possible if either write access wasn't requested, or
       the file isn't read-only, or if it's a directory, as there are
       no read-only directories on Windows. */
    return PyBool_FromLong(!(mode & 2)
                           || !(attr & FILE_ATTRIBUTE_READONLY)
                           || (attr & FILE_ATTRIBUTE_DIRECTORY));
#else /* MS_WINDOWS */
    int res;
    if (!PyArg_ParseTuple(args, "eti:access",
                          Py_FileSystemDefaultEncoding, &path, &mode))
        return NULL;

	// Verify if the directory is relative to our mod folder
	if( SrcPyPathIsInGameFolder(path) == 0 ) {
		srcos_error_outsidemodfolder();
		return NULL;
	}

    Py_BEGIN_ALLOW_THREADS
    res = access(path, mode);
    Py_END_ALLOW_THREADS
    PyMem_Free(path);
    return PyBool_FromLong(res == 0);
#endif /* MS_WINDOWS */
}

#ifndef F_OK
#define F_OK 0
#endif
#ifndef R_OK
#define R_OK 4
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef X_OK
#define X_OK 1
#endif

#ifdef HAVE_TTYNAME
PyDoc_STRVAR(srcos_ttyname__doc__,
"ttyname(fd) -> string\n\n\
Return the name of the terminal device connected to 'fd'.");

static PyObject *
srcos_ttyname(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif

#ifdef HAVE_CTERMID
PyDoc_STRVAR(srcos_ctermid__doc__,
"ctermid() -> string\n\n\
Return the name of the controlling terminal for this process.");

static PyObject *
srcos_ctermid(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif

PyDoc_STRVAR(srcos_chdir__doc__,
"chdir(path)\n\n\
Change the current working directory to the specified path.");

static PyObject *
srcos_chdir(PyObject *self, PyObject *args)
{
#ifdef MS_WINDOWS
	return win32_1str(args, "chdir", "s:chdir", win32_chdir, "U:chdir", win32_wchdir);
#elif defined(PYOS_OS2) && defined(PYCC_GCC)
	return posix_1str(args, "et:chdir", _chdir2);
#elif defined(__VMS)
	return posix_1str(args, "et:chdir", (int (*)(const char *))chdir);
#else
	return posix_1str(args, "et:chdir", chdir);
#endif
}

#ifdef HAVE_FCHDIR
PyDoc_STRVAR(srcos_fchdir__doc__,
"fchdir(fildes)\n\n\
Change to the directory of the given file descriptor.  fildes must be\n\
opened on a directory, not a file.");

static PyObject *
srcos_fchdir(PyObject *self, PyObject *fdobj)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_FCHDIR */


PyDoc_STRVAR(srcos_chmod__doc__,
"chmod(path, mode)\n\n\
Change the access permissions of a file.");

static PyObject *
srcos_chmod(PyObject *self, PyObject *args)
{
    char *path = NULL;
    int i;
    int res;
	char fullPath[_MAX_PATH];
#ifdef MS_WINDOWS
    DWORD attr;
    //PyUnicodeObject *po;

#if 0
    if (PyArg_ParseTuple(args, "Ui|:chmod", &po, &i)) {
        Py_BEGIN_ALLOW_THREADS
        attr = GetFileAttributesW(PyUnicode_AS_UNICODE(po));
        if (attr != 0xFFFFFFFF) {
            if (i & _S_IWRITE)
                attr &= ~FILE_ATTRIBUTE_READONLY;
            else
                attr |= FILE_ATTRIBUTE_READONLY;
            res = SetFileAttributesW(PyUnicode_AS_UNICODE(po), attr);
        }
        else
            res = 0;
        Py_END_ALLOW_THREADS
        if (!res)
            return win32_error_unicode("chmod",
                                       PyUnicode_AS_UNICODE(po));
        Py_INCREF(Py_None);
        return Py_None;
    }
    /* Drop the argument parsing error as narrow strings
       are also valid. */
    PyErr_Clear();
#endif // 0

    if (!PyArg_ParseTuple(args, "eti:chmod", Py_FileSystemDefaultEncoding,
                          &path, &i))
        return NULL;

	if( SrcPyGetFullPathSilent(path, fullPath, _MAX_PATH) == 0 ) {
		srcos_error_outsidemodfolder();
		return NULL;
	}

    Py_BEGIN_ALLOW_THREADS
    attr = GetFileAttributesA(path);
    if (attr != 0xFFFFFFFF) {
        if (i & _S_IWRITE)
            attr &= ~FILE_ATTRIBUTE_READONLY;
        else
            attr |= FILE_ATTRIBUTE_READONLY;
        res = SetFileAttributesA(path, attr);
    }
    else
        res = 0;
    Py_END_ALLOW_THREADS
    if (!res) {
        win32_error("chmod", path);
        PyMem_Free(path);
        return NULL;
    }
    PyMem_Free(path);
    Py_INCREF(Py_None);
    return Py_None;
#else /* MS_WINDOWS */
    if (!PyArg_ParseTuple(args, "eti:chmod", Py_FileSystemDefaultEncoding,
                          &path, &i))
        return NULL;

	if( SrcPyGetFullPathSilent(path, fullPath, _MAX_PATH) == 0 ) {
		srcos_error_outsidemodfolder();
		return NULL;
	}

    Py_BEGIN_ALLOW_THREADS
    res = chmod(path, i);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return srcos_error_with_allocated_filename(path);
    PyMem_Free(path);
    Py_INCREF(Py_None);
    return Py_None;
#endif
}

#ifdef HAVE_FCHMOD
PyDoc_STRVAR(srcos_fchmod__doc__,
"fchmod(fd, mode)\n\n\
Change the access permissions of the file given by file\n\
descriptor fd.");

static PyObject *
srcos_fchmod(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_FCHMOD */

#ifdef HAVE_LCHMOD
PyDoc_STRVAR(srcos_lchmod__doc__,
"lchmod(path, mode)\n\n\
Change the access permissions of a file. If path is a symlink, this\n\
affects the link itself rather than the target.");

static PyObject *
srcos_lchmod(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_LCHMOD */


#ifdef HAVE_CHFLAGS
PyDoc_STRVAR(srcos_chflags__doc__,
"chflags(path, flags)\n\n\
Set file flags.");

static PyObject *
srcos_chflags(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_CHFLAGS */

#ifdef HAVE_LCHFLAGS
PyDoc_STRVAR(srcos_lchflags__doc__,
"lchflags(path, flags)\n\n\
Set file flags.\n\
This function will not follow symbolic links.");

static PyObject *
srcos_lchflags(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_LCHFLAGS */

#ifdef HAVE_CHROOT
PyDoc_STRVAR(srcos_chroot__doc__,
"chroot(path)\n\n\
Change root directory to path.");

static PyObject *
srcos_chroot(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif

#ifdef HAVE_FSYNC
PyDoc_STRVAR(srcos_fsync__doc__,
"fsync(fildes)\n\n\
force write of file with filedescriptor to disk.");

static PyObject *
srcos_fsync(PyObject *self, PyObject *fdobj)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_FSYNC */

#ifdef HAVE_FDATASYNC

#ifdef __hpux
extern int fdatasync(int); /* On HP-UX, in libc but not in unistd.h */
#endif

PyDoc_STRVAR(srcos_fdatasync__doc__,
"fdatasync(fildes)\n\n\
force write of file with filedescriptor to disk.\n\
 does not force update of metadata.");

static PyObject *
srcos_fdatasync(PyObject *self, PyObject *fdobj)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_FDATASYNC */


#ifdef HAVE_CHOWN
PyDoc_STRVAR(srcos_chown__doc__,
"chown(path, uid, gid)\n\n\
Change the owner and group id of path to the numeric uid and gid.");

static PyObject *
srcos_chown(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_CHOWN */

#ifdef HAVE_FCHOWN
PyDoc_STRVAR(srcos_fchown__doc__,
"fchown(fd, uid, gid)\n\n\
Change the owner and group id of the file given by file descriptor\n\
fd to the numeric uid and gid.");

static PyObject *
srcos_fchown(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_FCHOWN */

#ifdef HAVE_LCHOWN
PyDoc_STRVAR(srcos_lchown__doc__,
"lchown(path, uid, gid)\n\n\
Change the owner and group id of path to the numeric uid and gid.\n\
This function will not follow symbolic links.");

static PyObject *
srcos_lchown(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_LCHOWN */


#ifdef HAVE_GETCWD
PyDoc_STRVAR(srcos_getcwd__doc__,
"getcwd() -> path\n\n\
Return a string representing the current working directory.");

static PyObject *
srcos_getcwd(PyObject *self, PyObject *noargs)
{
#if 0
	int bufsize_incr = 1024;
	int bufsize = 0;
	char *tmpbuf = NULL;
	char *res = NULL;
	PyObject *dynamic_return;

	Py_BEGIN_ALLOW_THREADS
		do {
			bufsize = bufsize + bufsize_incr;
			tmpbuf = (char *)malloc(bufsize);
			if (tmpbuf == NULL) {
				break;
			}

			res = (char *)g_pFullFileSystem->RelativePathToFullPath(".", "MOD", tmpbuf, bufsize);

			if (res == NULL) {
				free(tmpbuf);
			}
		} while ((res == NULL) && (errno == ERANGE));
	Py_END_ALLOW_THREADS

		if (res == NULL)
			return srcos_error();

	dynamic_return = PyString_FromString(tmpbuf);
	free(tmpbuf);

	return dynamic_return;
#else

	int bufsize_incr = 1024;
	int bufsize = 0;
	char *tmpbuf = NULL;
	int res = 0;
	PyObject *dynamic_return;

	Py_BEGIN_ALLOW_THREADS
		do {
			bufsize = bufsize + bufsize_incr;
			tmpbuf = (char *)malloc(bufsize);
			if (tmpbuf == NULL) {
				break;
			}

			res = SrcFileSystem_GetCurrentDirectory(tmpbuf, bufsize);

			if (res == 0) {
				free(tmpbuf);
			}
		} while ((res == 0) && (errno == ERANGE));
	Py_END_ALLOW_THREADS

		if (res == 0)
			return srcos_error();

	dynamic_return = PyString_FromString(tmpbuf);
	free(tmpbuf);

	return dynamic_return;
#endif // 0
}

#ifdef Py_USING_UNICODE
PyDoc_STRVAR(srcos_getcwdu__doc__,
"getcwdu() -> path\n\n\
Return a unicode string representing the current working directory.");

static PyObject *
srcos_getcwdu(PyObject *self, PyObject *noargs)
{
	char buf[1026];
	int res;

#ifdef Py_WIN_WIDE_FILENAMES
	DWORD len;
	if (unicode_file_names()) {
		wchar_t wbuf[1026];
		wchar_t *wbuf2 = wbuf;
		PyObject *resobj;
		Py_BEGIN_ALLOW_THREADS
		len = GetCurrentDirectoryW(sizeof wbuf/ sizeof wbuf[0], wbuf);
		/* If the buffer is large enough, len does not include the
		   terminating \0. If the buffer is too small, len includes
		   the space needed for the terminator. */
		if (len >= sizeof wbuf/ sizeof wbuf[0]) {
			wbuf2 = (wchar_t *)malloc(len * sizeof(wchar_t));
			if (wbuf2)
				len = GetCurrentDirectoryW(len, wbuf2);
		}
		Py_END_ALLOW_THREADS
		if (!wbuf2) {
			PyErr_NoMemory();
			return NULL;
		}
		if (!len) {
			if (wbuf2 != wbuf) free(wbuf2);
			return win32_error("getcwdu", NULL);
		}
		resobj = PyUnicode_FromWideChar(wbuf2, len);
		if (wbuf2 != wbuf) free(wbuf2);
		return resobj;
	}
#endif

	Py_BEGIN_ALLOW_THREADS
	res = SrcFileSystem_GetCurrentDirectory(buf, sizeof buf);
	Py_END_ALLOW_THREADS
	if (res == 0)
		return srcos_error();
	return PyUnicode_Decode(buf, strlen(buf), Py_FileSystemDefaultEncoding,"strict");
}
#endif
#endif


#ifdef HAVE_LINK
PyDoc_STRVAR(srcos_link__doc__,
"link(src, dst)\n\n\
Create a hard link to a file.");

static PyObject *
srcos_link(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_LINK */


PyObject *SrcFileSystem_ListDir(PyObject *self, PyObject *args);

PyDoc_STRVAR(srcos_listdir__doc__,
"listdir(path) -> list_of_strings\n\n\
Return a list containing the names of the entries in the directory.\n\
\n\
	path: path of directory to list\n\
\n\
The list is in arbitrary order.  It does not include the special\n\
entries '.' and '..' even if they are present in the directory.");

static PyObject *
srcos_listdir(PyObject *self, PyObject *args)
{
	//return SrcFileSystem_ListDir(self, args);

    /* XXX Should redo this putting the (now four) versions of opendir
       in separate files instead of having them all here... */
#if defined(MS_WINDOWS) && !defined(HAVE_OPENDIR)

    PyObject *d, *v;
    HANDLE hFindFile;
    BOOL result;
    WIN32_FIND_DATA FileData;
    char namebuf[MAX_PATH+5]; /* Overallocate for \\*.*\0 */
    char *bufptr = namebuf;
    Py_ssize_t len = sizeof(namebuf)-5; /* only claim to have space for MAX_PATH */

    PyObject *po;
    if (PyArg_ParseTuple(args, "U:listdir", &po)) {
        WIN32_FIND_DATAW wFileData;
        Py_UNICODE *wnamebuf;
        /* Overallocate for \\*.*\0 */
        len = PyUnicode_GET_SIZE(po);
        wnamebuf = malloc((len + 5) * sizeof(wchar_t));
        if (!wnamebuf) {
            PyErr_NoMemory();
            return NULL;
        }
        wcscpy(wnamebuf, PyUnicode_AS_UNICODE(po));
        if (len > 0) {
            Py_UNICODE wch = wnamebuf[len-1];
            if (wch != L'/' && wch != L'\\' && wch != L':')
                wnamebuf[len++] = L'\\';
            wcscpy(wnamebuf + len, L"*.*");
        }
        if ((d = PyList_New(0)) == NULL) {
            free(wnamebuf);
            return NULL;
        }
        hFindFile = FindFirstFileW(wnamebuf, &wFileData);
        if (hFindFile == INVALID_HANDLE_VALUE) {
            int error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND) {
                free(wnamebuf);
                return d;
            }
            Py_DECREF(d);
            win32_error_unicode("FindFirstFileW", wnamebuf);
            free(wnamebuf);
            return NULL;
        }
        do {
            /* Skip over . and .. */
            if (wcscmp(wFileData.cFileName, L".") != 0 &&
                wcscmp(wFileData.cFileName, L"..") != 0) {
                v = PyUnicode_FromUnicode(wFileData.cFileName, wcslen(wFileData.cFileName));
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
            result = FindNextFileW(hFindFile, &wFileData);
            Py_END_ALLOW_THREADS
            /* FindNextFile sets error to ERROR_NO_MORE_FILES if
               it got to the end of the directory. */
            if (!result && GetLastError() != ERROR_NO_MORE_FILES) {
                Py_DECREF(d);
                win32_error_unicode("FindNextFileW", wnamebuf);
                FindClose(hFindFile);
                free(wnamebuf);
                return NULL;
            }
        } while (result == TRUE);

        if (FindClose(hFindFile) == FALSE) {
            Py_DECREF(d);
            win32_error_unicode("FindClose", wnamebuf);
            free(wnamebuf);
            return NULL;
        }
        free(wnamebuf);
        return d;
    }
    /* Drop the argument parsing error as narrow strings
       are also valid. */
    PyErr_Clear();

    if (!PyArg_ParseTuple(args, "et#:listdir",
                          Py_FileSystemDefaultEncoding, &bufptr, &len))
        return NULL;
    if (len > 0) {
        char ch = namebuf[len-1];
        if (ch != SEP && ch != ALTSEP && ch != ':')
            namebuf[len++] = '/';
        strcpy(namebuf + len, "*.*");
    }

    if ((d = PyList_New(0)) == NULL)
        return NULL;

    hFindFile = FindFirstFile(namebuf, &FileData);
    if (hFindFile == INVALID_HANDLE_VALUE) {
        int error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND)
            return d;
        Py_DECREF(d);
        return win32_error("FindFirstFile", namebuf);
    }
    do {
        /* Skip over . and .. */
        if (strcmp(FileData.cFileName, ".") != 0 &&
            strcmp(FileData.cFileName, "..") != 0) {
            v = PyString_FromString(FileData.cFileName);
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
        result = FindNextFile(hFindFile, &FileData);
        Py_END_ALLOW_THREADS
        /* FindNextFile sets error to ERROR_NO_MORE_FILES if
           it got to the end of the directory. */
        if (!result && GetLastError() != ERROR_NO_MORE_FILES) {
            Py_DECREF(d);
            win32_error("FindNextFile", namebuf);
            FindClose(hFindFile);
            return NULL;
        }
    } while (result == TRUE);

    if (FindClose(hFindFile) == FALSE) {
        Py_DECREF(d);
        return win32_error("FindClose", namebuf);
    }

    return d;

#elif defined(PYOS_OS2)

#ifndef MAX_PATH
#define MAX_PATH    CCHMAXPATH
#endif
    char *name, *pt;
    Py_ssize_t len;
    PyObject *d, *v;
    char namebuf[MAX_PATH+5];
    HDIR  hdir = 1;
    ULONG srchcnt = 1;
    FILEFINDBUF3   ep;
    APIRET rc;

    if (!PyArg_ParseTuple(args, "t#:listdir", &name, &len))
        return NULL;
    if (len >= MAX_PATH) {
        PyErr_SetString(PyExc_ValueError, "path too long");
        return NULL;
    }
    strcpy(namebuf, name);
    for (pt = namebuf; *pt; pt++)
        if (*pt == ALTSEP)
            *pt = SEP;
    if (namebuf[len-1] != SEP)
        namebuf[len++] = SEP;
    strcpy(namebuf + len, "*.*");

    if ((d = PyList_New(0)) == NULL)
        return NULL;

    rc = DosFindFirst(namebuf,         /* Wildcard Pattern to Match */
                      &hdir,           /* Handle to Use While Search Directory */
                      FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM | FILE_DIRECTORY,
                      &ep, sizeof(ep), /* Structure to Receive Directory Entry */
                      &srchcnt,        /* Max and Actual Count of Entries Per Iteration */
                      FIL_STANDARD);   /* Format of Entry (EAs or Not) */

    if (rc != NO_ERROR) {
        errno = ENOENT;
        return srcos_error_with_filename(name);
    }

    if (srchcnt > 0) { /* If Directory is NOT Totally Empty, */
        do {
            if (ep.achName[0] == '.'
            && (ep.achName[1] == '\0' || (ep.achName[1] == '.' && ep.achName[2] == '\0')))
                continue; /* Skip Over "." and ".." Names */

            strcpy(namebuf, ep.achName);

            /* Leave Case of Name Alone -- In Native Form */
            /* (Removed Forced Lowercasing Code) */

            v = PyString_FromString(namebuf);
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
        } while (DosFindNext(hdir, &ep, sizeof(ep), &srchcnt) == NO_ERROR && srchcnt > 0);
    }

    return d;
#else

    char *name = NULL;
    PyObject *d, *v;
    DIR *dirp;
    struct dirent *ep;
    int arg_is_unicode = 1;
    char fullPath[_MAX_PATH];

    errno = 0;
    if (!PyArg_ParseTuple(args, "U:listdir", &v)) {
        arg_is_unicode = 0;
        PyErr_Clear();
    }
    if (!PyArg_ParseTuple(args, "et:listdir", Py_FileSystemDefaultEncoding, &name))
        return NULL;
	if( SrcPyGetFullPathSilent(name, fullPath, _MAX_PATH) == 0 ) {
		srcos_error_outsidemodfolder();
		return NULL;
	}
    if ((dirp = opendir(fullPath)) == NULL) {
        return srcos_error_with_allocated_filename(name);
    }
    if ((d = PyList_New(0)) == NULL) {
        closedir(dirp);
        PyMem_Free(name);
        return NULL;
    }
    for (;;) {
        errno = 0;
        Py_BEGIN_ALLOW_THREADS
        ep = readdir(dirp);
        Py_END_ALLOW_THREADS
        if (ep == NULL) {
            if (errno == 0) {
                break;
            } else {
                closedir(dirp);
                Py_DECREF(d);
                return srcos_error_with_allocated_filename(name);
            }
        }
        if (ep->d_name[0] == '.' &&
            (NAMLEN(ep) == 1 ||
             (ep->d_name[1] == '.' && NAMLEN(ep) == 2)))
            continue;
        v = PyString_FromStringAndSize(ep->d_name, NAMLEN(ep));
        if (v == NULL) {
            Py_DECREF(d);
            d = NULL;
            break;
        }
#ifdef Py_USING_UNICODE
        if (arg_is_unicode) {
            PyObject *w;

            w = PyUnicode_FromEncodedObject(v,
                                            Py_FileSystemDefaultEncoding,
                                            "strict");
            if (w != NULL) {
                Py_DECREF(v);
                v = w;
            }
            else {
                /* fall back to the original byte string, as
                   discussed in patch #683592 */
                PyErr_Clear();
            }
        }
#endif
        if (PyList_Append(d, v) != 0) {
            Py_DECREF(v);
            Py_DECREF(d);
            d = NULL;
            break;
        }
        Py_DECREF(v);
    }
    closedir(dirp);
    PyMem_Free(name);

    return d;

#endif /* which OS */
}

#ifdef MS_WINDOWS
/* A helper function for abspath on win32 */
static PyObject *
srcos__getfullpathname(PyObject *self, PyObject *args)
{
#if 0
	char inbuf[MAX_PATH*2];
	char *inbufp = inbuf;
	Py_ssize_t insize = sizeof(inbuf);
	char outbuf[MAX_PATH*2];
	if (!PyArg_ParseTuple (args, "et#:_getfullpathname",
		Py_FileSystemDefaultEncoding, &inbufp,
		&insize))
		return NULL;

	g_pFullFileSystem->RelativePathToFullPath(inbufp, "GAME", outbuf, MAX_PATH*2);
	if (PyUnicode_Check(PyTuple_GetItem(args, 0))) {
		return PyUnicode_Decode(outbuf, strlen(outbuf),
			Py_FileSystemDefaultEncoding, NULL);
	}
	return PyString_FromString(outbuf);
#else
	/* assume encoded strings won't more than double no of chars */
	char inbuf[MAX_PATH*2];
	char *inbufp = inbuf;
	Py_ssize_t insize = sizeof(inbuf);
	char outbuf[MAX_PATH*2];
	char *temp;
#ifdef Py_WIN_WIDE_FILENAMES
	if (unicode_file_names()) {
		PyUnicodeObject *po;
		if (PyArg_ParseTuple(args, "U|:_getfullpathname", &po)) {
			Py_UNICODE *wpath = PyUnicode_AS_UNICODE(po);
			Py_UNICODE woutbuf[MAX_PATH*2], *woutbufp = woutbuf;
			Py_UNICODE *wtemp;
			DWORD result;
			PyObject *v;
			result = GetFullPathNameW(wpath,
						   sizeof(woutbuf)/sizeof(woutbuf[0]),
						    woutbuf, &wtemp);
			if (result > sizeof(woutbuf)/sizeof(woutbuf[0])) {
				woutbufp = (Py_UNICODE *)malloc(result * sizeof(Py_UNICODE));
				if (!woutbufp)
					return PyErr_NoMemory();
				result = GetFullPathNameW(wpath, result, woutbufp, &wtemp);
			}
			if (result)
				v = PyUnicode_FromUnicode(woutbufp, wcslen(woutbufp));
			else
				v = win32_error_unicode("GetFullPathNameW", wpath);
			if (woutbufp != woutbuf)
				free(woutbufp);
			return v;
		}
		/* Drop the argument parsing error as narrow strings
		   are also valid. */
		PyErr_Clear();
	}
#endif
	if (!PyArg_ParseTuple (args, "et#:_getfullpathname",
	                       Py_FileSystemDefaultEncoding, &inbufp,
	                       &insize))
		return NULL;
	if (!GetFullPathName(inbuf, sizeof(outbuf)/sizeof(outbuf[0]),
		outbuf, &temp))
		return win32_error("GetFullPathName", inbuf);
	if (PyUnicode_Check(PyTuple_GetItem(args, 0))) {
		return PyUnicode_Decode(outbuf, strlen(outbuf),
			Py_FileSystemDefaultEncoding, NULL);
	}
	return PyString_FromString(outbuf);
#endif // 0
}
#endif /* MS_WINDOWS */

PyDoc_STRVAR(srcos_mkdir__doc__,
"mkdir(path [, mode=0777])\n\n\
Create a directory.");

static PyObject *
srcos_mkdir(PyObject *self, PyObject *args)
{
    int res;
    char *path = NULL;
    int mode = 0777;

#ifdef MS_WINDOWS
    PyUnicodeObject *po;
    if (PyArg_ParseTuple(args, "U|i:mkdir", &po, &mode)) {
        Py_BEGIN_ALLOW_THREADS
        /* PyUnicode_AS_UNICODE OK without thread lock as
           it is a simple dereference. */
        res = CreateDirectoryW(PyUnicode_AS_UNICODE(po), NULL);
        Py_END_ALLOW_THREADS
        if (!res)
            return win32_error_unicode("mkdir", PyUnicode_AS_UNICODE(po));
        Py_INCREF(Py_None);
        return Py_None;
    }
    /* Drop the argument parsing error as narrow strings
       are also valid. */
    PyErr_Clear();
    if (!PyArg_ParseTuple(args, "et|i:mkdir",
                          Py_FileSystemDefaultEncoding, &path, &mode))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    /* PyUnicode_AS_UNICODE OK without thread lock as
       it is a simple dereference. */
    res = CreateDirectoryA(path, NULL);
    Py_END_ALLOW_THREADS
    if (!res) {
        win32_error("mkdir", path);
        PyMem_Free(path);
        return NULL;
    }
    PyMem_Free(path);
    Py_INCREF(Py_None);
    return Py_None;
#else /* MS_WINDOWS */
    if (!PyArg_ParseTuple(args, "et|i:mkdir",
                          Py_FileSystemDefaultEncoding, &path, &mode))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
#if ( defined(__WATCOMC__) || defined(PYCC_VACPP) ) && !defined(__QNX__)
    res = mkdir(path);
#else
    res = mkdir(path, mode);
#endif
    Py_END_ALLOW_THREADS
    if (res < 0)
        return srcos_error_with_allocated_filename(path);
    PyMem_Free(path);
    Py_INCREF(Py_None);
    return Py_None;
#endif /* MS_WINDOWS */

#if 0
	char *path = NULL;
	int mode = 0777;
	char fullPath[_MAX_PATH];

	if (!PyArg_ParseTuple(args, "et|i:mkdir",
		Py_FileSystemDefaultEncoding, &path, &mode))
		return NULL;

	// Verify if the directory is relative to our mod folder
	if( SrcPyGetFullPath(path, fullPath, _MAX_PATH) == 0 ) {
		srcos_error_outsidemodfolder();
		return NULL;
	}

	// NOTE: The source engine filesystem doesn't provides any way to know if it succeeded
	// So we can't throw an exception or anything.
	Py_BEGIN_ALLOW_THREADS
	SrcFileSystem_CreateDirHierarchy(path, "MOD");
	Py_END_ALLOW_THREADS

	PyMem_Free(path);
	Py_INCREF(Py_None);
	return Py_None;
#endif // 0
}


/* sys/resource.h is needed for at least: wait3(), wait4(), broken nice. */
#if defined(HAVE_SYS_RESOURCE_H)
#include <sys/resource.h>
#endif


#ifdef HAVE_NICE
PyDoc_STRVAR(srcos_nice__doc__,
"nice(inc) -> new_priority\n\n\
Decrease the priority of process by inc and return the new priority.");

static PyObject *
srcos_nice(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_NICE */

PyDoc_STRVAR(srcos_rename__doc__,
"rename(old, new)\n\n\
Rename a file or directory.");

static PyObject *
srcos_rename(PyObject *self, PyObject *args)
{
#ifdef MS_WINDOWS
    PyObject *o1, *o2;
    char *p1, *p2;
    BOOL result;
	char fullPath[_MAX_PATH];
    if (!PyArg_ParseTuple(args, "OO:rename", &o1, &o2))
        goto error;
    if (!convert_to_unicode(&o1))
        goto error;
    if (!convert_to_unicode(&o2)) {
        Py_DECREF(o1);
        goto error;
    }

	// Verify if the directory is relative to our mod folder
	if( SrcPyGetFullPath(PyString_AsString(o1), fullPath, _MAX_PATH) == 0 ) {
		srcos_error_outsidemodfolder();
		return NULL;
	}
	if( SrcPyGetFullPath(PyString_AsString(o2), fullPath, _MAX_PATH) == 0 ) {
		srcos_error_outsidemodfolder();
		return NULL;
	}

    Py_BEGIN_ALLOW_THREADS
    result = MoveFileW(PyUnicode_AsUnicode(o1),
                       PyUnicode_AsUnicode(o2));
    Py_END_ALLOW_THREADS
    Py_DECREF(o1);
    Py_DECREF(o2);
    if (!result)
        return win32_error("rename", NULL);
    Py_INCREF(Py_None);
    return Py_None;
error:
    PyErr_Clear();
    if (!PyArg_ParseTuple(args, "ss:rename", &p1, &p2))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    result = MoveFileA(p1, p2);
    Py_END_ALLOW_THREADS
    if (!result)
        return win32_error("rename", NULL);
    Py_INCREF(Py_None);
    return Py_None;
#else
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
    //return srcos_2str(args, "etet:rename", rename);
#endif
}


PyDoc_STRVAR(srcos_rmdir__doc__,
"rmdir(path)\n\n\
Remove a directory.");

static PyObject *
srcos_rmdir(PyObject *self, PyObject *args)
{
#ifdef MS_WINDOWS
    return win32_1str(args, "rmdir", "s:rmdir", RemoveDirectoryA, "U:rmdir", RemoveDirectoryW);
#else
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
#endif
}


PyDoc_STRVAR(srcos_stat__doc__,
"stat(path) -> stat result\n\n\
Perform a stat system call on the given path.");

static PyObject *
srcos_stat(PyObject *self, PyObject *args)
{
#ifdef MS_WINDOWS
	return srcos_do_stat(self, args, "et:stat", STAT, "U:stat", win32_wstat);
#else
	return srcos_do_stat(self, args, "et:stat", STAT, NULL, NULL);
#endif
}


#ifdef HAVE_SYSTEM
PyDoc_STRVAR(srcos_system__doc__,
"system(command) -> exit_status\n\n\
Execute the command (a string) in a subshell.");

static PyObject *
srcos_system(PyObject *self, PyObject *args)
{
#if 0
    char *command;
    long sts;
    if (!PyArg_ParseTuple(args, "s:system", &command))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    sts = system(command);
    Py_END_ALLOW_THREADS
    return PyInt_FromLong(sts);
#else
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
#endif // 0
}
#endif


PyDoc_STRVAR(srcos_umask__doc__,
"umask(new_mask) -> old_mask\n\n\
Set the current numeric umask and return the previous umask.");

static PyObject *
srcos_umask(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}


PyDoc_STRVAR(srcos_unlink__doc__,
"unlink(path)\n\n\
Remove a file (same as remove(path)).");

PyDoc_STRVAR(srcos_remove__doc__,
"remove(path)\n\n\
Remove a file (same as unlink(path)).");

static PyObject *
srcos_unlink(PyObject *self, PyObject *args)
{
	char *ansi;
	if (!PyArg_ParseTuple(args, "s:remove", &ansi))
		return NULL;

	if( SrcPyPathIsInGameFolder(ansi) == 0 ) {
		srcos_error_outsidemodfolder();
		return NULL;
	}
	
	SrcFileSystem_RemoveFile(ansi, 0);

	Py_INCREF(Py_None);
	return Py_None;
}


#ifdef HAVE_UNAME
PyDoc_STRVAR(srcos_uname__doc__,
"uname() -> (sysname, nodename, release, version, machine)\n\n\
Return a tuple identifying the current operating system.");

static PyObject *
srcos_uname(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_UNAME */

static int
extract_time(PyObject *t, long* sec, long* usec)
{
	long intval;
	if (PyFloat_Check(t)) {
		double tval = PyFloat_AsDouble(t);
		PyObject *intobj = Py_TYPE(t)->tp_as_number->nb_int(t);
		if (!intobj)
			return -1;
		intval = PyInt_AsLong(intobj);
		Py_DECREF(intobj);
		if (intval == -1 && PyErr_Occurred())
			return -1;
		*sec = intval;
		*usec = (long)((tval - intval) * 1e6); /* can't exceed 1000000 */
		if (*usec < 0)
			/* If rounding gave us a negative number,
			   truncate.  */
			*usec = 0;
		return 0;
	}
	intval = PyInt_AsLong(t);
	if (intval == -1 && PyErr_Occurred())
		return -1;
	*sec = intval;
	*usec = 0;
        return 0;
}

PyDoc_STRVAR(srcos_utime__doc__,
"utime(path, (atime, mtime))\n\
utime(path, None)\n\n\
Set the access and modified time of the file to the given values.  If the\n\
second form is used, set the access and modified times to the current time.");

static PyObject *
srcos_utime(PyObject *self, PyObject *args)
{
#ifdef MS_WINDOWS
    PyObject *arg;
    PyUnicodeObject *obwpath;
    wchar_t *wpath = NULL;
    char *apath = NULL;
	char fullPath[_MAX_PATH];
    HANDLE hFile;
    long atimesec, mtimesec, ausec, musec;
    FILETIME atime, mtime;
    PyObject *result = NULL;

    if (PyArg_ParseTuple(args, "UO|:utime", &obwpath, &arg)) {
        wpath = PyUnicode_AS_UNICODE(obwpath);
		// Verify if the directory is relative to our mod folder
		if( SrcPyGetFullPath((char *)wpath, fullPath, _MAX_PATH) == 0 ) {
			srcos_error_outsidemodfolder();
			return NULL;
		}
        Py_BEGIN_ALLOW_THREADS
        hFile = CreateFileW(wpath, FILE_WRITE_ATTRIBUTES, 0,
                            NULL, OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS, NULL);
        Py_END_ALLOW_THREADS
        if (hFile == INVALID_HANDLE_VALUE)
            return win32_error_unicode("utime", wpath);
    } else
        /* Drop the argument parsing error as narrow strings
           are also valid. */
        PyErr_Clear();

    if (!wpath) {
        if (!PyArg_ParseTuple(args, "etO:utime",
                              Py_FileSystemDefaultEncoding, &apath, &arg))
            return NULL;
		// Verify if the directory is relative to our mod folder
		if( SrcPyGetFullPath(apath, fullPath, _MAX_PATH) == 0 ) {
			srcos_error_outsidemodfolder();
			return NULL;
		}
        Py_BEGIN_ALLOW_THREADS
        hFile = CreateFileA(apath, FILE_WRITE_ATTRIBUTES, 0,
                            NULL, OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS, NULL);
        Py_END_ALLOW_THREADS
        if (hFile == INVALID_HANDLE_VALUE) {
            win32_error("utime", apath);
            PyMem_Free(apath);
            return NULL;
        }
        PyMem_Free(apath);
    }

    if (arg == Py_None) {
        SYSTEMTIME now;
        GetSystemTime(&now);
        if (!SystemTimeToFileTime(&now, &mtime) ||
            !SystemTimeToFileTime(&now, &atime)) {
            win32_error("utime", NULL);
            goto done;
            }
    }
    else if (!PyTuple_Check(arg) || PyTuple_Size(arg) != 2) {
        PyErr_SetString(PyExc_TypeError,
                        "utime() arg 2 must be a tuple (atime, mtime)");
        goto done;
    }
    else {
        if (extract_time(PyTuple_GET_ITEM(arg, 0),
                         &atimesec, &ausec) == -1)
            goto done;
        time_t_to_FILE_TIME(atimesec, 1000*ausec, &atime);
        if (extract_time(PyTuple_GET_ITEM(arg, 1),
                         &mtimesec, &musec) == -1)
            goto done;
        time_t_to_FILE_TIME(mtimesec, 1000*musec, &mtime);
    }
    if (!SetFileTime(hFile, NULL, &atime, &mtime)) {
        /* Avoid putting the file name into the error here,
           as that may confuse the user into believing that
           something is wrong with the file, when it also
           could be the time stamp that gives a problem. */
        win32_error("utime", NULL);
    }
    Py_INCREF(Py_None);
    result = Py_None;
done:
    CloseHandle(hFile);
    return result;
#else /* MS_WINDOWS */
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
#endif /* MS_WINDOWS */
}


/* Process operations */

PyDoc_STRVAR(srcos__exit__doc__,
"_exit(status)\n\n\
Exit to the system with specified status, without normal exit processing.");

static PyObject *
srcos__exit(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}

#if defined(HAVE_EXECV) || defined(HAVE_SPAWNV)
static void
free_string_array(char **array, Py_ssize_t count)
{
	Py_ssize_t i;
	for (i = 0; i < count; i++)
		PyMem_Free(array[i]);
	PyMem_DEL(array);
}
#endif


#ifdef HAVE_EXECV
PyDoc_STRVAR(srcos_execv__doc__,
"execv(path, args)\n\n\
Execute an executable path with arguments, replacing current process.\n\
\n\
	path: path of executable file\n\
	args: tuple or list of strings");

static PyObject *
srcos_execv(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}


PyDoc_STRVAR(srcos_execve__doc__,
"execve(path, args, env)\n\n\
Execute a path with arguments and environment, replacing current process.\n\
\n\
	path: path of executable file\n\
	args: tuple or list of arguments\n\
	env: dictionary of strings mapping to strings");

static PyObject *
srcos_execve(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_EXECV */


#ifdef HAVE_SPAWNV
PyDoc_STRVAR(srcos_spawnv__doc__,
"spawnv(mode, path, args)\n\n\
Execute the program 'path' in a new process.\n\
\n\
	mode: mode of process creation\n\
	path: path of executable file\n\
	args: tuple or list of strings");

static PyObject *
srcos_spawnv(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}


PyDoc_STRVAR(srcos_spawnve__doc__,
"spawnve(mode, path, args, env)\n\n\
Execute the program 'path' in a new process.\n\
\n\
	mode: mode of process creation\n\
	path: path of executable file\n\
	args: tuple or list of arguments\n\
	env: dictionary of strings mapping to strings");

static PyObject *
srcos_spawnve(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}

/* OS/2 supports spawnvp & spawnvpe natively */
#if defined(PYOS_OS2)
PyDoc_STRVAR(srcos_spawnvp__doc__,
"spawnvp(mode, file, args)\n\n\
Execute the program 'file' in a new process, using the environment\n\
search path to find the file.\n\
\n\
	mode: mode of process creation\n\
	file: executable file name\n\
	args: tuple or list of strings");

static PyObject *
srcos_spawnvp(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}


PyDoc_STRVAR(srcos_spawnvpe__doc__,
"spawnvpe(mode, file, args, env)\n\n\
Execute the program 'file' in a new process, using the environment\n\
search path to find the file.\n\
\n\
	mode: mode of process creation\n\
	file: executable file name\n\
	args: tuple or list of arguments\n\
	env: dictionary of strings mapping to strings");

static PyObject *
srcos_spawnvpe(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* PYOS_OS2 */
#endif /* HAVE_SPAWNV */


#ifdef HAVE_FORK1
PyDoc_STRVAR(srcos_fork1__doc__,
"fork1() -> pid\n\n\
Fork a child process with a single multiplexed (i.e., not bound) thread.\n\
\n\
Return 0 to child process and PID of child to parent process.");

static PyObject *
srcos_fork1(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif


#ifdef HAVE_FORK
PyDoc_STRVAR(srcos_fork__doc__,
"fork() -> pid\n\n\
Fork a child process.\n\
Return 0 to child process and PID of child to parent process.");

static PyObject *
srcos_fork(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif

/* AIX uses /dev/ptc but is otherwise the same as /dev/ptmx */
/* IRIX has both /dev/ptc and /dev/ptmx, use ptmx */
#if defined(HAVE_DEV_PTC) && !defined(HAVE_DEV_PTMX)
#define DEV_PTY_FILE "/dev/ptc"
#define HAVE_DEV_PTMX
#else
#define DEV_PTY_FILE "/dev/ptmx"
#endif

#if defined(HAVE_OPENPTY) || defined(HAVE_FORKPTY) || defined(HAVE_DEV_PTMX)
#ifdef HAVE_PTY_H
#include <pty.h>
#else
#ifdef HAVE_LIBUTIL_H
#include <libutil.h>
#endif /* HAVE_LIBUTIL_H */
#endif /* HAVE_PTY_H */
#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif
#endif /* defined(HAVE_OPENPTY) || defined(HAVE_FORKPTY) || defined(HAVE_DEV_PTMX */

#if defined(HAVE_OPENPTY) || defined(HAVE__GETPTY) || defined(HAVE_DEV_PTMX)
PyDoc_STRVAR(srcos_openpty__doc__,
"openpty() -> (master_fd, slave_fd)\n\n\
Open a pseudo-terminal, returning open fd's for both master and slave end.\n");

static PyObject *
srcos_openpty(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* defined(HAVE_OPENPTY) || defined(HAVE__GETPTY) || defined(HAVE_DEV_PTMX) */

#ifdef HAVE_FORKPTY
PyDoc_STRVAR(srcos_forkpty__doc__,
"forkpty() -> (pid, master_fd)\n\n\
Fork a new process with a new pseudo-terminal as controlling tty.\n\n\
Like fork(), return 0 as pid to child process, and PID of child to parent.\n\
To both, return fd of newly opened pseudo-terminal.\n");

static PyObject *
srcos_forkpty(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif

#ifdef HAVE_GETEGID
PyDoc_STRVAR(srcos_getegid__doc__,
"getegid() -> egid\n\n\
Return the current process's effective group id.");

static PyObject *
srcos_getegid(PyObject *self, PyObject *noargs)
{
	return PyInt_FromLong((long)getegid());
}
#endif


#ifdef HAVE_GETEUID
PyDoc_STRVAR(srcos_geteuid__doc__,
"geteuid() -> euid\n\n\
Return the current process's effective user id.");

static PyObject *
srcos_geteuid(PyObject *self, PyObject *noargs)
{
	return PyInt_FromLong((long)geteuid());
}
#endif


#ifdef HAVE_GETGID
PyDoc_STRVAR(srcos_getgid__doc__,
"getgid() -> gid\n\n\
Return the current process's group id.");

static PyObject *
srcos_getgid(PyObject *self, PyObject *noargs)
{
	return PyInt_FromLong((long)getgid());
}
#endif


PyDoc_STRVAR(srcos_getpid__doc__,
"getpid() -> pid\n\n\
Return the current process id");

static PyObject *
srcos_getpid(PyObject *self, PyObject *noargs)
{
	return PyLong_FromPid(getpid());
}


#ifdef HAVE_GETGROUPS
PyDoc_STRVAR(srcos_getgroups__doc__,
"getgroups() -> list of group IDs\n\n\
Return list of supplemental group IDs for the process.");

static PyObject *
srcos_getgroups(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif

#ifdef HAVE_GETPGID
PyDoc_STRVAR(srcos_getpgid__doc__,
"getpgid(pid) -> pgid\n\n\
Call the system call getpgid().");

static PyObject *
srcos_getpgid(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_GETPGID */


#ifdef HAVE_GETPGRP
PyDoc_STRVAR(srcos_getpgrp__doc__,
"getpgrp() -> pgrp\n\n\
Return the current process group id.");

static PyObject *
srcos_getpgrp(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_GETPGRP */


#ifdef HAVE_SETPGRP
PyDoc_STRVAR(srcos_setpgrp__doc__,
"setpgrp()\n\n\
Make this process a session leader.");

static PyObject *
srcos_setpgrp(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}

#endif /* HAVE_SETPGRP */

#ifdef HAVE_GETPPID
PyDoc_STRVAR(srcos_getppid__doc__,
"getppid() -> ppid\n\n\
Return the parent's process id.");

static PyObject *
srcos_getppid(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif


#ifdef HAVE_GETLOGIN
PyDoc_STRVAR(srcos_getlogin__doc__,
"getlogin() -> string\n\n\
Return the actual login name.");

static PyObject *
srcos_getlogin(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif

#ifdef HAVE_GETUID
PyDoc_STRVAR(srcos_getuid__doc__,
"getuid() -> uid\n\n\
Return the current process's user id.");

static PyObject *
srcos_getuid(PyObject *self, PyObject *noargs)
{
	return PyInt_FromLong((long)getuid());
}
#endif


#ifdef HAVE_KILL
PyDoc_STRVAR(srcos_kill__doc__,
"kill(pid, sig)\n\n\
Kill a process with a signal.");

static PyObject *
srcos_kill(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif

#ifdef HAVE_KILLPG
PyDoc_STRVAR(srcos_killpg__doc__,
"killpg(pgid, sig)\n\n\
Kill a process group with a signal.");

static PyObject *
srcos_killpg(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif

#ifdef HAVE_PLOCK

#ifdef HAVE_SYS_LOCK_H
#include <sys/lock.h>
#endif

PyDoc_STRVAR(srcos_plock__doc__,
"plock(op)\n\n\
Lock program segments into memory.");

static PyObject *
srcos_plock(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif


#ifdef HAVE_POPEN
PyDoc_STRVAR(srcos_popen__doc__,
"popen(command [, mode='r' [, bufsize]]) -> pipe\n\n\
Open a pipe to/from a command returning a file object.");

#if defined(PYOS_OS2)
#if defined(PYCC_VACPP)
static int
async_system(const char *command)
{
	char errormsg[256], args[1024];
	RESULTCODES rcodes;
	APIRET rc;

	char *shell = getenv("COMSPEC");
	if (!shell)
		shell = "cmd";

	/* avoid overflowing the argument buffer */
	if (strlen(shell) + 3 + strlen(command) >= 1024)
		return ERROR_NOT_ENOUGH_MEMORY

	args[0] = '\0';
	strcat(args, shell);
	strcat(args, "/c ");
	strcat(args, command);

	/* execute asynchronously, inheriting the environment */
	rc = DosExecPgm(errormsg,
			sizeof(errormsg),
			EXEC_ASYNC,
			args,
			NULL,
			&rcodes,
			shell);
	return rc;
}

static FILE *
popen(const char *command, const char *mode, int pipesize, int *err)
{
	int oldfd, tgtfd;
	HFILE pipeh[2];
	APIRET rc;

	/* mode determines which of stdin or stdout is reconnected to
	 * the pipe to the child
	 */
	if (strchr(mode, 'r') != NULL) {
		tgt_fd = 1;	/* stdout */
	} else if (strchr(mode, 'w')) {
		tgt_fd = 0;	/* stdin */
	} else {
		*err = ERROR_INVALID_ACCESS;
		return NULL;
	}

	/* setup the pipe */
	if ((rc = DosCreatePipe(&pipeh[0], &pipeh[1], pipesize)) != NO_ERROR) {
		*err = rc;
		return NULL;
	}

	/* prevent other threads accessing stdio */
	DosEnterCritSec();

	/* reconnect stdio and execute child */
	oldfd = dup(tgtfd);
	close(tgtfd);
	if (dup2(pipeh[tgtfd], tgtfd) == 0) {
		DosClose(pipeh[tgtfd]);
		rc = async_system(command);
	}

	/* restore stdio */
	dup2(oldfd, tgtfd);
	close(oldfd);

	/* allow other threads access to stdio */
	DosExitCritSec();

	/* if execution of child was successful return file stream */
	if (rc == NO_ERROR)
		return fdopen(pipeh[1 - tgtfd], mode);
	else {
		DosClose(pipeh[1 - tgtfd]);
		*err = rc;
		return NULL;
	}
}

static PyObject *
srcos_popen(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}

#elif defined(PYCC_GCC)

/* standard posix version of popen() support */
static PyObject *
srcos_popen(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}

/* fork() under OS/2 has lots'o'warts
 * EMX supports pipe() and spawn*() so we can synthesize popen[234]()
 * most of this code is a ripoff of the win32 code, but using the
 * capabilities of EMX's C library routines
 */

/* These tell _PyPopen() whether to return 1, 2, or 3 file objects. */
#define POPEN_1 1
#define POPEN_2 2
#define POPEN_3 3
#define POPEN_4 4

static PyObject *_PyPopen(char *, int, int, int);
static int _PyPclose(FILE *file);

/*
 * Internal dictionary mapping popen* file pointers to process handles,
 * for use when retrieving the process exit code.  See _PyPclose() below
 * for more information on this dictionary's use.
 */
static PyObject *_PyPopenProcs = NULL;

/* os2emx version of popen2()
 *
 * The result of this function is a pipe (file) connected to the
 * process's stdin, and a pipe connected to the process's stdout.
 */

static PyObject *
os2emx_popen2(PyObject *self, PyObject  *args)
{
	PyObject *f;
	int tm=0;

	char *cmdstring;
	char *mode = "t";
	int bufsize = -1;
	if (!PyArg_ParseTuple(args, "s|si:popen2", &cmdstring, &mode, &bufsize))
		return NULL;

	if (*mode == 't')
		tm = O_TEXT;
	else if (*mode != 'b') {
		PyErr_SetString(PyExc_ValueError, "mode must be 't' or 'b'");
		return NULL;
	} else
		tm = O_BINARY;

	f = _PyPopen(cmdstring, tm, POPEN_2, bufsize);

	return f;
}

/*
 * Variation on os2emx.popen2
 *
 * The result of this function is 3 pipes - the process's stdin,
 * stdout and stderr
 */

static PyObject *
os2emx_popen3(PyObject *self, PyObject *args)
{
	PyObject *f;
	int tm = 0;

	char *cmdstring;
	char *mode = "t";
	int bufsize = -1;
	if (!PyArg_ParseTuple(args, "s|si:popen3", &cmdstring, &mode, &bufsize))
		return NULL;

	if (*mode == 't')
		tm = O_TEXT;
	else if (*mode != 'b') {
		PyErr_SetString(PyExc_ValueError, "mode must be 't' or 'b'");
		return NULL;
	} else
		tm = O_BINARY;

	f = _PyPopen(cmdstring, tm, POPEN_3, bufsize);

	return f;
}

/*
 * Variation on os2emx.popen2
 *
 * The result of this function is 2 pipes - the processes stdin,
 * and stdout+stderr combined as a single pipe.
 */

static PyObject *
os2emx_popen4(PyObject *self, PyObject  *args)
{
	PyObject *f;
	int tm = 0;

	char *cmdstring;
	char *mode = "t";
	int bufsize = -1;
	if (!PyArg_ParseTuple(args, "s|si:popen4", &cmdstring, &mode, &bufsize))
		return NULL;

	if (*mode == 't')
		tm = O_TEXT;
	else if (*mode != 'b') {
		PyErr_SetString(PyExc_ValueError, "mode must be 't' or 'b'");
		return NULL;
	} else
		tm = O_BINARY;

	f = _PyPopen(cmdstring, tm, POPEN_4, bufsize);

	return f;
}

/* a couple of structures for convenient handling of multiple
 * file handles and pipes
 */
struct file_ref
{
	int handle;
	int flags;
};

struct pipe_ref
{
	int rd;
	int wr;
};

/* The following code is derived from the win32 code */

static PyObject *
_PyPopen(char *cmdstring, int mode, int n, int bufsize)
{
	struct file_ref stdio[3];
	struct pipe_ref p_fd[3];
	FILE *p_s[3];
	int file_count, i, pipe_err;
	pid_t pipe_pid;
	char *shell, *sh_name, *opt, *rd_mode, *wr_mode;
	PyObject *f, *p_f[3];

	/* file modes for subsequent fdopen's on pipe handles */
	if (mode == O_TEXT)
	{
		rd_mode = "rt";
		wr_mode = "wt";
	}
	else
	{
		rd_mode = "rb";
		wr_mode = "wb";
	}

	/* prepare shell references */
	if ((shell = getenv("EMXSHELL")) == NULL)
		if ((shell = getenv("COMSPEC")) == NULL)
		{
			errno = ENOENT;
			return srcos_error();
		}

	sh_name = _getname(shell);
	if (stricmp(sh_name, "cmd.exe") == 0 || stricmp(sh_name, "4os2.exe") == 0)
		opt = "/c";
	else
		opt = "-c";

	/* save current stdio fds + their flags, and set not inheritable */
	i = pipe_err = 0;
	while (pipe_err >= 0 && i < 3)
	{
		pipe_err = stdio[i].handle = dup(i);
		stdio[i].flags = fcntl(i, F_GETFD, 0);
		fcntl(stdio[i].handle, F_SETFD, stdio[i].flags | FD_CLOEXEC);
		i++;
	}
	if (pipe_err < 0)
	{
		/* didn't get them all saved - clean up and bail out */
		int saved_err = errno;
		while (i-- > 0)
		{
			close(stdio[i].handle);
		}
		errno = saved_err;
		return srcos_error();
	}

	/* create pipe ends */
	file_count = 2;
	if (n == POPEN_3)
		file_count = 3;
	i = pipe_err = 0;
	while ((pipe_err == 0) && (i < file_count))
		pipe_err = pipe((int *)&p_fd[i++]);
	if (pipe_err < 0)
	{
		/* didn't get them all made - clean up and bail out */
		while (i-- > 0)
		{
			close(p_fd[i].wr);
			close(p_fd[i].rd);
		}
		errno = EPIPE;
		return srcos_error();
	}

	/* change the actual standard IO streams over temporarily,
	 * making the retained pipe ends non-inheritable
	 */
	pipe_err = 0;

	/* - stdin */
	if (dup2(p_fd[0].rd, 0) == 0)
	{
		close(p_fd[0].rd);
		i = fcntl(p_fd[0].wr, F_GETFD, 0);
		fcntl(p_fd[0].wr, F_SETFD, i | FD_CLOEXEC);
		if ((p_s[0] = fdopen(p_fd[0].wr, wr_mode)) == NULL)
		{
			close(p_fd[0].wr);
			pipe_err = -1;
		}
	}
	else
	{
		pipe_err = -1;
	}

	/* - stdout */
	if (pipe_err == 0)
	{
		if (dup2(p_fd[1].wr, 1) == 1)
		{
			close(p_fd[1].wr);
			i = fcntl(p_fd[1].rd, F_GETFD, 0);
			fcntl(p_fd[1].rd, F_SETFD, i | FD_CLOEXEC);
			if ((p_s[1] = fdopen(p_fd[1].rd, rd_mode)) == NULL)
			{
				close(p_fd[1].rd);
				pipe_err = -1;
			}
		}
		else
		{
			pipe_err = -1;
		}
	}

	/* - stderr, as required */
	if (pipe_err == 0)
		switch (n)
		{
			case POPEN_3:
			{
				if (dup2(p_fd[2].wr, 2) == 2)
				{
					close(p_fd[2].wr);
					i = fcntl(p_fd[2].rd, F_GETFD, 0);
					fcntl(p_fd[2].rd, F_SETFD, i | FD_CLOEXEC);
					if ((p_s[2] = fdopen(p_fd[2].rd, rd_mode)) == NULL)
					{
						close(p_fd[2].rd);
						pipe_err = -1;
					}
				}
				else
				{
					pipe_err = -1;
				}
				break;
			}

			case POPEN_4:
			{
				if (dup2(1, 2) != 2)
				{
					pipe_err = -1;
				}
				break;
			}
		}

	/* spawn the child process */
	if (pipe_err == 0)
	{
		pipe_pid = spawnlp(P_NOWAIT, shell, shell, opt, cmdstring, (char *)0);
		if (pipe_pid == -1)
		{
			pipe_err = -1;
		}
		else
		{
			/* save the PID into the FILE structure
			 * NOTE: this implementation doesn't actually
			 * take advantage of this, but do it for
			 * completeness - AIM Apr01
			 */
			for (i = 0; i < file_count; i++)
				p_s[i]->_pid = pipe_pid;
		}
	}

	/* reset standard IO to normal */
	for (i = 0; i < 3; i++)
	{
		dup2(stdio[i].handle, i);
		fcntl(i, F_SETFD, stdio[i].flags);
		close(stdio[i].handle);
	}

	/* if any remnant problems, clean up and bail out */
	if (pipe_err < 0)
	{
		for (i = 0; i < 3; i++)
		{
			close(p_fd[i].rd);
			close(p_fd[i].wr);
		}
		errno = EPIPE;
		return srcos_error_with_filename(cmdstring);
	}

	/* build tuple of file objects to return */
	if ((p_f[0] = PyFile_FromFile(p_s[0], cmdstring, wr_mode, _PyPclose)) != NULL)
		PyFile_SetBufSize(p_f[0], bufsize);
	if ((p_f[1] = PyFile_FromFile(p_s[1], cmdstring, rd_mode, _PyPclose)) != NULL)
		PyFile_SetBufSize(p_f[1], bufsize);
	if (n == POPEN_3)
	{
		if ((p_f[2] = PyFile_FromFile(p_s[2], cmdstring, rd_mode, _PyPclose)) != NULL)
			PyFile_SetBufSize(p_f[0], bufsize);
		f = PyTuple_Pack(3, p_f[0], p_f[1], p_f[2]);
	}
	else
		f = PyTuple_Pack(2, p_f[0], p_f[1]);

	/*
	 * Insert the files we've created into the process dictionary
	 * all referencing the list with the process handle and the
	 * initial number of files (see description below in _PyPclose).
	 * Since if _PyPclose later tried to wait on a process when all
	 * handles weren't closed, it could create a deadlock with the
	 * child, we spend some energy here to try to ensure that we
	 * either insert all file handles into the dictionary or none
	 * at all.  It's a little clumsy with the various popen modes
	 * and variable number of files involved.
	 */
	if (!_PyPopenProcs)
	{
		_PyPopenProcs = PyDict_New();
	}

	if (_PyPopenProcs)
	{
		PyObject *procObj, *pidObj, *intObj, *fileObj[3];
		int ins_rc[3];

		fileObj[0] = fileObj[1] = fileObj[2] = NULL;
		ins_rc[0]  = ins_rc[1]  = ins_rc[2]  = 0;

		procObj = PyList_New(2);
		pidObj = PyInt_FromLong((long) pipe_pid);
		intObj = PyInt_FromLong((long) file_count);

		if (procObj && pidObj && intObj)
		{
			PyList_SetItem(procObj, 0, pidObj);
			PyList_SetItem(procObj, 1, intObj);

			fileObj[0] = PyLong_FromVoidPtr(p_s[0]);
			if (fileObj[0])
			{
			    ins_rc[0] = PyDict_SetItem(_PyPopenProcs,
						       fileObj[0],
						       procObj);
			}
			fileObj[1] = PyLong_FromVoidPtr(p_s[1]);
			if (fileObj[1])
			{
			    ins_rc[1] = PyDict_SetItem(_PyPopenProcs,
						       fileObj[1],
						       procObj);
			}
			if (file_count >= 3)
			{
				fileObj[2] = PyLong_FromVoidPtr(p_s[2]);
				if (fileObj[2])
				{
				    ins_rc[2] = PyDict_SetItem(_PyPopenProcs,
							       fileObj[2],
							       procObj);
				}
			}

			if (ins_rc[0] < 0 || !fileObj[0] ||
			    ins_rc[1] < 0 || (file_count > 1 && !fileObj[1]) ||
			    ins_rc[2] < 0 || (file_count > 2 && !fileObj[2]))
			{
				/* Something failed - remove any dictionary
				 * entries that did make it.
				 */
				if (!ins_rc[0] && fileObj[0])
				{
					PyDict_DelItem(_PyPopenProcs,
							fileObj[0]);
				}
				if (!ins_rc[1] && fileObj[1])
				{
					PyDict_DelItem(_PyPopenProcs,
							fileObj[1]);
				}
				if (!ins_rc[2] && fileObj[2])
				{
					PyDict_DelItem(_PyPopenProcs,
							fileObj[2]);
				}
			}
		}

		/*
		 * Clean up our localized references for the dictionary keys
		 * and value since PyDict_SetItem will Py_INCREF any copies
		 * that got placed in the dictionary.
		 */
		Py_XDECREF(procObj);
		Py_XDECREF(fileObj[0]);
		Py_XDECREF(fileObj[1]);
		Py_XDECREF(fileObj[2]);
	}

	/* Child is launched. */
	return f;
}

/*
 * Wrapper for fclose() to use for popen* files, so we can retrieve the
 * exit code for the child process and return as a result of the close.
 *
 * This function uses the _PyPopenProcs dictionary in order to map the
 * input file pointer to information about the process that was
 * originally created by the popen* call that created the file pointer.
 * The dictionary uses the file pointer as a key (with one entry
 * inserted for each file returned by the original popen* call) and a
 * single list object as the value for all files from a single call.
 * The list object contains the Win32 process handle at [0], and a file
 * count at [1], which is initialized to the total number of file
 * handles using that list.
 *
 * This function closes whichever handle it is passed, and decrements
 * the file count in the dictionary for the process handle pointed to
 * by this file.  On the last close (when the file count reaches zero),
 * this function will wait for the child process and then return its
 * exit code as the result of the close() operation.  This permits the
 * files to be closed in any order - it is always the close() of the
 * final handle that will return the exit code.
 *
 * NOTE: This function is currently called with the GIL released.
 * hence we use the GILState API to manage our state.
 */

static int _PyPclose(FILE *file)
{
	int result;
	int exit_code;
	pid_t pipe_pid;
	PyObject *procObj, *pidObj, *intObj, *fileObj;
	int file_count;
#ifdef WITH_THREAD
	PyGILState_STATE state;
#endif

	/* Close the file handle first, to ensure it can't block the
	 * child from exiting if it's the last handle.
	 */
	result = fclose(file);

#ifdef WITH_THREAD
	state = PyGILState_Ensure();
#endif
	if (_PyPopenProcs)
	{
		if ((fileObj = PyLong_FromVoidPtr(file)) != NULL &&
		    (procObj = PyDict_GetItem(_PyPopenProcs,
					      fileObj)) != NULL &&
		    (pidObj = PyList_GetItem(procObj,0)) != NULL &&
		    (intObj = PyList_GetItem(procObj,1)) != NULL)
		{
			pipe_pid = (int) PyInt_AsLong(pidObj);
			file_count = (int) PyInt_AsLong(intObj);

			if (file_count > 1)
			{
				/* Still other files referencing process */
				file_count--;
				PyList_SetItem(procObj,1,
					       PyInt_FromLong((long) file_count));
			}
			else
			{
				/* Last file for this process */
				if (result != EOF &&
				    waitpid(pipe_pid, &exit_code, 0) == pipe_pid)
				{
					/* extract exit status */
					if (WIFEXITED(exit_code))
					{
						result = WEXITSTATUS(exit_code);
					}
					else
					{
						errno = EPIPE;
						result = -1;
					}
				}
				else
				{
					/* Indicate failure - this will cause the file object
					 * to raise an I/O error and translate the last
					 * error code from errno.  We do have a problem with
					 * last errors that overlap the normal errno table,
					 * but that's a consistent problem with the file object.
					 */
					result = -1;
				}
			}

			/* Remove this file pointer from dictionary */
			PyDict_DelItem(_PyPopenProcs, fileObj);

			if (PyDict_Size(_PyPopenProcs) == 0)
			{
				Py_DECREF(_PyPopenProcs);
				_PyPopenProcs = NULL;
			}

		} /* if object retrieval ok */

		Py_XDECREF(fileObj);
	} /* if _PyPopenProcs */

#ifdef WITH_THREAD
	PyGILState_Release(state);
#endif
	return result;
}

#endif /* PYCC_??? */

#elif defined(MS_WINDOWS)

/*
 * Portable 'popen' replacement for Win32.
 *
 * Written by Bill Tutt <billtut@microsoft.com>.  Minor tweaks
 * and 2.0 integration by Fredrik Lundh <fredrik@pythonware.com>
 * Return code handling by David Bolen <db3l@fitlinxx.com>.
 */

#include <malloc.h>
#include <io.h>
#include <fcntl.h>

/* These tell _PyPopen() wether to return 1, 2, or 3 file objects. */
#define POPEN_1 1
#define POPEN_2 2
#define POPEN_3 3
#define POPEN_4 4

static PyObject *_PyPopen(char *, int, int);
static int _PyPclose(FILE *file);

/*
 * Internal dictionary mapping popen* file pointers to process handles,
 * for use when retrieving the process exit code.  See _PyPclose() below
 * for more information on this dictionary's use.
 */
static PyObject *_PyPopenProcs = NULL;


/* popen that works from a GUI.
 *
 * The result of this function is a pipe (file) connected to the
 * processes stdin or stdout, depending on the requested mode.
 */

static PyObject *
srcos_popen(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}

/* Variation on win32pipe.popen
 *
 * The result of this function is a pipe (file) connected to the
 * process's stdin, and a pipe connected to the process's stdout.
 */

static PyObject *
win32_popen2(PyObject *self, PyObject  *args)
{
	PyObject *f;
	int tm=0;

	char *cmdstring;
	char *mode = "t";
	int bufsize = -1;
	if (!PyArg_ParseTuple(args, "s|si:popen2", &cmdstring, &mode, &bufsize))
		return NULL;

	if (*mode == 't')
		tm = _O_TEXT;
	else if (*mode != 'b') {
		PyErr_SetString(PyExc_ValueError, "popen2() arg 2 must be 't' or 'b'");
		return NULL;
	} else
		tm = _O_BINARY;

	if (bufsize != -1) {
		PyErr_SetString(PyExc_ValueError, "popen2() arg 3 must be -1");
		return NULL;
	}

	f = _PyPopen(cmdstring, tm, POPEN_2);

	return f;
}

/*
 * Variation on <om win32pipe.popen>
 *
 * The result of this function is 3 pipes - the process's stdin,
 * stdout and stderr
 */

static PyObject *
win32_popen3(PyObject *self, PyObject *args)
{
	PyObject *f;
	int tm = 0;

	char *cmdstring;
	char *mode = "t";
	int bufsize = -1;
	if (!PyArg_ParseTuple(args, "s|si:popen3", &cmdstring, &mode, &bufsize))
		return NULL;

	if (*mode == 't')
		tm = _O_TEXT;
	else if (*mode != 'b') {
		PyErr_SetString(PyExc_ValueError, "popen3() arg 2 must be 't' or 'b'");
		return NULL;
	} else
		tm = _O_BINARY;

	if (bufsize != -1) {
		PyErr_SetString(PyExc_ValueError, "popen3() arg 3 must be -1");
		return NULL;
	}

	f = _PyPopen(cmdstring, tm, POPEN_3);

	return f;
}

/*
 * Variation on win32pipe.popen
 *
 * The result of this function is 2 pipes - the processes stdin,
 * and stdout+stderr combined as a single pipe.
 */

static PyObject *
win32_popen4(PyObject *self, PyObject  *args)
{
	PyObject *f;
	int tm = 0;

	char *cmdstring;
	char *mode = "t";
	int bufsize = -1;
	if (!PyArg_ParseTuple(args, "s|si:popen4", &cmdstring, &mode, &bufsize))
		return NULL;

	if (*mode == 't')
		tm = _O_TEXT;
	else if (*mode != 'b') {
		PyErr_SetString(PyExc_ValueError, "popen4() arg 2 must be 't' or 'b'");
		return NULL;
	} else
		tm = _O_BINARY;

	if (bufsize != -1) {
		PyErr_SetString(PyExc_ValueError, "popen4() arg 3 must be -1");
		return NULL;
	}

	f = _PyPopen(cmdstring, tm, POPEN_4);

	return f;
}

static BOOL
_PyPopenCreateProcess(char *cmdstring,
		      HANDLE hStdin,
		      HANDLE hStdout,
		      HANDLE hStderr,
		      HANDLE *hProcess)
{
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;
	DWORD dwProcessFlags = 0;  /* no NEW_CONSOLE by default for Ctrl+C handling */
	char *s1,*s2, *s3 = " /c ";
	const char *szConsoleSpawn = "w9xpopen.exe";
	int i;
	Py_ssize_t x;

	if (i = GetEnvironmentVariable("COMSPEC",NULL,0)) {
		char *comshell;

		s1 = (char *)alloca(i);
		if (!(x = GetEnvironmentVariable("COMSPEC", s1, i)))
			/* x < i, so x fits into an integer */
			return (int)x;

		/* Explicitly check if we are using COMMAND.COM.  If we are
		 * then use the w9xpopen hack.
		 */
		comshell = s1 + x;
		while (comshell >= s1 && *comshell != '\\')
			--comshell;
		++comshell;

		if (GetVersion() < 0x80000000 &&
		    _stricmp(comshell, "command.com") != 0) {
			/* NT/2000 and not using command.com. */
			x = i + strlen(s3) + strlen(cmdstring) + 1;
			s2 = (char *)alloca(x);
			ZeroMemory(s2, x);
			PyOS_snprintf(s2, x, "%s%s%s", s1, s3, cmdstring);
		}
		else {
			/*
			 * Oh gag, we're on Win9x or using COMMAND.COM. Use
			 * the workaround listed in KB: Q150956
			 */
			char modulepath[_MAX_PATH];
			struct stat statinfo;
			GetModuleFileName(NULL, modulepath, sizeof(modulepath));
			for (x = i = 0; modulepath[i]; i++)
				if (modulepath[i] == SEP)
					x = i+1;
			modulepath[x] = '\0';
			/* Create the full-name to w9xpopen, so we can test it exists */
			strncat(modulepath,
			        szConsoleSpawn,
			        (sizeof(modulepath)/sizeof(modulepath[0]))
			               -strlen(modulepath));
			if (stat(modulepath, &statinfo) != 0) {
				size_t mplen = sizeof(modulepath)/sizeof(modulepath[0]);
				/* Eeek - file-not-found - possibly an embedding
				   situation - see if we can locate it in sys.prefix
				*/
				strncpy(modulepath,
				        Py_GetExecPrefix(),
				        mplen);
				modulepath[mplen-1] = '\0';
				if (modulepath[strlen(modulepath)-1] != '\\')
					strcat(modulepath, "\\");
				strncat(modulepath,
				        szConsoleSpawn,
				        mplen-strlen(modulepath));
				/* No where else to look - raise an easily identifiable
				   error, rather than leaving Windows to report
				   "file not found" - as the user is probably blissfully
				   unaware this shim EXE is used, and it will confuse them.
				   (well, it confused me for a while ;-)
				*/
				if (stat(modulepath, &statinfo) != 0) {
					PyErr_Format(PyExc_RuntimeError,
					    "Can not locate '%s' which is needed "
					    "for popen to work with your shell "
					    "or platform.",
					    szConsoleSpawn);
					return FALSE;
				}
			}
			x = i + strlen(s3) + strlen(cmdstring) + 1 +
				strlen(modulepath) +
				strlen(szConsoleSpawn) + 1;

			s2 = (char *)alloca(x);
			ZeroMemory(s2, x);
			/* To maintain correct argument passing semantics,
			   we pass the command-line as it stands, and allow
			   quoting to be applied.  w9xpopen.exe will then
			   use its argv vector, and re-quote the necessary
			   args for the ultimate child process.
			*/
			PyOS_snprintf(
				s2, x,
				"\"%s\" %s%s%s",
				modulepath,
				s1,
				s3,
				cmdstring);
			/* Not passing CREATE_NEW_CONSOLE has been known to
			   cause random failures on win9x.  Specifically a
			   dialog:
			   "Your program accessed mem currently in use at xxx"
			   and a hopeful warning about the stability of your
			   system.
			   Cost is Ctrl+C wont kill children, but anyone
			   who cares can have a go!
			*/
			dwProcessFlags |= CREATE_NEW_CONSOLE;
		}
	}

	/* Could be an else here to try cmd.exe / command.com in the path
	   Now we'll just error out.. */
	else {
		PyErr_SetString(PyExc_RuntimeError,
			"Cannot locate a COMSPEC environment variable to "
			"use as the shell");
		return FALSE;
	}

	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	siStartInfo.hStdInput = hStdin;
	siStartInfo.hStdOutput = hStdout;
	siStartInfo.hStdError = hStderr;
	siStartInfo.wShowWindow = SW_HIDE;

	if (CreateProcess(NULL,
			  s2,
			  NULL,
			  NULL,
			  TRUE,
			  dwProcessFlags,
			  NULL,
			  NULL,
			  &siStartInfo,
			  &piProcInfo) ) {
		/* Close the handles now so anyone waiting is woken. */
		CloseHandle(piProcInfo.hThread);

		/* Return process handle */
		*hProcess = piProcInfo.hProcess;
		return TRUE;
	}
	win32_error("CreateProcess", s2);
	return FALSE;
}

/* The following code is based off of KB: Q190351 */

static PyObject *
_PyPopen(char *cmdstring, int mode, int n)
{
	HANDLE hChildStdinRd, hChildStdinWr, hChildStdoutRd, hChildStdoutWr,
		hChildStderrRd, hChildStderrWr, hChildStdinWrDup, hChildStdoutRdDup,
		hChildStderrRdDup, hProcess; /* hChildStdoutWrDup; */

	SECURITY_ATTRIBUTES saAttr;
	BOOL fSuccess;
	int fd1, fd2, fd3;
	FILE *f1, *f2, *f3;
	long file_count;
	PyObject *f;

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
		return win32_error("CreatePipe", NULL);

	/* Create new output read handle and the input write handle. Set
	 * the inheritance properties to FALSE. Otherwise, the child inherits
	 * these handles; resulting in non-closeable handles to the pipes
	 * being created. */
	 fSuccess = DuplicateHandle(GetCurrentProcess(), hChildStdinWr,
				    GetCurrentProcess(), &hChildStdinWrDup, 0,
				    FALSE,
				    DUPLICATE_SAME_ACCESS);
	 if (!fSuccess)
		 return win32_error("DuplicateHandle", NULL);

	 /* Close the inheritable version of ChildStdin
	that we're using. */
	 CloseHandle(hChildStdinWr);

	 if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
		 return win32_error("CreatePipe", NULL);

	 fSuccess = DuplicateHandle(GetCurrentProcess(), hChildStdoutRd,
				    GetCurrentProcess(), &hChildStdoutRdDup, 0,
				    FALSE, DUPLICATE_SAME_ACCESS);
	 if (!fSuccess)
		 return win32_error("DuplicateHandle", NULL);

	 /* Close the inheritable version of ChildStdout
		that we're using. */
	 CloseHandle(hChildStdoutRd);

	 if (n != POPEN_4) {
		 if (!CreatePipe(&hChildStderrRd, &hChildStderrWr, &saAttr, 0))
			 return win32_error("CreatePipe", NULL);
		 fSuccess = DuplicateHandle(GetCurrentProcess(),
					    hChildStderrRd,
					    GetCurrentProcess(),
					    &hChildStderrRdDup, 0,
					    FALSE, DUPLICATE_SAME_ACCESS);
		 if (!fSuccess)
			 return win32_error("DuplicateHandle", NULL);
		 /* Close the inheritable version of ChildStdErr that we're using. */
		 CloseHandle(hChildStderrRd);
	 }

	 switch (n) {
	 case POPEN_1:
		 switch (mode & (_O_RDONLY | _O_TEXT | _O_BINARY | _O_WRONLY)) {
		 case _O_WRONLY | _O_TEXT:
			 /* Case for writing to child Stdin in text mode. */
			 fd1 = _open_osfhandle((Py_intptr_t)hChildStdinWrDup, mode);
			 f1 = _fdopen(fd1, "w");
			 f = PyFile_FromFile(f1, cmdstring, "w", _PyPclose);
			 PyFile_SetBufSize(f, 0);
			 /* We don't care about these pipes anymore, so close them. */
			 CloseHandle(hChildStdoutRdDup);
			 CloseHandle(hChildStderrRdDup);
			 break;

		 case _O_RDONLY | _O_TEXT:
			 /* Case for reading from child Stdout in text mode. */
			 fd1 = _open_osfhandle((Py_intptr_t)hChildStdoutRdDup, mode);
			 f1 = _fdopen(fd1, "r");
			 f = PyFile_FromFile(f1, cmdstring, "r", _PyPclose);
			 PyFile_SetBufSize(f, 0);
			 /* We don't care about these pipes anymore, so close them. */
			 CloseHandle(hChildStdinWrDup);
			 CloseHandle(hChildStderrRdDup);
			 break;

		 case _O_RDONLY | _O_BINARY:
			 /* Case for readinig from child Stdout in binary mode. */
			 fd1 = _open_osfhandle((Py_intptr_t)hChildStdoutRdDup, mode);
			 f1 = _fdopen(fd1, "rb");
			 f = PyFile_FromFile(f1, cmdstring, "rb", _PyPclose);
			 PyFile_SetBufSize(f, 0);
			 /* We don't care about these pipes anymore, so close them. */
			 CloseHandle(hChildStdinWrDup);
			 CloseHandle(hChildStderrRdDup);
			 break;

		 case _O_WRONLY | _O_BINARY:
			 /* Case for writing to child Stdin in binary mode. */
			 fd1 = _open_osfhandle((Py_intptr_t)hChildStdinWrDup, mode);
			 f1 = _fdopen(fd1, "wb");
			 f = PyFile_FromFile(f1, cmdstring, "wb", _PyPclose);
			 PyFile_SetBufSize(f, 0);
			 /* We don't care about these pipes anymore, so close them. */
			 CloseHandle(hChildStdoutRdDup);
			 CloseHandle(hChildStderrRdDup);
			 break;
		 }
		 file_count = 1;
		 break;

	 case POPEN_2:
	 case POPEN_4:
	 {
		 char *m1, *m2;
		 PyObject *p1, *p2;

		 if (mode & _O_TEXT) {
			 m1 = "r";
			 m2 = "w";
		 } else {
			 m1 = "rb";
			 m2 = "wb";
		 }

		 fd1 = _open_osfhandle((Py_intptr_t)hChildStdinWrDup, mode);
		 f1 = _fdopen(fd1, m2);
		 fd2 = _open_osfhandle((Py_intptr_t)hChildStdoutRdDup, mode);
		 f2 = _fdopen(fd2, m1);
		 p1 = PyFile_FromFile(f1, cmdstring, m2, _PyPclose);
		 PyFile_SetBufSize(p1, 0);
		 p2 = PyFile_FromFile(f2, cmdstring, m1, _PyPclose);
		 PyFile_SetBufSize(p2, 0);

		 if (n != 4)
			 CloseHandle(hChildStderrRdDup);

		 f = PyTuple_Pack(2,p1,p2);
		 Py_XDECREF(p1);
		 Py_XDECREF(p2);
		 file_count = 2;
		 break;
	 }

	 case POPEN_3:
	 {
		 char *m1, *m2;
		 PyObject *p1, *p2, *p3;

		 if (mode & _O_TEXT) {
			 m1 = "r";
			 m2 = "w";
		 } else {
			 m1 = "rb";
			 m2 = "wb";
		 }

		 fd1 = _open_osfhandle((Py_intptr_t)hChildStdinWrDup, mode);
		 f1 = _fdopen(fd1, m2);
		 fd2 = _open_osfhandle((Py_intptr_t)hChildStdoutRdDup, mode);
		 f2 = _fdopen(fd2, m1);
		 fd3 = _open_osfhandle((Py_intptr_t)hChildStderrRdDup, mode);
		 f3 = _fdopen(fd3, m1);
		 p1 = PyFile_FromFile(f1, cmdstring, m2, _PyPclose);
		 p2 = PyFile_FromFile(f2, cmdstring, m1, _PyPclose);
		 p3 = PyFile_FromFile(f3, cmdstring, m1, _PyPclose);
		 PyFile_SetBufSize(p1, 0);
		 PyFile_SetBufSize(p2, 0);
		 PyFile_SetBufSize(p3, 0);
		 f = PyTuple_Pack(3,p1,p2,p3);
		 Py_XDECREF(p1);
		 Py_XDECREF(p2);
		 Py_XDECREF(p3);
		 file_count = 3;
		 break;
	 }
	 }

	 if (n == POPEN_4) {
		 if (!_PyPopenCreateProcess(cmdstring,
					    hChildStdinRd,
					    hChildStdoutWr,
					    hChildStdoutWr,
					    &hProcess))
			 return NULL;
	 }
	 else {
		 if (!_PyPopenCreateProcess(cmdstring,
					    hChildStdinRd,
					    hChildStdoutWr,
					    hChildStderrWr,
					    &hProcess))
			 return NULL;
	 }

	 /*
	  * Insert the files we've created into the process dictionary
	  * all referencing the list with the process handle and the
	  * initial number of files (see description below in _PyPclose).
	  * Since if _PyPclose later tried to wait on a process when all
	  * handles weren't closed, it could create a deadlock with the
	  * child, we spend some energy here to try to ensure that we
	  * either insert all file handles into the dictionary or none
	  * at all.  It's a little clumsy with the various popen modes
	  * and variable number of files involved.
	  */
	 if (!_PyPopenProcs) {
		 _PyPopenProcs = PyDict_New();
	 }

	 if (_PyPopenProcs) {
		 PyObject *procObj, *hProcessObj, *intObj, *fileObj[3];
		 int ins_rc[3];

		 fileObj[0] = fileObj[1] = fileObj[2] = NULL;
		 ins_rc[0]  = ins_rc[1]  = ins_rc[2]  = 0;

		 procObj = PyList_New(2);
		 hProcessObj = PyLong_FromVoidPtr(hProcess);
		 intObj = PyInt_FromLong(file_count);

		 if (procObj && hProcessObj && intObj) {
			 PyList_SetItem(procObj,0,hProcessObj);
			 PyList_SetItem(procObj,1,intObj);

			 fileObj[0] = PyLong_FromVoidPtr(f1);
			 if (fileObj[0]) {
			    ins_rc[0] = PyDict_SetItem(_PyPopenProcs,
						       fileObj[0],
						       procObj);
			 }
			 if (file_count >= 2) {
				 fileObj[1] = PyLong_FromVoidPtr(f2);
				 if (fileObj[1]) {
				    ins_rc[1] = PyDict_SetItem(_PyPopenProcs,
							       fileObj[1],
							       procObj);
				 }
			 }
			 if (file_count >= 3) {
				 fileObj[2] = PyLong_FromVoidPtr(f3);
				 if (fileObj[2]) {
				    ins_rc[2] = PyDict_SetItem(_PyPopenProcs,
							       fileObj[2],
							       procObj);
				 }
			 }

			 if (ins_rc[0] < 0 || !fileObj[0] ||
			     ins_rc[1] < 0 || (file_count > 1 && !fileObj[1]) ||
			     ins_rc[2] < 0 || (file_count > 2 && !fileObj[2])) {
				 /* Something failed - remove any dictionary
				  * entries that did make it.
				  */
				 if (!ins_rc[0] && fileObj[0]) {
					 PyDict_DelItem(_PyPopenProcs,
							fileObj[0]);
				 }
				 if (!ins_rc[1] && fileObj[1]) {
					 PyDict_DelItem(_PyPopenProcs,
							fileObj[1]);
				 }
				 if (!ins_rc[2] && fileObj[2]) {
					 PyDict_DelItem(_PyPopenProcs,
							fileObj[2]);
				 }
			 }
		 }

		 /*
		  * Clean up our localized references for the dictionary keys
		  * and value since PyDict_SetItem will Py_INCREF any copies
		  * that got placed in the dictionary.
		  */
		 Py_XDECREF(procObj);
		 Py_XDECREF(fileObj[0]);
		 Py_XDECREF(fileObj[1]);
		 Py_XDECREF(fileObj[2]);
	 }

	 /* Child is launched. Close the parents copy of those pipe
	  * handles that only the child should have open.  You need to
	  * make sure that no handles to the write end of the output pipe
	  * are maintained in this process or else the pipe will not close
	  * when the child process exits and the ReadFile will hang. */

	 if (!CloseHandle(hChildStdinRd))
		 return win32_error("CloseHandle", NULL);

	 if (!CloseHandle(hChildStdoutWr))
		 return win32_error("CloseHandle", NULL);

	 if ((n != 4) && (!CloseHandle(hChildStderrWr)))
		 return win32_error("CloseHandle", NULL);

	 return f;
}

/*
 * Wrapper for fclose() to use for popen* files, so we can retrieve the
 * exit code for the child process and return as a result of the close.
 *
 * This function uses the _PyPopenProcs dictionary in order to map the
 * input file pointer to information about the process that was
 * originally created by the popen* call that created the file pointer.
 * The dictionary uses the file pointer as a key (with one entry
 * inserted for each file returned by the original popen* call) and a
 * single list object as the value for all files from a single call.
 * The list object contains the Win32 process handle at [0], and a file
 * count at [1], which is initialized to the total number of file
 * handles using that list.
 *
 * This function closes whichever handle it is passed, and decrements
 * the file count in the dictionary for the process handle pointed to
 * by this file.  On the last close (when the file count reaches zero),
 * this function will wait for the child process and then return its
 * exit code as the result of the close() operation.  This permits the
 * files to be closed in any order - it is always the close() of the
 * final handle that will return the exit code.
 *
 * NOTE: This function is currently called with the GIL released.
 * hence we use the GILState API to manage our state.
 */

static int _PyPclose(FILE *file)
{
	int result;
	DWORD exit_code;
	HANDLE hProcess;
	PyObject *procObj, *hProcessObj, *intObj, *fileObj;
	long file_count;
#ifdef WITH_THREAD
	PyGILState_STATE state;
#endif

	/* Close the file handle first, to ensure it can't block the
	 * child from exiting if it's the last handle.
	 */
	result = fclose(file);
#ifdef WITH_THREAD
	state = PyGILState_Ensure();
#endif
	if (_PyPopenProcs) {
		if ((fileObj = PyLong_FromVoidPtr(file)) != NULL &&
		    (procObj = PyDict_GetItem(_PyPopenProcs,
					      fileObj)) != NULL &&
		    (hProcessObj = PyList_GetItem(procObj,0)) != NULL &&
		    (intObj = PyList_GetItem(procObj,1)) != NULL) {

			hProcess = PyLong_AsVoidPtr(hProcessObj);
			file_count = PyInt_AsLong(intObj);

			if (file_count > 1) {
				/* Still other files referencing process */
				file_count--;
				PyList_SetItem(procObj,1,
					       PyInt_FromLong(file_count));
			} else {
				/* Last file for this process */
				if (result != EOF &&
				    WaitForSingleObject(hProcess, INFINITE) != WAIT_FAILED &&
				    GetExitCodeProcess(hProcess, &exit_code)) {
					/* Possible truncation here in 16-bit environments, but
					 * real exit codes are just the lower byte in any event.
					 */
					result = exit_code;
				} else {
					/* Indicate failure - this will cause the file object
					 * to raise an I/O error and translate the last Win32
					 * error code from errno.  We do have a problem with
					 * last errors that overlap the normal errno table,
					 * but that's a consistent problem with the file object.
					 */
					if (result != EOF) {
						/* If the error wasn't from the fclose(), then
						 * set errno for the file object error handling.
						 */
						errno = GetLastError();
					}
					result = -1;
				}

				/* Free up the native handle at this point */
				CloseHandle(hProcess);
			}

			/* Remove this file pointer from dictionary */
			PyDict_DelItem(_PyPopenProcs, fileObj);

			if (PyDict_Size(_PyPopenProcs) == 0) {
				Py_DECREF(_PyPopenProcs);
				_PyPopenProcs = NULL;
			}

		} /* if object retrieval ok */

		Py_XDECREF(fileObj);
	} /* if _PyPopenProcs */

#ifdef WITH_THREAD
	PyGILState_Release(state);
#endif
	return result;
}

#else /* which OS? */
static PyObject *
srcos_popen(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}

#endif /* PYOS_??? */
#endif /* HAVE_POPEN */


#ifdef HAVE_SETUID
PyDoc_STRVAR(srcos_setuid__doc__,
"setuid(uid)\n\n\
Set the current process's user id.");

static PyObject *
srcos_setuid(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_SETUID */


#ifdef HAVE_SETEUID
PyDoc_STRVAR(srcos_seteuid__doc__,
"seteuid(uid)\n\n\
Set the current process's effective user id.");

static PyObject *
srcos_seteuid (PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_SETEUID */

#ifdef HAVE_SETEGID
PyDoc_STRVAR(srcos_setegid__doc__,
"setegid(gid)\n\n\
Set the current process's effective group id.");

static PyObject *
srcos_setegid (PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_SETEGID */

#ifdef HAVE_SETREUID
PyDoc_STRVAR(srcos_setreuid__doc__,
"setreuid(ruid, euid)\n\n\
Set the current process's real and effective user ids.");

static PyObject *
srcos_setreuid (PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_SETREUID */

#ifdef HAVE_SETREGID
PyDoc_STRVAR(srcos_setregid__doc__,
"setregid(rgid, egid)\n\n\
Set the current process's real and effective group ids.");

static PyObject *
srcos_setregid (PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_SETREGID */

#ifdef HAVE_SETGID
PyDoc_STRVAR(srcos_setgid__doc__,
"setgid(gid)\n\n\
Set the current process's group id.");

static PyObject *
srcos_setgid(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_SETGID */

#ifdef HAVE_SETGROUPS
PyDoc_STRVAR(srcos_setgroups__doc__,
"setgroups(list)\n\n\
Set the groups of the current process to list.");

static PyObject *
srcos_setgroups(PyObject *self, PyObject *groups)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_SETGROUPS */

#if defined(HAVE_WAIT3) || defined(HAVE_WAIT4)
static PyObject *
wait_helper(pid_t pid, int status, struct rusage *ru)
{
	PyObject *result;
   	static PyObject *struct_rusage;

	if (pid == -1)
		return srcos_error();

	if (struct_rusage == NULL) {
		PyObject *m = PyImport_ImportModuleNoBlock("resource");
		if (m == NULL)
			return NULL;
		struct_rusage = PyObject_GetAttrString(m, "struct_rusage");
		Py_DECREF(m);
		if (struct_rusage == NULL)
			return NULL;
	}

	/* XXX(nnorwitz): Copied (w/mods) from resource.c, there should be only one. */
	result = PyStructSequence_New((PyTypeObject*) struct_rusage);
	if (!result)
		return NULL;

#ifndef doubletime
#define doubletime(TV) ((double)(TV).tv_sec + (TV).tv_usec * 0.000001)
#endif

	PyStructSequence_SET_ITEM(result, 0,
			PyFloat_FromDouble(doubletime(ru->ru_utime)));
	PyStructSequence_SET_ITEM(result, 1,
			PyFloat_FromDouble(doubletime(ru->ru_stime)));
#define SET_INT(result, index, value)\
		PyStructSequence_SET_ITEM(result, index, PyInt_FromLong(value))
	SET_INT(result, 2, ru->ru_maxrss);
	SET_INT(result, 3, ru->ru_ixrss);
	SET_INT(result, 4, ru->ru_idrss);
	SET_INT(result, 5, ru->ru_isrss);
	SET_INT(result, 6, ru->ru_minflt);
	SET_INT(result, 7, ru->ru_majflt);
	SET_INT(result, 8, ru->ru_nswap);
	SET_INT(result, 9, ru->ru_inblock);
	SET_INT(result, 10, ru->ru_oublock);
	SET_INT(result, 11, ru->ru_msgsnd);
	SET_INT(result, 12, ru->ru_msgrcv);
	SET_INT(result, 13, ru->ru_nsignals);
	SET_INT(result, 14, ru->ru_nvcsw);
	SET_INT(result, 15, ru->ru_nivcsw);
#undef SET_INT

	if (PyErr_Occurred()) {
		Py_DECREF(result);
		return NULL;
	}

	return Py_BuildValue("iiN", pid, status, result);
}
#endif /* HAVE_WAIT3 || HAVE_WAIT4 */

#ifdef HAVE_WAIT3
PyDoc_STRVAR(srcos_wait3__doc__,
"wait3(options) -> (pid, status, rusage)\n\n\
Wait for completion of a child process.");

static PyObject *
srcos_wait3(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_WAIT3 */

#ifdef HAVE_WAIT4
PyDoc_STRVAR(srcos_wait4__doc__,
"wait4(pid, options) -> (pid, status, rusage)\n\n\
Wait for completion of a given child process.");

static PyObject *
srcos_wait4(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_WAIT4 */

#ifdef HAVE_WAITPID
PyDoc_STRVAR(srcos_waitpid__doc__,
"waitpid(pid, options) -> (pid, status)\n\n\
Wait for completion of a given child process.");

static PyObject *
srcos_waitpid(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}

#elif defined(HAVE_CWAIT)

/* MS C has a variant of waitpid() that's usable for most purposes. */
PyDoc_STRVAR(srcos_waitpid__doc__,
"waitpid(pid, options) -> (pid, status << 8)\n\n"
"Wait for completion of a given process.  options is ignored on Windows.");

static PyObject *
srcos_waitpid(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_WAITPID || HAVE_CWAIT */

#ifdef HAVE_WAIT
PyDoc_STRVAR(srcos_wait__doc__,
"wait() -> (pid, status)\n\n\
Wait for completion of a child process.");

static PyObject *
srcos_wait(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif


PyDoc_STRVAR(srcos_lstat__doc__,
"lstat(path) -> stat result\n\n\
Like stat(path), but do not follow symbolic links.");

static PyObject *
srcos_lstat(PyObject *self, PyObject *args)
{
#ifdef HAVE_LSTAT
    return srcos_do_stat(self, args, "et:lstat", lstat, NULL, NULL);
#else /* !HAVE_LSTAT */
#ifdef MS_WINDOWS
    return srcos_do_stat(self, args, "et:lstat", STAT, "U:lstat", win32_wstat);
#else
    return srcos_do_stat(self, args, "et:lstat", STAT, NULL, NULL);
#endif
#endif /* !HAVE_LSTAT */
}


#ifdef HAVE_READLINK
PyDoc_STRVAR(srcos_readlink__doc__,
"readlink(path) -> path\n\n\
Return a string representing the path to which the symbolic link points.");

static PyObject *
srcos_readlink(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_READLINK */


#ifdef HAVE_SYMLINK
PyDoc_STRVAR(srcos_symlink__doc__,
"symlink(src, dst)\n\n\
Create a symbolic link pointing to src named dst.");

static PyObject *
srcos_symlink(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_SYMLINK */


#ifdef HAVE_TIMES
#if defined(PYCC_VACPP) && defined(PYOS_OS2)
static long
system_uptime(void)
{
    ULONG     value = 0;

    Py_BEGIN_ALLOW_THREADS
    DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &value, sizeof(value));
    Py_END_ALLOW_THREADS

    return value;
}

static PyObject *
srcos_times(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#else /* not OS2 */
#define NEED_TICKS_PER_SECOND
static long ticks_per_second = -1;
static PyObject *
srcos_times(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* not OS2 */
#endif /* HAVE_TIMES */


#ifdef MS_WINDOWS
#define HAVE_TIMES	/* so the method table will pick it up */
static PyObject *
srcos_times(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* MS_WINDOWS */

#ifdef HAVE_TIMES
PyDoc_STRVAR(srcos_times__doc__,
"times() -> (utime, stime, cutime, cstime, elapsed_time)\n\n\
Return a tuple of floating point numbers indicating process times.");
#endif


#ifdef HAVE_GETSID
PyDoc_STRVAR(srcos_getsid__doc__,
"getsid(pid) -> sid\n\n\
Call the system call getsid().");

static PyObject *
srcos_getsid(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_GETSID */


#ifdef HAVE_SETSID
PyDoc_STRVAR(srcos_setsid__doc__,
"setsid()\n\n\
Call the system call setsid().");

static PyObject *
srcos_setsid(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_SETSID */

#ifdef HAVE_SETPGID
PyDoc_STRVAR(srcos_setpgid__doc__,
"setpgid(pid, pgrp)\n\n\
Call the system call setpgid().");

static PyObject *
srcos_setpgid(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_SETPGID */


#ifdef HAVE_TCGETPGRP
PyDoc_STRVAR(srcos_tcgetpgrp__doc__,
"tcgetpgrp(fd) -> pgid\n\n\
Return the process group associated with the terminal given by a fd.");

static PyObject *
srcos_tcgetpgrp(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_TCGETPGRP */


#ifdef HAVE_TCSETPGRP
PyDoc_STRVAR(srcos_tcsetpgrp__doc__,
"tcsetpgrp(fd, pgid)\n\n\
Set the process group associated with the terminal given by a fd.");

static PyObject *
srcos_tcsetpgrp(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_TCSETPGRP */

/* Functions acting on file descriptors */

PyDoc_STRVAR(srcos_open__doc__,
"open(filename, flag [, mode=0777]) -> fd\n\n\
Open a file (for low level IO).");

static PyObject *
srcos_open(PyObject *self, PyObject *args)
{
    char *file = NULL;
	char fullPath[_MAX_PATH];
    int flag;
    int mode = 0777;
    int fd;

#if 0
#ifdef MS_WINDOWS
    PyUnicodeObject *po;
    if (PyArg_ParseTuple(args, "Ui|i:mkdir", &po, &flag, &mode)) {
        Py_BEGIN_ALLOW_THREADS
        /* PyUnicode_AS_UNICODE OK without thread
           lock as it is a simple dereference. */
        fd = _wopen(PyUnicode_AS_UNICODE(po), flag, mode);
        Py_END_ALLOW_THREADS
        if (fd < 0)
            return srcos_error();
        return PyInt_FromLong((long)fd);
    }
    /* Drop the argument parsing error as narrow strings
       are also valid. */
    PyErr_Clear();
#endif
#endif // 0

    if (!PyArg_ParseTuple(args, "eti|i",
                          Py_FileSystemDefaultEncoding, &file,
                          &flag, &mode))
        return NULL;

	if( SrcPyGetFullPathSilent(file, fullPath, _MAX_PATH) == 0 ) {
		srcos_error_outsidemodfolder();
		return NULL;
	}

    Py_BEGIN_ALLOW_THREADS
    fd = open(fullPath, flag, mode);
    Py_END_ALLOW_THREADS
    if (fd < 0)
        return srcos_error_with_allocated_filename(file);
    PyMem_Free(file);
    return PyInt_FromLong((long)fd);
}


PyDoc_STRVAR(srcos_close__doc__,
"close(fd)\n\n\
Close a file descriptor (for low level IO).");

static PyObject *
srcos_close(PyObject *self, PyObject *args)
{
    int fd, res;
    if (!PyArg_ParseTuple(args, "i:close", &fd))
        return NULL;
    if (!_PyVerify_fd(fd))
        return srcos_error();
    Py_BEGIN_ALLOW_THREADS
    res = close(fd);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return srcos_error();
    Py_INCREF(Py_None);
    return Py_None;
}


PyDoc_STRVAR(srcos_closerange__doc__, 
"closerange(fd_low, fd_high)\n\n\
Closes all file descriptors in [fd_low, fd_high), ignoring errors.");

static PyObject *
srcos_closerange(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}


PyDoc_STRVAR(srcos_dup__doc__,
"dup(fd) -> fd2\n\n\
Return a duplicate of a file descriptor.");

static PyObject *
srcos_dup(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}


PyDoc_STRVAR(srcos_dup2__doc__,
"dup2(old_fd, new_fd)\n\n\
Duplicate file descriptor.");

static PyObject *
srcos_dup2(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}


PyDoc_STRVAR(srcos_lseek__doc__,
"lseek(fd, pos, how) -> newpos\n\n\
Set the current position of a file descriptor.");

static PyObject *
srcos_lseek(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}


PyDoc_STRVAR(srcos_read__doc__,
"read(fd, buffersize) -> string\n\n\
Read a file descriptor.");

static PyObject *
srcos_read(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}


PyDoc_STRVAR(srcos_write__doc__,
"write(fd, string) -> byteswritten\n\n\
Write a string to a file descriptor.");

static PyObject *
srcos_write(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}


PyDoc_STRVAR(srcos_fstat__doc__,
"fstat(fd) -> stat result\n\n\
Like stat(), but for an open file descriptor.");

static PyObject *
srcos_fstat(PyObject *self, PyObject *args)
{
    int fd;
    STRUCT_STAT st;
    int res;
    if (!PyArg_ParseTuple(args, "i:fstat", &fd))
        return NULL;
#ifdef __VMS
    /* on OpenVMS we must ensure that all bytes are written to the file */
    fsync(fd);
#endif
    if (!_PyVerify_fd(fd))
        return srcos_error();
    Py_BEGIN_ALLOW_THREADS
    res = FSTAT(fd, &st);
    Py_END_ALLOW_THREADS
    if (res != 0) {
#ifdef MS_WINDOWS
        return win32_error("fstat", NULL);
#else
        return srcos_error();
#endif
    }

    return _pystat_fromstructstat(&st);
}


PyDoc_STRVAR(srcos_fdopen__doc__,
"fdopen(fd [, mode='r' [, bufsize]]) -> file_object\n\n\
Return an open file object connected to a file descriptor.");

static PyObject *
srcos_fdopen(PyObject *self, PyObject *args)
{
	int fd;
	char *orgmode = "r";
	int bufsize = -1;
	FILE *fp;
	PyObject *f;
	char *mode;
	if (!PyArg_ParseTuple(args, "i|si", &fd, &orgmode, &bufsize))
		return NULL;

	/* Sanitize mode.  See fileobject.c */
	mode = (char *)PyMem_MALLOC(strlen(orgmode)+3);
	if (!mode) {
		PyErr_NoMemory();
		return NULL;
	}
	strcpy(mode, orgmode);
	if (_PyFile_SanitizeMode(mode)) {
		PyMem_FREE(mode);
		return NULL;
	}
	Py_BEGIN_ALLOW_THREADS
#if !defined(MS_WINDOWS) && defined(HAVE_FCNTL_H)
		if (mode[0] == 'a') {
			/* try to make sure the O_APPEND flag is set */
			int flags;
			flags = fcntl(fd, F_GETFL);
			if (flags != -1)
				fcntl(fd, F_SETFL, flags | O_APPEND);
			fp = fdopen(fd, mode);
			if (fp == NULL && flags != -1)
				/* restore old mode if fdopen failed */
				fcntl(fd, F_SETFL, flags);
		} else {
			fp = fdopen(fd, mode);
		}
#else
		fp = fdopen(fd, mode);
#endif
	Py_END_ALLOW_THREADS
		PyMem_FREE(mode);
	if (fp == NULL)
		return srcos_error();
	f = PyFile_FromFile(fp, "<fdopen>", orgmode, fclose);
	if (f != NULL)
		PyFile_SetBufSize(f, bufsize);
	return f;
}

PyDoc_STRVAR(srcos_isatty__doc__,
"isatty(fd) -> bool\n\n\
Return True if the file descriptor 'fd' is an open file descriptor\n\
connected to the slave end of a terminal.");

static PyObject *
srcos_isatty(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}

#ifdef HAVE_PIPE
PyDoc_STRVAR(srcos_pipe__doc__,
"pipe() -> (read_end, write_end)\n\n\
Create a pipe.");

static PyObject *
srcos_pipe(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif  /* HAVE_PIPE */


#ifdef HAVE_MKFIFO
PyDoc_STRVAR(srcos_mkfifo__doc__,
"mkfifo(filename [, mode=0666])\n\n\
Create a FIFO (a POSIX named pipe).");

static PyObject *
srcos_mkfifo(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif


#if defined(HAVE_MKNOD) && defined(HAVE_MAKEDEV)
PyDoc_STRVAR(srcos_mknod__doc__,
"mknod(filename [, mode=0600, device])\n\n\
Create a filesystem node (file, device special file or named pipe)\n\
named filename. mode specifies both the permissions to use and the\n\
type of node to be created, being combined (bitwise OR) with one of\n\
S_IFREG, S_IFCHR, S_IFBLK, and S_IFIFO. For S_IFCHR and S_IFBLK,\n\
device defines the newly created device special file (probably using\n\
os.makedev()), otherwise it is ignored.");


static PyObject *
srcos_mknod(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif

#ifdef HAVE_DEVICE_MACROS
PyDoc_STRVAR(srcos_major__doc__,
"major(device) -> major number\n\
Extracts a device major number from a raw device number.");

static PyObject *
srcos_major(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}

PyDoc_STRVAR(srcos_minor__doc__,
"minor(device) -> minor number\n\
Extracts a device minor number from a raw device number.");

static PyObject *
srcos_minor(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}

PyDoc_STRVAR(srcos_makedev__doc__,
"makedev(major, minor) -> device number\n\
Composes a raw device number from the major and minor device numbers.");

static PyObject *
srcos_makedev(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* device macros */


#ifdef HAVE_FTRUNCATE
PyDoc_STRVAR(srcos_ftruncate__doc__,
"ftruncate(fd, length)\n\n\
Truncate a file to a specified length.");

static PyObject *
srcos_ftruncate(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif

#ifdef HAVE_PUTENV
PyDoc_STRVAR(srcos_putenv__doc__,
"putenv(key, value)\n\n\
Change or add an environment variable.");

/* Save putenv() parameters as values here, so we can collect them when they
 * get re-set with another call for the same key. */
static PyObject *srcos_putenv_garbage;

static PyObject *
srcos_putenv(PyObject *self, PyObject *args)
{
        char *s1, *s2;
        char *newenv;
	PyObject *newstr;
	size_t len;

	if (!PyArg_ParseTuple(args, "ss:putenv", &s1, &s2))
		return NULL;

#if defined(PYOS_OS2)
    if (stricmp(s1, "BEGINLIBPATH") == 0) {
        APIRET rc;

        rc = DosSetExtLIBPATH(s2, BEGIN_LIBPATH);
        if (rc != NO_ERROR)
            return os2_error(rc);

    } else if (stricmp(s1, "ENDLIBPATH") == 0) {
        APIRET rc;

        rc = DosSetExtLIBPATH(s2, END_LIBPATH);
        if (rc != NO_ERROR)
            return os2_error(rc);
    } else {
#endif

	/* XXX This can leak memory -- not easy to fix :-( */
	len = strlen(s1) + strlen(s2) + 2;
	/* len includes space for a trailing \0; the size arg to
	   PyString_FromStringAndSize does not count that */
	newstr = PyString_FromStringAndSize(NULL, (int)len - 1);
	if (newstr == NULL)
		return PyErr_NoMemory();
	newenv = PyString_AS_STRING(newstr);
	PyOS_snprintf(newenv, len, "%s=%s", s1, s2);
	if (putenv(newenv)) {
                Py_DECREF(newstr);
                srcos_error();
                return NULL;
	}
	/* Install the first arg and newstr in posix_putenv_garbage;
	 * this will cause previous value to be collected.  This has to
	 * happen after the real putenv() call because the old value
	 * was still accessible until then. */
	if (PyDict_SetItem(srcos_putenv_garbage,
			   PyTuple_GET_ITEM(args, 0), newstr)) {
		/* really not much we can do; just leak */
		PyErr_Clear();
	}
	else {
		Py_DECREF(newstr);
	}

#if defined(PYOS_OS2)
    }
#endif
	Py_INCREF(Py_None);
        return Py_None;
}
#endif /* putenv */

#ifdef HAVE_UNSETENV
PyDoc_STRVAR(srcos_unsetenv__doc__,
"unsetenv(key)\n\n\
Delete an environment variable.");

static PyObject *
srcos_unsetenv(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* unsetenv */

PyDoc_STRVAR(srcos_strerror__doc__,
"strerror(code) -> string\n\n\
Translate an error code to a message string.");

static PyObject *
srcos_strerror(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}


#ifdef HAVE_SYS_WAIT_H

#ifdef WCOREDUMP
PyDoc_STRVAR(srcos_WCOREDUMP__doc__,
"WCOREDUMP(status) -> bool\n\n\
Return True if the process returning 'status' was dumped to a core file.");

static PyObject *
srcos_WCOREDUMP(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* WCOREDUMP */

#ifdef WIFCONTINUED
PyDoc_STRVAR(srcos_WIFCONTINUED__doc__,
"WIFCONTINUED(status) -> bool\n\n\
Return True if the process returning 'status' was continued from a\n\
job control stop.");

static PyObject *
srcos_WIFCONTINUED(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* WIFCONTINUED */

#ifdef WIFSTOPPED
PyDoc_STRVAR(srcos_WIFSTOPPED__doc__,
"WIFSTOPPED(status) -> bool\n\n\
Return True if the process returning 'status' was stopped.");

static PyObject *
srcos_WIFSTOPPED(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* WIFSTOPPED */

#ifdef WIFSIGNALED
PyDoc_STRVAR(srcos_WIFSIGNALED__doc__,
"WIFSIGNALED(status) -> bool\n\n\
Return True if the process returning 'status' was terminated by a signal.");

static PyObject *
srcos_WIFSIGNALED(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* WIFSIGNALED */

#ifdef WIFEXITED
PyDoc_STRVAR(srcos_WIFEXITED__doc__,
"WIFEXITED(status) -> bool\n\n\
Return true if the process returning 'status' exited using the exit()\n\
system call.");

static PyObject *
srcos_WIFEXITED(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* WIFEXITED */

#ifdef WEXITSTATUS
PyDoc_STRVAR(srcos_WEXITSTATUS__doc__,
"WEXITSTATUS(status) -> integer\n\n\
Return the process return code from 'status'.");

static PyObject *
srcos_WEXITSTATUS(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* WEXITSTATUS */

#ifdef WTERMSIG
PyDoc_STRVAR(srcos_WTERMSIG__doc__,
"WTERMSIG(status) -> integer\n\n\
Return the signal that terminated the process that provided the 'status'\n\
value.");

static PyObject *
srcos_WTERMSIG(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* WTERMSIG */

#ifdef WSTOPSIG
PyDoc_STRVAR(srcos_WSTOPSIG__doc__,
"WSTOPSIG(status) -> integer\n\n\
Return the signal that stopped the process that provided\n\
the 'status' value.");

static PyObject *
srcos_WSTOPSIG(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* WSTOPSIG */

#endif /* HAVE_SYS_WAIT_H */


#if defined(HAVE_FSTATVFS) && defined(HAVE_SYS_STATVFS_H)
#ifdef _SCO_DS
/* SCO OpenServer 5.0 and later requires _SVID3 before it reveals the
   needed definitions in sys/statvfs.h */
#define _SVID3
#endif
#include <sys/statvfs.h>

static PyObject*
_pystatvfs_fromstructstatvfs(struct statvfs st) {
        PyObject *v = PyStructSequence_New(&StatVFSResultType);
	if (v == NULL)
		return NULL;

#if !defined(HAVE_LARGEFILE_SUPPORT)
        PyStructSequence_SET_ITEM(v, 0, PyInt_FromLong((long) st.f_bsize));
        PyStructSequence_SET_ITEM(v, 1, PyInt_FromLong((long) st.f_frsize));
        PyStructSequence_SET_ITEM(v, 2, PyInt_FromLong((long) st.f_blocks));
        PyStructSequence_SET_ITEM(v, 3, PyInt_FromLong((long) st.f_bfree));
        PyStructSequence_SET_ITEM(v, 4, PyInt_FromLong((long) st.f_bavail));
        PyStructSequence_SET_ITEM(v, 5, PyInt_FromLong((long) st.f_files));
        PyStructSequence_SET_ITEM(v, 6, PyInt_FromLong((long) st.f_ffree));
        PyStructSequence_SET_ITEM(v, 7, PyInt_FromLong((long) st.f_favail));
        PyStructSequence_SET_ITEM(v, 8, PyInt_FromLong((long) st.f_flag));
        PyStructSequence_SET_ITEM(v, 9, PyInt_FromLong((long) st.f_namemax));
#else
        PyStructSequence_SET_ITEM(v, 0, PyInt_FromLong((long) st.f_bsize));
        PyStructSequence_SET_ITEM(v, 1, PyInt_FromLong((long) st.f_frsize));
        PyStructSequence_SET_ITEM(v, 2,
			       PyLong_FromLongLong((PY_LONG_LONG) st.f_blocks));
        PyStructSequence_SET_ITEM(v, 3,
			       PyLong_FromLongLong((PY_LONG_LONG) st.f_bfree));
        PyStructSequence_SET_ITEM(v, 4,
			       PyLong_FromLongLong((PY_LONG_LONG) st.f_bavail));
        PyStructSequence_SET_ITEM(v, 5,
			       PyLong_FromLongLong((PY_LONG_LONG) st.f_files));
        PyStructSequence_SET_ITEM(v, 6,
			       PyLong_FromLongLong((PY_LONG_LONG) st.f_ffree));
        PyStructSequence_SET_ITEM(v, 7,
			       PyLong_FromLongLong((PY_LONG_LONG) st.f_favail));
        PyStructSequence_SET_ITEM(v, 8, PyInt_FromLong((long) st.f_flag));
        PyStructSequence_SET_ITEM(v, 9, PyInt_FromLong((long) st.f_namemax));
#endif

        return v;
}

PyDoc_STRVAR(srcos_fstatvfs__doc__,
"fstatvfs(fd) -> statvfs result\n\n\
Perform an fstatvfs system call on the given fd.");

static PyObject *
srcos_fstatvfs(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_FSTATVFS && HAVE_SYS_STATVFS_H */


#if defined(HAVE_STATVFS) && defined(HAVE_SYS_STATVFS_H)
#include <sys/statvfs.h>

PyDoc_STRVAR(srcos_statvfs__doc__,
"statvfs(path) -> statvfs result\n\n\
Perform a statvfs system call on the given path.");

static PyObject *
srcos_statvfs(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif /* HAVE_STATVFS */


#ifdef HAVE_TEMPNAM
PyDoc_STRVAR(srcos_tempnam__doc__,
"tempnam([dir[, prefix]]) -> string\n\n\
Return a unique name for a temporary file.\n\
The directory and a prefix may be specified as strings; they may be omitted\n\
or None if not needed.");

static PyObject *
srcos_tempnam(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif


#ifdef HAVE_TMPFILE
PyDoc_STRVAR(srcos_tmpfile__doc__,
"tmpfile() -> file object\n\n\
Create a temporary file with no directory entries.");

static PyObject *
srcos_tmpfile(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif


#ifdef HAVE_TMPNAM
PyDoc_STRVAR(srcos_tmpnam__doc__,
"tmpnam() -> string\n\n\
Return a unique name for a temporary file.");

static PyObject *
srcos_tmpnam(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif


/* This is used for fpathconf(), pathconf(), confstr() and sysconf().
 * It maps strings representing configuration variable names to
 * integer values, allowing those functions to be called with the
 * magic names instead of polluting the module's namespace with tons of
 * rarely-used constants.  There are three separate tables that use
 * these definitions.
 *
 * This code is always included, even if none of the interfaces that
 * need it are included.  The #if hackery needed to avoid it would be
 * sufficiently pervasive that it's not worth the loss of readability.
 */
struct constdef {
    char *name;
    long value;
};

static int
conv_confname(PyObject *arg, int *valuep, struct constdef *table,
              size_t tablesize)
{
    if (PyInt_Check(arg)) {
    *valuep = PyInt_AS_LONG(arg);
    return 1;
    }
    if (PyString_Check(arg)) {
    /* look up the value in the table using a binary search */
    size_t lo = 0;
        size_t mid;
    size_t hi = tablesize;
    int cmp;
    char *confname = PyString_AS_STRING(arg);
    while (lo < hi) {
        mid = (lo + hi) / 2;
        cmp = strcmp(confname, table[mid].name);
        if (cmp < 0)
        hi = mid;
        else if (cmp > 0)
        lo = mid + 1;
        else {
        *valuep = table[mid].value;
        return 1;
        }
    }
    PyErr_SetString(PyExc_ValueError, "unrecognized configuration name");
    }
    else
    PyErr_SetString(PyExc_TypeError,
                    "configuration names must be strings or integers");
    return 0;
}


#if defined(HAVE_FPATHCONF) || defined(HAVE_PATHCONF)
static struct constdef  srcos_constants_pathconf[] = {
#ifdef _PC_ABI_AIO_XFER_MAX
    {"PC_ABI_AIO_XFER_MAX",     _PC_ABI_AIO_XFER_MAX},
#endif
#ifdef _PC_ABI_ASYNC_IO
    {"PC_ABI_ASYNC_IO", _PC_ABI_ASYNC_IO},
#endif
#ifdef _PC_ASYNC_IO
    {"PC_ASYNC_IO",     _PC_ASYNC_IO},
#endif
#ifdef _PC_CHOWN_RESTRICTED
    {"PC_CHOWN_RESTRICTED",     _PC_CHOWN_RESTRICTED},
#endif
#ifdef _PC_FILESIZEBITS
    {"PC_FILESIZEBITS", _PC_FILESIZEBITS},
#endif
#ifdef _PC_LAST
    {"PC_LAST", _PC_LAST},
#endif
#ifdef _PC_LINK_MAX
    {"PC_LINK_MAX",     _PC_LINK_MAX},
#endif
#ifdef _PC_MAX_CANON
    {"PC_MAX_CANON",    _PC_MAX_CANON},
#endif
#ifdef _PC_MAX_INPUT
    {"PC_MAX_INPUT",    _PC_MAX_INPUT},
#endif
#ifdef _PC_NAME_MAX
    {"PC_NAME_MAX",     _PC_NAME_MAX},
#endif
#ifdef _PC_NO_TRUNC
    {"PC_NO_TRUNC",     _PC_NO_TRUNC},
#endif
#ifdef _PC_PATH_MAX
    {"PC_PATH_MAX",     _PC_PATH_MAX},
#endif
#ifdef _PC_PIPE_BUF
    {"PC_PIPE_BUF",     _PC_PIPE_BUF},
#endif
#ifdef _PC_PRIO_IO
    {"PC_PRIO_IO",      _PC_PRIO_IO},
#endif
#ifdef _PC_SOCK_MAXBUF
    {"PC_SOCK_MAXBUF",  _PC_SOCK_MAXBUF},
#endif
#ifdef _PC_SYNC_IO
    {"PC_SYNC_IO",      _PC_SYNC_IO},
#endif
#ifdef _PC_VDISABLE
    {"PC_VDISABLE",     _PC_VDISABLE},
#endif
};

static int
conv_path_confname(PyObject *arg, int *valuep)
{
    return conv_confname(arg, valuep, srcos_constants_pathconf,
                         sizeof(srcos_constants_pathconf)
                           / sizeof(struct constdef));
}
#endif

#ifdef HAVE_FPATHCONF
PyDoc_STRVAR(srcos_fpathconf__doc__,
"fpathconf(fd, name) -> integer\n\n\
Return the configuration limit name for the file descriptor fd.\n\
If there is no limit, return -1.");

static PyObject *
srcos_fpathconf(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif


#ifdef HAVE_PATHCONF
PyDoc_STRVAR(srcos_pathconf__doc__,
"pathconf(path, name) -> integer\n\n\
Return the configuration limit name for the file or directory path.\n\
If there is no limit, return -1.");

static PyObject *
srcos_pathconf(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif

#ifdef HAVE_CONFSTR
static struct constdef srcos_constants_confstr[] = {
#ifdef _CS_ARCHITECTURE
    {"CS_ARCHITECTURE", _CS_ARCHITECTURE},
#endif
#ifdef _CS_HOSTNAME
    {"CS_HOSTNAME",     _CS_HOSTNAME},
#endif
#ifdef _CS_HW_PROVIDER
    {"CS_HW_PROVIDER",  _CS_HW_PROVIDER},
#endif
#ifdef _CS_HW_SERIAL
    {"CS_HW_SERIAL",    _CS_HW_SERIAL},
#endif
#ifdef _CS_INITTAB_NAME
    {"CS_INITTAB_NAME", _CS_INITTAB_NAME},
#endif
#ifdef _CS_LFS64_CFLAGS
    {"CS_LFS64_CFLAGS", _CS_LFS64_CFLAGS},
#endif
#ifdef _CS_LFS64_LDFLAGS
    {"CS_LFS64_LDFLAGS",        _CS_LFS64_LDFLAGS},
#endif
#ifdef _CS_LFS64_LIBS
    {"CS_LFS64_LIBS",   _CS_LFS64_LIBS},
#endif
#ifdef _CS_LFS64_LINTFLAGS
    {"CS_LFS64_LINTFLAGS",      _CS_LFS64_LINTFLAGS},
#endif
#ifdef _CS_LFS_CFLAGS
    {"CS_LFS_CFLAGS",   _CS_LFS_CFLAGS},
#endif
#ifdef _CS_LFS_LDFLAGS
    {"CS_LFS_LDFLAGS",  _CS_LFS_LDFLAGS},
#endif
#ifdef _CS_LFS_LIBS
    {"CS_LFS_LIBS",     _CS_LFS_LIBS},
#endif
#ifdef _CS_LFS_LINTFLAGS
    {"CS_LFS_LINTFLAGS",        _CS_LFS_LINTFLAGS},
#endif
#ifdef _CS_MACHINE
    {"CS_MACHINE",      _CS_MACHINE},
#endif
#ifdef _CS_PATH
    {"CS_PATH", _CS_PATH},
#endif
#ifdef _CS_RELEASE
    {"CS_RELEASE",      _CS_RELEASE},
#endif
#ifdef _CS_SRPC_DOMAIN
    {"CS_SRPC_DOMAIN",  _CS_SRPC_DOMAIN},
#endif
#ifdef _CS_SYSNAME
    {"CS_SYSNAME",      _CS_SYSNAME},
#endif
#ifdef _CS_VERSION
    {"CS_VERSION",      _CS_VERSION},
#endif
#ifdef _CS_XBS5_ILP32_OFF32_CFLAGS
    {"CS_XBS5_ILP32_OFF32_CFLAGS",      _CS_XBS5_ILP32_OFF32_CFLAGS},
#endif
#ifdef _CS_XBS5_ILP32_OFF32_LDFLAGS
    {"CS_XBS5_ILP32_OFF32_LDFLAGS",     _CS_XBS5_ILP32_OFF32_LDFLAGS},
#endif
#ifdef _CS_XBS5_ILP32_OFF32_LIBS
    {"CS_XBS5_ILP32_OFF32_LIBS",        _CS_XBS5_ILP32_OFF32_LIBS},
#endif
#ifdef _CS_XBS5_ILP32_OFF32_LINTFLAGS
    {"CS_XBS5_ILP32_OFF32_LINTFLAGS",   _CS_XBS5_ILP32_OFF32_LINTFLAGS},
#endif
#ifdef _CS_XBS5_ILP32_OFFBIG_CFLAGS
    {"CS_XBS5_ILP32_OFFBIG_CFLAGS",     _CS_XBS5_ILP32_OFFBIG_CFLAGS},
#endif
#ifdef _CS_XBS5_ILP32_OFFBIG_LDFLAGS
    {"CS_XBS5_ILP32_OFFBIG_LDFLAGS",    _CS_XBS5_ILP32_OFFBIG_LDFLAGS},
#endif
#ifdef _CS_XBS5_ILP32_OFFBIG_LIBS
    {"CS_XBS5_ILP32_OFFBIG_LIBS",       _CS_XBS5_ILP32_OFFBIG_LIBS},
#endif
#ifdef _CS_XBS5_ILP32_OFFBIG_LINTFLAGS
    {"CS_XBS5_ILP32_OFFBIG_LINTFLAGS",  _CS_XBS5_ILP32_OFFBIG_LINTFLAGS},
#endif
#ifdef _CS_XBS5_LP64_OFF64_CFLAGS
    {"CS_XBS5_LP64_OFF64_CFLAGS",       _CS_XBS5_LP64_OFF64_CFLAGS},
#endif
#ifdef _CS_XBS5_LP64_OFF64_LDFLAGS
    {"CS_XBS5_LP64_OFF64_LDFLAGS",      _CS_XBS5_LP64_OFF64_LDFLAGS},
#endif
#ifdef _CS_XBS5_LP64_OFF64_LIBS
    {"CS_XBS5_LP64_OFF64_LIBS", _CS_XBS5_LP64_OFF64_LIBS},
#endif
#ifdef _CS_XBS5_LP64_OFF64_LINTFLAGS
    {"CS_XBS5_LP64_OFF64_LINTFLAGS",    _CS_XBS5_LP64_OFF64_LINTFLAGS},
#endif
#ifdef _CS_XBS5_LPBIG_OFFBIG_CFLAGS
    {"CS_XBS5_LPBIG_OFFBIG_CFLAGS",     _CS_XBS5_LPBIG_OFFBIG_CFLAGS},
#endif
#ifdef _CS_XBS5_LPBIG_OFFBIG_LDFLAGS
    {"CS_XBS5_LPBIG_OFFBIG_LDFLAGS",    _CS_XBS5_LPBIG_OFFBIG_LDFLAGS},
#endif
#ifdef _CS_XBS5_LPBIG_OFFBIG_LIBS
    {"CS_XBS5_LPBIG_OFFBIG_LIBS",       _CS_XBS5_LPBIG_OFFBIG_LIBS},
#endif
#ifdef _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS
    {"CS_XBS5_LPBIG_OFFBIG_LINTFLAGS",  _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS},
#endif
#ifdef _MIPS_CS_AVAIL_PROCESSORS
    {"MIPS_CS_AVAIL_PROCESSORS",        _MIPS_CS_AVAIL_PROCESSORS},
#endif
#ifdef _MIPS_CS_BASE
    {"MIPS_CS_BASE",    _MIPS_CS_BASE},
#endif
#ifdef _MIPS_CS_HOSTID
    {"MIPS_CS_HOSTID",  _MIPS_CS_HOSTID},
#endif
#ifdef _MIPS_CS_HW_NAME
    {"MIPS_CS_HW_NAME", _MIPS_CS_HW_NAME},
#endif
#ifdef _MIPS_CS_NUM_PROCESSORS
    {"MIPS_CS_NUM_PROCESSORS",  _MIPS_CS_NUM_PROCESSORS},
#endif
#ifdef _MIPS_CS_OSREL_MAJ
    {"MIPS_CS_OSREL_MAJ",       _MIPS_CS_OSREL_MAJ},
#endif
#ifdef _MIPS_CS_OSREL_MIN
    {"MIPS_CS_OSREL_MIN",       _MIPS_CS_OSREL_MIN},
#endif
#ifdef _MIPS_CS_OSREL_PATCH
    {"MIPS_CS_OSREL_PATCH",     _MIPS_CS_OSREL_PATCH},
#endif
#ifdef _MIPS_CS_OS_NAME
    {"MIPS_CS_OS_NAME", _MIPS_CS_OS_NAME},
#endif
#ifdef _MIPS_CS_OS_PROVIDER
    {"MIPS_CS_OS_PROVIDER",     _MIPS_CS_OS_PROVIDER},
#endif
#ifdef _MIPS_CS_PROCESSORS
    {"MIPS_CS_PROCESSORS",      _MIPS_CS_PROCESSORS},
#endif
#ifdef _MIPS_CS_SERIAL
    {"MIPS_CS_SERIAL",  _MIPS_CS_SERIAL},
#endif
#ifdef _MIPS_CS_VENDOR
    {"MIPS_CS_VENDOR",  _MIPS_CS_VENDOR},
#endif
};

static int
conv_confstr_confname(PyObject *arg, int *valuep)
{
    return conv_confname(arg, valuep, srcos_constants_confstr,
                         sizeof(srcos_constants_confstr)
                           / sizeof(struct constdef));
}

PyDoc_STRVAR(srcos_confstr__doc__,
"confstr(name) -> string\n\n\
Return a string-valued system configuration variable.");

static PyObject *
srcos_confstr(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif


#ifdef HAVE_SYSCONF
static struct constdef srcos_constants_sysconf[] = {
#ifdef _SC_2_CHAR_TERM
    {"SC_2_CHAR_TERM",  _SC_2_CHAR_TERM},
#endif
#ifdef _SC_2_C_BIND
    {"SC_2_C_BIND",     _SC_2_C_BIND},
#endif
#ifdef _SC_2_C_DEV
    {"SC_2_C_DEV",      _SC_2_C_DEV},
#endif
#ifdef _SC_2_C_VERSION
    {"SC_2_C_VERSION",  _SC_2_C_VERSION},
#endif
#ifdef _SC_2_FORT_DEV
    {"SC_2_FORT_DEV",   _SC_2_FORT_DEV},
#endif
#ifdef _SC_2_FORT_RUN
    {"SC_2_FORT_RUN",   _SC_2_FORT_RUN},
#endif
#ifdef _SC_2_LOCALEDEF
    {"SC_2_LOCALEDEF",  _SC_2_LOCALEDEF},
#endif
#ifdef _SC_2_SW_DEV
    {"SC_2_SW_DEV",     _SC_2_SW_DEV},
#endif
#ifdef _SC_2_UPE
    {"SC_2_UPE",        _SC_2_UPE},
#endif
#ifdef _SC_2_VERSION
    {"SC_2_VERSION",    _SC_2_VERSION},
#endif
#ifdef _SC_ABI_ASYNCHRONOUS_IO
    {"SC_ABI_ASYNCHRONOUS_IO",  _SC_ABI_ASYNCHRONOUS_IO},
#endif
#ifdef _SC_ACL
    {"SC_ACL",  _SC_ACL},
#endif
#ifdef _SC_AIO_LISTIO_MAX
    {"SC_AIO_LISTIO_MAX",       _SC_AIO_LISTIO_MAX},
#endif
#ifdef _SC_AIO_MAX
    {"SC_AIO_MAX",      _SC_AIO_MAX},
#endif
#ifdef _SC_AIO_PRIO_DELTA_MAX
    {"SC_AIO_PRIO_DELTA_MAX",   _SC_AIO_PRIO_DELTA_MAX},
#endif
#ifdef _SC_ARG_MAX
    {"SC_ARG_MAX",      _SC_ARG_MAX},
#endif
#ifdef _SC_ASYNCHRONOUS_IO
    {"SC_ASYNCHRONOUS_IO",      _SC_ASYNCHRONOUS_IO},
#endif
#ifdef _SC_ATEXIT_MAX
    {"SC_ATEXIT_MAX",   _SC_ATEXIT_MAX},
#endif
#ifdef _SC_AUDIT
    {"SC_AUDIT",        _SC_AUDIT},
#endif
#ifdef _SC_AVPHYS_PAGES
    {"SC_AVPHYS_PAGES", _SC_AVPHYS_PAGES},
#endif
#ifdef _SC_BC_BASE_MAX
    {"SC_BC_BASE_MAX",  _SC_BC_BASE_MAX},
#endif
#ifdef _SC_BC_DIM_MAX
    {"SC_BC_DIM_MAX",   _SC_BC_DIM_MAX},
#endif
#ifdef _SC_BC_SCALE_MAX
    {"SC_BC_SCALE_MAX", _SC_BC_SCALE_MAX},
#endif
#ifdef _SC_BC_STRING_MAX
    {"SC_BC_STRING_MAX",        _SC_BC_STRING_MAX},
#endif
#ifdef _SC_CAP
    {"SC_CAP",  _SC_CAP},
#endif
#ifdef _SC_CHARCLASS_NAME_MAX
    {"SC_CHARCLASS_NAME_MAX",   _SC_CHARCLASS_NAME_MAX},
#endif
#ifdef _SC_CHAR_BIT
    {"SC_CHAR_BIT",     _SC_CHAR_BIT},
#endif
#ifdef _SC_CHAR_MAX
    {"SC_CHAR_MAX",     _SC_CHAR_MAX},
#endif
#ifdef _SC_CHAR_MIN
    {"SC_CHAR_MIN",     _SC_CHAR_MIN},
#endif
#ifdef _SC_CHILD_MAX
    {"SC_CHILD_MAX",    _SC_CHILD_MAX},
#endif
#ifdef _SC_CLK_TCK
    {"SC_CLK_TCK",      _SC_CLK_TCK},
#endif
#ifdef _SC_COHER_BLKSZ
    {"SC_COHER_BLKSZ",  _SC_COHER_BLKSZ},
#endif
#ifdef _SC_COLL_WEIGHTS_MAX
    {"SC_COLL_WEIGHTS_MAX",     _SC_COLL_WEIGHTS_MAX},
#endif
#ifdef _SC_DCACHE_ASSOC
    {"SC_DCACHE_ASSOC", _SC_DCACHE_ASSOC},
#endif
#ifdef _SC_DCACHE_BLKSZ
    {"SC_DCACHE_BLKSZ", _SC_DCACHE_BLKSZ},
#endif
#ifdef _SC_DCACHE_LINESZ
    {"SC_DCACHE_LINESZ",        _SC_DCACHE_LINESZ},
#endif
#ifdef _SC_DCACHE_SZ
    {"SC_DCACHE_SZ",    _SC_DCACHE_SZ},
#endif
#ifdef _SC_DCACHE_TBLKSZ
    {"SC_DCACHE_TBLKSZ",        _SC_DCACHE_TBLKSZ},
#endif
#ifdef _SC_DELAYTIMER_MAX
    {"SC_DELAYTIMER_MAX",       _SC_DELAYTIMER_MAX},
#endif
#ifdef _SC_EQUIV_CLASS_MAX
    {"SC_EQUIV_CLASS_MAX",      _SC_EQUIV_CLASS_MAX},
#endif
#ifdef _SC_EXPR_NEST_MAX
    {"SC_EXPR_NEST_MAX",        _SC_EXPR_NEST_MAX},
#endif
#ifdef _SC_FSYNC
    {"SC_FSYNC",        _SC_FSYNC},
#endif
#ifdef _SC_GETGR_R_SIZE_MAX
    {"SC_GETGR_R_SIZE_MAX",     _SC_GETGR_R_SIZE_MAX},
#endif
#ifdef _SC_GETPW_R_SIZE_MAX
    {"SC_GETPW_R_SIZE_MAX",     _SC_GETPW_R_SIZE_MAX},
#endif
#ifdef _SC_ICACHE_ASSOC
    {"SC_ICACHE_ASSOC", _SC_ICACHE_ASSOC},
#endif
#ifdef _SC_ICACHE_BLKSZ
    {"SC_ICACHE_BLKSZ", _SC_ICACHE_BLKSZ},
#endif
#ifdef _SC_ICACHE_LINESZ
    {"SC_ICACHE_LINESZ",        _SC_ICACHE_LINESZ},
#endif
#ifdef _SC_ICACHE_SZ
    {"SC_ICACHE_SZ",    _SC_ICACHE_SZ},
#endif
#ifdef _SC_INF
    {"SC_INF",  _SC_INF},
#endif
#ifdef _SC_INT_MAX
    {"SC_INT_MAX",      _SC_INT_MAX},
#endif
#ifdef _SC_INT_MIN
    {"SC_INT_MIN",      _SC_INT_MIN},
#endif
#ifdef _SC_IOV_MAX
    {"SC_IOV_MAX",      _SC_IOV_MAX},
#endif
#ifdef _SC_IP_SECOPTS
    {"SC_IP_SECOPTS",   _SC_IP_SECOPTS},
#endif
#ifdef _SC_JOB_CONTROL
    {"SC_JOB_CONTROL",  _SC_JOB_CONTROL},
#endif
#ifdef _SC_KERN_POINTERS
    {"SC_KERN_POINTERS",        _SC_KERN_POINTERS},
#endif
#ifdef _SC_KERN_SIM
    {"SC_KERN_SIM",     _SC_KERN_SIM},
#endif
#ifdef _SC_LINE_MAX
    {"SC_LINE_MAX",     _SC_LINE_MAX},
#endif
#ifdef _SC_LOGIN_NAME_MAX
    {"SC_LOGIN_NAME_MAX",       _SC_LOGIN_NAME_MAX},
#endif
#ifdef _SC_LOGNAME_MAX
    {"SC_LOGNAME_MAX",  _SC_LOGNAME_MAX},
#endif
#ifdef _SC_LONG_BIT
    {"SC_LONG_BIT",     _SC_LONG_BIT},
#endif
#ifdef _SC_MAC
    {"SC_MAC",  _SC_MAC},
#endif
#ifdef _SC_MAPPED_FILES
    {"SC_MAPPED_FILES", _SC_MAPPED_FILES},
#endif
#ifdef _SC_MAXPID
    {"SC_MAXPID",       _SC_MAXPID},
#endif
#ifdef _SC_MB_LEN_MAX
    {"SC_MB_LEN_MAX",   _SC_MB_LEN_MAX},
#endif
#ifdef _SC_MEMLOCK
    {"SC_MEMLOCK",      _SC_MEMLOCK},
#endif
#ifdef _SC_MEMLOCK_RANGE
    {"SC_MEMLOCK_RANGE",        _SC_MEMLOCK_RANGE},
#endif
#ifdef _SC_MEMORY_PROTECTION
    {"SC_MEMORY_PROTECTION",    _SC_MEMORY_PROTECTION},
#endif
#ifdef _SC_MESSAGE_PASSING
    {"SC_MESSAGE_PASSING",      _SC_MESSAGE_PASSING},
#endif
#ifdef _SC_MMAP_FIXED_ALIGNMENT
    {"SC_MMAP_FIXED_ALIGNMENT", _SC_MMAP_FIXED_ALIGNMENT},
#endif
#ifdef _SC_MQ_OPEN_MAX
    {"SC_MQ_OPEN_MAX",  _SC_MQ_OPEN_MAX},
#endif
#ifdef _SC_MQ_PRIO_MAX
    {"SC_MQ_PRIO_MAX",  _SC_MQ_PRIO_MAX},
#endif
#ifdef _SC_NACLS_MAX
    {"SC_NACLS_MAX",    _SC_NACLS_MAX},
#endif
#ifdef _SC_NGROUPS_MAX
    {"SC_NGROUPS_MAX",  _SC_NGROUPS_MAX},
#endif
#ifdef _SC_NL_ARGMAX
    {"SC_NL_ARGMAX",    _SC_NL_ARGMAX},
#endif
#ifdef _SC_NL_LANGMAX
    {"SC_NL_LANGMAX",   _SC_NL_LANGMAX},
#endif
#ifdef _SC_NL_MSGMAX
    {"SC_NL_MSGMAX",    _SC_NL_MSGMAX},
#endif
#ifdef _SC_NL_NMAX
    {"SC_NL_NMAX",      _SC_NL_NMAX},
#endif
#ifdef _SC_NL_SETMAX
    {"SC_NL_SETMAX",    _SC_NL_SETMAX},
#endif
#ifdef _SC_NL_TEXTMAX
    {"SC_NL_TEXTMAX",   _SC_NL_TEXTMAX},
#endif
#ifdef _SC_NPROCESSORS_CONF
    {"SC_NPROCESSORS_CONF",     _SC_NPROCESSORS_CONF},
#endif
#ifdef _SC_NPROCESSORS_ONLN
    {"SC_NPROCESSORS_ONLN",     _SC_NPROCESSORS_ONLN},
#endif
#ifdef _SC_NPROC_CONF
    {"SC_NPROC_CONF",   _SC_NPROC_CONF},
#endif
#ifdef _SC_NPROC_ONLN
    {"SC_NPROC_ONLN",   _SC_NPROC_ONLN},
#endif
#ifdef _SC_NZERO
    {"SC_NZERO",        _SC_NZERO},
#endif
#ifdef _SC_OPEN_MAX
    {"SC_OPEN_MAX",     _SC_OPEN_MAX},
#endif
#ifdef _SC_PAGESIZE
    {"SC_PAGESIZE",     _SC_PAGESIZE},
#endif
#ifdef _SC_PAGE_SIZE
    {"SC_PAGE_SIZE",    _SC_PAGE_SIZE},
#endif
#ifdef _SC_PASS_MAX
    {"SC_PASS_MAX",     _SC_PASS_MAX},
#endif
#ifdef _SC_PHYS_PAGES
    {"SC_PHYS_PAGES",   _SC_PHYS_PAGES},
#endif
#ifdef _SC_PII
    {"SC_PII",  _SC_PII},
#endif
#ifdef _SC_PII_INTERNET
    {"SC_PII_INTERNET", _SC_PII_INTERNET},
#endif
#ifdef _SC_PII_INTERNET_DGRAM
    {"SC_PII_INTERNET_DGRAM",   _SC_PII_INTERNET_DGRAM},
#endif
#ifdef _SC_PII_INTERNET_STREAM
    {"SC_PII_INTERNET_STREAM",  _SC_PII_INTERNET_STREAM},
#endif
#ifdef _SC_PII_OSI
    {"SC_PII_OSI",      _SC_PII_OSI},
#endif
#ifdef _SC_PII_OSI_CLTS
    {"SC_PII_OSI_CLTS", _SC_PII_OSI_CLTS},
#endif
#ifdef _SC_PII_OSI_COTS
    {"SC_PII_OSI_COTS", _SC_PII_OSI_COTS},
#endif
#ifdef _SC_PII_OSI_M
    {"SC_PII_OSI_M",    _SC_PII_OSI_M},
#endif
#ifdef _SC_PII_SOCKET
    {"SC_PII_SOCKET",   _SC_PII_SOCKET},
#endif
#ifdef _SC_PII_XTI
    {"SC_PII_XTI",      _SC_PII_XTI},
#endif
#ifdef _SC_POLL
    {"SC_POLL", _SC_POLL},
#endif
#ifdef _SC_PRIORITIZED_IO
    {"SC_PRIORITIZED_IO",       _SC_PRIORITIZED_IO},
#endif
#ifdef _SC_PRIORITY_SCHEDULING
    {"SC_PRIORITY_SCHEDULING",  _SC_PRIORITY_SCHEDULING},
#endif
#ifdef _SC_REALTIME_SIGNALS
    {"SC_REALTIME_SIGNALS",     _SC_REALTIME_SIGNALS},
#endif
#ifdef _SC_RE_DUP_MAX
    {"SC_RE_DUP_MAX",   _SC_RE_DUP_MAX},
#endif
#ifdef _SC_RTSIG_MAX
    {"SC_RTSIG_MAX",    _SC_RTSIG_MAX},
#endif
#ifdef _SC_SAVED_IDS
    {"SC_SAVED_IDS",    _SC_SAVED_IDS},
#endif
#ifdef _SC_SCHAR_MAX
    {"SC_SCHAR_MAX",    _SC_SCHAR_MAX},
#endif
#ifdef _SC_SCHAR_MIN
    {"SC_SCHAR_MIN",    _SC_SCHAR_MIN},
#endif
#ifdef _SC_SELECT
    {"SC_SELECT",       _SC_SELECT},
#endif
#ifdef _SC_SEMAPHORES
    {"SC_SEMAPHORES",   _SC_SEMAPHORES},
#endif
#ifdef _SC_SEM_NSEMS_MAX
    {"SC_SEM_NSEMS_MAX",        _SC_SEM_NSEMS_MAX},
#endif
#ifdef _SC_SEM_VALUE_MAX
    {"SC_SEM_VALUE_MAX",        _SC_SEM_VALUE_MAX},
#endif
#ifdef _SC_SHARED_MEMORY_OBJECTS
    {"SC_SHARED_MEMORY_OBJECTS",        _SC_SHARED_MEMORY_OBJECTS},
#endif
#ifdef _SC_SHRT_MAX
    {"SC_SHRT_MAX",     _SC_SHRT_MAX},
#endif
#ifdef _SC_SHRT_MIN
    {"SC_SHRT_MIN",     _SC_SHRT_MIN},
#endif
#ifdef _SC_SIGQUEUE_MAX
    {"SC_SIGQUEUE_MAX", _SC_SIGQUEUE_MAX},
#endif
#ifdef _SC_SIGRT_MAX
    {"SC_SIGRT_MAX",    _SC_SIGRT_MAX},
#endif
#ifdef _SC_SIGRT_MIN
    {"SC_SIGRT_MIN",    _SC_SIGRT_MIN},
#endif
#ifdef _SC_SOFTPOWER
    {"SC_SOFTPOWER",    _SC_SOFTPOWER},
#endif
#ifdef _SC_SPLIT_CACHE
    {"SC_SPLIT_CACHE",  _SC_SPLIT_CACHE},
#endif
#ifdef _SC_SSIZE_MAX
    {"SC_SSIZE_MAX",    _SC_SSIZE_MAX},
#endif
#ifdef _SC_STACK_PROT
    {"SC_STACK_PROT",   _SC_STACK_PROT},
#endif
#ifdef _SC_STREAM_MAX
    {"SC_STREAM_MAX",   _SC_STREAM_MAX},
#endif
#ifdef _SC_SYNCHRONIZED_IO
    {"SC_SYNCHRONIZED_IO",      _SC_SYNCHRONIZED_IO},
#endif
#ifdef _SC_THREADS
    {"SC_THREADS",      _SC_THREADS},
#endif
#ifdef _SC_THREAD_ATTR_STACKADDR
    {"SC_THREAD_ATTR_STACKADDR",        _SC_THREAD_ATTR_STACKADDR},
#endif
#ifdef _SC_THREAD_ATTR_STACKSIZE
    {"SC_THREAD_ATTR_STACKSIZE",        _SC_THREAD_ATTR_STACKSIZE},
#endif
#ifdef _SC_THREAD_DESTRUCTOR_ITERATIONS
    {"SC_THREAD_DESTRUCTOR_ITERATIONS", _SC_THREAD_DESTRUCTOR_ITERATIONS},
#endif
#ifdef _SC_THREAD_KEYS_MAX
    {"SC_THREAD_KEYS_MAX",      _SC_THREAD_KEYS_MAX},
#endif
#ifdef _SC_THREAD_PRIORITY_SCHEDULING
    {"SC_THREAD_PRIORITY_SCHEDULING",   _SC_THREAD_PRIORITY_SCHEDULING},
#endif
#ifdef _SC_THREAD_PRIO_INHERIT
    {"SC_THREAD_PRIO_INHERIT",  _SC_THREAD_PRIO_INHERIT},
#endif
#ifdef _SC_THREAD_PRIO_PROTECT
    {"SC_THREAD_PRIO_PROTECT",  _SC_THREAD_PRIO_PROTECT},
#endif
#ifdef _SC_THREAD_PROCESS_SHARED
    {"SC_THREAD_PROCESS_SHARED",        _SC_THREAD_PROCESS_SHARED},
#endif
#ifdef _SC_THREAD_SAFE_FUNCTIONS
    {"SC_THREAD_SAFE_FUNCTIONS",        _SC_THREAD_SAFE_FUNCTIONS},
#endif
#ifdef _SC_THREAD_STACK_MIN
    {"SC_THREAD_STACK_MIN",     _SC_THREAD_STACK_MIN},
#endif
#ifdef _SC_THREAD_THREADS_MAX
    {"SC_THREAD_THREADS_MAX",   _SC_THREAD_THREADS_MAX},
#endif
#ifdef _SC_TIMERS
    {"SC_TIMERS",       _SC_TIMERS},
#endif
#ifdef _SC_TIMER_MAX
    {"SC_TIMER_MAX",    _SC_TIMER_MAX},
#endif
#ifdef _SC_TTY_NAME_MAX
    {"SC_TTY_NAME_MAX", _SC_TTY_NAME_MAX},
#endif
#ifdef _SC_TZNAME_MAX
    {"SC_TZNAME_MAX",   _SC_TZNAME_MAX},
#endif
#ifdef _SC_T_IOV_MAX
    {"SC_T_IOV_MAX",    _SC_T_IOV_MAX},
#endif
#ifdef _SC_UCHAR_MAX
    {"SC_UCHAR_MAX",    _SC_UCHAR_MAX},
#endif
#ifdef _SC_UINT_MAX
    {"SC_UINT_MAX",     _SC_UINT_MAX},
#endif
#ifdef _SC_UIO_MAXIOV
    {"SC_UIO_MAXIOV",   _SC_UIO_MAXIOV},
#endif
#ifdef _SC_ULONG_MAX
    {"SC_ULONG_MAX",    _SC_ULONG_MAX},
#endif
#ifdef _SC_USHRT_MAX
    {"SC_USHRT_MAX",    _SC_USHRT_MAX},
#endif
#ifdef _SC_VERSION
    {"SC_VERSION",      _SC_VERSION},
#endif
#ifdef _SC_WORD_BIT
    {"SC_WORD_BIT",     _SC_WORD_BIT},
#endif
#ifdef _SC_XBS5_ILP32_OFF32
    {"SC_XBS5_ILP32_OFF32",     _SC_XBS5_ILP32_OFF32},
#endif
#ifdef _SC_XBS5_ILP32_OFFBIG
    {"SC_XBS5_ILP32_OFFBIG",    _SC_XBS5_ILP32_OFFBIG},
#endif
#ifdef _SC_XBS5_LP64_OFF64
    {"SC_XBS5_LP64_OFF64",      _SC_XBS5_LP64_OFF64},
#endif
#ifdef _SC_XBS5_LPBIG_OFFBIG
    {"SC_XBS5_LPBIG_OFFBIG",    _SC_XBS5_LPBIG_OFFBIG},
#endif
#ifdef _SC_XOPEN_CRYPT
    {"SC_XOPEN_CRYPT",  _SC_XOPEN_CRYPT},
#endif
#ifdef _SC_XOPEN_ENH_I18N
    {"SC_XOPEN_ENH_I18N",       _SC_XOPEN_ENH_I18N},
#endif
#ifdef _SC_XOPEN_LEGACY
    {"SC_XOPEN_LEGACY", _SC_XOPEN_LEGACY},
#endif
#ifdef _SC_XOPEN_REALTIME
    {"SC_XOPEN_REALTIME",       _SC_XOPEN_REALTIME},
#endif
#ifdef _SC_XOPEN_REALTIME_THREADS
    {"SC_XOPEN_REALTIME_THREADS",       _SC_XOPEN_REALTIME_THREADS},
#endif
#ifdef _SC_XOPEN_SHM
    {"SC_XOPEN_SHM",    _SC_XOPEN_SHM},
#endif
#ifdef _SC_XOPEN_UNIX
    {"SC_XOPEN_UNIX",   _SC_XOPEN_UNIX},
#endif
#ifdef _SC_XOPEN_VERSION
    {"SC_XOPEN_VERSION",        _SC_XOPEN_VERSION},
#endif
#ifdef _SC_XOPEN_XCU_VERSION
    {"SC_XOPEN_XCU_VERSION",    _SC_XOPEN_XCU_VERSION},
#endif
#ifdef _SC_XOPEN_XPG2
    {"SC_XOPEN_XPG2",   _SC_XOPEN_XPG2},
#endif
#ifdef _SC_XOPEN_XPG3
    {"SC_XOPEN_XPG3",   _SC_XOPEN_XPG3},
#endif
#ifdef _SC_XOPEN_XPG4
    {"SC_XOPEN_XPG4",   _SC_XOPEN_XPG4},
#endif
};

static int
conv_sysconf_confname(PyObject *arg, int *valuep)
{
    return conv_confname(arg, valuep, srcos_constants_sysconf,
                         sizeof(srcos_constants_sysconf)
                           / sizeof(struct constdef));
}

PyDoc_STRVAR(srcos_sysconf__doc__,
"sysconf(name) -> integer\n\n\
Return an integer-valued system configuration variable.");

static PyObject *
srcos_sysconf(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif


/* This code is used to ensure that the tables of configuration value names
 * are in sorted order as required by conv_confname(), and also to build the
 * the exported dictionaries that are used to publish information about the
 * names available on the host platform.
 *
 * Sorting the table at runtime ensures that the table is properly ordered
 * when used, even for platforms we're not able to test on.  It also makes
 * it easier to add additional entries to the tables.
 */

static int
cmp_constdefs(const void *v1,  const void *v2)
{
    const struct constdef *c1 =
        (const struct constdef *) v1;
    const struct constdef *c2 =
        (const struct constdef *) v2;

    return strcmp(c1->name, c2->name);
}

static int
setup_confname_table(struct constdef *table, size_t tablesize,
		     char *tablename, PyObject *module)
{
    PyObject *d = NULL;
    size_t i;

    qsort(table, tablesize, sizeof(struct constdef), cmp_constdefs);
    d = PyDict_New();
    if (d == NULL)
	    return -1;

    for (i=0; i < tablesize; ++i) {
            PyObject *o = PyInt_FromLong(table[i].value);
            if (o == NULL || PyDict_SetItemString(d, table[i].name, o) == -1) {
		    Py_XDECREF(o);
		    Py_DECREF(d);
		    return -1;
            }
	    Py_DECREF(o);
    }
    return PyModule_AddObject(module, tablename, d);
}

/* Return -1 on failure, 0 on success. */
static int
setup_confname_tables(PyObject *module)
{
#if defined(HAVE_FPATHCONF) || defined(HAVE_PATHCONF)
    if (setup_confname_table(srcos_constants_pathconf,
                             sizeof(srcos_constants_pathconf)
                               / sizeof(struct constdef),
                             "pathconf_names", module))
        return -1;
#endif
#ifdef HAVE_CONFSTR
    if (setup_confname_table(srcos_constants_confstr,
                             sizeof(srcos_constants_confstr)
                               / sizeof(struct constdef),
                             "confstr_names", module))
        return -1;
#endif
#ifdef HAVE_SYSCONF
    if (setup_confname_table(srcos_constants_sysconf,
                             sizeof(srcos_constants_sysconf)
                               / sizeof(struct constdef),
                             "sysconf_names", module))
        return -1;
#endif
    return 0;
}


PyDoc_STRVAR(srcos_abort__doc__,
"abort() -> does not return!\n\n\
Abort the interpreter immediately.  This 'dumps core' or otherwise fails\n\
in the hardest way possible on the hosting operating system.");

static PyObject *
srcos_abort(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}

#ifdef MS_WINDOWS
PyDoc_STRVAR(win32_startfile__doc__,
"startfile(filepath [, operation]) - Start a file with its associated\n\
application.\n\
\n\
When \"operation\" is not specified or \"open\", this acts like\n\
double-clicking the file in Explorer, or giving the file name as an\n\
argument to the DOS \"start\" command: the file is opened with whatever\n\
application (if any) its extension is associated.\n\
When another \"operation\" is given, it specifies what should be done with\n\
the file.  A typical operation is \"print\".\n\
\n\
startfile returns as soon as the associated application is launched.\n\
There is no option to wait for the application to close, and no way\n\
to retrieve the application's exit status.\n\
\n\
The filepath is relative to the current directory.  If you want to use\n\
an absolute path, make sure the first character is not a slash (\"/\");\n\
the underlying Win32 ShellExecute function doesn't work if it is.");

static PyObject *
win32_startfile(PyObject *self, PyObject *args)
{
    char *filepath;
    char *operation = NULL;
    HINSTANCE rc;

    PyObject *unipath, *woperation = NULL;
    if (!PyArg_ParseTuple(args, "U|s:startfile",
                          &unipath, &operation)) {
        PyErr_Clear();
        goto normal;
    }

    if (operation) {
        woperation = PyUnicode_DecodeASCII(operation,
                                           strlen(operation), NULL);
        if (!woperation) {
            PyErr_Clear();
            operation = NULL;
            goto normal;
        }
    }

    Py_BEGIN_ALLOW_THREADS
    rc = ShellExecuteW((HWND)0, woperation ? PyUnicode_AS_UNICODE(woperation) : 0,
        PyUnicode_AS_UNICODE(unipath),
        NULL, NULL, SW_SHOWNORMAL);
    Py_END_ALLOW_THREADS

    Py_XDECREF(woperation);
    if (rc <= (HINSTANCE)32) {
        PyObject *errval = win32_error_unicode("startfile",
                                               PyUnicode_AS_UNICODE(unipath));
        return errval;
    }
    Py_INCREF(Py_None);
    return Py_None;

normal:
    if (!PyArg_ParseTuple(args, "et|s:startfile",
                          Py_FileSystemDefaultEncoding, &filepath,
                          &operation))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    rc = ShellExecute((HWND)0, operation, filepath,
                      NULL, NULL, SW_SHOWNORMAL);
    Py_END_ALLOW_THREADS
    if (rc <= (HINSTANCE)32) {
        PyObject *errval = win32_error("startfile", filepath);
        PyMem_Free(filepath);
        return errval;
    }
    PyMem_Free(filepath);
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* MS_WINDOWS */

#ifdef HAVE_GETLOADAVG
PyDoc_STRVAR(srcos_getloadavg__doc__,
"getloadavg() -> (float, float, float)\n\n\
Return the number of processes in the system run queue averaged over\n\
the last 1, 5, and 15 minutes or raises OSError if the load average\n\
was unobtainable");

static PyObject *
srcos_getloadavg(PyObject *self, PyObject *noargs)
{
	PyErr_SetString(PyExc_NotImplementedError, "Unsupported os method");
	return NULL;
}
#endif

#ifdef MS_WINDOWS

PyDoc_STRVAR(win32_urandom__doc__,
"urandom(n) -> str\n\n\
Return a string of n random bytes suitable for cryptographic use.");

typedef BOOL (WINAPI *CRYPTACQUIRECONTEXTA)(HCRYPTPROV *phProv,\
              LPCSTR pszContainer, LPCSTR pszProvider, DWORD dwProvType,\
              DWORD dwFlags );
typedef BOOL (WINAPI *CRYPTGENRANDOM)(HCRYPTPROV hProv, DWORD dwLen,\
              BYTE *pbBuffer );

static CRYPTGENRANDOM pCryptGenRandom = NULL;
/* This handle is never explicitly released. Instead, the operating
   system will release it when the process terminates. */
static HCRYPTPROV hCryptProv = 0;

static PyObject*
win32_urandom(PyObject *self, PyObject *args)
{
	int howMany;
	PyObject* result;

	/* Read arguments */
	if (! PyArg_ParseTuple(args, "i:urandom", &howMany))
		return NULL;
	if (howMany < 0)
		return PyErr_Format(PyExc_ValueError,
				    "negative argument not allowed");

	if (hCryptProv == 0) {
		HINSTANCE hAdvAPI32 = NULL;
		CRYPTACQUIRECONTEXTA pCryptAcquireContext = NULL;

		/* Obtain handle to the DLL containing CryptoAPI
		   This should not fail	*/
		hAdvAPI32 = GetModuleHandle("advapi32.dll");
		if(hAdvAPI32 == NULL)
			return win32_error("GetModuleHandle", NULL);

		/* Obtain pointers to the CryptoAPI functions
		   This will fail on some early versions of Win95 */
		pCryptAcquireContext = (CRYPTACQUIRECONTEXTA)GetProcAddress(
						hAdvAPI32,
						"CryptAcquireContextA");
		if (pCryptAcquireContext == NULL)
			return PyErr_Format(PyExc_NotImplementedError,
					    "CryptAcquireContextA not found");

		pCryptGenRandom = (CRYPTGENRANDOM)GetProcAddress(
						hAdvAPI32, "CryptGenRandom");
		if (pCryptGenRandom == NULL)
			return PyErr_Format(PyExc_NotImplementedError,
					    "CryptGenRandom not found");

		/* Acquire context */
		if (! pCryptAcquireContext(&hCryptProv, NULL, NULL,
					   PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
			return win32_error("CryptAcquireContext", NULL);
	}

	/* Allocate bytes */
	result = PyString_FromStringAndSize(NULL, howMany);
	if (result != NULL) {
		/* Get random data */
		memset(PyString_AS_STRING(result), 0, howMany); /* zero seed */
		if (! pCryptGenRandom(hCryptProv, howMany, (unsigned char*)
				      PyString_AS_STRING(result))) {
			Py_DECREF(result);
			return win32_error("CryptGenRandom", NULL);
		}
	}
	return result;
}
#endif

#ifdef __VMS
/* Use openssl random routine */
#include <openssl/rand.h>
PyDoc_STRVAR(vms_urandom__doc__,
"urandom(n) -> str\n\n\
Return a string of n random bytes suitable for cryptographic use.");

static PyObject*
vms_urandom(PyObject *self, PyObject *args)
{
	int howMany;
	PyObject* result;

	/* Read arguments */
	if (! PyArg_ParseTuple(args, "i:urandom", &howMany))
		return NULL;
	if (howMany < 0)
		return PyErr_Format(PyExc_ValueError,
				    "negative argument not allowed");

	/* Allocate bytes */
	result = PyString_FromStringAndSize(NULL, howMany);
	if (result != NULL) {
		/* Get random data */
		if (RAND_pseudo_bytes((unsigned char*)
				      PyString_AS_STRING(result),
				      howMany) < 0) {
			Py_DECREF(result);
			return PyErr_Format(PyExc_ValueError,
					    "RAND_pseudo_bytes");
		}
	}
	return result;
}
#endif

static PyMethodDef srcos_methods[] = {
	{"access",	srcos_access, METH_VARARGS, srcos_access__doc__},
#ifdef HAVE_TTYNAME
	{"ttyname",	srcos_ttyname, METH_VARARGS, srcos_ttyname__doc__},
#endif
	{"chdir",	srcos_chdir, METH_VARARGS, srcos_chdir__doc__},
#ifdef HAVE_CHFLAGS
	{"chflags",	srcos_chflags, METH_VARARGS, srcos_chflags__doc__},
#endif /* HAVE_CHFLAGS */
	{"chmod",	srcos_chmod, METH_VARARGS, srcos_chmod__doc__},
#ifdef HAVE_FCHMOD
	{"fchmod",	srcos_fchmod, METH_VARARGS, srcos_fchmod__doc__},
#endif /* HAVE_FCHMOD */
#ifdef HAVE_CHOWN
	{"chown",	srcos_chown, METH_VARARGS, srcos_chown__doc__},
#endif /* HAVE_CHOWN */
#ifdef HAVE_LCHMOD
	{"lchmod",	srcos_lchmod, METH_VARARGS, srcos_lchmod__doc__},
#endif /* HAVE_LCHMOD */
#ifdef HAVE_FCHOWN
	{"fchown",	srcos_fchown, METH_VARARGS, srcos_fchown__doc__},
#endif /* HAVE_FCHOWN */
#ifdef HAVE_LCHFLAGS
	{"lchflags",	srcos_lchflags, METH_VARARGS, srcos_lchflags__doc__},
#endif /* HAVE_LCHFLAGS */
#ifdef HAVE_LCHOWN
	{"lchown",	srcos_lchown, METH_VARARGS, srcos_lchown__doc__},
#endif /* HAVE_LCHOWN */
#ifdef HAVE_CHROOT
	{"chroot",	srcos_chroot, METH_VARARGS, srcos_chroot__doc__},
#endif
#ifdef HAVE_CTERMID
	{"ctermid",	srcos_ctermid, METH_NOARGS, srcos_ctermid__doc__},
#endif
#ifdef HAVE_GETCWD
	{"getcwd",	srcos_getcwd, METH_NOARGS, srcos_getcwd__doc__},
#ifdef Py_USING_UNICODE
	{"getcwdu",	srcos_getcwdu, METH_NOARGS, srcos_getcwdu__doc__},
#endif
#endif
#ifdef HAVE_LINK
	{"link",	srcos_link, METH_VARARGS, srcos_link__doc__},
#endif /* HAVE_LINK */
	{"listdir",	srcos_listdir, METH_VARARGS, srcos_listdir__doc__},
	{"lstat",	srcos_lstat, METH_VARARGS, srcos_lstat__doc__},
	{"mkdir",	srcos_mkdir, METH_VARARGS, srcos_mkdir__doc__},
#ifdef HAVE_NICE
	{"nice",	srcos_nice, METH_VARARGS, srcos_nice__doc__},
#endif /* HAVE_NICE */
#ifdef HAVE_READLINK
	{"readlink",	srcos_readlink, METH_VARARGS, srcos_readlink__doc__},
#endif /* HAVE_READLINK */
	{"rename",	srcos_rename, METH_VARARGS, srcos_rename__doc__},
	{"rmdir",	srcos_rmdir, METH_VARARGS, srcos_rmdir__doc__},
	{"stat",	srcos_stat, METH_VARARGS, srcos_stat__doc__},
	{"stat_float_times", stat_float_times, METH_VARARGS, stat_float_times__doc__},
#ifdef HAVE_SYMLINK
	{"symlink",	srcos_symlink, METH_VARARGS, srcos_symlink__doc__},
#endif /* HAVE_SYMLINK */
#ifdef HAVE_SYSTEM
	{"system",	srcos_system, METH_VARARGS, srcos_system__doc__},
#endif
	{"umask",	srcos_umask, METH_VARARGS, srcos_umask__doc__},
#ifdef HAVE_UNAME
	{"uname",	srcos_uname, METH_NOARGS, srcos_uname__doc__},
#endif /* HAVE_UNAME */
	{"unlink",	srcos_unlink, METH_VARARGS, srcos_unlink__doc__},
	{"remove",	srcos_unlink, METH_VARARGS, srcos_remove__doc__},
	{"utime",	srcos_utime, METH_VARARGS, srcos_utime__doc__},
#ifdef HAVE_TIMES
	{"times",	srcos_times, METH_NOARGS, srcos_times__doc__},
#endif /* HAVE_TIMES */
	{"_exit",	srcos__exit, METH_VARARGS, srcos__exit__doc__},
#ifdef HAVE_EXECV
	{"execv",	srcos_execv, METH_VARARGS, srcos_execv__doc__},
	{"execve",	srcos_execve, METH_VARARGS, srcos_execve__doc__},
#endif /* HAVE_EXECV */
#ifdef HAVE_SPAWNV
	{"spawnv",	srcos_spawnv, METH_VARARGS, srcos_spawnv__doc__},
	{"spawnve",	srcos_spawnve, METH_VARARGS, srcos_spawnve__doc__},
#if defined(PYOS_OS2)
	{"spawnvp",	srcos_spawnvp, METH_VARARGS, srcos_spawnvp__doc__},
	{"spawnvpe",	srcos_spawnvpe, METH_VARARGS, srcos_spawnvpe__doc__},
#endif /* PYOS_OS2 */
#endif /* HAVE_SPAWNV */
#ifdef HAVE_FORK1
	{"fork1",       srcos_fork1, METH_NOARGS, srcos_fork1__doc__},
#endif /* HAVE_FORK1 */
#ifdef HAVE_FORK
	{"fork",	srcos_fork, METH_NOARGS, srcos_fork__doc__},
#endif /* HAVE_FORK */
#if defined(HAVE_OPENPTY) || defined(HAVE__GETPTY) || defined(HAVE_DEV_PTMX)
	{"openpty",	srcos_openpty, METH_NOARGS, srcos_openpty__doc__},
#endif /* HAVE_OPENPTY || HAVE__GETPTY || HAVE_DEV_PTMX */
#ifdef HAVE_FORKPTY
	{"forkpty",	srcos_forkpty, METH_NOARGS, srcos_forkpty__doc__},
#endif /* HAVE_FORKPTY */
#ifdef HAVE_GETEGID
	{"getegid",	srcos_getegid, METH_NOARGS, srcos_getegid__doc__},
#endif /* HAVE_GETEGID */
#ifdef HAVE_GETEUID
	{"geteuid",	srcos_geteuid, METH_NOARGS, srcos_geteuid__doc__},
#endif /* HAVE_GETEUID */
#ifdef HAVE_GETGID
	{"getgid",	srcos_getgid, METH_NOARGS, srcos_getgid__doc__},
#endif /* HAVE_GETGID */
#ifdef HAVE_GETGROUPS
	{"getgroups",	srcos_getgroups, METH_NOARGS, srcos_getgroups__doc__},
#endif
	{"getpid",	srcos_getpid, METH_NOARGS, srcos_getpid__doc__},
#ifdef HAVE_GETPGRP
	{"getpgrp",	srcos_getpgrp, METH_NOARGS, srcos_getpgrp__doc__},
#endif /* HAVE_GETPGRP */
#ifdef HAVE_GETPPID
	{"getppid",	srcos_getppid, METH_NOARGS, srcos_getppid__doc__},
#endif /* HAVE_GETPPID */
#ifdef HAVE_GETUID
	{"getuid",	srcos_getuid, METH_NOARGS, srcos_getuid__doc__},
#endif /* HAVE_GETUID */
#ifdef HAVE_GETLOGIN
	{"getlogin",	srcos_getlogin, METH_NOARGS, srcos_getlogin__doc__},
#endif
#ifdef HAVE_KILL
	{"kill",	srcos_kill, METH_VARARGS, srcos_kill__doc__},
#endif /* HAVE_KILL */
#ifdef HAVE_KILLPG
	{"killpg",	srcos_killpg, METH_VARARGS, srcos_killpg__doc__},
#endif /* HAVE_KILLPG */
#ifdef HAVE_PLOCK
	{"plock",	srcos_plock, METH_VARARGS, srcos_plock__doc__},
#endif /* HAVE_PLOCK */
#ifdef HAVE_POPEN
	{"popen",	srcos_popen, METH_VARARGS, srcos_popen__doc__},
#ifdef MS_WINDOWS
	{"popen2",	win32_popen2, METH_VARARGS},
	{"popen3",	win32_popen3, METH_VARARGS},
	{"popen4",	win32_popen4, METH_VARARGS},
	{"startfile",	win32_startfile, METH_VARARGS, win32_startfile__doc__},
#else
#if defined(PYOS_OS2) && defined(PYCC_GCC)
	{"popen2",	os2emx_popen2, METH_VARARGS},
	{"popen3",	os2emx_popen3, METH_VARARGS},
	{"popen4",	os2emx_popen4, METH_VARARGS},
#endif
#endif
#endif /* HAVE_POPEN */
#ifdef HAVE_SETUID
	{"setuid",	srcos_setuid, METH_VARARGS, srcos_setuid__doc__},
#endif /* HAVE_SETUID */
#ifdef HAVE_SETEUID
	{"seteuid",	srcos_seteuid, METH_VARARGS, srcos_seteuid__doc__},
#endif /* HAVE_SETEUID */
#ifdef HAVE_SETEGID
	{"setegid",	srcos_setegid, METH_VARARGS, srcos_setegid__doc__},
#endif /* HAVE_SETEGID */
#ifdef HAVE_SETREUID
	{"setreuid",	srcos_setreuid, METH_VARARGS, srcos_setreuid__doc__},
#endif /* HAVE_SETREUID */
#ifdef HAVE_SETREGID
	{"setregid",	srcos_setregid,	METH_VARARGS, srcos_setregid__doc__},
#endif /* HAVE_SETREGID */
#ifdef HAVE_SETGID
	{"setgid",	srcos_setgid, METH_VARARGS, srcos_setgid__doc__},
#endif /* HAVE_SETGID */
#ifdef HAVE_SETGROUPS
	{"setgroups",	srcos_setgroups, METH_O, srcos_setgroups__doc__},
#endif /* HAVE_SETGROUPS */
#ifdef HAVE_GETPGID
	{"getpgid",	srcos_getpgid, METH_VARARGS, srcos_getpgid__doc__},
#endif /* HAVE_GETPGID */
#ifdef HAVE_SETPGRP
	{"setpgrp",	srcos_setpgrp, METH_NOARGS, srcos_setpgrp__doc__},
#endif /* HAVE_SETPGRP */
#ifdef HAVE_WAIT
	{"wait",	srcos_wait, METH_NOARGS, srcos_wait__doc__},
#endif /* HAVE_WAIT */
#ifdef HAVE_WAIT3
        {"wait3",	srcos_wait3, METH_VARARGS, srcos_wait3__doc__},
#endif /* HAVE_WAIT3 */
#ifdef HAVE_WAIT4
        {"wait4",	srcos_wait4, METH_VARARGS, srcos_wait4__doc__},
#endif /* HAVE_WAIT4 */
#if defined(HAVE_WAITPID) || defined(HAVE_CWAIT)
	{"waitpid",	srcos_waitpid, METH_VARARGS, srcos_waitpid__doc__},
#endif /* HAVE_WAITPID */
#ifdef HAVE_GETSID
	{"getsid",	srcos_getsid, METH_VARARGS, srcos_getsid__doc__},
#endif /* HAVE_GETSID */
#ifdef HAVE_SETSID
	{"setsid",	srcos_setsid, METH_NOARGS, srcos_setsid__doc__},
#endif /* HAVE_SETSID */
#ifdef HAVE_SETPGID
	{"setpgid",	srcos_setpgid, METH_VARARGS, srcos_setpgid__doc__},
#endif /* HAVE_SETPGID */
#ifdef HAVE_TCGETPGRP
	{"tcgetpgrp",	srcos_tcgetpgrp, METH_VARARGS, srcos_tcgetpgrp__doc__},
#endif /* HAVE_TCGETPGRP */
#ifdef HAVE_TCSETPGRP
	{"tcsetpgrp",	srcos_tcsetpgrp, METH_VARARGS, srcos_tcsetpgrp__doc__},
#endif /* HAVE_TCSETPGRP */
	{"open",	srcos_open, METH_VARARGS, srcos_open__doc__},
	{"close",	srcos_close, METH_VARARGS, srcos_close__doc__},
	{"closerange",	srcos_closerange, METH_VARARGS, srcos_closerange__doc__},
	{"dup",		srcos_dup, METH_VARARGS, srcos_dup__doc__},
	{"dup2",	srcos_dup2, METH_VARARGS, srcos_dup2__doc__},
	{"lseek",	srcos_lseek, METH_VARARGS, srcos_lseek__doc__},
	{"read",	srcos_read, METH_VARARGS, srcos_read__doc__},
	{"write",	srcos_write, METH_VARARGS, srcos_write__doc__},
	{"fstat",	srcos_fstat, METH_VARARGS, srcos_fstat__doc__},
	{"fdopen",	srcos_fdopen, METH_VARARGS, srcos_fdopen__doc__},
	{"isatty",	srcos_isatty, METH_VARARGS, srcos_isatty__doc__},
#ifdef HAVE_PIPE
	{"pipe",	srcos_pipe, METH_NOARGS, srcos_pipe__doc__},
#endif
#ifdef HAVE_MKFIFO
	{"mkfifo",	srcos_mkfifo, METH_VARARGS, srcos_mkfifo__doc__},
#endif
#if defined(HAVE_MKNOD) && defined(HAVE_MAKEDEV)
	{"mknod",	srcos_mknod, METH_VARARGS, srcos_mknod__doc__},
#endif
#ifdef HAVE_DEVICE_MACROS
	{"major",	srcos_major, METH_VARARGS, srcos_major__doc__},
	{"minor",	srcos_minor, METH_VARARGS, srcos_minor__doc__},
	{"makedev",	srcos_makedev, METH_VARARGS, srcos_makedev__doc__},
#endif
#ifdef HAVE_FTRUNCATE
	{"ftruncate",	srcos_ftruncate, METH_VARARGS, srcos_ftruncate__doc__},
#endif
#ifdef HAVE_PUTENV
	{"putenv",	srcos_putenv, METH_VARARGS, srcos_putenv__doc__},
#endif
#ifdef HAVE_UNSETENV
	{"unsetenv",	srcos_unsetenv, METH_VARARGS, srcos_unsetenv__doc__},
#endif
	{"strerror",	srcos_strerror, METH_VARARGS, srcos_strerror__doc__},
#ifdef HAVE_FCHDIR
	{"fchdir",	srcos_fchdir, METH_O, srcos_fchdir__doc__},
#endif
#ifdef HAVE_FSYNC
	{"fsync",       srcos_fsync, METH_O, srcos_fsync__doc__},
#endif
#ifdef HAVE_FDATASYNC
	{"fdatasync",   srcos_fdatasync,  METH_O, srcos_fdatasync__doc__},
#endif
#ifdef HAVE_SYS_WAIT_H
#ifdef WCOREDUMP
        {"WCOREDUMP",	srcos_WCOREDUMP, METH_VARARGS, srcos_WCOREDUMP__doc__},
#endif /* WCOREDUMP */
#ifdef WIFCONTINUED
        {"WIFCONTINUED",srcos_WIFCONTINUED, METH_VARARGS, srcos_WIFCONTINUED__doc__},
#endif /* WIFCONTINUED */
#ifdef WIFSTOPPED
        {"WIFSTOPPED",	srcos_WIFSTOPPED, METH_VARARGS, srcos_WIFSTOPPED__doc__},
#endif /* WIFSTOPPED */
#ifdef WIFSIGNALED
        {"WIFSIGNALED",	srcos_WIFSIGNALED, METH_VARARGS, srcos_WIFSIGNALED__doc__},
#endif /* WIFSIGNALED */
#ifdef WIFEXITED
        {"WIFEXITED",	srcos_WIFEXITED, METH_VARARGS, srcos_WIFEXITED__doc__},
#endif /* WIFEXITED */
#ifdef WEXITSTATUS
        {"WEXITSTATUS",	srcos_WEXITSTATUS, METH_VARARGS, srcos_WEXITSTATUS__doc__},
#endif /* WEXITSTATUS */
#ifdef WTERMSIG
        {"WTERMSIG",	srcos_WTERMSIG, METH_VARARGS, srcos_WTERMSIG__doc__},
#endif /* WTERMSIG */
#ifdef WSTOPSIG
        {"WSTOPSIG",	srcos_WSTOPSIG, METH_VARARGS, srcos_WSTOPSIG__doc__},
#endif /* WSTOPSIG */
#endif /* HAVE_SYS_WAIT_H */
#if defined(HAVE_FSTATVFS) && defined(HAVE_SYS_STATVFS_H)
	{"fstatvfs",	srcos_fstatvfs, METH_VARARGS, srcos_fstatvfs__doc__},
#endif
#if defined(HAVE_STATVFS) && defined(HAVE_SYS_STATVFS_H)
	{"statvfs",	srcos_statvfs, METH_VARARGS, srcos_statvfs__doc__},
#endif
#ifdef HAVE_TMPFILE
	{"tmpfile",	srcos_tmpfile, METH_NOARGS, srcos_tmpfile__doc__},
#endif
#ifdef HAVE_TEMPNAM
	{"tempnam",	srcos_tempnam, METH_VARARGS, srcos_tempnam__doc__},
#endif
#ifdef HAVE_TMPNAM
	{"tmpnam",	srcos_tmpnam, METH_NOARGS, srcos_tmpnam__doc__},
#endif
#ifdef HAVE_CONFSTR
	{"confstr",	srcos_confstr, METH_VARARGS, srcos_confstr__doc__},
#endif
#ifdef HAVE_SYSCONF
	{"sysconf",	srcos_sysconf, METH_VARARGS, srcos_sysconf__doc__},
#endif
#ifdef HAVE_FPATHCONF
	{"fpathconf",	srcos_fpathconf, METH_VARARGS, srcos_fpathconf__doc__},
#endif
#ifdef HAVE_PATHCONF
	{"pathconf",	srcos_pathconf, METH_VARARGS, srcos_pathconf__doc__},
#endif
	{"abort",	srcos_abort, METH_NOARGS, srcos_abort__doc__},
#ifdef MS_WINDOWS
	{"_getfullpathname",	srcos__getfullpathname, METH_VARARGS, NULL},
#endif
#ifdef HAVE_GETLOADAVG
	{"getloadavg",	srcos_getloadavg, METH_NOARGS, srcos_getloadavg__doc__},
#endif
 #ifdef MS_WINDOWS
 	{"urandom", win32_urandom, METH_VARARGS, win32_urandom__doc__},
 #endif
 #ifdef __VMS
 	{"urandom", vms_urandom, METH_VARARGS, vms_urandom__doc__},
 #endif
	{NULL,		NULL}		 /* Sentinel */
};


static int
ins(PyObject *module, char *symbol, long value)
{
        return PyModule_AddIntConstant(module, symbol, value);
}

#if defined(PYOS_OS2)
/* Insert Platform-Specific Constant Values (Strings & Numbers) of Common Use */
static int insertvalues(PyObject *module)
{
    APIRET    rc;
    ULONG     values[QSV_MAX+1];
    PyObject *v;
    char     *ver, tmp[50];

    Py_BEGIN_ALLOW_THREADS
    rc = DosQuerySysInfo(1L, QSV_MAX, &values[1], sizeof(ULONG) * QSV_MAX);
    Py_END_ALLOW_THREADS

    if (rc != NO_ERROR) {
        os2_error(rc);
        return -1;
    }

    if (ins(module, "meminstalled", values[QSV_TOTPHYSMEM])) return -1;
    if (ins(module, "memkernel",    values[QSV_TOTRESMEM])) return -1;
    if (ins(module, "memvirtual",   values[QSV_TOTAVAILMEM])) return -1;
    if (ins(module, "maxpathlen",   values[QSV_MAX_PATH_LENGTH])) return -1;
    if (ins(module, "maxnamelen",   values[QSV_MAX_COMP_LENGTH])) return -1;
    if (ins(module, "revision",     values[QSV_VERSION_REVISION])) return -1;
    if (ins(module, "timeslice",    values[QSV_MIN_SLICE])) return -1;

    switch (values[QSV_VERSION_MINOR]) {
    case 0:  ver = "2.00"; break;
    case 10: ver = "2.10"; break;
    case 11: ver = "2.11"; break;
    case 30: ver = "3.00"; break;
    case 40: ver = "4.00"; break;
    case 50: ver = "5.00"; break;
    default:
        PyOS_snprintf(tmp, sizeof(tmp),
        	      "%d-%d", values[QSV_VERSION_MAJOR],
                      values[QSV_VERSION_MINOR]);
        ver = &tmp[0];
    }

    /* Add Indicator of the Version of the Operating System */
    if (PyModule_AddStringConstant(module, "version", tmp) < 0)
        return -1;

    /* Add Indicator of Which Drive was Used to Boot the System */
    tmp[0] = 'A' + values[QSV_BOOT_DRIVE] - 1;
    tmp[1] = ':';
    tmp[2] = '\0';

    return PyModule_AddStringConstant(module, "bootdrive", tmp);
}
#endif

static int
all_ins(PyObject *d)
{
#ifdef F_OK
    if (ins(d, "F_OK", (long)F_OK)) return -1;
#endif
#ifdef R_OK
    if (ins(d, "R_OK", (long)R_OK)) return -1;
#endif
#ifdef W_OK
    if (ins(d, "W_OK", (long)W_OK)) return -1;
#endif
#ifdef X_OK
    if (ins(d, "X_OK", (long)X_OK)) return -1;
#endif
#ifdef NGROUPS_MAX
    if (ins(d, "NGROUPS_MAX", (long)NGROUPS_MAX)) return -1;
#endif
#ifdef TMP_MAX
    if (ins(d, "TMP_MAX", (long)TMP_MAX)) return -1;
#endif
#ifdef WCONTINUED
    if (ins(d, "WCONTINUED", (long)WCONTINUED)) return -1;
#endif
#ifdef WNOHANG
    if (ins(d, "WNOHANG", (long)WNOHANG)) return -1;
#endif
#ifdef WUNTRACED
    if (ins(d, "WUNTRACED", (long)WUNTRACED)) return -1;
#endif
#ifdef O_RDONLY
    if (ins(d, "O_RDONLY", (long)O_RDONLY)) return -1;
#endif
#ifdef O_WRONLY
    if (ins(d, "O_WRONLY", (long)O_WRONLY)) return -1;
#endif
#ifdef O_RDWR
    if (ins(d, "O_RDWR", (long)O_RDWR)) return -1;
#endif
#ifdef O_NDELAY
    if (ins(d, "O_NDELAY", (long)O_NDELAY)) return -1;
#endif
#ifdef O_NONBLOCK
    if (ins(d, "O_NONBLOCK", (long)O_NONBLOCK)) return -1;
#endif
#ifdef O_APPEND
    if (ins(d, "O_APPEND", (long)O_APPEND)) return -1;
#endif
#ifdef O_DSYNC
    if (ins(d, "O_DSYNC", (long)O_DSYNC)) return -1;
#endif
#ifdef O_RSYNC
    if (ins(d, "O_RSYNC", (long)O_RSYNC)) return -1;
#endif
#ifdef O_SYNC
    if (ins(d, "O_SYNC", (long)O_SYNC)) return -1;
#endif
#ifdef O_NOCTTY
    if (ins(d, "O_NOCTTY", (long)O_NOCTTY)) return -1;
#endif
#ifdef O_CREAT
    if (ins(d, "O_CREAT", (long)O_CREAT)) return -1;
#endif
#ifdef O_EXCL
    if (ins(d, "O_EXCL", (long)O_EXCL)) return -1;
#endif
#ifdef O_TRUNC
    if (ins(d, "O_TRUNC", (long)O_TRUNC)) return -1;
#endif
#ifdef O_BINARY
    if (ins(d, "O_BINARY", (long)O_BINARY)) return -1;
#endif
#ifdef O_TEXT
    if (ins(d, "O_TEXT", (long)O_TEXT)) return -1;
#endif
#ifdef O_LARGEFILE
    if (ins(d, "O_LARGEFILE", (long)O_LARGEFILE)) return -1;
#endif
#ifdef O_SHLOCK
    if (ins(d, "O_SHLOCK", (long)O_SHLOCK)) return -1;
#endif
#ifdef O_EXLOCK
    if (ins(d, "O_EXLOCK", (long)O_EXLOCK)) return -1;
#endif

/* MS Windows */
#ifdef O_NOINHERIT
    /* Don't inherit in child processes. */
    if (ins(d, "O_NOINHERIT", (long)O_NOINHERIT)) return -1;
#endif
#ifdef _O_SHORT_LIVED
    /* Optimize for short life (keep in memory). */
    /* MS forgot to define this one with a non-underscore form too. */
    if (ins(d, "O_SHORT_LIVED", (long)_O_SHORT_LIVED)) return -1;
#endif
#ifdef O_TEMPORARY
    /* Automatically delete when last handle is closed. */
    if (ins(d, "O_TEMPORARY", (long)O_TEMPORARY)) return -1;
#endif
#ifdef O_RANDOM
    /* Optimize for random access. */
    if (ins(d, "O_RANDOM", (long)O_RANDOM)) return -1;
#endif
#ifdef O_SEQUENTIAL
    /* Optimize for sequential access. */
    if (ins(d, "O_SEQUENTIAL", (long)O_SEQUENTIAL)) return -1;
#endif

/* GNU extensions. */
#ifdef O_ASYNC
    /* Send a SIGIO signal whenever input or output
       becomes available on file descriptor */
    if (ins(d, "O_ASYNC", (long)O_ASYNC)) return -1;
#endif
#ifdef O_DIRECT
    /* Direct disk access. */
    if (ins(d, "O_DIRECT", (long)O_DIRECT)) return -1;
#endif
#ifdef O_DIRECTORY
    /* Must be a directory.      */
    if (ins(d, "O_DIRECTORY", (long)O_DIRECTORY)) return -1;
#endif
#ifdef O_NOFOLLOW
    /* Do not follow links.      */
    if (ins(d, "O_NOFOLLOW", (long)O_NOFOLLOW)) return -1;
#endif
#ifdef O_NOATIME
    /* Do not update the access time. */
    if (ins(d, "O_NOATIME", (long)O_NOATIME)) return -1;
#endif

    /* These come from sysexits.h */
#ifdef EX_OK
    if (ins(d, "EX_OK", (long)EX_OK)) return -1;
#endif /* EX_OK */
#ifdef EX_USAGE
    if (ins(d, "EX_USAGE", (long)EX_USAGE)) return -1;
#endif /* EX_USAGE */
#ifdef EX_DATAERR
    if (ins(d, "EX_DATAERR", (long)EX_DATAERR)) return -1;
#endif /* EX_DATAERR */
#ifdef EX_NOINPUT
    if (ins(d, "EX_NOINPUT", (long)EX_NOINPUT)) return -1;
#endif /* EX_NOINPUT */
#ifdef EX_NOUSER
    if (ins(d, "EX_NOUSER", (long)EX_NOUSER)) return -1;
#endif /* EX_NOUSER */
#ifdef EX_NOHOST
    if (ins(d, "EX_NOHOST", (long)EX_NOHOST)) return -1;
#endif /* EX_NOHOST */
#ifdef EX_UNAVAILABLE
    if (ins(d, "EX_UNAVAILABLE", (long)EX_UNAVAILABLE)) return -1;
#endif /* EX_UNAVAILABLE */
#ifdef EX_SOFTWARE
    if (ins(d, "EX_SOFTWARE", (long)EX_SOFTWARE)) return -1;
#endif /* EX_SOFTWARE */
#ifdef EX_OSERR
    if (ins(d, "EX_OSERR", (long)EX_OSERR)) return -1;
#endif /* EX_OSERR */
#ifdef EX_OSFILE
    if (ins(d, "EX_OSFILE", (long)EX_OSFILE)) return -1;
#endif /* EX_OSFILE */
#ifdef EX_CANTCREAT
    if (ins(d, "EX_CANTCREAT", (long)EX_CANTCREAT)) return -1;
#endif /* EX_CANTCREAT */
#ifdef EX_IOERR
    if (ins(d, "EX_IOERR", (long)EX_IOERR)) return -1;
#endif /* EX_IOERR */
#ifdef EX_TEMPFAIL
    if (ins(d, "EX_TEMPFAIL", (long)EX_TEMPFAIL)) return -1;
#endif /* EX_TEMPFAIL */
#ifdef EX_PROTOCOL
    if (ins(d, "EX_PROTOCOL", (long)EX_PROTOCOL)) return -1;
#endif /* EX_PROTOCOL */
#ifdef EX_NOPERM
    if (ins(d, "EX_NOPERM", (long)EX_NOPERM)) return -1;
#endif /* EX_NOPERM */
#ifdef EX_CONFIG
    if (ins(d, "EX_CONFIG", (long)EX_CONFIG)) return -1;
#endif /* EX_CONFIG */
#ifdef EX_NOTFOUND
    if (ins(d, "EX_NOTFOUND", (long)EX_NOTFOUND)) return -1;
#endif /* EX_NOTFOUND */

#ifdef HAVE_SPAWNV
#if defined(PYOS_OS2) && defined(PYCC_GCC)
    if (ins(d, "P_WAIT", (long)P_WAIT)) return -1;
    if (ins(d, "P_NOWAIT", (long)P_NOWAIT)) return -1;
    if (ins(d, "P_OVERLAY", (long)P_OVERLAY)) return -1;
    if (ins(d, "P_DEBUG", (long)P_DEBUG)) return -1;
    if (ins(d, "P_SESSION", (long)P_SESSION)) return -1;
    if (ins(d, "P_DETACH", (long)P_DETACH)) return -1;
    if (ins(d, "P_PM", (long)P_PM)) return -1;
    if (ins(d, "P_DEFAULT", (long)P_DEFAULT)) return -1;
    if (ins(d, "P_MINIMIZE", (long)P_MINIMIZE)) return -1;
    if (ins(d, "P_MAXIMIZE", (long)P_MAXIMIZE)) return -1;
    if (ins(d, "P_FULLSCREEN", (long)P_FULLSCREEN)) return -1;
    if (ins(d, "P_WINDOWED", (long)P_WINDOWED)) return -1;
    if (ins(d, "P_FOREGROUND", (long)P_FOREGROUND)) return -1;
    if (ins(d, "P_BACKGROUND", (long)P_BACKGROUND)) return -1;
    if (ins(d, "P_NOCLOSE", (long)P_NOCLOSE)) return -1;
    if (ins(d, "P_NOSESSION", (long)P_NOSESSION)) return -1;
    if (ins(d, "P_QUOTE", (long)P_QUOTE)) return -1;
    if (ins(d, "P_TILDE", (long)P_TILDE)) return -1;
    if (ins(d, "P_UNRELATED", (long)P_UNRELATED)) return -1;
    if (ins(d, "P_DEBUGDESC", (long)P_DEBUGDESC)) return -1;
#else
    if (ins(d, "P_WAIT", (long)_P_WAIT)) return -1;
    if (ins(d, "P_NOWAIT", (long)_P_NOWAIT)) return -1;
    if (ins(d, "P_OVERLAY", (long)_OLD_P_OVERLAY)) return -1;
    if (ins(d, "P_NOWAITO", (long)_P_NOWAITO)) return -1;
    if (ins(d, "P_DETACH", (long)_P_DETACH)) return -1;
#endif
#endif

#if defined(PYOS_OS2)
    if (insertvalues(d)) return -1;
#endif
    return 0;
}


#if (defined(_MSC_VER) || defined(__WATCOMC__) || defined(__BORLANDC__)) && !defined(__QNX__)
#define INITFUNC initsrcos
#define MODNAME "nt"
#else
#define INITFUNC initposix
#define MODNAME "posix"
#endif

PyMODINIT_FUNC
INITFUNC(void)
{
    PyObject *m, *v;

    m = Py_InitModule3(MODNAME,
			   srcos_methods,
			   srcos__doc__);
    if (m == NULL)
        return;

    /* Initialize environ dictionary */
    v = convertenviron();
    Py_XINCREF(v);
    if (v == NULL || PyModule_AddObject(m, "environ", v) != 0)
        return;
    Py_DECREF(v);

    if (all_ins(m))
        return;

    if (setup_confname_tables(m))
        return;

    Py_INCREF(PyExc_OSError);
    PyModule_AddObject(m, "error", PyExc_OSError);

#ifdef HAVE_PUTENV
	if (srcos_putenv_garbage == NULL)
		srcos_putenv_garbage = PyDict_New();
#endif

    if (!initialized) {
        stat_result_desc.name = MODNAME ".stat_result";
        stat_result_desc.fields[7].name = PyStructSequence_UnnamedField;
        stat_result_desc.fields[8].name = PyStructSequence_UnnamedField;
        stat_result_desc.fields[9].name = PyStructSequence_UnnamedField;
        PyStructSequence_InitType(&StatResultType, &stat_result_desc);
        structseq_new = StatResultType.tp_new;
        StatResultType.tp_new = statresult_new;

        statvfs_result_desc.name = MODNAME ".statvfs_result";
        PyStructSequence_InitType(&StatVFSResultType, &statvfs_result_desc);
#ifdef NEED_TICKS_PER_SECOND
#  if defined(HAVE_SYSCONF) && defined(_SC_CLK_TCK)
        ticks_per_second = sysconf(_SC_CLK_TCK);
#  elif defined(HZ)
        ticks_per_second = HZ;
#  else
        ticks_per_second = 60; /* magic fallback value; may be bogus */
#  endif
#endif
    }
    Py_INCREF((PyObject*) &StatResultType);
    PyModule_AddObject(m, "stat_result", (PyObject*) &StatResultType);
    Py_INCREF((PyObject*) &StatVFSResultType);
    PyModule_AddObject(m, "statvfs_result",
                       (PyObject*) &StatVFSResultType);
    initialized = 1;

#ifdef __APPLE__
    /*
     * Step 2 of weak-linking support on Mac OS X.
     *
     * The code below removes functions that are not available on the
     * currently active platform.
     *
     * This block allow one to use a python binary that was build on
     * OSX 10.4 on OSX 10.3, without loosing access to new APIs on
     * OSX 10.4.
     */
#ifdef HAVE_FSTATVFS
    if (fstatvfs == NULL) {
        if (PyObject_DelAttrString(m, "fstatvfs") == -1) {
            return;
        }
    }
#endif /* HAVE_FSTATVFS */

#ifdef HAVE_STATVFS
    if (statvfs == NULL) {
        if (PyObject_DelAttrString(m, "statvfs") == -1) {
            return;
        }
    }
#endif /* HAVE_STATVFS */

# ifdef HAVE_LCHOWN
    if (lchown == NULL) {
        if (PyObject_DelAttrString(m, "lchown") == -1) {
            return;
        }
    }
#endif /* HAVE_LCHOWN */


#endif /* __APPLE__ */

}


