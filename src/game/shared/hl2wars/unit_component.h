//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose:		Base Unit
//
//=============================================================================//

#ifndef UNIT_COMPONENT_H
#define UNIT_COMPONENT_H

#ifdef _WIN32
#pragma once
#endif

#include "unit_base_shared.h"

class UnitComponent 
{
	DECLARE_CLASS_NOBASE(UnitComponent);
public:
#ifndef DISABLE_PYTHON
	UnitComponent( boost::python::object outer );
#endif // DISABLE_PYTHON

	CUnitBase *					GetOuter();
	const CUnitBase *			GetOuter() const;
#ifndef DISABLE_PYTHON
	boost::python::object		GetPyOuter();
#endif // DISABLE_PYTHON

public:
	//
	// Common services provided by CUnitBase, Convenience methods to simplify derived code
	//
	int					GetFlags( void ) const;

	const Vector &		GetLocalOrigin() const;
	void 				SetLocalOrigin( const Vector &origin );

	const Vector &		GetAbsOrigin() const;
	const QAngle&		GetAbsAngles() const;

	void				SetLocalAngles( const QAngle& angles );
	const QAngle &		GetLocalAngles( void ) const;

	void				SetLocalVelocity( const Vector &vecVelocity );
	void				SetAbsVelocity( const Vector &vecVelocity );
	const Vector&		GetLocalVelocity( ) const;
	const Vector&		GetAbsVelocity( ) const;

	const Vector&		WorldAlignMins() const;
	const Vector&		WorldAlignMaxs() const;
	Vector 				WorldSpaceCenter() const;
	const Vector&		WorldAlignSize( ) const;

	int 				GetCollisionGroup() const;

	void				SetSolid( SolidType_t val );
	SolidType_t			GetSolid() const;

	float				GetGravity() const;
	void				SetGravity( float );

	void				SetGroundEntity( CBaseEntity *ground );
	CBaseEntity			*GetGroundEntity();

	// Animation related
	int					GetSequence();
	void				SetSequence(int nSequence);
	void				ResetSequence(int nSequence);
	bool				IsActivityFinished( void );
	bool				IsSequenceFinished( void );

	void				SetCycle( float flCycle );
	float				GetCycle() const;

	void				StudioFrameAdvance();

protected:
	CUnitBase *m_pOuter;

#ifndef DISABLE_PYTHON
private:
	boost::python::object m_pyOuter;
#endif // DISABLE_PYTHON
};

//-----------------------------------------------------------------------------
inline CUnitBase *UnitComponent::GetOuter() 			
{ 
	return m_pOuter; 
}

inline const CUnitBase *UnitComponent::GetOuter() const 	
{ 
	return m_pOuter; 
}

#ifndef DISABLE_PYTHON
inline boost::python::object UnitComponent::GetPyOuter()		
{ 
	return m_pyOuter; 
}
#endif // DISABLE_PYTHON

//-----------------------------------------------------------------------------

inline int UnitComponent::GetFlags( void ) const
{
	return GetOuter()->GetFlags();
}

//-----------------------------------------------------------------------------

inline void UnitComponent::SetLocalAngles( const QAngle& angles )
{
	GetOuter()->SetLocalAngles( angles );
}

//-----------------------------------------------------------------------------

inline const QAngle &UnitComponent::GetLocalAngles( void ) const
{
	return GetOuter()->GetLocalAngles();
}

//-----------------------------------------------------------------------------

inline const Vector &UnitComponent::GetLocalOrigin() const
{
	return GetOuter()->GetLocalOrigin();
}

inline void UnitComponent::SetLocalOrigin(const Vector &origin)
{
	GetOuter()->SetLocalOrigin(origin);
}

//-----------------------------------------------------------------------------
inline void UnitComponent::SetLocalVelocity( const Vector &vecVelocity )
{
	GetOuter()->SetLocalVelocity(vecVelocity);
}

inline void UnitComponent::SetAbsVelocity( const Vector &vecVelocity )
{
	GetOuter()->SetAbsVelocity(vecVelocity);
}

inline const Vector& UnitComponent::GetLocalVelocity( ) const
{
	return GetOuter()->GetLocalVelocity();
}

inline const Vector& UnitComponent::GetAbsVelocity( ) const
{
	return GetOuter()->GetAbsVelocity();
}

//-----------------------------------------------------------------------------

inline const Vector &UnitComponent::GetAbsOrigin() const
{
	return GetOuter()->GetAbsOrigin();
}

//-----------------------------------------------------------------------------

inline const QAngle &UnitComponent::GetAbsAngles() const
{
	return GetOuter()->GetAbsAngles();
}

//-----------------------------------------------------------------------------

inline void UnitComponent::SetSolid( SolidType_t val )
{
	GetOuter()->SetSolid(val);
}

//-----------------------------------------------------------------------------

inline SolidType_t UnitComponent::GetSolid() const
{
	return GetOuter()->GetSolid();
}

//-----------------------------------------------------------------------------

inline const Vector &UnitComponent::WorldAlignMins() const
{
	return GetOuter()->WorldAlignMins();
}

//-----------------------------------------------------------------------------

inline const Vector &UnitComponent::WorldAlignMaxs() const
{
	return GetOuter()->WorldAlignMaxs();
}

//-----------------------------------------------------------------------------

inline Vector UnitComponent::WorldSpaceCenter() const
{
	return GetOuter()->WorldSpaceCenter();
}

//-----------------------------------------------------------------------------

inline const Vector& UnitComponent::WorldAlignSize( ) const 
{
	return GetOuter()->WorldAlignSize();
}

//-----------------------------------------------------------------------------

inline float UnitComponent::GetGravity() const
{
	return GetOuter()->GetGravity();
}

//-----------------------------------------------------------------------------

inline void UnitComponent::SetGravity( float flGravity )
{
	GetOuter()->SetGravity( flGravity );
}

//-----------------------------------------------------------------------------

inline int UnitComponent::GetCollisionGroup() const
{
	return GetOuter()->GetCollisionGroup();
}

//-----------------------------------------------------------------------------

inline void	UnitComponent::SetGroundEntity( CBaseEntity *ground )
{
	GetOuter()->SetGroundEntity(ground);
}

inline CBaseEntity *UnitComponent::GetGroundEntity()
{
	return GetOuter()->GetGroundEntity();
}

//-----------------------------------------------------------------------------
inline int UnitComponent::GetSequence()
{
	return GetOuter()->GetSequence();
}

inline void UnitComponent::SetSequence(int nSequence)
{
	GetOuter()->SetSequence(nSequence);
}

inline void UnitComponent::ResetSequence(int nSequence)
{
	GetOuter()->ResetSequence(nSequence);
}

//-----------------------------------------------------------------------------

inline bool UnitComponent::IsActivityFinished( void )
{
	return GetOuter()->IsActivityFinished();
}

inline bool UnitComponent::IsSequenceFinished( void )
{
	return GetOuter()->IsSequenceFinished();
}

//-----------------------------------------------------------------------------

inline void UnitComponent::SetCycle( float flCycle )
{
	GetOuter()->SetCycle(flCycle);
}

inline float UnitComponent::GetCycle() const
{
	return GetOuter()->GetCycle();
}

//-----------------------------------------------------------------------------

inline void UnitComponent::StudioFrameAdvance()
{
	GetOuter()->StudioFrameAdvance();
}


#endif // UNIT_COMPONENT_H