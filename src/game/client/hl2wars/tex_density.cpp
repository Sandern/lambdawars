//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2wars/tex_density.h"
#include "density_weight_map.h"
#include "unit_base_shared.h"
#include "materialsystem/IMaterialVar.h"
#include "hl2wars/fowmgr.h"
#include "materialsystem/ITexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DENSITY_TILESIZE 16

void CC_Generate_Density_Tex( void )
{
	CMaterialReference DensityMaterial("dev/density", TEXTURE_GROUP_CLIENT_EFFECTS);
	ITexture *pTex = materials->FindTexture("dev/density", TEXTURE_GROUP_CLIENT_EFFECTS, true);
	Msg("density_generate_tex: Generating density texture\n");
	if( pTex )
		pTex->Download();
	else
		Msg("Texture not found\n");
	DensityMaterial.Shutdown();
}
static ConCommand density_generate_tex("density_generate_tex", CC_Generate_Density_Tex, "", FCVAR_CHEAT);


//-----------------------------------------------------------------------------
// Regenerate the texture
//-----------------------------------------------------------------------------
void CDensityTextureRegen::RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect )
{
	unsigned char *imageData = pVTFTexture->ImageData( 0, 0, 0 );
	int x, y, unitx, unity, offset, width, height, radius;

	Msg("Generating density texture...\n");

	width = pVTFTexture->Width();
	height = pVTFTexture->Height();

	memset( imageData, 0, width * height );

	CUnitBase *pUnit;
	CUnitBase **pList = g_Unit_Manager.AccessUnits();
	for( int i = 0; i < g_Unit_Manager.NumUnits(); i++ )
	{
		pUnit = pList[i];
		DensityWeightsMap map;
		map.Init( pUnit );
		map.SetType( DENSITY_GAUSSIAN );
		map.OnCollisionSizeChanged();
		

		const Vector &vOrigin = pUnit->GetAbsOrigin();
		unitx = (vOrigin.x + (FOW_WORLDSIZE / 2)) / (float)DENSITY_TILESIZE;
		unity = (vOrigin.y + (FOW_WORLDSIZE / 2)) / (float)DENSITY_TILESIZE;
		radius = (int)((pUnit->CollisionProp()->BoundingRadius2D() * 2) / DENSITY_TILESIZE);

		Msg("%d Density %d %d %d %d %d %d %d\n", pUnit->entindex(), map.GetSizeX(), map.GetSizeY(), unitx, unity, radius, MAX(unity-radius, 0), MIN(unity+radius, height) );

		for( y = MAX(unity-radius, 0); y < MIN(unity+radius, height); y++ )
		{
			for( x = MAX(unitx-radius, 0); x < MIN(unitx+radius, width); x++ )
			{
				offset = ( x + y * width );

				Vector vTest = Vector(
							x * DENSITY_TILESIZE - ( FOW_WORLDSIZE / 2 ),
							y * DENSITY_TILESIZE - ( FOW_WORLDSIZE / 2 ),
							0.0f	
						);

				imageData[offset] += map.Get( vTest ) * 255;
			}
		}
	}
}


#pragma warning (disable:4355)

CDensityMaterialProxy::CDensityMaterialProxy()
{
	m_pMaterial = NULL;
	m_pFOWTextureVar = NULL;
	m_pTexture = NULL;
	m_pTextureRegen = NULL;

}

#pragma warning (default:4355)

CDensityMaterialProxy::~CDensityMaterialProxy()
{
	// Disconnect the texture regenerator...
	if (m_pFOWTextureVar)
	{
		ITexture *pFOWTexture = m_pFOWTextureVar->GetTextureValue();
		if (pFOWTexture)
			pFOWTexture->SetTextureRegenerator( NULL );
	}
}

bool CDensityMaterialProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	bool found;
	m_pFOWTextureVar = pMaterial->FindVar("$basetexture", &found, false);  // Get a reference to our base texture variable
	if( !found )
	{
		m_pFOWTextureVar = NULL;
		return false;
	}

	m_pTexture = m_pFOWTextureVar->GetTextureValue();  // Now grab a ref to the actual texture
	if (m_pTexture != NULL)
	{
		m_pTextureRegen = new CDensityTextureRegen( this );  // Here we create our regenerator
		m_pTexture->SetTextureRegenerator(m_pTextureRegen); // And here we attach it to the texture.
	}

	return true;
}

//-----------------------------------------------------------------------------
// Called when the texture is bound...
//-----------------------------------------------------------------------------
void CDensityMaterialProxy::OnBind( void *pC_BaseEntity )
{
}

IMaterial *CDensityMaterialProxy::GetMaterial()
{
	if ( !m_pFOWTextureVar )
		return NULL;

	return m_pFOWTextureVar->GetOwningMaterial();
}

EXPOSE_INTERFACE( CDensityMaterialProxy, IMaterialProxy, "density" IMATERIAL_PROXY_INTERFACE_VERSION );
