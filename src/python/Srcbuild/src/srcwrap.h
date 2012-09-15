#ifndef SRCWRAP_H
#define SRCWRAP_H



#ifdef __cplusplus
extern "C" {
#endif
	void SrcMsg(const char *pMsg, ...);
	void SrcWarning(const char *pMsg, ...);
	void SrcDevMsg(int level, const char *pMsg, ...);

	// Filesystem wrapers (annoying..)
	int SrcFileSystem_GetCurrentDirectory( char *pDirectory, int maxlen );
	void SrcFileSystem_CreateDirHierarchy( const char *path, const char *pathID );
	void SrcFileSystem_RemoveFile( char const* pRelativePath, const char *pathID );

#ifdef __cplusplus
}
#endif

extern int SrcPyGetFullPathSilent( const char *pAssumedRelativePath, char *pFullPath, int size );
extern int SrcPyIsClient( void );
extern int getmoddir( const char *path, int len );

#endif // SRCWRAP_H