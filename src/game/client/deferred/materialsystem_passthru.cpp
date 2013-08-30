
#include "cbase.h"
#include "deferred/deferred_shared_common.h"

#include "materialsystem/imaterialvar.h"

static void ShaderReplace( const char *szShadername, IMaterial *pMat )
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

	const bool bAlphaBlending = pMat->IsTranslucent() || pMat->GetMaterialVarFlag( MATERIAL_VAR_TRANSLUCENT );
	const bool bAlphaTesting = pMat->IsAlphaTested() || pMat->GetMaterialVarFlag( MATERIAL_VAR_ALPHATEST );
	const bool bSelfillum = pMat->GetMaterialVarFlag( MATERIAL_VAR_SELFILLUM );
	const bool bDecal = pszOldShadername != NULL && Q_stristr( pszOldShadername,"decal" ) != NULL ||
		pszMatname != NULL && Q_stristr( pszMatname, "decal" ) != NULL ||
		pMat->GetMaterialVarFlag( MATERIAL_VAR_DECAL );

	if ( bDecal )
	{
		msg->SetInt( "$decal", 1 );
	}

	if ( bAlphaTesting )
	{
		msg->SetInt( "$alphatest", 1 );
	}
	else if ( bAlphaBlending )
	{
		msg->SetInt( "$translucent", 1 );
	}

	if ( pMat->IsTwoSided() )
	{
		msg->SetInt( "$nocull", 1 );
	}

	if ( bSelfillum )
	{
		msg->SetInt( "$selfillum", 1 );
	}

	pMat->SetShaderAndParams(msg);

	pMat->RefreshPreservingMaterialVars();

	msg->deleteThis();
}

static const char *pszShaderReplaceDict[][2] = {
	"vertexlitgeneric",			"DEFERRED_MODEL",

	"lightmappedgeneric",		"DEFERRED_BRUSH",
	"worldvertextransition",	"DEFERRED_BRUSH",
	"multiblend",				"DEFERRED_BRUSH",

	"decalmodulate",			"DEFERRED_DECALMODULATE",
};
static const int iNumShaderReplaceDict = ARRAYSIZE( pszShaderReplaceDict );

IMaterial *CDeferredMaterialSystem::FindMaterial( char const* pMaterialName, const char *pTextureGroupName, bool complain, const char *pComplainPrefix )
{
	IMaterial *pMat = m_pBaseMaterialsPassThru->FindMaterial( pMaterialName, pTextureGroupName, complain );

	if ( pMat != NULL )
	{
		const char *pszShaderName = pMat->GetShaderName();
		if ( pszShaderName && *pszShaderName )
		{
			for ( int i = 0; i < iNumShaderReplaceDict; i++ )
			{
				if ( Q_stristr( pszShaderName, pszShaderReplaceDict[i][0] ) == pszShaderName )
				{
					ShaderReplace( pszShaderReplaceDict[i][1], pMat );
					break;
				}
			}
		}
	}

	return pMat;
}
