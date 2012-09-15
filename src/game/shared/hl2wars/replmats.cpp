//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "filesystem.h"
#include "utlbuffer.h"
#include "igamesystem.h"
#include "materialsystem_passtru.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/ITexture.h"

#ifdef HL2WARS_ASW_DLL
	#include "deferred/cdeferred_manager_client.h"
#endif // HL2WARS_ASW_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#if !defined( SWARM_DLL ) && !( HL2WARS_ASW_DLL )
	#define REPLMAT_WRITE_MATERIAL
//#endif 

#ifdef HL2WARS_ASW_DLL
	#define REPLMAT_USE_REPL_NAME
#endif // HL2WARS_ASW_DLL

static ConVar g_debug_replmat("g_debug_replmat", "0", FCVAR_CHEAT);

extern bool CheckMaterial( const char *pMaterial );

//-----------------------------------------------------------------------------
// List of materials that should be replaced
//-----------------------------------------------------------------------------
#ifndef REPLMAT_WRITE_MATERIAL
static const char *pszShaderReplaceDict[][2] = {
#ifdef HL2WARS_ASW_DLL
	"globallitsimple",			"DEFERRED_MODEL",
	"vertexlitgeneric",			"DEFERRED_MODEL",
	"lightmappedgeneric",		"DEFERRED_BRUSH",
	"worldvertextransition",	"DEFERRED_BRUSH",
	"multiblend",				"DEFERRED_BRUSH",
#else
	"globallitsimple",			"VertexLitGeneric",
	"vertexlitgeneric",			"SDK_VertexLitGeneric",
	"lightmappedgeneric",		"SDK_LightmappedGeneric",
	"worldvertextransition",	"SDK_WorldVertexTransition",
	"WorldVertexTransition_DX9", "SDK_WorldVertexTransition",
	"water",					"SDK_Water",
#endif // HL2WARS_ASW_DLL
};
#else
static const char *pszShaderReplaceDict[][2] = {
#ifndef HL2WARS_ASW_DLL
	"globallitsimple",				"\"SDK_VertexLitGeneric\"",
	"vertexlitgeneric",			"\"SDK_VertexLitGeneric\"",
	"lightmappedgeneric",		"\"SDK_LightmappedGeneric\"",
	"worldvertextransition",	"\"SDK_WorldVertexTransition\"",
	"water",					"\"SDK_Water\"",
#else
	"globallitsimple",			"\"DEFERRED_MODEL\"",
	"vertexlitgeneric",			"\"DEFERRED_MODEL\"",
	"lightmappedgeneric",		"\"DEFERRED_BRUSH\"",
	"worldvertextransition",	"\"DEFERRED_BRUSH\"",
	"multiblend",				"\"DEFERRED_BRUSH\"",
#endif // HL2WARS_ASW_DLL
};
#endif 
static const int iNumShaderReplaceDict = ARRAYSIZE( pszShaderReplaceDict );

