//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CLIENTSIDEEFFECTS_H
#define CLIENTSIDEEFFECTS_H
#ifdef _WIN32
#pragma once
#endif

class	Vector;
struct	FXQuadData_t;
struct	FXLineData_t;

#ifdef HL2WARS_DLL
enum ClientSideEffectFlag_t
{
	BITS_CLIENTEFFECT_NORMAL			= 0x00000001,
	BITS_CLIENTEFFECT_POSTSCREEN		= 0x00000002,
};
#endif // HL2WARS_DLL

//-----------------------------------------------------------------------------
// Purpose: Base class for client side effects
//-----------------------------------------------------------------------------
abstract_class CClientSideEffect
{
public:
	// Constructs the named effect
#ifdef HL2WARS_DLL
						CClientSideEffect( const char *name, int flags = BITS_CLIENTEFFECT_NORMAL );
#else
						CClientSideEffect( const char *name );
#endif // HL2WARS_DLL
	virtual				~CClientSideEffect( void );

	// Update/Draw the effect
	// Derived classes must implement this method!
	virtual void		Draw( double frametime ) = 0;
	// Returns name of effect
	virtual const char	*GetName( void );
	// Retuns whether the effect is still active
	virtual bool		IsActive( void );
	// Sets the effect to inactive so it can be destroed
	virtual void		Destroy( void );
#ifdef HL2WARS_DLL
	// Get draw flags
	int					GetFlags() { return m_iFlags; }
#endif // HL2WARS_DLL

// =======================================
// PySource Additions
// =======================================
#ifdef ENABLE_PYTHON
	friend class CEffectsList;

	// Get python instance
	boost::python::object	GetPyInstance();

protected:
	// Python allocated?
	boost::python::object	m_pyRef;
#endif // ENABLE_PYTHON
// =======================================
// END PySource Additions
// =======================================

private:
	// Name of effect ( static data )
	const char			*m_pszName;
	// Is the effect active
	bool				m_bActive;
#ifdef HL2WARS_DLL
	// Draw flags
	int					m_iFlags;
#endif // HL2WARS_DLL
};

//-----------------------------------------------------------------------------
// Purpose: Base interface to effects list
//-----------------------------------------------------------------------------
abstract_class IEffectsList
{
public:
	virtual			~IEffectsList( void ) {}

	// Add an effect to the list of effects
	virtual void	AddEffect( CClientSideEffect *effect ) = 0;
#ifdef HL2WARS_DLL
	// Simulate/Update/Draw effects on list
	virtual void	DrawEffects( double frametime, int flags = BITS_CLIENTEFFECT_NORMAL ) = 0;
#endif // HL2WARS_DLL
	// Flush out all effects fbrom the list
	virtual void	Flush( void ) = 0;
};

extern IEffectsList *clienteffects;

class IMaterialSystem;

//Actual function references
void FX_AddCube( const Vector &mins, const Vector &maxs, const Vector &vColor, float life, const char *materialName );
void FX_AddCenteredCube( const Vector &center, float size, const Vector &vColor, float life, const char *materialName );
void FX_AddStaticLine( const Vector& start, const Vector& end, float scale, float life, const char *materialName, unsigned char flags );
void FX_AddDiscreetLine( const Vector& start, const Vector& direction, float velocity, float length, float clipLength, float scale, float life, const char *shader );

void FX_AddLine( const FXLineData_t &data );

void FX_AddQuad( const FXQuadData_t &data );

void FX_AddQuad( const Vector &origin, 
				 const Vector &normal, 
				 float startSize, 
				 float endSize, 
				 float sizeBias,
				 float startAlpha, 
				 float endAlpha,
				 float alphaBias,
				 float yaw,
				 float deltaYaw,
				 const Vector &color, 
				 float lifeTime, 
				 const char *shader, 
				 unsigned int flags );

// For safe addition of client effects
void SetFXCreationAllowed( bool state );
bool FXCreationAllowed( void );

#endif // CLIENTSIDEEFFECTS_H
