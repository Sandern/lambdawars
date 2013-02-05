# Module still needs a lot of cleaning up
from generate_mods_helper import GenerateModuleSemiShared
from src_helper import *
import settings

from pyplusplus import function_transformers as FT
from pyplusplus.module_builder import call_policies
from pyplusplus import messages
from pygccxml.declarations import matchers
from pyplusplus import code_creators

class Entities(GenerateModuleSemiShared):
    module_name = '_entities'
    split = True
    
    files = [
        'shared_classnames.h',
        'npcevent.h',
        'studio.h',
        'src_python_entities.h',
        'isaverestore.h',
        'saverestore.h',
        'mapentities_shared.h',
        'vcollide_parse.h',
        'hl2wars_player_shared.h',
        'imouse.h',
        'props_shared.h',
        'beam_shared.h',
        'basecombatweapon_shared.h',
        'wars_mapboundary.h',
    ]
    
    if settings.ASW_CODE_BASE:
        client_files = ['videocfg/videocfg.h']
    else:
        client_files = [
            'wchartypes.h',
            'shake.h',
        ]
    client_files = client_files + [
            'cbase.h',
            'takedamageinfo.h',
            'c_baseanimating.h',
            'c_baseanimatingoverlay.h',
            'c_baseflex.h',
            'c_basecombatcharacter.h',
            'basegrenade_shared.h',
            'c_baseplayer.h',
            'c_hl2wars_player.h',
            'unit_base_shared.h',
            'wars_func_unit.h',
            'c_playerresource.h',
            'sprite.h',
            'SpriteTrail.h',
            'c_smoke_trail.h',
            'c_wars_weapon.h',
            
            'tempent.h',
    ]

    server_files = [
        'cbase.h', 
        'mathlib/vmatrix.h', 
        'utlvector.h', 
        'shareddefs.h', 
        'util.h',

        'takedamageinfo.h',
        'baseanimating.h',
        'BaseAnimatingOverlay.h',
        'baseflex.h',
        'basecombatcharacter.h',
        'basegrenade_shared.h',
        'player.h',
        'hl2wars_player.h',
        'unit_base_shared.h',
        'unit_sense.h',
        'wars_func_unit.h',
        'soundent.h',
        'gib.h',
        'Sprite.h',
        'SpriteTrail.h',
        'smoke_trail.h',
        'entityoutput.h',
        'props.h',
        'modelentities.h',    
        'triggers.h',
        'wars_weapon.h',
        'spark.h',
        'physics_prop_ragdoll.h',
        'filters.h',
    ]
    
    def GetFiles(self):
        if self.isClient:
            return self.client_files + self.files 
        return self.server_files + self.files 
        
    client_entities = [ 
        'C_BaseEntity', 
        'C_BaseAnimating', 
        'C_BaseAnimatingOverlay',  
        'C_BaseFlex', 
        'C_BaseCombatCharacter',
        'C_BaseGrenade',
        'C_BasePlayer',
        'C_HL2WarsPlayer',
        'C_UnitBase',
        'C_FuncUnit',
        'C_PlayerResource',
        'C_Sprite',
        'C_SpriteTrail',
        'C_BaseParticleEntity',
        'C_SmokeTrail',
        'C_RocketTrail',
        'C_Beam',
        'C_BaseCombatWeapon',
        'C_WarsWeapon',
        'C_FuncBrush',
        'C_BaseFuncMapBoundary',
    ]
    
    server_entities = [ 
        'CBaseEntity', 
        'CBaseAnimating', 
        'CBaseAnimatingOverlay', 
        'CBaseFlex', 
        'CBaseCombatCharacter',
        'CBaseGrenade',
        'CBasePlayer',
        'CHL2WarsPlayer',
        'CUnitBase',
        'CFuncUnit',
        'CGib',
        'CSprite',
        'CSpriteTrail',
        'CBaseParticleEntity',
        'SmokeTrail',
        'RocketTrail',
        'CBeam',
        'CPointEntity',
        'CServerOnlyEntity',
        'CServerOnlyPointEntity',
        'CLogicalEntity',
        'CFuncBrush',
        'CBaseToggle',
        'CBaseTrigger',
        'CTriggerMultiple',
        'CBaseCombatWeapon',
        'CWarsWeapon',
        'CBaseFuncMapBoundary',
        'CBaseProp',
        'CBreakableProp',
        'CPhysicsProp',
        'CRagdollProp',
        'CBaseFilter',
    ]
    
    def SetupProperty(self, mb, cls, pyname, gettername, settername):
        mb.mem_funs(gettername).exclude()
        mb.mem_funs(settername).exclude()
        
        cls.add_property(pyname
                         , cls.member_function(gettername)
                         , cls.member_function(settername))
        
    def SetupClassShared(self, mb, cls_name):
        cls = mb.class_(cls_name)
        
        #IncludeEmptyClass(mb, cls_name)
        cls.include()
        cls.calldefs( matchers.access_type_matcher_t( 'protected' ), allow_empty=True ).exclude()
        
        # Be selective about we need to override
        # DO NOT REMOVE. Some functions are not thread safe, which will cause runtime errors because we didn't setup python threadsafe (slower)
        cls.mem_funs().virtuality = 'not virtual' 

        # Add converters + a python handle class
        AddEntityConverter(mb, cls_name)    
    
    # Need to place this in GetClientClass, but this situation is difficult to fix. The particle system calls it from a different thread.
    """
                                    '    #if defined(_WIN32)\r\n' + \
                                    '    #if defined(_DEBUG)\r\n' + \
                                    '    Assert( GetCurrentThreadId() == g_hPythonThreadID );\r\n' + \
                                    '    #elif defined(PY_CHECKTHREADID)\r\n' + \
                                    '    if( GetCurrentThreadId() != g_hPythonThreadID )\r\n' + \
                                    '        Error( "GetClientClass: Client? %d. Thread ID is not the same as in which the python interpreter is initialized! %d != %d. Tell a developer.\\n", CBaseEntity::IsClient(), g_hPythonThreadID, GetCurrentThreadId() );\r\n' + \
                                    '    #endif // _DEBUG/PY_CHECKTHREADID\r\n' + \
                                    '    #endif // _WIN32\r\n' + \
                                    '    #if defined(_DEBUG) || defined(PY_CHECK_LOG_OVERRIDES)\r\n' + \
                                    '    if( py_log_overrides.GetBool() )\r\n' + \
                                    '        Msg("Calling GetClientClass(  ) of Class: %s\\n");\r\n' % (cls_name) + \
                                    '    #endif // _DEBUG/PY_CHECK_LOG_OVERRIDES\r\n' + \
    """
    
    def AddTestCollisionMethod(self, cls, cls_name):
        # Test collision
        cls.add_wrapper_code(
        '''
            virtual bool TestCollision( ::Ray_t const & ray, unsigned int mask, ::trace_t & trace ) {
                #if defined(_WIN32)
                #if defined(_DEBUG)
                Assert( GetCurrentThreadId() == g_hPythonThreadID );
                #elif defined(PY_CHECKTHREADID)
                if( GetCurrentThreadId() != g_hPythonThreadID )
                    Error( "TestCollision: Client? %%d. Thread ID is not the same as in which the python interpreter is initialized! %%d != %%d. Tell a developer.\\n", CBaseEntity::IsClient(), g_hPythonThreadID, GetCurrentThreadId() );
                #endif // _DEBUG/PY_CHECKTHREADID
                #endif // _WIN32
                #if defined(_DEBUG) || defined(PY_CHECK_LOG_OVERRIDES)
                if( py_log_overrides.GetBool() )
                    Msg("Calling TestCollision( boost::ref(ray), mask, boost::ref(trace) ) of Class: %(cls_name)s\\n");
                #endif // _DEBUG/PY_CHECK_LOG_OVERRIDES
                bp::override func_TestCollision = this->get_override( "TestCollision" );
                if( func_TestCollision.ptr() != Py_None )
                    try {
                        return func_TestCollision( PyRay_t(ray), mask, boost::ref(trace) );
                    } catch(bp::error_already_set &) {
                        PyErr_Print();
                        return this->%(cls_name)s::TestCollision( boost::ref(ray), mask, boost::ref(trace) );
                    }
                else
                    return this->%(cls_name)s::TestCollision( boost::ref(ray), mask, boost::ref(trace) );
            }
            
            bool default_TestCollision( ::Ray_t const & ray, unsigned int mask, ::trace_t & trace ) {
                return %(cls_name)s::TestCollision( boost::ref(ray), mask, boost::ref(trace) );
            }
        ''' % { 'cls_name' : cls_name} )
        
        cls.add_registration_code(
            '''
            { //::%(cls_name)s::TestCollision
            
                typedef bool ( ::%(cls_name)s::*TestCollision_function_type )( ::Ray_t const &,unsigned int,::trace_t & ) ;
                typedef bool ( %(cls_name)s_wrapper::*default_TestCollision_function_type )( ::Ray_t const &,unsigned int,::trace_t & ) ;

                %(cls_name)s_exposer.def( 
                    "TestCollision"
                    , TestCollision_function_type(&::%(cls_name)s::TestCollision)
                    , default_TestCollision_function_type(&%(cls_name)s_wrapper::default_TestCollision)
                    , ( bp::arg("ray"), bp::arg("mask"), bp::arg("trace") ) );

            }
            ''' % { 'cls_name' : cls_name}
        , False )
    
    def ParseClientEntities(self, mb):
        for cls_name in self.client_entities:
            self.SetupClassShared(mb, cls_name)

            # Check if the python class is networkable. Return the right client class.
            cls = mb.class_(cls_name)
            cls.add_wrapper_code(   'virtual ClientClass* GetClientClass() {\r\n' + \
                                    '    if( GetCurrentThreadId() != g_hPythonThreadID )\r\n' + \
                                    '        return '+cls_name+'::GetClientClass();\r\n' + \
                                    '    ClientClass *pClientClass = SrcPySystem()->Get<ClientClass *>("pyClientClass", GetPyInstance(), NULL, true);\r\n' + \
                                    '    if( pClientClass )\r\n' + \
                                    '        return pClientClass;\r\n' + \
                                    '    return '+cls_name+'::GetClientClass();\r\n' + \
                                    '}\r\n'
            )
            
            self.AddTestCollisionMethod(cls, cls_name)
            
            cls.add_wrapper_code(
                'virtual PyObject *GetPySelf() const { return bp::detail::wrapper_base_::get_owner(*this); }'
            )
    
        mb.vars( lambda decl: 'NetworkVar' in decl.name ).exclude()        # Don't care or needs a better look
        mb.classes( lambda decl: 'NetworkVar' in decl.name ).exclude()        # Don't care or needs a better look
        mb.mem_funs( lambda decl: 'YouForgotToImplement' in decl.name ).exclude()   # Don't care
        
        mb.class_('C_BaseEntity').add_property( 'lifestate'
                         , mb.class_('C_BaseEntity').member_function( 'PyGetLifeState' )
                         , mb.class_('C_BaseEntity').member_function( 'PySetLifeState' ) )
        mb.class_('C_BaseEntity').add_property( 'takedamage'
                         , mb.class_('C_BaseEntity').member_function( 'PyGetTakeDamage' )
                         , mb.class_('C_BaseEntity').member_function( 'PySetTakeDamage' ) )
                         
    def ParseServerEntities(self, mb):
        for cls_name in self.server_entities:
            self.SetupClassShared(mb, cls_name)

            # Check if the python class is networkable. Return the right serverclass.
            cls = mb.class_(cls_name)
            if cls_name not in ['CPointEntity','CServerOnlyEntity', 'CServerOnlyPointEntity', 'CLogicalEntity', 'CFuncBrush', 'CBaseFilter']:
                cls.add_wrapper_code(   'virtual ServerClass* GetServerClass() {\r\n' + \
                                        '    #if defined(_WIN32)\r\n' + \
                                        '    #if defined(_DEBUG)\r\n' + \
                                        '    Assert( GetCurrentThreadId() == g_hPythonThreadID );\r\n' + \
                                        '    #elif defined(PY_CHECKTHREADID)\r\n' + \
                                        '    if( GetCurrentThreadId() != g_hPythonThreadID )\r\n' + \
                                        '        Error( "GetServerClass: Client? %d. Thread ID is not the same as in which the python interpreter is initialized! %d != %d. Tell a developer.\\n", CBaseEntity::IsClient(), g_hPythonThreadID, GetCurrentThreadId() );\r\n' + \
                                        '    #endif // _DEBUG/PY_CHECKTHREADID\r\n' + \
                                        '    #endif // _WIN32\r\n' + \
                                        '    #if defined(_DEBUG) || defined(PY_CHECK_LOG_OVERRIDES)\r\n' + \
                                        '    if( py_log_overrides.GetBool() )\r\n' + \
                                        '        Msg("Calling GetServerClass(  ) of Class: %s\\n");\r\n' % (cls_name) + \
                                        '    #endif // _DEBUG/PY_CHECK_LOG_OVERRIDES\r\n' + \
                                        '    ServerClass *pServerClass = SrcPySystem()->Get<ServerClass *>("pyServerClass", GetPyInstance(), NULL, true);\r\n' + \
                                        '    if( pServerClass )\r\n' + \
                                        '        return pServerClass;\r\n' + \
                                        '    return '+cls_name+'::GetServerClass();\r\n' + \
                                        '}\r\n'
                )
                
                cls.add_wrapper_code(
                    'virtual PyObject *GetPySelf() const { return bp::detail::wrapper_base_::get_owner(*this); }'
                )
            
            #AddWrapReg( mb, cls_name, mb.member_function('CanBeSeenBy', lambda decl: HasArgType(decl, 'CBaseEntity')), [CreateEntityArg('pEnt')] )  
            
            self.AddTestCollisionMethod(cls, cls_name)
            
        mb.vars( lambda decl: 'NetworkVar' in decl.name ).exclude()        # Don't care or needs a better look
        mb.classes( lambda decl: 'NetworkVar' in decl.name ).exclude()        # Don't care or needs a better look
        mb.mem_funs( lambda decl: 'YouForgotToImplement' in decl.name ).exclude()   # Don't care
        
        # Network var accessors
        # TODO: clrender, renderfx
        mb.class_('CBaseEntity').add_property( 'health'
                         , mb.class_('CBaseEntity').member_function( 'GetHealth' )
                         , mb.class_('CBaseEntity').member_function( 'SetHealth' ) )
        mb.class_('CBaseEntity').add_property( 'maxhealth'
                         , mb.class_('CBaseEntity').member_function( 'GetMaxHealth' )
                         , mb.class_('CBaseEntity').member_function( 'SetMaxHealth' ) )
        mb.class_('CBaseEntity').add_property( 'lifestate'
                         , mb.class_('CBaseEntity').member_function( 'PyGetLifeState' )
                         , mb.class_('CBaseEntity').member_function( 'PySetLifeState' ) )
        mb.class_('CBaseEntity').add_property( 'takedamage'
                         , mb.class_('CBaseEntity').member_function( 'PyGetTakeDamage' )
                         , mb.class_('CBaseEntity').member_function( 'PySetTakeDamage' ) )
        mb.class_('CBaseEntity').add_property( 'animtime'
                         , mb.class_('CBaseEntity').member_function( 'GetAnimTime' )
                         , mb.class_('CBaseEntity').member_function( 'SetAnimTime' ) )
        mb.class_('CBaseEntity').add_property( 'simulationtime'
                         , mb.class_('CBaseEntity').member_function( 'GetSimulationTime' )
                         , mb.class_('CBaseEntity').member_function( 'SetSimulationTime' ) )
        mb.class_('CBaseEntity').add_property( 'rendermode'
                         , mb.class_('CBaseEntity').member_function( 'GetRenderMode' )
                         , mb.class_('CBaseEntity').member_function( 'SetRenderMode' ) )

        mb.class_('CBaseAnimating').add_property( 'skin'
                         , mb.class_('CBaseAnimating').member_function( 'GetSkin' )
                         , mb.class_('CBaseAnimating').member_function( 'SetSkin' ) )
        mb.mem_funs('GetSkin').exclude()
        mb.mem_funs('SetSkin').exclude()
            
        # //--------------------------------------------------------------------------------------------------------------------------------
        # Useful free functions
        mb.free_functions('CreateEntityByName').include()
        mb.free_functions('CreateEntityByName').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        #mb.free_functions('CreateNetworkableByName').include()     # Gives problems on linux and I can't find the definition. Don't think we need it anyway.
        #mb.free_functions('CreateNetworkableByName').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        #mb.free_functions('SpawnEntityByName').include()           # <-- LOL, declaration only.
        #mb.free_functions('SpawnEntityByName').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        mb.free_functions('DispatchSpawn').include()

    
    def ParseBaseEntity(self, mb):
        cls = mb.class_('C_BaseEntity') if self.isClient else mb.class_('CBaseEntity')
    
        # Properties
        self.SetupProperty(mb, cls, 'propint1', 'PyGetPropInt1', 'PySetPropInt1')
        self.SetupProperty(mb, cls, 'propint2', 'PyGetPropInt2', 'PySetPropInt2')
        
        # Exclude operators
        mb.global_ns.mem_opers('new').exclude()
        mb.global_ns.mem_opers('delete').exclude()
        
        # Excludes
        mb.vars('m_pfnThink').exclude()
        mb.vars('m_pfnTouch').exclude()
        mb.vars('m_pPredictionPlayer').exclude()
        mb.vars('m_DataMap').exclude()
        if not settings.ASW_CODE_BASE: # TODO: Maybe check symbol USE_PREDICTABLEID ?
            mb.vars('m_PredictableID').exclude()
        mb.vars('m_lifeState').exclude()
        mb.vars('m_takedamage').exclude()
        
        mb.mem_funs('Release').exclude()                # Don't care
        mb.mem_funs('GetPyInstance').exclude()          # Not needed, used when converting entities to python
        mb.mem_funs('ClearPyInstance').exclude()        # Not needed, used for cleaning up python entities
        mb.mem_funs('GetBaseMap').exclude()             # Not needed
        mb.mem_funs('GetDataDescMap').exclude()         # Not needed
        mb.mem_funs('GetRefEHandle').exclude()          # We already got an auto conversion to a safe handle
        mb.mem_funs('SetRefEHandle').exclude()          # We already got an auto conversion to a safe handle
        mb.mem_funs('GetCollideable').exclude()         # Don't care for now
        mb.mem_funs('GetModel').exclude()               # Do we want this? 
        mb.mem_funs('VPhysicsInitStatic').exclude()     # Replaced by wrapper
        mb.mem_funs('VPhysicsInitNormal').exclude()     # Replaced by wrapper
        mb.mem_funs('VPhysicsInitShadow').exclude()     # Replaced by wrapper
        mb.mem_funs('VPhysicsGetObject').exclude()      # Replaced by wrapper
        mb.mem_funs('VPhysicsSetObject').exclude()      # Replace by wrapper
        mb.mem_funs('VPhysicsGetObjectList').exclude()  # Don't care for now
        mb.mem_funs('Instance', function=lambda decl: not HasArgType(decl, 'int')).exclude()               # I don't think this is needed
        mb.mem_funs('PhysicsDispatchThink').exclude()   # Don't care
        mb.mem_funs('ThinkSet').exclude()               # Maybe write a python version
        mb.mem_funs('GetBeamTraceFilter').exclude()     # Don't care
        mb.mem_funs('GetBaseEntity').exclude()          # Automatically done by converter
        mb.mem_funs('MyCombatCharacterPointer').exclude() # Automatically done by converter
        mb.mem_funs('GetBaseAnimating').exclude() # Automatically done by converter
        mb.mem_funs('MyUnitPointer').exclude() # Automatically done by converter
        mb.mem_funs('GetIUnit').exclude()
        
        mb.mem_funs('GetHealth').exclude() # Use property
        mb.mem_funs('SetHealth').exclude() # Use property
        mb.mem_funs('GetMaxHealth').exclude() # Use property
        mb.mem_funs('PyGetLifeState').exclude() # Use property
        mb.mem_funs('PySetLifeState').exclude() # Use property
        mb.mem_funs('PyGetTakeDamage').exclude() # Use property
        mb.mem_funs('PySetTakeDamage').exclude() # Use property
        if self.isServer:
            mb.mem_funs('SetMaxHealth').exclude() # Use property
        
        #mb.mem_funs('GetParametersForSound').exclude()  # Don't care for now
        #mb.mem_funs('LookupSoundLevel').exclude()       # Don't care for now
        
        mb.mem_funs('PhysicsRunThink').exclude()            # Don't care  
        mb.mem_funs('PhysicsRunSpecificThink').exclude()    # Don't care   
        #mb.mem_funs('PhysicsSimulate').exclude()        # Likely too slow
        mb.mem_funs('GetDataObject').exclude() # Don't care
        mb.mem_funs('CreateDataObject').exclude() # Don't care
        mb.mem_funs('DestroyDataObject').exclude() # Don't care
        mb.mem_funs('DestroyAllDataObjects').exclude() # Don't care
        mb.mem_funs('AddDataObjectType').exclude() # Don't care
        mb.mem_funs('RemoveDataObjectType').exclude() # Don't care
        mb.mem_funs('HasDataObjectType').exclude() # Don't care
        
        mb.mem_funs('OnChangeOwnerNumberInternal').exclude()
        mb.mem_funs('GetTextureFrameIndex').exclude() # Don't care
        mb.mem_funs('SetTextureFrameIndex').exclude() # Don't care
        
        mb.mem_funs('GetParentToWorldTransform').exclude() # Problem with argument/return type
        if self.isClient: mb.mem_funs('BuildJiggleTransformations').exclude() # No declaration
        
        # Use isclient/isserver globals/builtins
        mb.mem_funs('IsServer').exclude() 
        mb.mem_funs('IsClient').exclude() 
        mb.mem_funs('GetDLLType').exclude() 
            
        mb.mem_funs('PyAllocate').exclude()
        mb.mem_funs('PyDeallocate').exclude()
        
        # IPhysicsObject 
        mb.mem_funs('PyVPhysicsInitStatic').rename('VPhysicsInitStatic')  
        mb.mem_funs('PyVPhysicsInitNormal').rename('VPhysicsInitNormal') 
        mb.mem_funs('PyVPhysicsInitShadow').rename('VPhysicsInitShadow')   
        mb.mem_funs('PyVPhysicsGetObject').rename('VPhysicsGetObject')  
        mb.mem_funs('PyVPhysicsSetObject').rename('VPhysicsSetObject')
        
        # Call policies
        mb.mem_funs('CollisionProp').call_policies = call_policies.return_internal_reference() 
        mb.mem_funs('CreatePredictedEntityByName').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('GetBaseAnimating').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('GetOwnerEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('GetEffectEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('GetTeam').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        mb.mem_funs('GetMoveParent').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('GetRootMoveParent').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('FirstMoveChild').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('NextMovePeer').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('MyNPCPointer').call_policies = call_policies.return_value_policy( call_policies.return_by_value )      # Remove?
        mb.mem_funs('MyCombatCharacterPointer').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  # Remove?
        mb.mem_funs('GetFollowedEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('GetSimulatingPlayer').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        mb.mem_funs('GetGroundEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        mb.mem_funs('GetPredictionPlayer').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        mb.mem_funs('GetBeamTraceFilter').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('GetIMouse').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('Instance').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('GetMousePassEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        
        # Transformations
        #mb.mem_funs( name='EmitSound', function=lambda decl: HasArgType(decl, 'HSOUNDSCRIPTHANDLE') ).add_transformation( FT.input("handle") )
        #mb.mem_funs( name='StopSound', function=lambda decl: HasArgType(decl, 'HSOUNDSCRIPTHANDLE') ).add_transformation( FT.input("handle") )
        
        # Emit sound replacements
        mb.mem_funs('EmitSound').exclude()
        mb.mem_funs('StopSound').exclude()
        mb.mem_funs('PyEmitSound').rename('EmitSound')
        mb.mem_funs('PyEmitSoundFilter').rename('EmitSoundFilter')
        mb.mem_funs('PyStopSound').rename('StopSound')
        
        # Rename python methods to match the c++ names ( but in c++ they got python prefixes )
        mb.mem_funs('SetPyTouch').rename('SetTouch')
        mb.mem_funs('SetPyThink').rename('SetThink')
        mb.mem_funs('GetPyThink').rename('GetThink')
        mb.mem_funs('CreatePyHandle').exclude() # Use GetHandle instead.
        #mb.mem_funs('CreatePyHandle').rename('CreateHandle')
        #mb.mem_funs('CreatePyHandle').virtuality = 'not virtual'
        mb.mem_funs('GetPyHandle').rename('GetHandle')
        mb.mem_funs('GetPySelf').exclude()
        
        # Don't give a shit about the following functions
        mb.mem_funs( lambda decl: HasArgType(decl, 'IRestore') ).exclude()
        mb.mem_funs( lambda decl: HasArgType(decl, 'ISave') ).exclude()
        mb.mem_funs( lambda decl: HasArgType(decl, 'CEntityMapData') ).exclude()
        mb.mem_funs( lambda decl: HasArgType(decl, 'CUserCmd') ).exclude()
        
        # Exclude with certain arguments
        mb.mem_funs( lambda decl: HasArgType(decl, 'groundlink_t') ).exclude()
        mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('groundlink_t') != -1 ).exclude()
        mb.mem_funs( lambda decl: HasArgType(decl, 'touchlink_t') ).exclude()    
        #mb.mem_funs( lambda decl: HasArgType(decl, 'matrix3x4_t') ).exclude()
        mb.mem_funs( lambda decl: HasArgType(decl, 'bf_read') ).exclude()
        
        # Exclude trace stuff for now
        mb.mem_funs( lambda decl: HasArgType(decl, 'Ray_t') ).exclude()    
        
        # LIST OF FUNCTIONS TO OVERRIDE IN PYTHON
        mb.mem_funs('Precache').virtuality = 'virtual'
        mb.mem_funs('Spawn').virtuality = 'virtual'
        mb.mem_funs('Activate').virtuality = 'virtual'
        mb.mem_funs('KeyValue').virtuality = 'virtual'
        mb.mem_funs('UpdateOnRemove').virtuality = 'virtual'
        mb.mem_funs('CreateVPhysics').virtuality = 'virtual'
        mb.mem_funs('GetTracerType').virtuality = 'virtual'
        mb.mem_funs('MakeTracer').virtuality = 'virtual'
        mb.mem_funs('DoImpactEffect').virtuality = 'virtual'
        mb.mem_funs('GetIMouse').virtuality = 'virtual'
        mb.mem_funs('OnChangeOwnerNumber').virtuality = 'virtual'
        mb.mem_funs('StartTouch').virtuality = 'virtual'
        mb.mem_funs('EndTouch').virtuality = 'virtual'
        #mb.mem_funs('PhysicsSimulate').virtuality = 'virtual'
        mb.mem_funs('UpdateTransmitState').virtuality = 'virtual'
        mb.mem_funs('ComputeWorldSpaceSurroundingBox').virtuality = 'virtual'
        #mb.mem_funs('TestCollision').virtuality = 'virtual'
        
        if self.isClient:
            # LIST OF CLIENT FUNCTIONS TO OVERRIDE
            # Client Simulation functions
            mb.mem_funs('ClientThink').virtuality = 'virtual'
            mb.mem_funs('OnDataChanged').virtuality = 'virtual'
            mb.mem_funs('Simulate').virtuality = 'virtual'
            
            # Interpolation
            #mb.mem_funs('ShouldInterpolate').virtuality = 'virtual'
            
            # Drawing/model
            #mb.mem_funs('UpdateClientSideAnimation').virtuality = 'virtual'
            #mb.mem_funs('OnNewModel').virtuality = 'virtual'
            mb.mem_funs('ShouldDraw').virtuality = 'virtual' # Called when visibility is updated, doesn't happens a lot.
            
            mb.mem_funs('NotifyShouldTransmit').exclude()
            mb.mem_funs('PyNotifyShouldTransmit').rename('NotifyShouldTransmit')
            mb.mem_funs('PyNotifyShouldTransmit').virtuality = 'virtual'
            
            mb.mem_funs('PyReceiveMessage').virtuality = 'virtual'
            mb.mem_funs('PyReceiveMessage').rename('ReceiveMessage')
            mb.mem_funs('GetCollideType').virtuality = 'virtual'

            # Excludes
            mb.mem_funs('GetDataTableBasePtr').exclude()
            mb.mem_funs('OnNewModel').exclude()  

            # Excludes
            mb.vars('m_pClassRecvTable').exclude()
            mb.vars('m_PredDesc').exclude()
            mb.vars('m_PredMap').exclude()
            mb.vars('m_pOriginalData').exclude()
            mb.vars('m_VarMap').exclude()
            mb.vars('m_clrRender').exclude()
            mb.vars('m_pyInstance').exclude()
            mb.vars('m_nModelIndex').exclude()
            mb.vars('m_iTeamNum').exclude()
            mb.vars('m_hRender').exclude()
            if not settings.ASW_CODE_BASE: # TODO: Maybe check symbol USE_PREDICTABLEID ?
                mb.vars('m_pPredictionContext').exclude()
            
            mb.mem_funs('GetClientClass').exclude()         # Don't care about this one
            mb.mem_funs('GetPredDescMap').exclude()         # Don't care about this one
            mb.mem_funs('GetIClientUnknown').exclude()      # Don't care about this one
            mb.mem_funs('GetClientNetworkable').exclude()   # Don't care about this one
            mb.mem_funs('GetClientRenderable').exclude()    # Don't care about this one
            mb.mem_funs('GetIClientEntity').exclude()       # Don't care about this one
            mb.mem_funs('GetClientThinkable').exclude()     # Don't care about this one
            mb.mem_funs('GetPVSNotifyInterface').exclude()  # Do we need this?
            mb.mem_funs('GetThinkHandle').exclude()         # Don't care about this one
            mb.mem_funs('GetVarMapping').exclude()          # Don't care about this one
            #mb.mem_funs('ParticleProp').exclude()           # Don't care for now
            mb.mem_funs('GetMouth').exclude()               # Do we need this?
            mb.mem_funs('RenderHandle').exclude()           # Don't care about this one
            mb.mem_funs('GetRotationInterpolator').exclude()    # Not needed
            mb.mem_funs('SetThinkHandle').exclude()         # Don't care
            mb.mem_funs('GetClientHandle').exclude()        # Not needed
            mb.mem_funs('GetRenderClipPlane').exclude()     # Don't care for now
            mb.mem_funs('GetIDString').exclude()            # Don't care for now
            mb.mem_funs('PhysicsAddHalfGravity').exclude()  # This one only seems to have a declaration on the client!!!
            
            mb.mem_funs('AllocateIntermediateData').exclude()            # Don't care
            mb.mem_funs('DestroyIntermediateData').exclude()            # Don't care
            mb.mem_funs('ShiftIntermediateDataForward').exclude()            # Don't care
            mb.mem_funs('GetPredictedFrame').exclude()            # Don't care
            mb.mem_funs('GetOriginalNetworkDataObject').exclude()            # Don't care
            mb.mem_funs('IsIntermediateDataAllocated').exclude()            # Don't care
            
            mb.mem_funs('MyCombatWeaponPointer').exclude()
            
            if settings.ASW_CODE_BASE:
                mb.mem_funs('GetClientAlphaProperty').exclude()
                mb.mem_funs('GetClientModelRenderable').exclude()
                mb.mem_funs('GetResponseSystem').exclude()
                mb.mem_funs('GetScriptDesc').exclude()
                mb.mem_funs('GetScriptInstance').exclude()
                mb.mem_funs('AlphaProp').exclude()
                
            # Transform
            #mb.mem_funs( 'GetShadowCastDistance' ).add_transformation( FT.output('pDist') )
                
            # Call policies
            mb.mem_funs('GetClientClass').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetShadowUseOtherEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('EntityToWorldTransform').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetTeamColor').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetPredictionOwner').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            
            # Rename public variables
            mb.vars('m_iHealth').rename('health')
            mb.vars('m_lifeState').rename('lifestate')
            mb.vars('m_nRenderFX').rename('renderfx')
            if not settings.ASW_CODE_BASE:
                mb.vars('m_nRenderFXBlend').rename('renderfxblend')
            mb.vars('m_nRenderMode').rename('rendermode')
            mb.vars('m_clrRender').rename('clrender')
            mb.vars('m_takedamage').rename('takedamage')
            mb.vars('m_flAnimTime').rename('animtime')
            mb.vars('m_flOldAnimTime').rename('oldanimtime')
            mb.vars('m_flSimulationTime').rename('simulationtime')
            mb.vars('m_flOldSimulationTime').rename('oldsimulationtime')
            mb.vars('m_nNextThinkTick').rename('nextthinktick')
            mb.vars('m_nLastThinkTick').rename('lastthinktick')  
            mb.vars('m_iClassname').rename('classname')    
            mb.vars('m_flSpeed').rename('speed')
            
            mb.mem_funs('GetViewDistance').exclude()
            mb.class_('C_BaseEntity').add_property( 'viewdistance'
                             , mb.class_('C_BaseEntity').member_function( 'GetViewDistance' ) )

            # Don't give a shit about the following functions
            mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('C_AI_BaseNPC') != -1 ).exclude()
            mb.mem_funs( lambda decl: 'Interp_' in decl.name ).exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'IInterpolatedVar') ).exclude()
            mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('CInterpolatedVar') != -1 ).exclude()
            
            # Exclude the following for now until we decide if we want them ( need fixes/exposed classes )
            mb.mem_funs( lambda decl: HasArgType(decl, 'C_Team') ).exclude()
            mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('C_Team') != -1 ).exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'IClientVehicle') ).exclude()
            mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('IClientVehicle') != -1 ).exclude()
            mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('IClientRenderable') != -1 ).exclude()
            mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('touchlink_t') != -1 ).exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'C_RecipientFilter') ).exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'CNewParticleEffect') ).exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'IClientEntity') ).exclude()
            mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('CBaseHandle') != -1 ).exclude()
            #mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('matrix3x4_t') != -1 ).exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'CDamageModifier') ).exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'Quaternion') ).exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'SpatializationInfo_t') ).exclude()
            
            # Client thinking vars
            mb.add_registration_code( "bp::scope().attr( \"CLIENT_THINK_ALWAYS\" ) = CLIENT_THINK_ALWAYS;" )
            mb.add_registration_code( "bp::scope().attr( \"CLIENT_THINK_NEVER\" ) = CLIENT_THINK_NEVER;" )
            
            # Exclude
            mb.mem_funs( function=lambda decl: 'NetworkStateChanged_' in decl.name ).exclude()
            mb.vars( function=lambda decl: 'NetworkVar_' in decl.name ).exclude()
            mb.classes( function=lambda decl: 'NetworkVar_' in decl.name ).exclude()
            mb.classes( function=lambda decl: 'CNetworkVarBase' in decl.name ).exclude()
            
            # Include protected
            mb.mem_funs('AddToEntityList').include()
            mb.mem_funs('RemoveFromEntityList').include()
            
            # Enums
            mb.enum('entity_list_ids_t').include()
        else:
            mb.mem_funs('PySendEvent').include()
            mb.mem_funs('PySendEvent').rename('SendEvent')
            mb.vars('m_flPrevAnimTime').rename('prevanimtime')
            mb.vars('m_nNextThinkTick').rename('nextthinktick')
            mb.vars('m_nLastThinkTick').rename('lastthinktick')
            mb.vars('m_iClassname').rename('classname')
            mb.vars('m_iGlobalname').rename('globalname')
            mb.vars('m_iParent').rename('parent')
            mb.vars('m_iHammerID').rename('hammerid')
            mb.vars('m_flSpeed').rename('speed')
            mb.vars('m_debugOverlays').rename('debugoverlays')
            mb.vars('m_bAllowPrecache').rename('allowprecache')
            mb.vars('m_bInDebugSelect').rename('indebugselect')
            mb.vars('m_nDebugPlayer').rename('debugplayer')
            mb.vars('m_target').rename('target')
            mb.vars('m_iszDamageFilterName').rename('damagefiltername')
            
            mb.mem_funs('GetViewDistance').exclude()
            mb.mem_funs('SetViewDistance').exclude()
            mb.class_('CBaseEntity').add_property( 'viewdistance'
                             , mb.class_('CBaseEntity').member_function( 'GetViewDistance' )
                             , mb.class_('CBaseEntity').member_function( 'SetViewDistance' ) )

            
            # Replace and rename
            mb.mem_funs('SetModel').exclude()
            mb.mem_funs('PySetModel').rename('SetModel')
            mb.mem_funs('SetSize').exclude()
            mb.mem_funs('PySetSize').rename('SetSize')
            
            mb.mem_funs('PySendMessage').rename('SendMessage')
            
            # LIST OF FUNCTIONS TO OVERRIDE
            mb.mem_funs('PostConstructor').virtuality = 'virtual'
            mb.mem_funs('PostClientActive').virtuality = 'virtual'
            #mb.mem_funs('HandleAnimEvent').virtuality = 'virtual'
            mb.mem_funs('StopLoopingSounds').virtuality = 'virtual'
            mb.mem_funs('Event_Killed').virtuality = 'virtual'
            mb.mem_funs('Event_Gibbed').virtuality = 'virtual'
            mb.mem_funs('PassesDamageFilter').virtuality = 'virtual'
            mb.mem_funs('OnTakeDamage').virtuality = 'virtual'
            mb.mem_funs('OnTakeDamage_Alive').virtuality = 'virtual'
            mb.mem_funs('StopLoopingSounds').virtuality = 'virtual'
            mb.mem_funs('VPhysicsCollision').virtuality = 'virtual'
            mb.mem_funs('CanBecomeRagdoll').virtuality = 'virtual'
            mb.mem_funs('BecomeRagdoll').virtuality = 'virtual'
            mb.mem_funs('ShouldGib').virtuality = 'virtual'
            mb.mem_funs('CorpseGib').virtuality = 'virtual'
            mb.mem_funs('DrawDebugGeometryOverlays').virtuality = 'virtual'
            mb.mem_funs('DrawDebugTextOverlays').virtuality = 'virtual'
            mb.mem_funs('ModifyOrAppendCriteria').virtuality = 'virtual'
            mb.mem_funs('DeathNotice').virtuality = 'virtual'
        
            # Excludes
            mb.vars('m_pTimedOverlay').exclude()
            mb.vars('m_pClassSendTable').exclude()
            mb.vars('m_pfnUse').exclude()
            mb.vars('m_pfnBlocked').exclude()
            mb.vars('m_hDamageFilter').exclude()            # Don't think we need this one
            mb.vars('s_bAbsQueriesValid').exclude()
            mb.vars('sm_bAccurateTriggerBboxChecks').exclude()
            mb.vars('m_nRenderMode').exclude()
            mb.vars('m_nRenderFX').exclude()
            mb.vars('m_nModelIndex').exclude()
            mb.vars('m_iHealth').exclude()
            mb.vars('m_flSimulationTime').exclude()
            mb.vars('m_iMaxHealth').exclude()
            mb.vars('m_clrRender').exclude()
            mb.vars('m_pfnMoveDone').exclude()
            mb.vars('m_flAnimTime').exclude()
            mb.vars('m_iTeamNum').exclude()

            mb.mem_funs('GetServerClass').exclude()         # Don't care about this one
            mb.mem_funs('GetNetworkable').exclude()         # Don't care for now
            mb.mem_funs('NetworkProp').exclude()            # Don't care
            mb.mem_funs('edict').exclude()                  # Not needed
            mb.mem_funs('PhysicsMarkEntityAsTouched').exclude() # Don't care for now
            #mb.mem_funs('EntityToWorldTransform').exclude() # Don't care for now
            #mb.mem_funs('GetParentToWorldTransform').exclude() # Don't care for now
            mb.mem_funs('PhysicsMarkEntityAsTouched').exclude() # Don't care for now
            mb.mem_funs('PhysicsMarkEntityAsTouched').exclude() # Don't care for now
            mb.mem_funs('PhysicsMarkEntityAsTouched').exclude() # Don't care for now
            mb.mem_funs('PhysicsMarkEntityAsTouched').exclude() # Don't care for now
            mb.mem_funs('NotifySystemEvent').exclude()          # Don't care
            mb.mem_funs('Entity').exclude()          # Don't care
            
            mb.mem_funs('FVisible').exclude()               # Don't care for now
            mb.mem_funs('EmitSentenceByIndex').exclude()    # Don't care for now
            
            mb.mem_funs('PhysicsTestEntityPosition').exclude()  # Don't care  
            mb.mem_funs('PhysicsCheckRotateMove').exclude()     # Don't care  
            mb.mem_funs('PhysicsCheckPushMove').exclude()       # Don't care

            mb.mem_funs('ForceVPhysicsCollide').exclude() # Don't care
            mb.mem_funs('GetGroundVelocityToApply').exclude() # Don't care
            mb.mem_funs('GetMaxHealth').exclude() # Use property maxhealth
            mb.mem_funs('FOWShouldTransmit').exclude()
            mb.mem_funs('SetModelIndex').exclude()
            
            mb.mem_funs('SendProxy_AnglesX').exclude()
            mb.mem_funs('SendProxy_AnglesY').exclude()
            mb.mem_funs('SendProxy_AnglesZ').exclude()
            
            mb.mem_funs('MyCombatWeaponPointer').exclude()
            
            if settings.ASW_CODE_BASE:
                mb.mem_funs('FindNamedOutput').exclude()
                mb.mem_funs('GetBaseAnimatingOverlay').exclude()
                mb.mem_funs('GetContextData').exclude()
                mb.mem_funs('GetScriptDesc').exclude()
                mb.mem_funs('GetScriptInstance').exclude()
                mb.mem_funs('GetScriptOwnerEntity').exclude()
                mb.mem_funs('GetScriptScope').exclude()
                mb.mem_funs('MyNextBotPointer').exclude()
                mb.mem_funs('ScriptFirstMoveChild').exclude()
                mb.mem_funs('ScriptGetModelKeyValues').exclude()
                mb.mem_funs('ScriptGetMoveParent').exclude()
                mb.mem_funs('ScriptGetRootMoveParent').exclude()
                mb.mem_funs('ScriptNextMovePeer').exclude()
                mb.mem_funs('InputDispatchEffect').exclude() # No def?
                
                mb.vars('m_pEvent').exclude()
                mb.vars('m_pScriptModelKeyValues').exclude()
                mb.vars('m_hScriptInstance').exclude()
                
            # Replaced
            mb.mem_funs('CanBeSeenBy').exclude()      
            mb.mem_funs('DensityMap').exclude() # Don't care for now.
            
            # Call policies 
            mb.mem_funs('GetServerClass').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetEntitySkybox').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetParent').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetResponseSystem').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetServerVehicle').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('GetEnemy').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('GetNextTarget').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('Respawn').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('Create').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('CreateNoSpawn').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('PhysicsPushMove').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('PhysicsPushRotate').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('PhysicsCheckRotateMove').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('PhysicsCheckPushMove').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('AddEntityToGroundList').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('HasPhysicsAttacker').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('EntityToWorldTransform').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetParentToWorldTransform').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            
            mb.mem_funs('GetTouchTrace').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object )  # Temp fix
            
            # Don't give a shit about the following functions
           # mb.mem_funs( lambda decl: HasArgType(decl, 'CAI_BaseNPC') ).exclude()
            mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('CAI_BaseNPC') != -1 ).exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'edict_t') ).exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'IEntitySaveUtils') ).exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'CEventAction') ).exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'CCheckTransmitInfo') ).exclude()
            
            # Exclude the following for now until we decide if we want them ( need fixes/exposed classes )
            #if not settings.ASW_CODE_BASE:
            #    mb.mem_funs( lambda decl: HasArgType(decl, 'AI_CriteriaSet') ).exclude()

            mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('IResponseSystem') != -1 ).exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'CRecipientFilter') ).exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'IServerVehicle') ).exclude()
            mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('IServerVehicle') != -1 ).exclude()
            mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('CTeam') != -1 ).exclude()
            mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('CSkyCamera') != -1 ).exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'CTeam') ).exclude()
            
            mb.mem_funs( function=lambda decl: 'NetworkStateChanged_' in decl.name ).exclude()
            mb.vars( function=lambda decl: 'NetworkVar_' in decl.name ).exclude()
            mb.classes( function=lambda decl: 'NetworkVar_' in decl.name ).exclude()
            mb.classes( function=lambda decl: 'CNetworkVarBase' in decl.name ).exclude()
            
            # Do not want the firebullets function with multiple arguments. Only the one with the struct.
            mb.mem_funs( name='FireBullets', function=lambda decl: HasArgType(decl, 'int') ).exclude()
            
            # Manually exclude network vars
            mb.vars( 'm_nHitboxSet' ).exclude()
            mb.vars( 'm_nSkin' ).exclude()
            mb.vars( 'm_flPlaybackRate' ).exclude()
            if not settings.ASW_CODE_BASE:
                mb.vars( 'm_flModelWidthScale' ).exclude()
            mb.vars( 'm_nBody' ).exclude()
            mb.vars( 'm_bIsLive' ).exclude()
            mb.vars( 'm_vecForce' ).exclude()
            mb.vars( 'm_flNextAttack' ).exclude()
            mb.vars( 'm_DmgRadius' ).exclude()
            mb.vars( 'm_nForceBone' ).exclude()

    def ParseBaseAnimating(self, mb):
        cls = mb.class_('C_BaseAnimating') if self.isClient else mb.class_('CBaseAnimating')
    
        # Transformations
        mb.mem_funs( 'GetPoseParameterRange' ).add_transformation( FT.output('minValue'), FT.output('maxValue') )

        # Call policies   
        mb.mem_funs('GetModelPtr').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object )  
        
        # C_BaseAnimatingOverlay    
        mb.mem_funs('GetAnimOverlay').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
    
        if self.isClient:
            mb.vars('m_nSkin').rename('skin')
            mb.vars('m_nSkin').exclude() # Use property
            
            cls.add_property( 'skin'
                             , cls.member_function( 'GetSkin' )
                             , cls.member_function( 'SetSkin' ) )
            cls.mem_funs('GetSkin').exclude()
            cls.mem_funs('SetSkin').exclude()
            
            cls.vars('m_SequenceTransitioner').exclude()
            cls.vars('m_nHitboxSet').exclude()
            cls.vars('m_pClientsideRagdoll').exclude()
            cls.vars('m_pRagdoll').exclude()
            
            # Call policies
            mb.mem_funs('BecomeRagdollOnClient').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('CreateRagdollCopy').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('FindFollowedEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('BecomeRagdollOnClient').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('BecomeRagdollOnClient').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            
            #mb.mem_funs('OnNewModel').call_policies = call_policies.return_internal_reference() #call_policies.return_value_policy( call_policies.reference_existing_object ) 
            mb.mem_funs('ParticleProp').call_policies = call_policies.return_internal_reference() 
            
            mb.mem_funs('PyOnNewModel').rename('OnNewModel')
            mb.mem_funs('PyOnNewModel').virtuality = 'virtual'
            
            # Exclude
            if not settings.ASW_CODE_BASE:
                mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('CBoneCache') != -1 ).exclude()
            else:
                mb.mem_funs('GetBoneArrayForWrite').exclude()
                mb.mem_funs('CreateClientRagdoll').exclude()
            mb.mem_funs('GetBoneForWrite').exclude() # Don't care for now        
        else:
            mb.mem_funs('PyOnNewModel').include()
            mb.mem_funs('PyOnNewModel').rename('OnNewModel')
            mb.mem_funs('PyOnNewModel').virtuality = 'virtual'
            mb.mem_funs('OnSequenceSet').virtuality = 'virtual'
            
            # excludes
            if not settings.ASW_CODE_BASE:
                mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('CBoneCache') != -1 ).exclude()
            mb.mem_funs('GetPoseParameterArray').exclude()
            mb.mem_funs('GetEncodedControllerArray').exclude()

            if settings.ASW_CODE_BASE:
                mb.mem_funs('GetBoneCache').exclude()
                mb.mem_funs('InputIgniteNumHitboxFires').exclude()
                mb.mem_funs('InputIgniteHitboxFireScale').exclude()
                mb.mem_funs( name='GetBonePosition', function=lambda decl: HasArgType(decl, 'char') ).exclude() 
                
            # Rename vars
            mb.vars('m_OnIgnite').rename('onignite')
            mb.vars('m_flGroundSpeed').rename('groundspeed')
            mb.vars('m_flLastEventCheck').rename('lastevencheck')
            
            # Call policies
            mb.mem_funs('GetSequenceKeyValues').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('GetLightingOriginRelative').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('GetLightingOrigin').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            
            # Transformations
            
            mb.mem_funs( 'GotoSequence' ).add_transformation( FT.output('iNextSequence'), FT.output('flCycle'), FT.output('iDir') )
            mb.mem_funs( 'LookupHitbox' ).add_transformation( FT.output('outSet'), FT.output('outBox') )
            mb.mem_funs( 'GetIntervalMovement' ).add_transformation( FT.output('bMoveSeqFinished') )
            
            # Enums
            mb.enums('LocalFlexController_t').include()
        
    def ParseBaseCombatCharacter(self, mb):
        cls = mb.class_('C_BaseCombatCharacter') if self.isClient else mb.class_('CBaseCombatCharacter')
        
        # call policies    
        mb.mem_funs('Weapon_OwnsThisType').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        mb.mem_funs('GetActiveWeapon').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        
        # enums
        mb.enum('Disposition_t').include()
        
        cls.member_function( 'GetActiveWeapon' ).exclude()
        
        mb.vars('m_HackedGunPos').rename('hackedgunpos')
        
        mb.mem_funs('GetLastKnownArea').exclude()
        mb.mem_funs('IsAreaTraversable').exclude()
        mb.mem_funs('ClearLastKnownArea').exclude()
        mb.mem_funs('UpdateLastKnownArea').exclude()
        mb.mem_funs('OnNavAreaChanged').exclude()
        mb.mem_funs('OnNavAreaRemoved').exclude()
        
        if self.isClient:
            mb.mem_funs('GetWeapon').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            cls.add_property( 'activeweapon'
                             , cls.member_function( 'GetActiveWeapon' ) )
                             
            # LIST OF CLIENT FUNCTIONS TO OVERRIDE
            mb.mem_funs('OnActiveWeaponChanged').virtuality = 'virtual'
            
        else:
            cls.member_function('SetActiveWeapon').exclude()
            cls.add_property( 'activeweapon'
                             , cls.member_function( 'GetActiveWeapon' )
                             , cls.member_function('SetActiveWeapon') )        
        
            # Exclude
            #mb.mem_funs( lambda decl: decl.return_type.build_decl_string().find('CBaseCombatWeapon') != -1 ).exclude()
            #mb.mem_funs( lambda decl: HasArgType(decl, 'CBaseCombatWeapon') ).exclude() 
            mb.mem_funs('RemoveWeapon').exclude() # Declaration only, lol
            mb.mem_funs('CauseDeath').exclude()
            mb.mem_funs('FInViewCone').exclude()
            mb.mem_funs('OnPursuedBy').exclude()
            mb.vars('m_DefaultRelationship').exclude()

            if settings.ASW_CODE_BASE:
                mb.mem_funs('GetEntitiesInFaction').exclude()
            mb.mem_funs('GetFogTrigger').exclude()
            mb.mem_funs('PlayFootstepSound').exclude()

            # call policies
            mb.mem_funs('FindMissTarget').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('Weapon_Create').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('Weapon_FindUsable').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('Weapon_GetSlot').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('Weapon_GetWpnForAmmo').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('FindHealthItem').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('CheckTraceHullAttack').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('GetVehicleEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('GetWeapon').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            
            mb.free_function('RadiusDamage').include()

            mb.vars('m_bForceServerRagdoll').rename('forceserverragdoll')
            mb.vars('m_bPreventWeaponPickup').rename('preventweaponpickup')
            
            # LIST OF SERVER FUNCTIONS TO OVERRIDE
            mb.mem_funs('Weapon_Equip').virtuality = 'virtual'
            mb.mem_funs('Weapon_Switch').virtuality = 'virtual'
            mb.mem_funs('Weapon_Drop').virtuality = 'virtual'
            mb.mem_funs('Event_KilledOther').virtuality = 'virtual'
            
    def ParseBasePlayer(self, mb):
        cls = mb.class_('C_BasePlayer') if self.isClient else mb.class_('CBasePlayer')
        cls.calldefs().virtuality = 'not virtual'   
        mb.vars('m_nButtons').include()
        mb.vars('m_nButtons').rename('buttons')
        mb.vars('m_afButtonLast').include()
        mb.vars('m_afButtonLast').rename('buttonslast')
        mb.vars('m_afButtonPressed').include()
        mb.vars('m_afButtonPressed').rename('buttonspressed')
        mb.vars('m_afButtonReleased').include()
        mb.vars('m_afButtonReleased').rename('buttonsreleased')
        if self.isClient:
            mb.mem_funs('GetRenderedWeaponModel').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetRepresentativeRagdoll').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetSurfaceData').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetUseEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetViewModel').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('FindUseEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetFogParams').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetFootstepSurface').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetLadderSurface').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetLocalPlayer').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetObserverTarget').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetActiveWeaponForSelection').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetLastWeapon').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            
            # Exclude for now
            mb.mem_funs('Hints').exclude()
            mb.mem_funs('GetCommandContext').exclude()
            mb.mem_funs('GetCurrentUserCommand').exclude()
            mb.mem_funs('GetHeadLabelMaterial').exclude()
            mb.mem_funs('OverrideView').exclude()
            mb.mem_funs('ShouldGoSouth').exclude() # <-- Declaration only :)
            mb.mem_funs('GetFootstepSurface').exclude()
            
            if settings.ASW_CODE_BASE:
                mb.mem_funs('ActivePlayerCombatCharacter').exclude()
                mb.mem_funs('GetActiveColorCorrection').exclude()
                mb.mem_funs('GetActivePostProcessController').exclude()
                mb.mem_funs('GetPotentialUseEntity').exclude()
                mb.mem_funs('GetSoundscapeListener').exclude()
                mb.mem_funs('GetSplitScreenPlayers').exclude()
                mb.mem_funs('GetViewEntity').exclude()
                mb.mem_funs('IsReplay').exclude()

            mb.mem_funs('CalcView').add_transformation( FT.output('zNear'), FT.output('zFar'), FT.output('fov') )
        else:
            # NO DECLARATION
            mb.mem_funs('SetWeaponAnimType').exclude()
            mb.mem_funs('SetTargetInfo').exclude()
            mb.mem_funs('SendAmmoUpdate').exclude()
            mb.mem_funs('DeathMessage').exclude()
            
            # Excludes
            mb.mem_funs('GetPlayerInfo').exclude()
            mb.mem_funs('GetBotController').exclude()
            mb.mem_funs('GetViewModel').exclude()
            mb.mem_funs('PlayerData').exclude()
            mb.mem_funs('GetLadderSurface').exclude()
            if not settings.ASW_CODE_BASE:
                mb.mem_funs('GetCurrentCommand').exclude()
            mb.mem_funs('GetLastKnownArea').exclude()
            mb.mem_funs('GetPhysicsController').exclude()
            mb.mem_funs('GetGroundVPhysics').exclude()
            mb.mem_funs('Hints').exclude()
            mb.mem_funs('GetLastUserCommand').exclude()
            mb.mem_funs('GetCurrentUserCommand').exclude()
            mb.mem_funs('GetSurfaceData').exclude()
            mb.mem_funs('GetExpresser').exclude()
            mb.mem_funs('GetCommandContext').exclude()
            mb.mem_funs('AllocCommandContext').exclude()
            mb.mem_funs('RemoveAllCommandContextsExceptNewest').exclude()
            mb.mem_funs('GetAudioParams').exclude()
            mb.mem_funs('SetupVPhysicsShadow').exclude()
            
            if settings.ASW_CODE_BASE:
                mb.mem_funs('ActivePlayerCombatCharacter').exclude()
                mb.mem_funs('FindPickerAILink').exclude()
                mb.mem_funs('GetPlayerProxy').exclude()
                mb.mem_funs('GetSoundscapeListener').exclude()
                mb.mem_funs('GetSplitScreenPlayerOwner').exclude()
                mb.mem_funs('GetSplitScreenPlayers').exclude()
                mb.mem_funs('GetTonemapController').exclude()
                mb.mem_funs('FindPickerAINode').exclude()
                
                mb.mem_funs('FindEntityClassForward').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
                mb.mem_funs('FindEntityForward').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
                mb.mem_funs('GetPotentialUseEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
                mb.mem_funs('FindPickerEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
                mb.mem_funs('FindPickerEntityClass').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            
            # Call poilicies
            mb.mem_funs('GetObserverTarget').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('FindNextObserverTarget').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('HasNamedPlayerItem').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('GiveNamedItem').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('FindUseEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('DoubleCheckUseNPC').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            if not settings.ASW_CODE_BASE:
                mb.mem_funs('GetHeldObject').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('GetViewEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('GetFOVOwner').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('GetUseEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            mb.mem_funs('Weapon_GetLast').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            
        # Player cls functions are never overriden for now
        cls.mem_funs().virtuality = 'not virtual' 

    def ParseHL2WarsPlayer(self, mb):
        cls = mb.class_('C_HL2WarsPlayer') if self.isClient else mb.class_('CHL2WarsPlayer')
        #cls.calldefs().virtuality = 'not virtual'  

        cls.mem_funs('GetUnit').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        cls.mem_funs('GetGroupUnit').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        cls.mem_funs('GetControlledUnit').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        cls.mem_funs('GetMouseCapture').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        
        cls.mem_funs('OnLeftMouseButtonPressed').virtuality = 'virtual'
        cls.mem_funs('OnLeftMouseButtonDoublePressed').virtuality = 'virtual'
        cls.mem_funs('OnLeftMouseButtonReleased').virtuality = 'virtual'
        cls.mem_funs('OnRightMouseButtonPressed').virtuality = 'virtual'
        cls.mem_funs('OnRightMouseButtonDoublePressed').virtuality = 'virtual'
        cls.mem_funs('OnRightMouseButtonReleased').virtuality = 'virtual'
    
        if self.isClient:
            mb.mem_funs('GetLocalHL2WarsPlayer').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('GetSelectedUnitTypeRange').add_transformation( FT.output('iMin'), FT.output('iMax') )
            
            cls.add_property( 'unit'
                             , cls.mem_fun('GetControlledUnit')) 
        else:
            mb.mem_funs('EntSelectSpawnPoint').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            
            cls.add_property( 'unit'
                             , cls.mem_fun('GetControlledUnit')
                             , cls.mem_fun('SetControlledUnit')) 
                             
        mb.add_registration_code( "bp::scope().attr( \"PLAYER_MAX_GROUPS\" ) = PLAYER_MAX_GROUPS;" )
            
        # Player cls functions are never overriden for now
        #cls.mem_funs().virtuality = 'not virtual' 
        
    def ParseUnitBaseShared(self, mb, cls_name):
        cls = mb.class_(cls_name)
        AddWrapReg( mb, cls_name, cls.mem_fun('IsSelectableByPlayer'), [CreateEntityArg('pPlayer'), 'target_selection'] )
        AddWrapReg( mb, cls_name, cls.mem_fun('Select'), [CreateEntityArg('pPlayer'), 'bTriggerOnSel'] )
        AddWrapReg( mb, cls_name, cls.mem_fun('OnSelected'), [CreateEntityArg('pPlayer')] )
        AddWrapReg( mb, cls_name, cls.mem_fun('OnDeSelected'), [CreateEntityArg('pPlayer')] )
        AddWrapReg( mb, cls_name, cls.mem_fun('Order'), [CreateEntityArg('pPlayer')] )
        #AddWrapReg( mb, cls_name, cls.mem_fun('UserCmd'), ['*pMoveData'] )   
        cls.mem_funs('UserCmd').include()
        AddWrapReg( mb, cls_name, cls.mem_fun('OnUserControl'), [CreateEntityArg('pPlayer')] )
        AddWrapReg( mb, cls_name, cls.mem_fun('OnUserLeftControl'), [CreateEntityArg('pPlayer')] )
        AddWrapReg( mb, cls_name, cls.mem_fun('CanUserControl'), [CreateEntityArg('pPlayer')] )
        cls.mem_funs( lambda decl: 'OnClick' in decl.name ).exclude()
        cls.mem_funs( lambda decl: 'OnCursor' in decl.name ).exclude()
        AddWrapRegs( mb, cls_name, cls.mem_funs( lambda decl: 'OnClick' in decl.name ), [CreateEntityArg('player')] )
        AddWrapRegs( mb, cls_name, cls.mem_funs( lambda decl: 'OnCursor' in decl.name ), [CreateEntityArg('player')] )
        
        if self.isClient:
            cls.mem_funs('OnInSelectionBox').virtuality = 'virtual'
            cls.mem_funs('OnOutSelectionBox').virtuality = 'virtual'
            
        cls.add_property( 'selectionpriority'
                         , cls.mem_fun('GetSelectionPriority')
                         , cls.mem_fun('SetSelectionPriority')) 
        cls.add_property( 'attackpriority'
                         , cls.mem_fun('GetAttackPriority')
                         , cls.mem_fun('SetAttackPriority')) 
                         
        cls.mem_fun('GetSelectionPriority').exclude()
        cls.mem_fun('SetSelectionPriority').exclude()
        cls.mem_fun('GetAttackPriority').exclude()
        cls.mem_fun('SetAttackPriority').exclude()
        
        if self.isClient:
            cls.add_property( 'energy'
                             , cls.member_function( 'GetEnergy' ) )
            cls.add_property( 'maxenergy'
                             , cls.member_function( 'GetMaxEnergy' ) )
        else:
            cls.add_property( 'energy'
                             , cls.member_function( 'GetEnergy' )
                             , cls.member_function( 'SetEnergy' ) )
            cls.add_property( 'maxenergy'
                             , cls.member_function( 'GetMaxEnergy' )
                             , cls.member_function( 'SetMaxEnergy' ) )
        
    def ParseUnitBase(self, mb):
        cls_name = 'C_UnitBase' if self.isClient else 'CUnitBase'
        cls = mb.class_(cls_name)
        #mb.vars('g_playerrelationships').include()
        mb.free_function('SetPlayerRelationShip').include()
        mb.free_function('GetPlayerRelationShip').include()
        
        mb.vars('m_bFOWFilterFriendly').rename('fowfilterfriendly')
        mb.vars('m_iFOWPosX').exclude()
        mb.vars('m_iFOWPosY').exclude()
        
        mb.vars('m_fEyePitch').rename('eyepitch')
        mb.vars('m_fEyeYaw').rename('eyeyaw')
        
        mb.vars('m_bNeverIgnoreAttacks').rename('neverignoreattacks')
        
        mb.vars('m_fAccuracy').rename('accuracy')
        
        # List of overridables
        mb.mem_funs('OnUnitTypeChanged').virtuality = 'virtual'
        mb.mem_funs('UserCmd').virtuality = 'virtual'
        mb.mem_funs('OnButtonsChanged').virtuality = 'virtual'
        mb.mem_funs('CustomCanBeSeen').virtuality = 'virtual'
        if self.isClient:
            mb.mem_funs('OnHoverPaint').virtuality = 'virtual'
            mb.mem_funs('GetCursor').virtuality = 'virtual'

        mb.mem_funs('OnClickLeftPressed').exclude()
        mb.mem_funs('OnClickRightPressed').exclude()
        mb.mem_funs('OnClickLeftReleased').exclude()
        mb.mem_funs('OnClickRightReleased').exclude()
        mb.mem_funs('OnClickLeftDoublePressed').exclude()
        mb.mem_funs('OnClickRightDoublePressed').exclude()
        mb.mem_funs('IsSelectableByPlayer').exclude()
        mb.mem_funs('Select').exclude()
        mb.mem_funs('OnSelected').exclude()
        mb.mem_funs('OnDeSelected').exclude()
        mb.mem_funs('Order').exclude()
        #mb.mem_funs('UserCmd').exclude()
        mb.mem_funs('OnUserControl').exclude()
        mb.mem_funs('OnUserLeftControl').exclude()
        mb.mem_funs('CanUserControl').exclude() 
        
        mb.mem_funs('GetSquad').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        mb.mem_funs('GetNext').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        mb.mem_funs('GetCommander').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        
        mb.free_function('MapUnits').include()
        
        self.ParseUnitBaseShared(mb, cls_name)
        
        cls.mem_funs('GetEnemy').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        mb.mem_funs('GetEnemy').exclude() 
        if self.isClient:
            mb.vars('m_iMaxHealth').rename('maxhealth')
            cls.add_property( 'enemy'
                             , cls.mem_fun('GetEnemy'))
                             
            cls.add_property( 'crouching'
                             , cls.mem_fun('IsCrouching')) 
            cls.add_property( 'climbing'
                             , cls.mem_fun('IsClimbing')) 
                             
            cls.var('m_bUpdateClientAnimations').rename('updateclientanimations')
        else:
            cls.mem_funs('EnemyDistance').exclude() 
            AddWrapReg( mb, 'CUnitBase', cls.mem_fun('EnemyDistance'), [CreateEntityArg('pEnemy')] )
                 
            cls.vars('m_fDeathDrop').rename('deathdrop')
            cls.vars('m_fSaveDrop').rename('savedrop')
            cls.vars('m_fMaxClimbHeight').rename('maxclimbheight')
            cls.vars('m_fTestRouteStartHeight').rename('testroutestartheight')
            cls.vars('m_fMinSlope').rename('minslope')
            
            cls.vars('m_fEnemyChangeToleranceSqr').rename('enemychangetolerancesqr')
            
            cls.mem_funs('SetEnemy').exclude() 
            cls.add_property( 'enemy'
                             , cls.mem_fun('GetEnemy')
                             , cls.mem_fun('SetEnemy')) 
                             
            cls.mem_funs('SetCrouching').exclude()
            cls.mem_funs('SetClimbing').exclude() 
            cls.add_property( 'crouching'
                             , cls.mem_fun('IsCrouching')
                             , cls.mem_fun('SetCrouching')) 
            cls.add_property( 'climbing'
                             , cls.mem_fun('IsClimbing')
                             , cls.mem_fun('SetClimbing')) 
                             
            cls.mem_funs('PyGetNavigator').exclude() 
            cls.mem_funs('GetNavigator').exclude() 
            cls.mem_funs('SetNavigator').exclude() 
            cls.add_property( 'navigator'
                             , cls.mem_fun('PyGetNavigator')
                             , cls.mem_fun('SetNavigator') )
                             
            # cls.mem_funs('PyGetExpresser').exclude() 
            # cls.mem_funs('GetExpresser').exclude() 
            # cls.mem_funs('SetExpresser').exclude() 
            # cls.add_property( 'expresser'
                             # , cls.mem_fun('PyGetExpresser')
                             # , cls.mem_fun('SetExpresser') )
                             
            cls.mem_funs('GetAttackLOSMask').exclude() 
            cls.mem_funs('SetAttackLOSMask').exclude() 
            cls.add_property( 'attacklosmask'
                             , cls.mem_fun('GetAttackLOSMask')
                             , cls.mem_fun('SetAttackLOSMask') )
                             
            # LIST OF SERVER FUNCTIONS TO OVERRIDE
            mb.mem_funs('OnFullHealth').virtuality = 'virtual'
            mb.mem_funs('OnLostFullHealth').virtuality = 'virtual'
            
        # CFuncUnit
        cls_name = 'CFuncUnit' if self.isServer else 'C_FuncUnit'
        cls = mb.class_(cls_name)
        cls.no_init = False
        self.ParseUnitBaseShared(mb, cls_name)
        if self.isClient:
            cls.vars('m_iMaxHealth').rename('maxhealth')
            
    def ParseTriggers(self, mb):
        mb.class_('CBaseTrigger').no_init = False
        mb.class_('CTriggerMultiple').no_init = False
        mb.mem_funs('GetTouchedEntityOfType').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        mb.vars('m_bDisabled').rename('disabled')
        mb.vars('m_hFilter').rename('filter')
        mb.vars('m_hFilter').exclude()
        mb.vars('m_iFilterName').rename('filtername')
        
        if settings.ASW_CODE_BASE:
            mb.mem_funs('GetTouchingEntities').exclude()
        
        for clsname in ['CBaseTrigger', 'CTriggerMultiple']:
            triggers = mb.class_(clsname)
            triggers.add_wrapper_code(    
            'virtual boost::python::list GetTouchingEntities( void ) {\r\n' + \
            '    return UtlVectorToListByValue<EHANDLE>(m_hTouchingEntities);\r\n' + \
            '}\r\n'        
            )
            triggers.add_registration_code(
                'def( \r\n'
                '    "GetTouchingEntities"\r\n'
                '    , (boost::python::list ( ::%s_wrapper::* )( void ) )(&::%s_wrapper::GetTouchingEntities)\r\n'
                ') \r\n' % (clsname, clsname)
            )
        
    def ParseTempEnts(self, mb):
        mb.class_('C_LocalTempEntity').include()
        mb.class_('C_LocalTempEntity').mem_funs('Prepare').exclude()

    def ParseBaseCombatWeapon(self, mb):
        cls_name = 'C_BaseCombatWeapon' if self.isClient else 'CBaseCombatWeapon'
        cls_name2 = 'C_WarsWeapon' if self.isClient else 'CWarsWeapon'
        all_cls = [cls_name, cls_name2]
        cls = mb.class_(cls_name)
        
        # Overridable
        mb.mem_funs('PrimaryAttack').virtuality = 'virtual'
        mb.mem_funs('SecondaryAttack').virtuality = 'virtual'
        
        # Basecombatweapon
        mb.mem_funs('ActivityList').exclude()
        mb.mem_funs('GetConstraint').exclude()
        mb.mem_funs('GetDeathNoticeName').exclude()
        mb.mem_funs('GetEncryptionKey').exclude()
        mb.mem_funs('GetProficiencyValues').exclude()
        mb.mem_funs('GetSpriteActive').exclude()
        mb.mem_funs('GetSpriteAmmo').exclude()
        mb.mem_funs('GetSpriteAmmo2').exclude()
        mb.mem_funs('GetSpriteAutoaim').exclude()
        mb.mem_funs('GetSpriteCrosshair').exclude()
        mb.mem_funs('GetSpriteInactive').exclude()
        mb.mem_funs('GetSpriteZoomedAutoaim').exclude()
        mb.mem_funs('GetSpriteZoomedCrosshair').exclude()
        mb.mem_funs('GetControlPanelClassName').exclude()
        mb.mem_funs('GetControlPanelInfo').exclude()

        mb.mem_funs('GetLastWeapon').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        
        if self.isServer:
            mb.mem_funs('RepositionWeapon').exclude() # Declaration only...
            mb.mem_funs('IsInBadPosition').exclude() # Declaration only...
            if settings.ASW_CODE_BASE:
                mb.mem_funs('IsCarrierAlive').exclude()
        else:
            if settings.ASW_CODE_BASE:
                mb.mem_funs('GetWeaponList').exclude()

        mb.mem_funs('GetOwner').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        
        # Rename variables
        mb.var('m_iViewModelIndex').exclude()
        mb.var('m_iWorldModelIndex').exclude()

        mb.var('m_bAltFiresUnderwater').rename('altfiresunderwater')
        mb.var('m_bFireOnEmpty').rename('fireonempty')
        mb.var('m_bFiresUnderwater').rename('firesunderwater')
        mb.var('m_bInReload').rename('inreload')
        mb.var('m_bReloadsSingly').rename('reloadssingly')
        mb.var('m_fFireDuration').rename('fireduration')
        mb.var('m_fMaxRange1').rename('maxrange1')
        mb.var('m_fMaxRange2').rename('maxrange2')
        mb.var('m_fMinRange1').rename('minrange1')
        mb.var('m_fMinRange2').rename('minrange2')
        mb.var('m_flNextEmptySoundTime').rename('nextemptysoundtime')
        mb.var('m_flNextPrimaryAttack').rename('nextprimaryattack')
        mb.var('m_flNextSecondaryAttack').rename('nextsecondaryattack')
        mb.vars('m_flTimeWeaponIdle').rename('timeweaponidle')
        mb.var('m_flUnlockTime').rename('unlocktime')
        mb.var('m_hLocker').rename('locker')
        mb.var('m_iClip1').rename('clip1')
        mb.var('m_iClip2').rename('clip2')
        mb.var('m_iPrimaryAmmoType').rename('primaryammotype')
        mb.var('m_iSecondaryAmmoType').rename('secondaryammotype')
        mb.var('m_iState').rename('state')
        mb.var('m_iSubType').rename('subtype')
        mb.var('m_iViewModelIndex').rename('viewmodelindex')
        mb.var('m_iWorldModelIndex').rename('worldmodelindex')
        mb.vars('m_iszName').rename('name')
        mb.vars('m_nViewModelIndex').rename('viewmodelindex')
        
        for cls_name3 in all_cls:
            AddNetworkVarProperty( mb, 'nextprimaryattack', 'm_flNextPrimaryAttack', 'float', cls_name3, self.isClient )
            AddNetworkVarProperty( mb, 'nextsecondaryattack', 'm_flNextSecondaryAttack', 'float', cls_name3, self.isClient )
            AddNetworkVarProperty( mb, 'timeweaponidle', 'm_flTimeWeaponIdle', 'float', cls_name3, self.isClient )
            AddNetworkVarProperty( mb, 'state', 'm_iState', 'int', cls_name3, self.isClient )
            AddNetworkVarProperty( mb, 'primaryammotype', 'm_iPrimaryAmmoType', 'int', cls_name3, self.isClient )
            AddNetworkVarProperty( mb, 'secondaryammotype', 'm_iSecondaryAmmoType', 'int', cls_name3, self.isClient )
            AddNetworkVarProperty( mb, 'clip1', 'm_iClip1', 'int', cls_name3, self.isClient )
            AddNetworkVarProperty( mb, 'clip2', 'm_iClip2', 'int', cls_name3, self.isClient )
            
        cls.var('m_flNextPrimaryAttack').exclude()
        cls.var('m_flTimeWeaponIdle').exclude()
        cls.var('m_flNextSecondaryAttack').exclude()
        cls.var('m_iState').exclude()
        cls.var('m_iPrimaryAmmoType').exclude()
        cls.var('m_iSecondaryAmmoType').exclude()
        cls.var('m_iClip1').exclude()
        cls.var('m_iClip2').exclude()
            
        # Misc
        mb.enum('WeaponSound_t').include()
        mb.enum('WeaponSound_t').rename('WeaponSound')
        
        # Wars Weapon
        cls = mb.class_(cls_name2)
        cls.mem_funs('GetCommander').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        if self.isClient:
            cls.mem_funs('GetPredictionOwner').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            cls.mem_funs('GetMuzzleAttachEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            
        cls.mem_funs( 'GetShootOriginAndDirection' ).add_transformation( FT.output('vShootOrigin'), FT.output('vShootDirection') )
        cls.var('m_fFireRate').rename('firerate')
        cls.var('m_vBulletSpread').rename('bulletspread')
        cls.var('m_fOverrideAmmoDamage').rename('overrideammodamage')
        cls.var('m_fMaxBulletRange').rename('maxbulletrange')
        cls.var('m_iMinBurst').rename('minburst')
        cls.var('m_iMaxBurst').rename('maxburst')
        cls.var('m_fMinRestTime').rename('minresttime')
        cls.var('m_fMaxRestTime').rename('maxresttime')
        cls.var('m_bEnableBurst').rename('enableburst')
        cls.var('m_nBurstShotsRemaining').rename('burstshotsremaining')
        
        # For c++ only
        #cls.mem_fun('GetMinBurst').exclude()
        #cls.mem_fun('GetMaxBurst').exclude()
        #cls.mem_fun('GetMinRestTime').exclude()
        #cls.mem_fun('GetMaxRestTime').exclude()
        
        if self.isClient:
            cls.vars('m_vTracerColor').rename('tracercolor')
        
        cls.mem_funs('GetPrimaryAttackActivity').exclude()
        cls.mem_funs('SetPrimaryAttackActivity').exclude()
        cls.mem_funs('GetSecondaryAttackActivity').exclude()
        cls.mem_funs('SetSecondaryAttackActivity').exclude()
        cls.add_property( 'primaryattackactivity'
                         , cls.member_function( 'GetPrimaryAttackActivity' )
                         , cls.member_function( 'SetPrimaryAttackActivity' ) )
        cls.add_property( 'secondaryattackactivity'
                         , cls.member_function( 'GetSecondaryAttackActivity' )
                         , cls.member_function( 'SetSecondaryAttackActivity' ) )
                         
    def ParseProps(self, mb):
        if self.isServer:
            cls = mb.class_('CBreakableProp')
            cls.mem_funs('GetRootPhysicsObjectForBreak').exclude()
            
            cls = mb.class_('CPhysicsProp')
            if settings.ASW_CODE_BASE:
                cls.mem_funs('GetObstructingEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
                
            cls = mb.class_('CRagdollProp')
            cls.mem_funs('GetKiller').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            cls.mem_funs('GetRagdoll').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        
    def ParseRemainingEntities(self, mb):
        # CBaseFlex
        mb.mem_funs( lambda decl: HasArgType(decl, 'CChoreoScene') ).exclude() 
        if self.isServer:
            mb.mem_funs('FlexSettingLessFunc').exclude()
            mb.class_('CBaseFlex').class_('FS_LocalToGlobal_t').exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'AI_Response') ).exclude() 
            if settings.ASW_CODE_BASE:
                mb.mem_funs('ScriptGetOldestScene').exclude()
                mb.mem_funs('ScriptGetSceneByIndex').exclude()
        else:
            mb.mem_funs('FindSceneFile').exclude() # Don't care
    
        # CBaseGrenade
        cls_name = 'C_BaseGrenade' if self.isClient else 'CBaseGrenade'
        cls = mb.class_(cls_name)
        
        # call policies
        cls.mem_funs('GetThrower').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        cls.mem_funs('GetOriginalThrower').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        #cls.mem_funs('GetTrueOwnerEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value )    
        
        cls.add_property( 'damage'
                         , cls.member_function( 'GetDamage' )
                         , cls.member_function( 'SetDamage' ) )
        cls.add_property( 'damageradius'
                         , cls.member_function( 'GetDamageRadius' )
                         , cls.member_function( 'SetDamageRadius' ) )
                         
        cls.mem_fun('GetDamage').exclude()
        cls.mem_fun('SetDamage').exclude()
        cls.mem_fun('GetDamageRadius').exclude()
        cls.mem_fun('SetDamageRadius').exclude()
                         
        # //--------------------------------------------------------------------------------------------------------------------------------
        # CSprite
        mb.mem_funs('SpriteCreatePredictable').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        
        # //--------------------------------------------------------------------------------------------------------------------------------
        # CBeam
        mb.mem_funs('BeamCreate').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('BeamCreatePredictable').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('GetEndEntityPtr').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('GetStartEntityPtr').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('RandomTargetname').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        
        if self.isServer:
            # Not sure where to put this
            mb.free_function('DoSpark').include()
        
            # CSoundEnt
            IncludeEmptyClass(mb, 'CSoundEnt')
            mb.mem_funs('InsertSound').include()
            
            # CGib
            mb.free_functions('CreateRagGib').include()
            mb.enum('GibType_e').include()
            
            # CSprite
            mb.mem_funs('SpriteCreate').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('SpriteTrailCreate').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.free_functions('SpawnBlood').include()
            
            # SmokeTrail
            mb.mem_funs('CreateSmokeTrail').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        
            cls = mb.class_('SmokeTrail')
            cls.var('m_EndColor').exclude()#rename('endcolor')
            cls.var('m_EndSize').rename('endsize')
            cls.var('m_MaxSpeed').rename('maxspeed')
            cls.var('m_MinSpeed').rename('minspeed')
            cls.var('m_MaxDirectedSpeed').rename('maxdirectedspeed')
            cls.var('m_MinDirectedSpeed').rename('mindirectedspeed')
            cls.var('m_Opacity').rename('opacity')
            cls.var('m_ParticleLifetime').rename('particlelifetime')
            cls.var('m_SpawnRadius').rename('spawnradius')
            cls.var('m_SpawnRate').rename('spawnrate')
            cls.var('m_StartColor').exclude()#rename('startcolor')
            cls.var('m_StartSize').rename('startsize')
            cls.var('m_StopEmitTime').rename('stopemittime')
            cls.var('m_bEmit').rename('emit')
            cls.var('m_nAttachment').rename('attachment')


            cls.add_registration_code('''
            { //property "endcolor"[fget=::SmokeTrail_wrapper::GetEndColor, fset=::SmokeTrail_wrapper::SetEndColor]
            
                typedef Vector ( ::SmokeTrail_wrapper::*fget )(  ) const;
                typedef void ( ::SmokeTrail_wrapper::*fset )( Vector & ) ;
                
                SmokeTrail_exposer.add_property( 
                    "endcolor"
                    , fget( &::SmokeTrail_wrapper::GetEndColor )
                    , fset( &::SmokeTrail_wrapper::SetEndColor ) );
            
            }''', False)
            cls.add_wrapper_code('Vector GetEndColor() { return m_EndColor; }')
            cls.add_wrapper_code('void SetEndColor(Vector &endcolor) { m_EndColor = endcolor; }')
            
            cls.add_registration_code('''
            { //property "startcolor"[fget=::SmokeTrail_wrapper::GetStartColor, fset=::SmokeTrail_wrapper::SetStartColor]
            
                typedef Vector ( ::SmokeTrail_wrapper::*fget )(  ) const;
                typedef void ( ::SmokeTrail_wrapper::*fset )( Vector & ) ;
                
                SmokeTrail_exposer.add_property( 
                    "startcolor"
                    , fget( &::SmokeTrail_wrapper::GetStartColor )
                    , fset( &::SmokeTrail_wrapper::SetStartColor ) );
            
            }''', False)
            cls.add_wrapper_code('Vector GetStartColor() { return m_StartColor.Get(); }')
            cls.add_wrapper_code('void SetStartColor(Vector &startcolor) { m_StartColor = startcolor; }')
            
            # RocketTrail (looks like smoketrail..)
            mb.mem_funs('CreateRocketTrail').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        
            cls = mb.class_('RocketTrail')
            cls.var('m_EndColor').exclude()#rename('endcolor')
            cls.var('m_EndSize').rename('endsize')
            cls.var('m_MaxSpeed').rename('maxspeed')
            cls.var('m_MinSpeed').rename('minspeed')
            cls.var('m_Opacity').rename('opacity')
            cls.var('m_ParticleLifetime').rename('particlelifetime')
            cls.var('m_SpawnRadius').rename('spawnradius')
            cls.var('m_SpawnRate').rename('spawnrate')
            cls.var('m_StartColor').exclude()#rename('startcolor')
            cls.var('m_StartSize').rename('startsize')
            cls.var('m_StopEmitTime').rename('stopemittime')
            cls.var('m_bEmit').rename('emit')
            cls.var('m_nAttachment').rename('attachment')
            cls.var('m_bDamaged').rename('damaged')
            cls.var('m_flFlareScale').rename('flarescale')
            
            cls.add_registration_code('''
            { //property "endcolor"[fget=::RocketTrail_wrapper::GetEndColor, fset=::RocketTrail_wrapper::SetEndColor]
            
                typedef Vector ( ::RocketTrail_wrapper::*fget )(  ) const;
                typedef void ( ::RocketTrail_wrapper::*fset )( Vector & ) ;
                
                RocketTrail_exposer.add_property( 
                    "endcolor"
                    , fget( &::RocketTrail_wrapper::GetEndColor )
                    , fset( &::RocketTrail_wrapper::SetEndColor ) );
            
            }''', False)
            cls.add_wrapper_code('Vector GetEndColor() { return m_EndColor; }')
            cls.add_wrapper_code('void SetEndColor(Vector &endcolor) { m_EndColor = endcolor; }')
            
            cls.add_registration_code('''
            { //property "startcolor"[fget=::RocketTrail_wrapper::GetStartColor, fset=::RocketTrail_wrapper::SetStartColor]
            
                typedef Vector ( ::RocketTrail_wrapper::*fget )(  ) const;
                typedef void ( ::RocketTrail_wrapper::*fset )( Vector & ) ;
                
                RocketTrail_exposer.add_property( 
                    "startcolor"
                    , fget( &::RocketTrail_wrapper::GetStartColor )
                    , fset( &::RocketTrail_wrapper::SetStartColor ) );
            
            }''', False)
            cls.add_wrapper_code('Vector GetStartColor() { return m_StartColor.Get(); }')
            cls.add_wrapper_code('void SetStartColor(Vector &startcolor) { m_StartColor = startcolor; }')
            
            # Props
            mb.free_functions('PropBreakablePrecacheAll').include()
          
            # CFuncBrush  
            mb.class_('CFuncBrush').vars('m_iSolidity').rename('solidity')
            mb.class_('CFuncBrush').vars('m_iDisabled').rename('disabled')
            mb.class_('CFuncBrush').vars('m_bSolidBsp').rename('solidbsp')
            mb.class_('CFuncBrush').vars('m_iszExcludedClass').rename('excludedclass')
            mb.class_('CFuncBrush').vars('m_bInvertExclusion').rename('invertexclusion')

            mb.add_registration_code( "bp::scope().attr( \"SF_WALL_START_OFF\" ) = (int)SF_WALL_START_OFF;" )
            mb.add_registration_code( "bp::scope().attr( \"SF_IGNORE_PLAYERUSE\" ) = (int)SF_IGNORE_PLAYERUSE;" )
            
            # Call poilicies
            mb.free_functions('CreateRagGib').call_policies = call_policies.return_value_policy( call_policies.return_by_value )    
            mb.mem_funs('GetSprite').call_policies = call_policies.return_value_policy( call_policies.return_by_value )    
            mb.mem_funs('GetFlame').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  
        
            # Base filter
            cls = mb.class_('CBaseFilter')
            cls.no_init = False
            cls.mem_funs('PassesFilterImpl').virtuality = 'virtual' 
            cls.mem_funs('PassesDamageFilterImpl').virtuality = 'virtual' 
        else:
            # C_PlayerResource
            mb.add_declaration_code( "C_PlayerResource *wrap_PlayerResource( void )\r\n{\r\n\treturn g_PR;\r\n}\r\n" )
            mb.add_registration_code( 'bp::def( "PlayerResource", wrap_PlayerResource, bp::return_value_policy< bp::return_by_value >() );' )   
           
            # C_Sprite
            mb.mem_funs('GlowBlend').exclude()
            
        # Map boundary
        cls_name = 'C_BaseFuncMapBoundary' if self.isClient else 'CBaseFuncMapBoundary'
        cls = mb.class_(cls_name)
        cls.vars('m_pNext').exclude()
        mb.mem_funs('IsWithinAnyMapBoundary').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  
        
    def ParseEntities(self, mb):
        if self.isClient:
            self.ParseClientEntities(mb)
        else:
            self.ParseServerEntities(mb)
        mb.mem_funs('GetPyNetworkType').include()

        # Dead entity
        cls = mb.class_('DeadEntity')
        cls.include()
        cls.mem_fun('NonZero').rename('__nonzero__')
        
        # Entity Handles
        cls = mb.class_('CBaseHandle')
        cls.include()
        cls.mem_funs().exclude()
        cls.mem_funs('GetEntryIndex').include()
        cls.mem_funs('GetSerialNumber').include()
        cls.mem_funs('ToInt').include()
        cls.mem_funs('IsValid').include()
        
        cls = mb.class_('PyHandle')
        cls.include()
        cls.mem_fun('Get').exclude()
        cls.mem_fun('PyGet').rename('Get')
        cls.mem_fun('GetAttr').rename('__getattr__')
        cls.mem_fun('GetAttribute').rename('__getattribute__')
        cls.mem_fun('SetAttr').rename('__setattr__')
        cls.mem_fun('Cmp').rename('__cmp__')
        cls.mem_fun('NonZero').rename('__nonzero__')
        cls.mem_funs('GetPySelf').exclude()
        
        cls.add_wrapper_code(
            'virtual PyObject *GetPySelf() { return boost::python::detail::wrapper_base_::get_owner(*this); }'
        )
        
        self.ParseBaseEntity(mb)
        self.ParseBaseAnimating(mb)
        self.ParseBaseCombatCharacter(mb)
        self.ParseBasePlayer(mb)
        self.ParseHL2WarsPlayer(mb)
        self.ParseUnitBase(mb)
        if self.isClient:
            self.ParseTempEnts(mb)
        if self.isServer:
            self.ParseTriggers(mb)
        self.ParseBaseCombatWeapon(mb)
        self.ParseProps(mb)
        self.ParseRemainingEntities(mb)
        
        # Add converters
        mb.add_registration_code( "ptr_ihandleentity_to_pyhandle();" )
        mb.add_registration_code( "py_ent_to_ihandleentity();" )

    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        self.ParseEntities(mb)
        
        # Remove any protected function 
        #mb.calldefs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
        
        # Protected functions we do want:
        if self.isServer:
            mb.mem_funs('TraceAttack').include()
            mb.mem_funs('PassesFilterImpl').include()
            mb.mem_funs('PassesDamageFilterImpl').include()
            
        # //--------------------------------------------------------------------------------------------------------------------------------
        # Disable warning about classes that are redefined in a wrapper. Just a notification? 
        mb.classes().disable_warnings( messages.W1023 )
        mb.classes().disable_warnings( messages.W1027 )
        mb.classes().disable_warnings( messages.W1029 )
        
        # Disable shared warnings
        DisableKnownWarnings(mb)
        
    def AddAdditionalCode(self, mb):
        header = code_creators.include_t( 'src_python_converters_ents.h' )
        mb.code_creator.adopt_include(header)
        super(Entities, self).AddAdditionalCode(mb)

    