// Copied from cdeferred_manager_client.cpp
static void ShaderReplaceReplMat( const char *pMaterialName, const char *szShadername, IMaterial *pMat )
{
	const char *pszOldShadername = pMat->GetShaderName();
	const char *pszMatname = pMat->GetName();

	KeyValues *msg = new KeyValues( szShadername );

	int nParams = pMat->ShaderParamCount();
	IMaterialVar **pParams = pMat->GetShaderParams();

	char str[ 512 ];

	for ( int i = 0; i < nParams; ++i )
	{
		IMaterialVar *pVar = pParams[ i ];
		const char *pVarName = pVar->GetName();

		if (!stricmp("$flags", pVarName) || 
			!stricmp("$flags_defined", pVarName) || 
			!stricmp("$flags2", pVarName) || 
			!stricmp("$flags_defined2", pVarName) )
			continue;

		MaterialVarType_t vartype = pVar->GetType();
		switch ( vartype )
		{
		case MATERIAL_VAR_TYPE_FLOAT:
			msg->SetFloat( pVarName, pVar->GetFloatValue() );
			break;

		case MATERIAL_VAR_TYPE_INT:
			msg->SetInt( pVarName, pVar->GetIntValue() );
			break;

		case MATERIAL_VAR_TYPE_STRING:
			msg->SetString( pVarName, pVar->GetStringValue() );
			break;

		case MATERIAL_VAR_TYPE_FOURCC:
			//Assert( 0 ); // JDTODO
			break;

		case MATERIAL_VAR_TYPE_VECTOR:
			{
				const float *pVal = pVar->GetVecValue();
				int dim = pVar->VectorSize();
				switch ( dim )
				{
				case 1:
					V_snprintf( str, sizeof( str ), "[%f]", pVal[ 0 ] );
					break;
				case 2:
					V_snprintf( str, sizeof( str ), "[%f %f]", pVal[ 0 ], pVal[ 1 ] );
					break;
				case 3:
					V_snprintf( str, sizeof( str ), "[%f %f %f]", pVal[ 0 ], pVal[ 1 ], pVal[ 2 ] );
					break;
				case 4:
					V_snprintf( str, sizeof( str ), "[%f %f %f %f]", pVal[ 0 ], pVal[ 1 ], pVal[ 2 ], pVal[ 3 ] );
					break;
				default:
					Assert( 0 );
					*str = 0;
				}
				msg->SetString( pVarName, str );
			}
			break;

		case MATERIAL_VAR_TYPE_MATRIX:
			{
				const VMatrix &matrix = pVar->GetMatrixValue();
				const float *pVal = matrix.Base();
				V_snprintf( str, sizeof( str ),
					"[%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f]",
					pVal[ 0 ],  pVal[ 1 ],  pVal[ 2 ],  pVal[ 3 ],
					pVal[ 4 ],  pVal[ 5 ],  pVal[ 6 ],  pVal[ 7 ],
					pVal[ 8 ],  pVal[ 9 ],  pVal[ 10 ], pVal[ 11 ],
					pVal[ 12 ], pVal[ 13 ], pVal[ 14 ], pVal[ 15 ] );
				msg->SetString( pVarName, str );
			}
			break;

		case MATERIAL_VAR_TYPE_TEXTURE:
						msg->SetString( pVarName, pVar->GetTextureValue()->GetName() );
			break;

		case MATERIAL_VAR_TYPE_MATERIAL:
						msg->SetString( pVarName, pVar->GetMaterialValue()->GetName() );
			break;
		}
	}

	bool alphaBlending = pMat->IsTranslucent() || pMat->GetMaterialVarFlag( MATERIAL_VAR_TRANSLUCENT );
	bool translucentOverride = pMat->IsAlphaTested() || pMat->GetMaterialVarFlag( MATERIAL_VAR_ALPHATEST ) || alphaBlending;

	bool bDecal = pszOldShadername != NULL && Q_stristr( pszOldShadername,"decal" ) != NULL ||
		pszMatname != NULL && Q_stristr( pszMatname, "decal" ) != NULL ||
		pMat->GetMaterialVarFlag( MATERIAL_VAR_DECAL );

	if ( bDecal )
	{
		msg->SetInt( "$decal", 1 );

		if ( alphaBlending )
			msg->SetInt( "$translucent", 1 );
	}
	else if ( translucentOverride )
	{
		msg->SetInt( "$alphatest", 1 );
	}

	if ( pMat->IsTwoSided() )
	{
		msg->SetInt( "$nocull", 1 );
	}

	//KeyValuesDumpAsDevMsg( msg );
	
	pMat->SetShaderAndParams(msg);

	pMat->RefreshPreservingMaterialVars();

	msg->deleteThis();
}

//-----------------------------------------------------------------------------
// Overrides FindMaterial and replaces the material if the shader name is found
// in "shadernames_tocheck".
// TODO: Also override the reload material functions and update the material, 
//		 otherwise we keep reading from the replacement directory.
//-----------------------------------------------------------------------------
static const char *suffix_replmat = "__replmat";

