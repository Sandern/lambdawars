//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef FOWMGR_H
#define FOWMGR_H

#ifdef _WIN32
#pragma once
#endif

#include "hl2wars/hl2wars_shareddefs.h"
#include "igamesystem.h"

// Forward declarations
#ifdef CLIENT_DLL
	#include "view_shared.h"

	class ITexture;
	class CFOWTextureRegen;
#endif // CLIENT_DLL

// A simple list of all entities updating the fog of war.
// For each player/owner a list is created.
struct FOWListInfo
{
	int m_iOwnerNumber;
	CUtlVector<EHANDLE> m_EntityList;
	FOWListInfo *m_pNext;
};
void FOWAddEntity( FOWListInfo **pHead, int ownernumber, CBaseEntity *pEnt );
void FOWRemoveEntity( FOWListInfo **pHead, int ownernumber, CBaseEntity *pEnt );

extern ConVar sv_fogofwar;

// The client uses a smaller size type, since we only need to compute the fog of war of the local player
#ifdef CLIENT_DLL
	#define FOWSIZE_TYPE unsigned char
	#define FOWHIDDEN_MASK 0x00
	#define FOWCLEAR_MASK 0xff
	#define FOWINDEX(x, y) (x + y*m_nGridSize)
#else
	#define FOWSIZE_TYPE unsigned short
	#define FOWHIDDEN_MASK 0x0000
	#define FOWCLEAR_MASK 0xffff
	#define FOWINDEX(x, y) x + y*m_nGridSize
#endif // CLIENT_DLL

class CBaseEntity;
class CBasePlayer;

//-----------------------------------------------------------------------------
// The class responsible for updating the fog of war. Runs on both client and server.
//
// Entities can interact in three ways with the fog of war, determined by the fow flags.
//-----------------------------------------------------------------------------
class CFogOfWarMgr : public CBaseGameSystemPerFrame
{
public:
	CFogOfWarMgr();
	virtual ~CFogOfWarMgr();

	friend class CFOWTextureRegen;
	friend class NavFOWTestAreas;
	friend class NavFOWTestAreasVisibility;

public: // CBaseGameSystem overrides

	virtual char const	*Name() { return "CFogOfWarMgr"; }

	virtual bool		Init();
	virtual void		Shutdown();

	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPostEntity();

	// Heightmap
	bool				CanCalculateHeightMap();
	void				CalculateHeightMap();
	void				SaveHeightMap();
	void				LoadHeightMap();
	void				ModifyHeightAtTile( int x, int y, float fHeight );
	void				ModifyHeightAtPoint( const Vector &vPoint, float fHeight );
	void				ModifyHeightAtExtent( const Vector &vMins, const Vector &vMaxs, float fHeight );
	float				GetHeightAtTile( int x, int y );
	float				GetHeightAtPoint( const Vector &vPoint );

	// Gets called each frame
#ifdef CLIENT_DLL
	int					CalculateRenderTargetSize( void );
	void				InitRenderTargets( void );
	void				ShutdownRenderTargets( void );
	void				OnResolutionChanged();

	virtual void		Update( float frametime );
#else
	virtual void		PreClientUpdate();
#endif // CLIENT_DLL
	virtual void		UpdateShared();

	virtual bool		NeedsUpdate();

	void				ResetExplored( void );

private:
	void				AllocateFogOfWar();
	void				DeallocateFogOfWar();

#ifdef CLIENT_DLL
	void				UpdateTexture( bool bConverge, float fTime = 0.0f );
	
	void				BeginRenderFow( void );
	void				RenderFow( CUtlVector< CUtlVector< FowPos_t > > &DrawPoints, int n, int cx, int cy );
	void				EndRenderFow();
	void				RenderFowClear();
	void				RenderFogOfWar( float frametime );

	void				CopyExploredToRenderBuffer( CTextureReference &RenderBuffer );
	void				CopyCurrentStateToExplored( void );
#endif // CLIENT_DLL

protected:
	// Fog of war updating
	void				ClearFogOfWarTo( FOWSIZE_TYPE state );
	void				ClearNewPositions( FOWListInfo *pFOWList, int iOwner, bool bClear = true );
	
	void				FillLine( int x1, int x2, int y, FOWSIZE_TYPE mask );
	void				UpdateFogOfWarState( int x, int y, int radius, FOWSIZE_TYPE mask );

	void				ShadowCast( int cx, int cy, int row, float start, float end,
									int radius, int xx, int xy, int yx, int yys, FOWSIZE_TYPE mask, int eyez
#ifdef CLIENT_DLL
									, CUtlVector< FowPos_t > &EndPos, bool bInverseSlope 
#endif // CLIENT_DLL		
						);
	void				DoShadowCasting( CBaseEntity *pEnt, int radius, FOWSIZE_TYPE mask );

public:
	FOWSIZE_TYPE		CalculatePlayerVisibilityMask( int iOwner );
	void				UpdateVisibility();

	Vector				ComputeWorldPosition( int x, int y );
	void				ComputeFOWPosition( const Vector &vPos, int &x, int &y );

