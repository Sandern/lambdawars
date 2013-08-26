//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy.h"
#include "srcpy_physics.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace bp = boost::python;

//-----------------------------------------------------------------------------
// Purpose: IPhysicsShadowController base wrapper for python.
//-----------------------------------------------------------------------------
PyPhysicsShadowController::PyPhysicsShadowController( boost::python::object refPyPhysObj )
{
	m_pPyPhysObj = NULL;
	m_pShadCont = NULL;
	if( refPyPhysObj.ptr() == Py_None )
		return;

	m_pPyPhysObj = boost::python::extract<PyPhysicsObject *>(refPyPhysObj);
	m_pPyPhysObj->CheckValid();
	m_pShadCont = m_pPyPhysObj->m_pPhysObj->GetShadowController();
}

void PyPhysicsShadowController::CheckValid()
{
	if( m_pPyPhysObj == NULL )
	{
		PyErr_SetString(PyExc_ValueError, "PhysicsObject invalid" );
		throw boost::python::error_already_set();
	}
	m_pPyPhysObj->CheckValid();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
// ? Is using memcmp on a PyObject correct ?
bool PyPhysicsShadowController::Cmp( boost::python::object other )
{
	if( other.ptr() == Py_None ) 
	{
		return m_pShadCont != NULL;
	}

	if( PyObject_IsInstance(other.ptr(), boost::python::object(_physics.attr("PyPhysicsShadowController")).ptr()) )
	{
		IPhysicsShadowController *other_ext = boost::python::extract<IPhysicsShadowController *>(other);
		return V_memcmp(other_ext, m_pShadCont, sizeof(IPhysicsShadowController *));
	}

	return V_memcmp(other.ptr(), m_pShadCont, sizeof(IPhysicsShadowController *));
}

bool PyPhysicsShadowController::NonZero()
{
	return m_pShadCont != NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsShadowController::Update( const Vector &position, const QAngle &angles, float timeOffset )
{
	CheckValid();
	m_pShadCont->Update(position, angles, timeOffset);
}

void PyPhysicsShadowController::MaxSpeed( float maxSpeed, float maxAngularSpeed )
{
	CheckValid();
	m_pShadCont->MaxSpeed(maxSpeed, maxAngularSpeed);
}

void PyPhysicsShadowController::StepUp( float height )
{
	CheckValid();
	m_pShadCont->StepUp(height);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsShadowController::SetTeleportDistance( float teleportDistance )
{
	CheckValid();
	m_pShadCont->SetTeleportDistance(teleportDistance);
}

bool PyPhysicsShadowController::AllowsTranslation()
{
	CheckValid();
	return m_pShadCont->AllowsTranslation();
}

bool PyPhysicsShadowController::AllowsRotation()
{
	CheckValid();
	return m_pShadCont->AllowsRotation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsShadowController::SetPhysicallyControlled( bool isPhysicallyControlled )
{
	CheckValid();
	m_pShadCont->SetPhysicallyControlled(isPhysicallyControlled);
}

bool PyPhysicsShadowController::IsPhysicallyControlled()
{
	CheckValid();
	return m_pShadCont->IsPhysicallyControlled();
}

void PyPhysicsShadowController::GetLastImpulse( Vector *pOut )
{
	CheckValid();
	m_pShadCont->GetLastImpulse(pOut);
}

void PyPhysicsShadowController::UseShadowMaterial( bool bUseShadowMaterial )
{
	CheckValid();
	m_pShadCont->UseShadowMaterial(bUseShadowMaterial);
}

void PyPhysicsShadowController::ObjectMaterialChanged( int materialIndex )
{
	CheckValid();
	m_pShadCont->ObjectMaterialChanged(materialIndex);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float PyPhysicsShadowController::GetTargetPosition( Vector *pPositionOut, QAngle *pAnglesOut )
{
	CheckValid();
	return m_pShadCont->GetTargetPosition(pPositionOut, pAnglesOut);
}

float PyPhysicsShadowController::GetTeleportDistance( void )
{
	CheckValid();
	return m_pShadCont->GetTeleportDistance();
}

void PyPhysicsShadowController::GetMaxSpeed( float *pMaxSpeedOut, float *pMaxAngularSpeedOut )
{
	CheckValid();
	m_pShadCont->GetMaxSpeed(pMaxSpeedOut, pMaxAngularSpeedOut);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
PyPhysicsObject::PyPhysicsObject() :
	 m_pPhysObj(NULL), m_hEnt(NULL), m_bOwnsPhysObject(false)
{
}

PyPhysicsObject::PyPhysicsObject( IPhysicsObject *pPhysObj ) : m_hEnt(NULL), m_bOwnsPhysObject(false)
{
	if( !pPhysObj )
	{
		PyErr_SetString(PyExc_ValueError, "Invalid physics object" );
		throw boost::python::error_already_set();
		return;
	}

	InitFromPhysicsObject( pPhysObj );
}

PyPhysicsObject::PyPhysicsObject( CBaseEntity *pEnt ) : m_hEnt(NULL), m_bOwnsPhysObject(false)
{
	if( !pEnt || !pEnt->VPhysicsGetObject() )
	{
		m_hEnt = NULL;
		PyErr_SetString(PyExc_ValueError, "Invalid Entity or physics object" );
		throw boost::python::error_already_set();
		return;
	}

	m_hEnt = pEnt;
	m_pPhysObj = pEnt->VPhysicsGetObject();
}

PyPhysicsObject::~PyPhysicsObject()
{
	Destroy();
}

void PyPhysicsObject::InitFromPhysicsObject( IPhysicsObject *pPhysObj )
{
	m_pPhysObj = pPhysObj;

	m_hEnt = reinterpret_cast<CBaseEntity *>( pPhysObj->GetGameData() );
}

void PyPhysicsObject::CheckValid() 
{
	// Fast check: physic object is owned by entity
	if( m_hEnt )
	{
		if( m_hEnt->VPhysicsGetObject() == m_pPhysObj )
			return; // Valid
	}

	// None of the above checks worked, so invalid object
	PyErr_SetString(PyExc_ValueError, "PhysicsObject invalid" );
	throw boost::python::error_already_set();
}

void PyPhysicsObject::Destroy()
{
	if( !m_bOwnsPhysObject )
		return;

	CheckValid();

	if( !g_EntityCollisionHash )
		return;

	PhysDestroyObject(m_pPhysObj, m_hEnt);
}

//-----------------------------------------------------------------------------
// Purpose: IPhysicsObject base wrapper for python.
//-----------------------------------------------------------------------------
bool PyPhysicsObject::Cmp( boost::python::object other )
{
	if( other.ptr() == Py_None ) 
	{
		return m_pPhysObj != NULL;
	}

	if( PyObject_IsInstance(other.ptr(), boost::python::object(_physics.attr("PyPhysicsObject")).ptr()) )
	{
		IPhysicsObject *other_ext = boost::python::extract<IPhysicsObject *>(other);
		return V_memcmp(other_ext, m_pPhysObj, sizeof(IPhysicsObject *));
	}

	return V_memcmp(other.ptr(), m_pPhysObj, sizeof(IPhysicsObject *));
}

bool PyPhysicsObject::NonZero()
{
	return m_pPhysObj != NULL;
}

bool PyPhysicsObject::IsStatic()
{
	CheckValid();
	return m_pPhysObj->IsStatic();
}

bool PyPhysicsObject::IsAsleep()
{
	CheckValid();
	return m_pPhysObj->IsAsleep();
}

bool PyPhysicsObject::IsTrigger()
{
	CheckValid();
	return m_pPhysObj->IsTrigger();
}

bool PyPhysicsObject::IsFluid()
{
	CheckValid();
	return m_pPhysObj->IsFluid();
}

bool PyPhysicsObject::IsHinged()
{
	CheckValid();
	return m_pPhysObj->IsHinged();
}

bool PyPhysicsObject::IsCollisionEnabled()
{
	CheckValid();
	return m_pPhysObj->IsCollisionEnabled();
}

bool PyPhysicsObject::IsGravityEnabled()
{
	CheckValid();
	return m_pPhysObj->IsGravityEnabled();
}

bool PyPhysicsObject::IsDragEnabled()
{
	CheckValid();
	return m_pPhysObj->IsDragEnabled();
}

bool PyPhysicsObject::IsMotionEnabled()
{
	CheckValid();
	return m_pPhysObj->IsMotionEnabled();
}

bool PyPhysicsObject::IsMoveable()
{
	CheckValid();
	return m_pPhysObj->IsMoveable();
}

bool PyPhysicsObject::IsAttachedToConstraint(bool bExternalOnly)
{
	CheckValid();
	return m_pPhysObj->IsAttachedToConstraint(bExternalOnly);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsObject::EnableCollisions( bool enable )
{
	CheckValid();
	m_pPhysObj->EnableCollisions(enable);
}

void PyPhysicsObject::EnableGravity( bool enable )
{
	CheckValid();
	m_pPhysObj->EnableGravity(enable);
}

void PyPhysicsObject::EnableDrag( bool enable )
{
	CheckValid();
	m_pPhysObj->EnableDrag(enable);
}

void PyPhysicsObject::EnableMotion( bool enable )
{
	CheckValid();
	m_pPhysObj->EnableMotion(enable);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsObject::SetGameFlags( unsigned short userFlags )
{
	CheckValid();
	m_pPhysObj->SetGameFlags(userFlags);
}

unsigned short PyPhysicsObject::GetGameFlags( void )
{
	CheckValid();
	return m_pPhysObj->GetGameFlags();
}

void PyPhysicsObject::SetGameIndex( unsigned short gameIndex )
{
	CheckValid();
	m_pPhysObj->SetGameIndex(gameIndex);
}

unsigned short PyPhysicsObject::GetGameIndex( void )
{
	CheckValid();
	return m_pPhysObj->GetGameIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsObject::SetCallbackFlags( unsigned short callbackflags )
{
	CheckValid();
	m_pPhysObj->SetCallbackFlags(callbackflags);
}

unsigned short PyPhysicsObject::GetCallbackFlags( void )
{
	CheckValid();
	return m_pPhysObj->GetCallbackFlags();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsObject::Wake( void )
{
	CheckValid();
	m_pPhysObj->Wake();
}

void PyPhysicsObject::Sleep( void )
{
	CheckValid();
	m_pPhysObj->Sleep();
}

void PyPhysicsObject::RecheckCollisionFilter()
{
	CheckValid();
	m_pPhysObj->RecheckCollisionFilter();
}

void PyPhysicsObject::RecheckContactPoints()
{
	CheckValid();
	m_pPhysObj->RecheckContactPoints();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsObject::SetMass( float mass )
{
	CheckValid();
	m_pPhysObj->SetMass(mass);
}

float PyPhysicsObject::GetMass( void )
{
	CheckValid();
	return m_pPhysObj->GetMass();
}

float PyPhysicsObject::GetInvMass( void )
{
	CheckValid();
	return m_pPhysObj->GetInvMass();
}

Vector PyPhysicsObject::GetInertia( void )
{
	CheckValid();
	return m_pPhysObj->GetInertia();
}

Vector PyPhysicsObject::GetInvInertia( void )
{
	CheckValid();
	return m_pPhysObj->GetInvInertia();
}

void PyPhysicsObject::SetInertia( const Vector &inertia )
{
	CheckValid();
	m_pPhysObj->SetInertia(inertia);
}

void PyPhysicsObject::SetDamping( float speed, float rot )
{
	CheckValid();
	m_pPhysObj->SetDamping(&speed, &rot);
}
bp::tuple PyPhysicsObject::GetDamping()
{
	CheckValid();
    float speed;
    float rot;
	m_pPhysObj->GetDamping(&speed, &rot);
	return bp::make_tuple(speed, rot);
}

void PyPhysicsObject::SetDragCoefficient( float *pDrag, float *pAngularDrag )
{
	CheckValid();
	m_pPhysObj->SetDragCoefficient(pDrag, pAngularDrag);
}
void PyPhysicsObject::SetBuoyancyRatio( float ratio )
{
	CheckValid();
	m_pPhysObj->SetBuoyancyRatio(ratio);
}

int	PyPhysicsObject::GetMaterialIndex()
{
	CheckValid();
	return m_pPhysObj->GetMaterialIndex();
}

void PyPhysicsObject::SetMaterialIndex( int materialIndex )
{
	CheckValid();
	m_pPhysObj->SetMaterialIndex(materialIndex);
}

// contents bits
unsigned int PyPhysicsObject::GetContents()
{
	CheckValid();
	return m_pPhysObj->GetContents();
}

void PyPhysicsObject::SetContents( unsigned int contents )
{
	CheckValid();
	m_pPhysObj->SetContents(contents);
}

float PyPhysicsObject::GetSphereRadius()
{
	CheckValid();
	return m_pPhysObj->GetSphereRadius();
}

float PyPhysicsObject::GetEnergy()
{
	CheckValid();
	return m_pPhysObj->GetEnergy();
}

Vector PyPhysicsObject::GetMassCenterLocalSpace()
{
	CheckValid();
	return m_pPhysObj->GetMassCenterLocalSpace();
}

void PyPhysicsObject::SetPosition( const Vector &worldPosition, const QAngle &angles, bool isTeleport )
{
	CheckValid();
	m_pPhysObj->SetPosition(worldPosition, angles, isTeleport);
}

void PyPhysicsObject::SetPositionMatrix( const matrix3x4_t&matrix, bool isTeleport )
{
	CheckValid();
	m_pPhysObj->SetPositionMatrix(matrix, isTeleport);
}

void PyPhysicsObject::GetPosition( Vector *worldPosition, QAngle *angles )
{
	CheckValid();
	m_pPhysObj->GetPosition(worldPosition, angles);
}

void PyPhysicsObject::GetPositionMatrix( matrix3x4_t *positionMatrix )
{
	CheckValid();
	m_pPhysObj->GetPositionMatrix(positionMatrix);
}

void PyPhysicsObject::SetVelocity( const Vector *velocity, const AngularImpulse *angularVelocity )
{
	CheckValid();
	m_pPhysObj->SetVelocity( velocity, angularVelocity );
}

void PyPhysicsObject::SetVelocityInstantaneous( const Vector *velocity, const AngularImpulse *angularVelocity )
{
	CheckValid();
	m_pPhysObj->SetVelocityInstantaneous( velocity, angularVelocity );
}

void PyPhysicsObject::GetVelocity( Vector *velocity, AngularImpulse *angularVelocity )
{
	CheckValid();
	m_pPhysObj->GetVelocity( velocity, angularVelocity );
}

void PyPhysicsObject::AddVelocity( const Vector *velocity, const AngularImpulse *angularVelocity )
{
	CheckValid();
	m_pPhysObj->AddVelocity( velocity, angularVelocity );
}

void PyPhysicsObject::GetVelocityAtPoint( const Vector &worldPosition, Vector *pVelocity )
{
	CheckValid();
	m_pPhysObj->GetVelocityAtPoint( worldPosition, pVelocity );
}

void PyPhysicsObject::GetImplicitVelocity( Vector *velocity, AngularImpulse *angularVelocity )
{
	CheckValid();
	m_pPhysObj->GetImplicitVelocity( velocity, angularVelocity );
}

void PyPhysicsObject::LocalToWorld( Vector *worldPosition, const Vector &localPosition )
{
	CheckValid();
	m_pPhysObj->LocalToWorld( worldPosition, localPosition );
}

void PyPhysicsObject::WorldToLocal( Vector *localPosition, const Vector &worldPosition )
{
	CheckValid();
	m_pPhysObj->WorldToLocal( localPosition, worldPosition );
}

void PyPhysicsObject::LocalToWorldVector( Vector *worldVector, const Vector &localVector )
{
	CheckValid();
	m_pPhysObj->LocalToWorldVector( worldVector, localVector );
}

void PyPhysicsObject::WorldToLocalVector( Vector *localVector, const Vector &worldVector )
{
	CheckValid();
	m_pPhysObj->WorldToLocalVector( localVector, worldVector );
}

void PyPhysicsObject::ApplyForceCenter( const Vector &forceVector )
{
	CheckValid();
	m_pPhysObj->ApplyForceCenter( forceVector );
}

void PyPhysicsObject::ApplyForceOffset( const Vector &forceVector, const Vector &worldPosition )
{
	CheckValid();
	m_pPhysObj->ApplyForceOffset( forceVector, worldPosition );
}

void PyPhysicsObject::ApplyTorqueCenter( const AngularImpulse &torque )
{
	CheckValid();
	m_pPhysObj->ApplyTorqueCenter( torque );
}

void PyPhysicsObject::CalculateForceOffset( const Vector &forceVector, const Vector &worldPosition, Vector *centerForce, AngularImpulse *centerTorque )
{
	CheckValid();
	m_pPhysObj->CalculateForceOffset( forceVector, worldPosition, centerForce, centerTorque );
}

void PyPhysicsObject::CalculateVelocityOffset( const Vector &forceVector, const Vector &worldPosition, Vector *centerVelocity, AngularImpulse *centerAngularVelocity )
{
	CheckValid();
	m_pPhysObj->CalculateVelocityOffset( forceVector, worldPosition, centerVelocity, centerAngularVelocity );
}

float PyPhysicsObject::CalculateLinearDrag( const Vector &unitDirection )
{
	CheckValid();
	return m_pPhysObj->CalculateLinearDrag( unitDirection );
}

float PyPhysicsObject::CalculateAngularDrag( const Vector &objectSpaceRotationAxis )
{
	CheckValid();
	return m_pPhysObj->CalculateAngularDrag( objectSpaceRotationAxis );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyPhysicsObject::SetShadow( float maxSpeed, float maxAngularSpeed, bool allowPhysicsMovement, bool allowPhysicsRotation )
{
	CheckValid();
	m_pPhysObj->SetShadow( maxSpeed, maxAngularSpeed, allowPhysicsMovement, allowPhysicsRotation );
}

void PyPhysicsObject::UpdateShadow( const Vector &targetPosition, const QAngle &targetAngles, bool tempDisableGravity, float timeOffset )
{
	CheckValid();
	m_pPhysObj->UpdateShadow( targetPosition, targetAngles, tempDisableGravity, timeOffset );
}

int PyPhysicsObject::GetShadowPosition( Vector *position, QAngle *angles )
{
	CheckValid();
	return m_pPhysObj->GetShadowPosition( position, angles );
}

PyPhysicsShadowController PyPhysicsObject::GetShadowController( void )
{
	CheckValid();

	boost::python::object ref = boost::python::object(
		boost::python::handle<>(
		boost::python::borrowed(GetPySelf())
		)
	);

	return PyPhysicsShadowController(ref);
}

void PyPhysicsObject::RemoveShadowController()
{
	CheckValid();
	m_pPhysObj->RemoveShadowController();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *PyPhysicsObject::GetName()
{
	CheckValid();
	return m_pPhysObj->GetName();
}

void PyPhysicsObject::BecomeTrigger()
{
	CheckValid();
	m_pPhysObj->BecomeTrigger();
}

void PyPhysicsObject::RemoveTrigger()
{
	CheckValid();
	m_pPhysObj->RemoveTrigger();
}

void PyPhysicsObject::BecomeHinged( int localAxis )
{
	CheckValid();
	m_pPhysObj->BecomeHinged(localAxis);
}

void PyPhysicsObject::RemoveHinged()
{
	CheckValid();
	m_pPhysObj->RemoveHinged();
}

void PyPhysicsObject::OutputDebugInfo()
{
	CheckValid();
	m_pPhysObj->OutputDebugInfo();
}

bp::object PyCreatePhysicsObject( IPhysicsObject *pPhysObj )
{
	bp::object pyphysref = _physics.attr("PhysicsObject")();
	PyPhysicsObject *pPyPhysObj = bp::extract<PyPhysicsObject *>(pyphysref);
	if( !pPyPhysObj )
		return bp::object();
	pPyPhysObj->InitFromPhysicsObject( pPhysObj );
	return pyphysref;
}

bp::object PyCreatePhysicsObject( CBaseEntity *pEnt )
{
	bp::object physobj = _physics.attr("PhysicsObject")( *pEnt );
	//pPyPhysObj = bp::extract<PyPhysicsObject *)(physobj);
	//if( !pPyPhysObj )
	//	return;
	return physobj;
}

void PyPhysDestroyObject( PyPhysicsObject *pPyPhysObj, CBaseEntity *pEntity )
{
	if( !pPyPhysObj )
		return;
	pPyPhysObj->Destroy();
}

//-----------------------------------------------------------------------------
// Purpose: Physic collision interface
//-----------------------------------------------------------------------------
boost::python::tuple PyPhysicsCollision::CollideGetAABB( PyPhysicsObject *pPhysObj, const Vector &collideOrigin, const QAngle &collideAngles )
{
	Vector mins, maxs;

	if( !pPhysObj ) {
		PyErr_SetString(PyExc_ValueError, "Invalid Physic Object" );
		throw boost::python::error_already_set();
	}
	pPhysObj->CheckValid();

	physcollision->CollideGetAABB(&mins, &maxs, pPhysObj->m_pPhysObj->GetCollide(), collideOrigin, collideAngles);
	return boost::python::make_tuple(mins, maxs);
}

void PyPhysicsCollision::TraceBox( PyRay_t &ray, PyPhysicsObject &physObj, const Vector &collideOrigin, const QAngle &collideAngles, trace_t &ptr )
{
	physObj.CheckValid();
	const CPhysCollide *pCollide = physObj.GetVPhysicsObject()->GetCollide();
	physcollision->TraceBox( ray.ToRay(), pCollide, collideOrigin, collideAngles, &ptr );
}

static PyPhysicsCollision s_pyphyscollision;
PyPhysicsCollision *pyphyscollision = &s_pyphyscollision;

//-----------------------------------------------------------------------------
// Purpose: Surface Props
//-----------------------------------------------------------------------------
// parses a text file containing surface prop keys
int PyPhysicsSurfaceProps::ParseSurfaceData( const char *pFilename, const char *pTextfile )
{
	return physprops->ParseSurfaceData( pFilename, pTextfile );
}

// current number of entries in the database
int PyPhysicsSurfaceProps::SurfacePropCount( void ) const
{
	return physprops->SurfacePropCount();
}

int PyPhysicsSurfaceProps::GetSurfaceIndex( const char *pSurfacePropName ) const
{
	return physprops->GetSurfaceIndex( pSurfacePropName );
}
//void	GetPhysicsProperties( int surfaceDataIndex, float *density, float *thickness, float *friction, float *elasticity ) const;

surfacedata_t PyPhysicsSurfaceProps::GetSurfaceData( int surfaceDataIndex )
{
	surfacedata_t *pSurfData = physprops->GetSurfaceData( surfaceDataIndex );
	return *pSurfData;
}

const char *PyPhysicsSurfaceProps::GetString( unsigned short stringTableIndex ) const
{
	return physprops->GetString( stringTableIndex );
}

const char *PyPhysicsSurfaceProps::GetPropName( int surfaceDataIndex ) const
{
	return physprops->GetPropName( surfaceDataIndex );
}

void PyPhysicsSurfaceProps::GetPhysicsParameters( int surfaceDataIndex, surfacephysicsparams_t &paramsout ) const
{
	physprops->GetPhysicsParameters( surfaceDataIndex, &paramsout );
}

static PyPhysicsSurfaceProps s_pyphysprops;
PyPhysicsSurfaceProps *pyphysprops = &s_pyphysprops;

#ifndef CLIENT_DLL
// Callbacks
void PyPhysCallbackImpulse( PyPhysicsObject &pyPhysicsObject, const Vector &vecCenterForce, const AngularImpulse &vecCenterTorque )
{
	if( pyPhysicsObject.GetVPhysicsObject() )
		PhysCallbackImpulse( pyPhysicsObject.GetVPhysicsObject(), vecCenterForce, vecCenterTorque );
	else
	{
		PyErr_SetString(PyExc_ValueError, "Invalid Physics Object" );
		throw boost::python::error_already_set();
	}
}

void PyPhysCallbackSetVelocity( PyPhysicsObject &pyPhysicsObject, const Vector &vecVelocity )
{
	if( pyPhysicsObject.GetVPhysicsObject() )
		PhysCallbackSetVelocity( pyPhysicsObject.GetVPhysicsObject(), vecVelocity );
	else
	{
		PyErr_SetString(PyExc_ValueError, "Invalid Physics Object" );
		throw boost::python::error_already_set();
	}
}

void PyPhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info, gamevcollisionevent_t &event, int hurtIndex )
{
	if( !pEntity ) 
	{
		PyErr_SetString(PyExc_ValueError, "Invalid entity" );
		throw boost::python::error_already_set();
		return;
	}
	PhysCallbackDamage( pEntity, info, event, hurtIndex );
}

void PyPhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info )
{
	if( !pEntity ) 
	{
		PyErr_SetString(PyExc_ValueError, "Invalid entity" );
		throw boost::python::error_already_set();
		return;
	}
	PhysCallbackDamage( pEntity, info );
}

void PyPhysCallbackRemove(CBaseEntity *pRemove)
{
	if( !pRemove ) 
	{
		PyErr_SetString(PyExc_ValueError, "Invalid entity" );
		throw boost::python::error_already_set();
		return;
	}
	PhysCallbackRemove( pRemove->NetworkProp() );
}

// Impact damage 
float PyCalculateDefaultPhysicsDamage( int index, gamevcollisionevent_t *pEvent, float energyScale, 
									  bool allowStaticDamage, int &damageTypeOut, const char *iszDamageTableName, bool bDamageFromHeldObjects )
{
	return CalculateDefaultPhysicsDamage(index, pEvent, energyScale, allowStaticDamage, damageTypeOut, MAKE_STRING(iszDamageTableName), bDamageFromHeldObjects);
}
#endif // CLIENT_DLL

void PyForcePhysicsSimulate()
{
	IGameSystemPerFrame *pPerFrame = dynamic_cast<IGameSystemPerFrame *>( PhysicsGameSystem() );
	if( pPerFrame )
	{
#ifdef CLIENT_DLL
		PhysicsSimulate();
#else
		pPerFrame->FrameUpdatePostEntityThink();
#endif // CLIENT_DLL
	}
	else
	{
		PyErr_SetString(PyExc_ValueError, "Invalid game system" );
		throw boost::python::error_already_set();
	}
}