class CReplMaterialSystem : public CWarsPassThruMaterialSystem 
{
public:
	~CReplMaterialSystem()
	{
		m_CheckedDictionary.Purge();
#ifdef REPLMAT_USE_REPL_NAME
		m_ReplacementDictionary.PurgeAndDeleteElements();
#endif // REPLMAT_USE_REPL_NAME
	}

	virtual IMaterial *	FindMaterial( char const* pMaterialName, const char *pTextureGroupName, bool complain = true, const char *pComplainPrefix = NULL ) 
	{
#ifdef REPLMAT_WRITE_MATERIAL
		int idx;
		bool bReplaced = false;

		if( pMaterialName )
		{
#ifdef REPLMAT_USE_REPL_NAME
			idx = m_ReplacementDictionary.Find( pMaterialName );
			if( idx != m_ReplacementDictionary.InvalidIndex( ) )
			{
				return BaseClass::FindMaterial( m_ReplacementDictionary[idx], pTextureGroupName, complain, pComplainPrefix ); 
			}
#endif // REPLMAT_USE_REPL_NAME

			idx = m_CheckedDictionary.Find( pMaterialName );
			if( idx == m_CheckedDictionary.InvalidIndex( ) )
			{
				if( g_debug_replmat.GetBool() )
					Msg( "FindMaterial: Checking %s\n", pMaterialName );

				if( CheckMaterial( pMaterialName ) )
				{
					if( g_debug_replmat.GetBool() )
						Msg( "FindMaterial: Replaced material %s\n", pMaterialName );
					bReplaced = true;
					m_CheckedDictionary.Insert( pMaterialName, true );

#ifdef REPLMAT_USE_REPL_NAME
					int n = Q_strlen( suffix_replmat ) + Q_strlen( pMaterialName ) + 1;
					char *pReplacementMat = new char[n];
					Q_snprintf( pReplacementMat, n, "%s%s", pMaterialName, suffix_replmat );
					m_ReplacementDictionary.Insert( pMaterialName, pReplacementMat );
#endif // REPLMAT_USE_REPL_NAME
				}
				else
				{
					m_CheckedDictionary.Insert( pMaterialName, false );
				}
			}
		}
		else
		{
			Warning("FindMaterial: invalid material name\n");
		}

		return BaseClass::FindMaterial( pMaterialName, pTextureGroupName, complain, pComplainPrefix ); 
#else
		IMaterial *pMat = BaseClass::FindMaterial( pMaterialName, pTextureGroupName, complain, pComplainPrefix );
		if( !pMat || pMat->IsErrorMaterial() )
			return pMat;

		//if( Q_strlen( pMaterialName ) < 5 || (Q_strnicmp( pMaterialName, "tools", 5) && Q_strnicmp( pMaterialName, "debug", 5)) )
		//{
			const char *pShaderName = pMat->GetShaderName();
			if( pShaderName )
			{
				for(int i=0; i< iNumShaderReplaceDict; i++)
				{
					if( Q_stristr(pShaderName, pszShaderReplaceDict[i][0]) == pShaderName )
					{
						ShaderReplaceReplMat( pMaterialName, pszShaderReplaceDict[i][1], pMat );
						break;
					}
				}
			}
		//}
		return pMat;
#endif // REPLMAT_WRITE_MATERIAL
	}

private:
	CUtlDict< bool, int > m_CheckedDictionary;
#ifdef REPLMAT_USE_REPL_NAME
	CUtlDict< char *, int > m_ReplacementDictionary;
#endif // REPLMAT_USE_REPL_NAME
};