	bool				FOWShouldShow( CBaseEntity *pEnt, CBasePlayer *pPlayer );
#ifdef CLIENT_DLL
	bool				FOWShouldShow( CBaseEntity *pEnt );
#else
	bool				FOWShouldShow( CBaseEntity *pEnt, int owner );
#endif // CLIENT_DLL
public:
	// Lists
	void				AddFogUpdater(int owner, CBaseEntity *pEnt );
	void				RemoveFogUpdater(int owner, CBaseEntity *pEnt );

	void				AddFogEntity(CBaseEntity *pEnt);
	void				RemoveFogEntity(CBaseEntity *pEnt );

public:
#ifndef CLIENT_DLL
	// Querying
	bool				PointInFOW( const Vector &vPoint, int iOwner );

	// For entities that are not hidden in the fog of war, but not updated (FOWFLAG_NOTRANSMIT, but not FOWFLAG_HIDDEN)
	// These entities should be transmitted at least once.
	bool				IsEntityKnown( int iPlayerIndex, int iEntIndex );
	void				MarkEntityKnown( int iPlayerIndex, int iEntIndex );
	void				MarkEntityUnKnown( int iPlayerIndex, int iEntIndex );
	void				ResetToUnknown( int iPlayerIndex );
	void				ResetToKnown( int iPlayerIndex );
#endif // CLIENT_DLL

#if 0
	// Marker
	virtual void		AddSpecialUpdater(  Vector origin, float radius, float delay, float lifetime, int owner );
	virtual void		AddSpecialUpdater( fogofwar_t data );
	virtual void		UpdateSpecialUpdateList();
#endif // 0

	// Debug
	void				DebugPrintEntities();

private:
	bool		m_bActive;
	float		m_fNeedsUpdate;
	float		m_fNextConvergeUpdate;
	bool		m_bWasFogofwarOn;
	bool		m_bHeightMapLoaded;

	int			m_nGridSize, m_nTileSize;
	CUtlVector< FOWSIZE_TYPE > m_FogOfWar;
	CUtlVector< int > m_TileHeights;
	
	FOWListInfo *m_pFogUpdaterListHead;
	CUtlVector<CBaseEntity *> m_FogEntities;
	
#if 0
	CUtlVector< fogofwar_t > m_SpecialUpdateList;
#endif // 0

	// FOWFLAG_NOTRANSMIT related
#ifndef CLIENT_DLL
	CBitVec<MAX_EDICTS> m_KnownEntities[MAX_PLAYERS]; // Used for known functions
#endif // CLIENT_DLL

	// Visualization on the client
#ifdef CLIENT_DLL
	CMaterialReference m_FOWMaterial;
	CMaterialReference m_FOWImMaterial;
	CMaterialReference m_FOWBlurMaterial;
	CMaterialReference m_FOWExploredMaterial;
	CTextureReference m_RenderBuffer;
	CTextureReference m_RenderBufferBlur;
	CTextureReference m_RenderBufferIM;
	CTextureReference m_RenderBufferExplored;

	CFOWTextureRegen *m_pTextureRegen;
	CUtlVector< FOWSIZE_TYPE > m_FogOfWarTextureData;

	Frustum m_Frustum;
	bool m_bRenderingFOW;
#endif // CLIENT_DLL
};

extern CFogOfWarMgr* FogOfWarMgr();

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
inline Vector CFogOfWarMgr::ComputeWorldPosition( int x, int y )
{
	return Vector(
		x * m_nTileSize - ( FOW_WORLDSIZE / 2 ),
		y * m_nTileSize - ( FOW_WORLDSIZE / 2 ),
		0.0f	
	);
}

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
inline void	CFogOfWarMgr::ComputeFOWPosition( const Vector &vPos, int &x, int &y )
{
	x = floor( ( (vPos.x + (FOW_WORLDSIZE / 2)) / (float)m_nTileSize ) + 0.5f );
	y = floor( ( (vPos.y + (FOW_WORLDSIZE / 2)) / (float)m_nTileSize ) + 0.5f );
}

#ifndef CLIENT_DLL
inline bool CFogOfWarMgr::PointInFOW( const Vector &vPoint, int iOwner )
{
	int x, y;
	ComputeFOWPosition( vPoint, x, y );
	return ( m_FogOfWar[FOWINDEX(x, y)] & (1 << iOwner) ) == 0;
}

inline bool CFogOfWarMgr::IsEntityKnown( int iPlayerIndex, int iEntIndex )
{
	return m_KnownEntities[iPlayerIndex].IsBitSet( iEntIndex );
}

inline void CFogOfWarMgr::MarkEntityKnown( int iPlayerIndex, int iEntIndex )
{
	m_KnownEntities[iPlayerIndex].Set( iEntIndex );
}

inline void CFogOfWarMgr::MarkEntityUnKnown( int iPlayerIndex, int iEntIndex )
{
	m_KnownEntities[iPlayerIndex].Clear( iEntIndex );
}

inline void CFogOfWarMgr::ResetToUnknown( int iPlayerIndex )
{
	m_KnownEntities[iPlayerIndex].ClearAll();
}

inline void CFogOfWarMgr::ResetToKnown( int iPlayerIndex )
{
	m_KnownEntities[iPlayerIndex].SetAll();
}
#endif // CLIENT_DLL

#endif // FOWMGR_H