//-----------------------------------------------------------------------------
// Purpose: Read a single token from buffer (0 terminated)
//-----------------------------------------------------------------------------
#define KEYVALUES_TOKEN_SIZE	1024
static char s_pReplMatTokenBuf[KEYVALUES_TOKEN_SIZE];
#pragma warning (disable:4706)
const char *ReadToken( CUtlBuffer &buf, bool &wasQuoted, bool &wasConditional, bool bHasEscapeSequences=false )
{
	wasQuoted = false;
	wasConditional = false;

	if ( !buf.IsValid() )
		return NULL; 

	// eating white spaces and remarks loop
	while ( true )
	{
		buf.EatWhiteSpace();
		if ( !buf.IsValid() )
			return NULL;	// file ends after reading whitespaces

		// stop if it's not a comment; a new token starts here
		if ( !buf.EatCPPComment() )
			break;
	}

	const char *c = (const char*)buf.PeekGet( sizeof(char), 0 );
	if ( !c )
		return NULL;

	// read quoted strings specially
	if ( *c == '\"' )
	{
		wasQuoted = true;
		buf.GetDelimitedString( bHasEscapeSequences ? GetCStringCharConversion() : GetNoEscCharConversion(), 
			s_pReplMatTokenBuf, KEYVALUES_TOKEN_SIZE );
		return s_pReplMatTokenBuf;
	}

	if ( *c == '{' || *c == '}' )
	{
		// it's a control char, just add this one char and stop reading
		s_pReplMatTokenBuf[0] = *c;
		s_pReplMatTokenBuf[1] = 0;
		buf.SeekGet( CUtlBuffer::SEEK_CURRENT, 1 );
		return s_pReplMatTokenBuf;
	}

	// read in the token until we hit a whitespace or a control character
	bool bReportedError = false;
	bool bConditionalStart = false;
	int nCount = 0;
	while ( c = (const char*)buf.PeekGet( sizeof(char), 0 ) )
	{
		// end of file
		if ( *c == 0 )
			break;

		// break if any control character appears in non quoted tokens
		if ( *c == '"' || *c == '{' || *c == '}' )
			break;

		if ( *c == '[' )
			bConditionalStart = true;

		if ( *c == ']' && bConditionalStart )
		{
			wasConditional = true;
		}

		// break on whitespace
		if ( V_isspace(*c) )
			break;

		if (nCount < (KEYVALUES_TOKEN_SIZE-1) )
		{
			s_pReplMatTokenBuf[nCount++] = *c;	// add char to buffer
		}
		else if ( !bReportedError )
		{
			bReportedError = true;
		}

		buf.SeekGet( CUtlBuffer::SEEK_CURRENT, 1 );
	}
	s_pReplMatTokenBuf[ nCount ] = 0;
	return s_pReplMatTokenBuf;
}

//-----------------------------------------------------------------------------
// Purpose: Check a material to see if it should be replaced.
//			Returns true if replaced.
//-----------------------------------------------------------------------------
bool CheckMaterial( const char *pMaterialName )
{
	bool wasQuoted;
	bool wasConditional;

	// Get filepath and load it into a buffer
	char filepath[MAX_PATH];
	Q_snprintf(filepath, MAX_PATH, "materials/%s.vmt", pMaterialName);

	CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
	bool bOk = filesystem->ReadFile( filepath, NULL, buf );
	if( !bOk )
		return false;

	// the first thing must be a key
	const char *s = ReadToken( buf, wasQuoted, wasConditional );
	if ( !buf.IsValid() || !s || *s == 0 )
		return false;

	if( Q_stricmp( "patch", s ) == 0 )
	{
		KeyValues *pRealMat = new KeyValues( s );
		if( !pRealMat || !pRealMat->LoadFromFile( filesystem, filepath ) )
		{
			return false;
		}

		char buf[MAX_PATH];
		Q_strncpy( buf, pRealMat->GetString("include", ""), MAX_PATH );
		Q_StripExtension( buf, buf, MAX_PATH );

		bool bReplaced = CheckMaterial( buf + 10 ); // Assume name starts with "materials/", so offset 10
		if( bReplaced )
		{
#ifdef REPLMAT_USE_REPL_NAME
			// Set new include path
			char transname[MAX_PATH];
			Q_snprintf( transname, MAX_PATH, "%s%s.vmt", buf, suffix_replmat);
			pRealMat->SetString( "include", transname );
			Q_snprintf( transname, MAX_PATH, "replacementmaterials/materials/%s%s.vmt", pMaterialName, suffix_replmat );

			// Ensure the directory is created
			char replacementfilepathnofile[MAX_PATH];
			Q_strncpy( replacementfilepathnofile, transname, MAX_PATH );
			Q_StripFilename( replacementfilepathnofile );
			if( filesystem->FileExists(replacementfilepathnofile) == false )
			{
				filesystem->CreateDirHierarchy(replacementfilepathnofile, "MOD");
			}

			pRealMat->SaveToFile( filesystem, transname, "MOD" );
#endif // REPLMAT_USE_REPL_NAME
		}

		pRealMat->deleteThis();
		return bReplaced;
	}

	for(int i=0; i< iNumShaderReplaceDict; i++)
	{
		if( Q_stristr(s_pReplMatTokenBuf, pszShaderReplaceDict[i][0]) == s_pReplMatTokenBuf )
		{
			char replacementfilepath[MAX_PATH];
#ifdef REPLMAT_USE_REPL_NAME
			Q_snprintf( replacementfilepath, MAX_PATH, "replacementmaterials/materials/%s%s.vmt", pMaterialName, suffix_replmat );
#else
			Q_snprintf( replacementfilepath, MAX_PATH, "replacementmaterials/%s", filepath );
#endif // REPLMAT_USE_REPL_NAME

			// Ensure the directory is created
			char replacementfilepathnofile[MAX_PATH];
			Q_strncpy( replacementfilepathnofile, replacementfilepath, MAX_PATH );
			Q_StripFilename( replacementfilepathnofile );
			if( filesystem->FileExists(replacementfilepathnofile) == false )
			{
				filesystem->CreateDirHierarchy(replacementfilepathnofile, "MOD");
			}

			// Now write the new vmt
			FileHandle_t fh;
			fh = filesystem->Open(replacementfilepath, "wb");
			if ( FILESYSTEM_INVALID_HANDLE != fh )
			{	
				filesystem->Write(pszShaderReplaceDict[i][1], Q_strlen(pszShaderReplaceDict[i][1]), fh);
				filesystem->Write((char *)buf.Base()+buf.TellGet(), buf.TellMaxPut()-buf.TellGet(), fh);
				filesystem->Close(fh);
			}
			else
			{
				Warning("CheckMaterial: Failed to open %s!\n", replacementfilepath);
			}

			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Delete a folder tree
//-----------------------------------------------------------------------------
static void DeleteFolderRecursive( const char *pFolder )
{
	FileFindHandle_t findHandle;
	char wildcard[MAX_PATH];

	Q_snprintf(wildcard, MAX_PATH, "%s/*", pFolder);
	const char *pFileName = filesystem->FindFirstEx( wildcard, "MOD", &findHandle );

	while ( pFileName )
	{
		char fullpath[MAX_PATH];
		Q_snprintf(fullpath, MAX_PATH, "%s/%s", pFolder, pFileName );

		if( Q_strncmp(pFileName, ".", 1) == 0 || Q_strncmp(pFileName, "..", 2) == 0 )
		{
			// do nothing
		}
		else if( filesystem->FindIsDirectory( findHandle ) )
		{
			DeleteFolderRecursive( fullpath );
			// TODO: Delete empty folder
		}
		else
		{
			filesystem->RemoveFile( fullpath, "MOD" );
		}
		pFileName = filesystem->FindNext( findHandle );
	}
	filesystem->FindClose( findHandle );
}

void ClearReplacementFolder()
{
	DeleteFolderRecursive("replacementmaterials/");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class ReplacementSystem : public CAutoGameSystem
{
public:
	ReplacementSystem() : m_pOldMaterialSystem(NULL) {}

	virtual bool Init() { Enable(); return true; }
	virtual void Shutdown() { Disable(); }
	virtual void LevelInitPreEntity();

	void Enable();
	void Disable();
	bool IsEnabled() { return m_pOldMaterialSystem != NULL; }

private:
	CReplMaterialSystem m_MatSysPassTru;
	IMaterialSystem *m_pOldMaterialSystem;
};

static ReplacementSystem s_ReplacementSystem;

void ReplacementSystem::LevelInitPreEntity()
{
#ifdef REPLMAT_WRITE_MATERIAL
	// Create replacement directory if needed
	const char *replacementpath = "replacementmaterials\\";
	if( filesystem->FileExists( replacementpath ) == false )
	{
		filesystem->CreateDirHierarchy( replacementpath, "MOD" );
	}

	// Force replacement path in front of the path
	char searchPaths[_MAX_PATH];
	filesystem->GetSearchPath( "MOD", true, searchPaths, sizeof( searchPaths ) );
	char searchpathreplacements[MAX_PATH];
	Q_snprintf(searchpathreplacements, MAX_PATH, "%sreplacementmaterials\\", searchPaths);
	filesystem->AddSearchPath(searchpathreplacements, "GAME", PATH_ADD_TO_HEAD);
#endif // REPLMAT_WRITE_MATERIAL
}

extern void ReadLightingConfig();
void ReplacementSystem::Enable()
{
	if( m_pOldMaterialSystem )
		return;

	return;

#ifdef HL2WARS_ASW_DLL
	bool bDeferredRendering = GetDeferredManager()->IsDeferredRenderingEnabled();
	if( !bDeferredRendering )
		return;
#endif // HL2WARS_ASW_DLL

	Msg("Enabled material replacement system\n");
#ifdef REPLMAT_WRITE_MATERIAL
	ClearReplacementFolder();

	// Add replacement folder to our path
	char searchPaths[_MAX_PATH];
	filesystem->GetSearchPath( "MOD", true, searchPaths, sizeof( searchPaths ) );
	char searchpathreplacements[MAX_PATH];
	Q_snprintf(searchpathreplacements, MAX_PATH, "%sreplacementmaterials\\", searchPaths);
	filesystem->AddSearchPath(searchpathreplacements, "GAME", PATH_ADD_TO_HEAD);
#endif // REPLMAT_WRITE_MATERIAL

	// Replace material system
	m_MatSysPassTru.InitPassThru( materials );

	m_pOldMaterialSystem = materials;
	materials = &m_MatSysPassTru;
	engine->Mat_Stub( materials );
}

void ReplacementSystem::Disable()
{
	if( m_pOldMaterialSystem )
	{
		Msg("Disabled material replacement system\n");

#ifdef REPLMAT_WRITE_MATERIAL
		// Remove replacement folder from our path
		char searchPaths[_MAX_PATH];
		filesystem->GetSearchPath( "MOD", true, searchPaths, sizeof( searchPaths ) );
		char searchpathreplacements[MAX_PATH];
		Q_snprintf(searchpathreplacements, MAX_PATH, "%sreplacementmaterials\\", searchPaths);
		filesystem->RemoveSearchPath(searchpathreplacements, "GAME");
#endif // REPLMAT_WRITE_MATERIAL

		materials = m_pOldMaterialSystem;
		engine->Mat_Stub( materials );
		m_pOldMaterialSystem = NULL;
		ClearReplacementFolder();
	}
}

CON_COMMAND_F( toggle_replmat, "Toggles the material replacement system", FCVAR_CHEAT )
{
	if( s_ReplacementSystem.IsEnabled() )
	{
		s_ReplacementSystem.Disable();
	}
	else
	{
		s_ReplacementSystem.Enable();
	}

	materials->UncacheAllMaterials();
	materials->CacheUsedMaterials();
	materials->ReloadMaterials();
}

#ifdef REPLMAT_WRITE_MATERIAL
CON_COMMAND_F( replmat_readdpath, "Add replacement directory in front of the path again.", FCVAR_CHEAT )
{
	// Add replacement folder to our path
	char searchPaths[_MAX_PATH];
	filesystem->GetSearchPath( "MOD", true, searchPaths, sizeof( searchPaths ) );
	char searchpathreplacements[MAX_PATH];
	Q_snprintf(searchpathreplacements, MAX_PATH, "%sreplacementmaterials\\", searchPaths);
	filesystem->AddSearchPath(searchpathreplacements, "GAME", PATH_ADD_TO_HEAD);
}
#endif // REPLMAT_WRITE_MATERIAL
