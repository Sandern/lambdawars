# Module still needs a lot of cleaning up
from srcpy.module_generators import SemiSharedModuleGenerator
from srcpy.matchers import calldef_withtypes

from pyplusplus import function_transformers as FT
from pyplusplus.module_builder import call_policies
from pyplusplus import messages
from pygccxml.declarations import matcher, matchers, pointer_t, const_t, reference_t, declarated_t, char_t
from pyplusplus import code_creators

from src_helper import HasArgType, AddWrapReg, AddWrapRegs, CreateEntityArg

# Templates for client and server class
tmpl_clientclass = '''virtual ClientClass* GetClientClass() {
#if defined(_WIN32) // POSIX: TODO
        if( GetCurrentThreadId() != g_hPythonThreadID )
            return %(clsname)s::GetClientClass();
#endif // _WIN32
        ClientClass *pClientClass = SrcPySystem()->Get<ClientClass *>( "pyClientClass", GetPyInstance(), NULL, true );
        if( pClientClass )
            return pClientClass;
        return %(clsname)s::GetClientClass();
    }
'''

tmpl_serverclass = '''virtual ServerClass* GetServerClass() {
#if defined(_WIN32)
#if defined(_DEBUG)
        Assert( GetCurrentThreadId() == g_hPythonThreadID );
#elif defined(PY_CHECKTHREADID)
        if( GetCurrentThreadId() != g_hPythonThreadID )
            Error( "GetServerClass: Client? %%d. Thread ID is not the same as in which the python interpreter is initialized! %%d != %%d. Tell a developer.\\n", CBaseEntity::IsClient(), g_hPythonThreadID, GetCurrentThreadId() );
#endif // _DEBUG/PY_CHECKTHREADID
#endif // _WIN32
#if defined(_DEBUG) || defined(PY_CHECK_LOG_OVERRIDES)
        if( py_log_overrides.GetBool() )
            Msg("Calling GetServerClass(  ) of Class: %(clsname)s\\n");
#endif // _DEBUG/PY_CHECK_LOG_OVERRIDES
        ServerClass *pServerClass = SrcPySystem()->Get<ServerClass *>( "pyServerClass", GetPyInstance(), NULL, true );
        if( pServerClass )
            return pServerClass;
        return %(clsname)s::GetServerClass();
    }
'''

# Templates for entities handles and converters
tmpl_enthandle = '''{ //::%(handlename)s
        typedef bp::class_< %(handlename)s, bp::bases< CBaseHandle > > %(handlename)s_exposer_t;
        %(handlename)s_exposer_t %(handlename)s_exposer = %(handlename)s_exposer_t( "%(handlename)s", bp::init< >() );
        %(handlename)s_exposer.def( bp::init< %(clsname)s * >(( bp::arg("pVal") )) );
        %(handlename)s_exposer.def( bp::init< int, int >(( bp::arg("iEntry"), bp::arg("iSerialNumber") )) );
        { //::%(handlename)s::GetAttr
        
            typedef bp::object ( ::%(handlename)s::*GetAttr_function_type )( const char * ) const;
            
            %(handlename)s_exposer.def( 
                "__getattr__"
                , GetAttr_function_type( &::%(handlename)s::GetAttr )
            );
        
        }
        { //::%(handlename)s::Cmp
        
            typedef bool ( ::%(handlename)s::*Cmp_function_type )( bp::object ) const;
            
            %(handlename)s_exposer.def( 
                "__cmp__"
                , Cmp_function_type( &::%(handlename)s::Cmp )
            );
        
        }
        { //::%(handlename)s::NonZero
        
            typedef bool ( ::%(handlename)s::*NonZero_function_type )( ) const;
            
            %(handlename)s_exposer.def( 
                "__nonzero__"
                , NonZero_function_type( &::%(handlename)s::NonZero )
            );
        
        }
        { //::%(handlename)s::Set
        
            typedef void ( ::%(handlename)s::*Set_function_type )( %(clsname)s * ) const;
            
            %(handlename)s_exposer.def( 
                "Set"
                , Set_function_type( &::%(handlename)s::Set )
            );
        
        }
        { //::%(handlename)s::GetSerialNumber
        
            typedef int ( ::%(handlename)s::*GetSerialNumber_function_type )( ) const;
            
            %(handlename)s_exposer.def( 
                "GetSerialNumber"
                , GetSerialNumber_function_type( &::%(handlename)s::GetSerialNumber )
            );
        
        }
        { //::%(handlename)s::GetEntryIndex
        
            typedef int ( ::%(handlename)s::*GetEntryIndex_function_type )(  ) const;
            
            %(handlename)s_exposer.def( 
                "GetEntryIndex"
                , GetEntryIndex_function_type( &::%(handlename)s::GetEntryIndex )
            );
        
        }
        %(handlename)s_exposer.def( bp::self != bp::self );
        %(handlename)s_exposer.def( bp::self == bp::self );
    }
'''

tmpl_ent_converters_to = '''
struct %(ptr_convert_to_py_name)s : bp::to_python_converter<%(clsname)s *, ptr_%(clsname)s_to_handle>
{
    static PyObject* convert(%(clsname)s *s)
    {
        return s ? bp::incref(s->GetPyHandle().ptr()) : bp::incref(Py_None);
    }
};

struct %(convert_to_py_name)s : bp::to_python_converter<%(clsname)s, %(clsname)s_to_handle>
{
    static PyObject* convert(const %(clsname)s &s)
    {
        return bp::incref(s.GetPyHandle().ptr());
    }
};
'''

tmpl_ent_converters_from = '''
struct %(convert_from_py_name)s
{
    handle_to_%(clsname)s()
    {
        bp::converter::registry::insert(
            &extract_%(clsname)s, 
            bp::type_id<%(clsname)s>()
            );
    }

    static void* extract_%(clsname)s(PyObject* op){
       CBaseHandle h = bp::extract<CBaseHandle>(op);
       if( h.Get() == NULL )
           return Py_None;
       return h.Get();
    }
};
'''

class Entities(SemiSharedModuleGenerator):
    module_name = '_entities'
    split = True
    
    # Includes
    files = [
        'cbase.h',
        'npcevent.h',
        'srcpy_entities.h',
        'bone_setup.h',
        'basegrenade_shared.h',
        '$takedamageinfo.h',
        '$c_ai_basenpc.h',
        '#SkyCamera.h',
        '#ai_basenpc.h',
        '#modelentities.h',
        '$c_basetoggle.h',
        '#basetoggle.h',
        '$c_triggers.h',
        '#triggers.h',
        '$soundinfo.h',
        '#nav_area.h',
        '#AI_Criteria.h',
        'saverestore.h',
        'vcollide_parse.h', # solid_t
        '#iservervehicle.h',
        '$iclientvehicle.h',
        '%choreoscene.h',
        '%choreoactor.h',
        '$steam/steamclientpublic.h', # CSteamID
        '$view_shared.h', # CViewSetup
        '#gib.h',
        '#spark.h',
        '#physics_prop_ragdoll.h',
        '#filters.h',
        '#EntityFlame.h',
        '$c_playerresource.h',
        '#player_resource.h',
        '#props.h',
        
        # For parsing only (used to exclude functions based on return value)
        '$%baseviewmodel_shared.h',
        '#%team.h',
        '$%c_team.h',
        '%mapentities_shared.h',
        '%ai_responsesystem.h',
        
        # Effects. Would be nice to remove these since they can be achieved using the particle system.
        'Sprite.h',
        'SpriteTrail.h',
        '$c_smoke_trail.h',
        '#smoke_trail.h',
        'beam_shared.h',
        
        # HL2Wars
        '$c_hl2wars_player.h',
        '#hl2wars_player.h',
        'unit_base_shared.h',
        'wars_func_unit.h',
        'hl2wars_player_shared.h',
        'wars_mapboundary.h',
        'srcpy_util.h', # for PyRay_t
        '$c_wars_weapon.h',
        '#wars_weapon.h',
    ]
        
    # List of entity classes want to have exposed
    cliententities = [ 
        'C_BaseEntity', 
        'C_BaseAnimating',
        'C_BaseAnimatingOverlay',
        'C_BaseFlex',
        'C_BaseCombatCharacter',
        'C_BaseCombatWeapon',
        'C_BaseGrenade',
        'C_BasePlayer',
        'C_PlayerResource',
        'C_BaseToggle',
        'C_BaseTrigger',
        'C_FuncBrush',
        
        # Effects
        'C_Sprite',
        'C_SpriteTrail',
        'C_BaseParticleEntity',
        'C_SmokeTrail',
        'C_RocketTrail',
        'C_Beam',
        
        # Wars
        'C_HL2WarsPlayer',
        'C_UnitBase',
        'C_FuncUnit',
        'C_WarsWeapon',
        'C_BaseFuncMapBoundary',
    ]
    
    serverentities = [ 
        'CBaseEntity', 
        'CBaseAnimating',
        'CBaseAnimatingOverlay',
        'CBaseFlex',
        'CBaseCombatCharacter',
        'CBaseCombatWeapon',
        'CBaseGrenade',
        'CBasePlayer',
        'CPlayerResource',
        'CPointEntity',
        'CServerOnlyEntity',
        'CServerOnlyPointEntity',
        'CLogicalEntity',
        'CFuncBrush',
        'CBaseToggle',
        'CBaseTrigger',
        'CTriggerMultiple',
        'CBaseFilter',
        'CGib',
        'CBaseProp',
        'CBreakableProp',
        'CPhysicsProp',
        'CRagdollProp',
        'CEntityFlame',
        
        # Effects. Would be nice to remove these since they can be achieved using the particle system.
        'CSprite',
        'CSpriteTrail',
        'CBaseParticleEntity', # Baseclass for SmokeTrail
        'SmokeTrail',
        'RocketTrail',
        'CBeam',
        
        # Wars
        'CHL2WarsPlayer',
        'CUnitBase',
        'CFuncUnit',
        'CWarsWeapon',
        'CBaseFuncMapBoundary',
    ]
    
    def AddEntityConverter(self, mb, clsname, pyhandletoptronly=False):
        ''' Creates entities converters/handles for Python. '''
        cls = mb.class_(clsname)
        
        handlename = '%sHANDLE' % (clsname)
        
        ptr_convert_to_py_name = 'ptr_%s_to_handle' % (clsname)
        convert_to_py_name = '%s_to_handle' % (clsname)
        convert_from_py_name = 'handle_to_%s' % (clsname)
        
        if not pyhandletoptronly:
            # Add handle
            mb.add_declaration_code( 'typedef CEPyHandle< %s > %s;\r\n'% (clsname, handlename) )
            mb.add_registration_code( tmpl_enthandle % {'clsname' : clsname, 'handlename' : handlename}, True )
        
        # Add declaration code for converters
        if not pyhandletoptronly:
            mb.add_declaration_code( tmpl_ent_converters_to % {
                'clsname' : clsname,
                'ptr_convert_to_py_name' : ptr_convert_to_py_name,
                'convert_to_py_name' : convert_to_py_name,
                'convert_from_py_name' : convert_from_py_name,
            })
        
        mb.add_declaration_code( tmpl_ent_converters_from % {
            'clsname' : clsname,
            'ptr_convert_to_py_name' : ptr_convert_to_py_name,
            'convert_to_py_name' : convert_to_py_name,
            'convert_from_py_name' : convert_from_py_name,
        })
        
        # Add registration code
        if not pyhandletoptronly:
            mb.add_registration_code( "%s();" % (ptr_convert_to_py_name) )
            mb.add_registration_code( "%s();" % (convert_to_py_name) )
        mb.add_registration_code( "%s();" % (convert_from_py_name) )
        
    # Parse methods
    def SetupEntityClass(self, mb, clsname):
        ''' This function is called for both exposed client and server entities and 
            applies shared functionality. '''
        cls = mb.class_(clsname)
        
        self.entclasses.append(cls)
        
        cls.include()
        cls.calldefs(matchers.access_type_matcher_t('protected'), allow_empty=True).exclude()
        
        # Be selective about what we need to override
        # DO NOT REMOVE. Some functions are not thread safe, which will cause runtime errors because we did not setup python threadsafe (slower)
        cls.mem_funs(allow_empty=True).virtuality = 'not virtual' 

        # Add converters + a python handle class
        self.AddEntityConverter(mb, clsname)  
        
        # Use by converters to check if a Python Object is attached
        cls.add_wrapper_code('virtual PyObject *GetPySelf() const { return bp::detail::wrapper_base_::get_owner(*this); }')
        
        # Test if the Entity class is setup right
        try:
            cls.mem_funs('CreatePyHandle').exclude() # Use GetHandle instead.
        except (matcher.declaration_not_found_t, RuntimeError):
            raise Exception('Class %s has no CreatePyHandle function. Did you forgot to declare the entity as a Python class?' % (clsname))
        
        # Apply common rules to the entity class
        # Don't care about the following:
        cls.vars(lambda decl: 'NetworkVar' in decl.name, allow_empty=True).exclude()
        cls.classes(lambda decl: 'NetworkVar' in decl.name, allow_empty=True).exclude()
        cls.mem_funs(lambda decl: 'YouForgotToImplement' in decl.name, allow_empty=True).exclude()
        cls.mem_funs(function=lambda decl: 'NetworkStateChanged_' in decl.name, allow_empty=True).exclude()
        cls.mem_funs('GetBaseMap', allow_empty=True).exclude()             # Not needed
        cls.mem_funs('GetDataDescMap', allow_empty=True).exclude()         # Not needed
        cls.mem_funs('GetDataDescMap', allow_empty=True).exclude()         # Not needed
        
        cls.mem_funs('DestroyPyInstance', allow_empty=True).exclude()      # Not needed, used for cleaning up python entities

        # matrix3x4_t is always returned by value
        matrix3x4 = mb.class_('matrix3x4_t')
        cls.calldefs(matchers.calldef_matcher_t(return_type=reference_t(declarated_t(matrix3x4))), allow_empty=True).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        cls.calldefs(matchers.calldef_matcher_t(return_type=reference_t(const_t(declarated_t(matrix3x4)))), allow_empty=True).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        
        # Exclude functions with CTeam, CUserCmd, edict_t
        usercmd = mb.class_('CUserCmd')
        edict = mb.class_('edict_t')
        groundlink = mb.class_('groundlink_t')
        touchlink = mb.class_('touchlink_t')
        bf_read = mb.class_('bf_read')
        ray = mb.class_('Ray_t')
        excludetypes = [
            pointer_t(declarated_t(mb.class_('CTeam' if self.isserver else 'C_Team'))),
            pointer_t(const_t(declarated_t(usercmd))),
            pointer_t(declarated_t(usercmd)),
            pointer_t(const_t(declarated_t(edict))),
            pointer_t(declarated_t(edict)),
            pointer_t(declarated_t(mb.class_('CEntityMapData'))),
            pointer_t(declarated_t(groundlink)),
            pointer_t(declarated_t(touchlink)),
            pointer_t(declarated_t(bf_read)),
            reference_t(declarated_t(bf_read)),
            reference_t(const_t(declarated_t(ray))),
            pointer_t(declarated_t(mb.class_('ICollideable'))),
        ]
        if self.settings.branch == 'swarm':
            # In source2013, this only exists on the server
            excludetypes.append(pointer_t(declarated_t(mb.class_('IResponseSystem'))))
        cls.calldefs(calldef_withtypes(excludetypes), allow_empty=True).exclude()
        
        # Returning a physics object -> Convert by value, which results in the wrapper object being returned
        physicsobject = mb.class_('IPhysicsObject')
        cls.calldefs(matchers.calldef_matcher_t(return_type=pointer_t(declarated_t(physicsobject))), allow_empty=True).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        
        # All public variables are excluded by default
        cls.vars(allow_empty=True).exclude()
        
        return cls
            
    def ParseClientEntities(self, mb):
        # Made not virtual so no wrapper code is generated in IClientUnknown and IClientEntity
        mb.class_('IClientRenderable').mem_funs().virtuality = 'not virtual' 
        mb.class_('IClientNetworkable').mem_funs().virtuality = 'not virtual' 
        mb.class_('IClientThinkable').mem_funs().virtuality = 'not virtual'

        self.IncludeEmptyClass(mb, 'IClientUnknown')
        self.IncludeEmptyClass(mb, 'IClientEntity')
        
        for clsname in self.cliententities:
            cls = self.SetupEntityClass(mb, clsname)

            # Check if the python class is networkable. Add code for getting the "ClientClass" if that's the case.
            decl = cls.mem_funs('GetPyNetworkType', allow_empty=True)
            if decl:
                decl.include()
                cls.add_wrapper_code( tmpl_clientclass % {'clsname' : clsname})
                
            # Apply common rules
            # Excludes
            cls.mem_funs('GetClientClass', allow_empty=True).exclude()
            cls.mem_funs('GetPredDescMap', allow_empty=True).exclude()
            cls.mem_funs('GetVarMapping', allow_empty=True).exclude()
            cls.mem_funs('GetDataTableBasePtr', allow_empty=True).exclude()
            cls.mem_funs('OnNewModel', allow_empty=True).exclude() # Don't care for now
            cls.mem_funs('GetMouth', allow_empty=True).exclude()
            
            # Remove anything returning a pointer to C_CommandContext
            commandcontext = mb.class_('C_CommandContext')
            cls.calldefs(matchers.calldef_matcher_t(return_type=pointer_t(declarated_t(commandcontext))), allow_empty=True).exclude()
            
            # Exclude a list of functions with certain types
            interpvarvector = mb.class_('CInterpolatedVar< Vector >')
            interpvarqangle = mb.class_('CInterpolatedVar< QAngle >')
            excludetypes = [
                pointer_t(declarated_t(mb.class_('IClientRenderable'))),
                pointer_t(declarated_t(mb.class_('IClientNetworkable'))),
                pointer_t(declarated_t(mb.class_('IClientThinkable'))),
                pointer_t(declarated_t(mb.class_('IClientVehicle'))),
                pointer_t(const_t(declarated_t(mb.class_('IClientVehicle')))),
                pointer_t(declarated_t(mb.class_('IInterpolatedVar'))),
                pointer_t(declarated_t(interpvarvector)),
                reference_t(declarated_t(interpvarvector)),
                pointer_t(declarated_t(interpvarqangle)),
                reference_t(declarated_t(interpvarqangle)),
            ]
            cls.calldefs(calldef_withtypes(excludetypes), allow_empty=True).exclude()
            
        mb.mem_funs('SetThinkHandle').exclude()
        mb.mem_funs('GetThinkHandle').exclude()
            
    def ParseServerEntities(self, mb):
        self.IncludeEmptyClass(mb, 'IServerUnknown')
        self.IncludeEmptyClass(mb, 'IServerEntity')
        
        for clsname in self.serverentities:
            cls = self.SetupEntityClass(mb, clsname)
            
            # Check if the python class is networkable. Add code for getting the "ServerClass" if that's the case.
            decl = cls.mem_funs('GetPyNetworkType', allow_empty=True)
            if decl:
                decl.include()
                cls.add_wrapper_code(tmpl_serverclass % {'clsname' : clsname})
                
            # Apply common rules
            cls.mem_funs('GetServerClass', allow_empty=True).exclude()
            
            excludetypes = [
                pointer_t(declarated_t(mb.class_('IServerVehicle'))),
                pointer_t(const_t(declarated_t(mb.class_('IServerVehicle')))),
                pointer_t(declarated_t(mb.class_('IServerNetworkable'))),
                pointer_t(declarated_t(mb.class_('CEventAction'))),
                pointer_t(declarated_t(mb.class_('CCheckTransmitInfo'))),
                pointer_t(const_t(declarated_t(mb.class_('CCheckTransmitInfo')))),
            ]
            if self.settings.branch == 'source2013':
                # In swarm, this also exists on the server
                excludetypes.append(pointer_t(declarated_t(mb.class_('IResponseSystem'))))
            cls.calldefs(calldef_withtypes(excludetypes), allow_empty=True).exclude()

        # Spawning helper
        mb.free_functions('DispatchSpawn').include()

    def ParseBaseEntityHandles(self, mb):
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
        cls.mem_fun('Str').rename('__str__')
        
        cls.add_wrapper_code(
            'virtual PyObject *GetPySelf() { return boost::python::detail::wrapper_base_::get_owner(*this); }'
        )
    
    def ParseBaseEntity(self, mb):
        cls = mb.class_('C_BaseEntity' if self.isclient else 'CBaseEntity')
    
        # Free Send Properties
        count = 4
        datatypes = [
            { 'name' : 'Float', 'type' : 'float'}, 
            { 'name' : 'Int', 'type' : 'int'}, 
        ]
        
        for d in datatypes:
            for i in range(1, count+1):
                self.SetupProperty(cls, 'prop%s%d' % (d['type'], i), 'PyGetProp%s%d' % (d['name'], i), 'PySetProp%s%d' % (d['name'], i))
        
        # Exclude operators
        mb.global_ns.mem_opers('new').exclude()
        mb.global_ns.mem_opers('delete').exclude()
    
        # List of shared functions overridable in Python
        mb.mem_funs('Precache').virtuality = 'virtual'
        mb.mem_funs('Spawn').virtuality = 'virtual'
        mb.mem_funs('Activate').virtuality = 'virtual'
        mb.mem_funs('KeyValue').virtuality = 'virtual'
        mb.mem_funs('UpdateOnRemove').virtuality = 'virtual'
        mb.mem_funs('CreateVPhysics').virtuality = 'virtual'
        mb.mem_funs('GetTracerType').virtuality = 'virtual'
        mb.mem_funs('MakeTracer').virtuality = 'virtual'
        mb.mem_funs('DoImpactEffect').virtuality = 'virtual'
        mb.mem_funs('StartTouch').virtuality = 'virtual'
        mb.mem_funs('EndTouch').virtuality = 'virtual'
        mb.mem_funs('UpdateTransmitState').virtuality = 'virtual'
        mb.mem_funs('ComputeWorldSpaceSurroundingBox').virtuality = 'virtual'
        mb.mem_funs('OnRestore').virtuality = 'virtual'
        
        # Call policies
        cls.mem_funs('CollisionProp').call_policies = call_policies.return_internal_reference()
        mb.mem_funs('GetBeamTraceFilter').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs('GetTouchTrace').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)  # unsafe
        
        # Rename python methods to match the c++ names ( but in c++ they got python prefixes )
        mb.mem_funs('SetPyTouch').rename('SetTouch')
        mb.mem_funs('SetPyThink').rename('SetThink')
        mb.mem_funs('GetPyThink').rename('GetThink')
        mb.mem_funs('GetPyHandle').rename('GetHandle')
        mb.mem_funs('PyThink').exclude()
        mb.mem_funs('PyTouch').exclude()
        
        # Excludes
        cls.mem_funs('GetPyInstance').exclude()          # Not needed, used when converting entities to python
        cls.mem_funs('SetPyInstance').exclude()          # Not needed, used when converting entities to python
        cls.mem_funs('PyAllocate').exclude()             # Python Intern only
        cls.mem_funs('PyDeallocate').exclude()           # Python Intern only
        
        mb.mem_funs('GetRefEHandle').exclude()          # We already got an auto conversion to a safe handle
        mb.mem_funs('SetRefEHandle').exclude()          # We already got an auto conversion to a safe handle
        mb.mem_funs('GetModel').exclude()               # Probably not needed

        mb.mem_funs('GetBaseEntity').exclude()          # Automatically done by converter
        mb.mem_funs('GetBaseAnimating').exclude() # Automatically done by converter
        mb.mem_funs('MyCombatCharacterPointer').exclude() # Automatically done by converter
        mb.mem_funs('MyNPCPointer').exclude()
        mb.mem_funs('MyCombatWeaponPointer').exclude() # Automatically done by converter
        
        mb.mem_funs('ThinkSet').exclude()               # Replaced by SetPyThink
        mb.mem_funs('PhysicsRunThink').exclude()            # Don't care  
        mb.mem_funs('PhysicsRunSpecificThink').exclude()    # Don't care   
        mb.mem_funs('PhysicsDispatchThink').exclude()   # Don't care
        mb.mem_funs('VPhysicsGetObjectList').exclude()  # Don't care for now
        
        mb.mem_funs('GetDataObject').exclude() # Don't care
        mb.mem_funs('CreateDataObject').exclude() # Don't care
        mb.mem_funs('DestroyDataObject').exclude() # Don't care
        mb.mem_funs('DestroyAllDataObjects').exclude() # Don't care
        mb.mem_funs('AddDataObjectType').exclude() # Don't care
        mb.mem_funs('RemoveDataObjectType').exclude() # Don't care
        mb.mem_funs('HasDataObjectType').exclude() # Don't care
        
        # Use isclient/isserver globals/builtins
        mb.mem_funs('IsServer').exclude() 
        mb.mem_funs('IsClient').exclude() 
        mb.mem_funs('GetDLLType').exclude() 
        
        # Transform EmitSound
        # There seems to be a problem with static and member functions with the same name
        # Rename EmitSound with filter into "EmitSoundFilter"
        # Rename static StopSound to StopSoundStatic
        decls = mb.mem_funs('EmitSound')
        
        mb.mem_funs('EmitSound', calldef_withtypes(reference_t(declarated_t(mb.class_('IRecipientFilter'))))).rename('EmitSoundFilter')
        mb.mem_funs('StopSound', lambda decl: decl.has_static).rename('StopSoundStatic')
        
        for decl in decls:
            for arg in decl.arguments:
                if arg.name == 'duration':
                    decl.add_transformation(FT.output('duration'))
        mb.typedef('HSOUNDSCRIPTHANDLE').include()
        
        # Create properties for the following variables, since they are networked
        for entcls in self.entclasses:
            if entcls == cls or next((x for x in entcls.recursive_bases if x.related_class == cls), None):
                self.AddNetworkVarProperty('lifestate', 'm_lifeState', 'int', entcls)
                self.AddNetworkVarProperty('takedamage', 'm_takedamage', 'int', entcls)
        
        if self.isclient:
            cls.mem_funs('ParticleProp').call_policies = call_policies.return_internal_reference() 
            
            # List of client functions overridable in Python
            mb.mem_funs('ShouldDraw').virtuality = 'virtual' # Called when visibility is updated, doesn't happens a lot.
            mb.mem_funs('GetCollideType').virtuality = 'virtual'
            mb.mem_funs('ClientThink').virtuality = 'virtual'
            mb.mem_funs('OnDataChanged').virtuality = 'virtual'
            mb.mem_funs('Simulate').virtuality = 'virtual'
            mb.mem_funs('NotifyShouldTransmit').virtuality = 'virtual'
            mb.mem_funs('PyReceiveMessage').virtuality = 'virtual'
            mb.mem_funs('PyReceiveMessage').rename('ReceiveMessage')

            # Excludes
            mb.mem_funs('Release').exclude() # Should not be called directly from Python
            cls.mem_funs('GetIClientUnknown').exclude()
            cls.mem_funs('GetIClientEntity').exclude()
            cls.mem_funs('GetClientHandle').exclude()
            cls.mem_funs('RenderHandle').exclude()
            
            cls.mem_funs('GetPVSNotifyInterface').exclude()
            cls.mem_funs('GetRenderClipPlane').exclude() # Pointer to 4 floats, requires manual conversion...
            cls.mem_funs('GetIDString').exclude()
            cls.mem_funs('PhysicsAddHalfGravity').exclude() # No definition on the client
            cls.mem_funs('SetModelPointer').exclude() # Likely never needed, can use SetModel or SetModelIndex
            mb.mem_funs('OnNewModel').exclude() # TODO
            cls.mem_fun('OnNewParticleEffect').exclude()
            if self.settings.branch == 'swarm':
                cls.mem_fun('OnParticleEffectDeleted').exclude()
            
            mb.mem_funs('AllocateIntermediateData').exclude()
            mb.mem_funs('DestroyIntermediateData').exclude()
            mb.mem_funs('ShiftIntermediateDataForward').exclude()
            mb.mem_funs('GetPredictedFrame').exclude()
            mb.mem_funs('GetOriginalNetworkDataObject').exclude()
            mb.mem_funs('IsIntermediateDataAllocated').exclude()
            cls.mem_funs('AttemptToPowerup').exclude() # CDamageModifier has no class on the client...
            
            mb.mem_funs('PyUpdateNetworkVar').exclude() # Internal for network vars
            
            if self.settings.branch == 'swarm':
                mb.mem_funs('GetClientAlphaProperty').exclude()
                mb.mem_funs('GetClientModelRenderable').exclude()
                mb.mem_funs('GetResponseSystem').exclude()
                mb.mem_funs('GetScriptDesc').exclude()
                mb.mem_funs('GetScriptInstance').exclude()
                mb.mem_funs('AlphaProp').exclude()
                
            # Not interested in Interpolation related functions
            mb.mem_funs( lambda decl: 'Interp_' in decl.name ).exclude()
                
            # Transform
            mb.mem_funs('GetShadowCastDistance').add_transformation(FT.output('pDist'))
            
            # Rename public variables
            self.IncludeVarAndRename('m_iHealth', 'health')
            self.IncludeVarAndRename('m_nRenderFX', 'renderfx')
            if self.settings.branch == 'source2013':
                self.IncludeVarAndRename('m_nRenderFXBlend', 'renderfxblend')
            self.IncludeVarAndRename('m_nRenderMode', 'rendermode')
            self.IncludeVarAndRename('m_clrRender', 'clrender')
            self.IncludeVarAndRename('m_flAnimTime', 'animtime')
            self.IncludeVarAndRename('m_flOldAnimTime', 'oldanimtime')
            self.IncludeVarAndRename('m_flSimulationTime', 'simulationtime')
            self.IncludeVarAndRename('m_flOldSimulationTime', 'oldsimulationtime')
            self.IncludeVarAndRename('m_nNextThinkTick', 'nextthinktick')
            self.IncludeVarAndRename('m_nLastThinkTick', 'lastthinktick')  
            self.IncludeVarAndRename('m_iClassname', 'classname')    
            self.IncludeVarAndRename('m_flSpeed', 'speed')

            # Client thinking vars
            mb.add_registration_code( "bp::scope().attr( \"CLIENT_THINK_ALWAYS\" ) = CLIENT_THINK_ALWAYS;" )
            mb.add_registration_code( "bp::scope().attr( \"CLIENT_THINK_NEVER\" ) = CLIENT_THINK_NEVER;" )
            
            if self.settings.branch == 'swarm':
                # Entity lists, swarm only
                mb.mem_funs('AddToEntityList').include()
                mb.mem_funs('RemoveFromEntityList').include()
                mb.enum('entity_list_ids_t').include()
        else:
            cls.mem_funs('PySendEvent').include()
            cls.mem_funs('PySendEvent').rename('SendEvent')
            self.IncludeVarAndRename('m_flPrevAnimTime', 'prevanimtime')
            self.IncludeVarAndRename('m_nNextThinkTick', 'nextthinktick')
            self.IncludeVarAndRename('m_nLastThinkTick', 'lastthinktick')
            self.IncludeVarAndRename('m_iClassname', 'classname')
            self.IncludeVarAndRename('m_iGlobalname', 'globalname')
            self.IncludeVarAndRename('m_iParent', 'parent')
            self.IncludeVarAndRename('m_iHammerID', 'hammerid')
            self.IncludeVarAndRename('m_flSpeed', 'speed')
            self.IncludeVarAndRename('m_debugOverlays', 'debugoverlays')
            self.IncludeVarAndRename('m_bAllowPrecache', 'allowprecache')
            self.IncludeVarAndRename('m_bInDebugSelect', 'indebugselect')
            self.IncludeVarAndRename('m_nDebugPlayer', 'debugplayer')
            self.IncludeVarAndRename('m_target', 'target')
            self.IncludeVarAndRename('m_iszDamageFilterName', 'damagefiltername')

            # Properties
            self.SetupProperty(cls, 'health', 'GetHealth', 'SetHealth')
            self.SetupProperty(cls, 'maxhealth', 'GetMaxHealth', 'SetMaxHealth')
            self.SetupProperty(cls, 'animtime', 'GetAnimTime', 'SetAnimTime')
            self.SetupProperty(cls, 'simulationtime', 'GetSimulationTime', 'SetSimulationTime')
            self.SetupProperty(cls, 'rendermode', 'GetRenderMode', 'SetRenderMode', excludesetget=False)
            
            mb.mem_funs('PySendMessage').rename('SendMessage')
            
            # List of server functions overridable in Python
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
            cls.mem_funs('NetworkProp').exclude()
            cls.mem_funs('MyNextBotPointer').exclude()
            mb.mem_funs('NotifySystemEvent').exclude()
            mb.mem_funs('Entity').exclude()
            
            mb.mem_funs('PhysicsTestEntityPosition').exclude()  # Don't care  
            mb.mem_funs('PhysicsCheckRotateMove').exclude()     # Don't care  
            mb.mem_funs('PhysicsCheckPushMove').exclude()       # Don't care

            mb.mem_funs('ForceVPhysicsCollide').exclude() # Don't care
            mb.mem_funs('GetGroundVelocityToApply').exclude() # Don't care
            mb.mem_funs('GetMaxHealth').exclude() # Use property maxhealth
            mb.mem_funs('SetModelIndex').exclude()
            
            if self.settings.branch == 'swarm':
                mb.mem_funs('GetEntityNameAsCStr').exclude() # Always use GetEntityName()
                
                mb.mem_funs('SendProxy_AnglesX').exclude()
                mb.mem_funs('SendProxy_AnglesY').exclude()
                mb.mem_funs('SendProxy_AnglesZ').exclude()
                
                mb.mem_funs('FindNamedOutput').exclude()
                mb.mem_funs('GetBaseAnimatingOverlay').exclude()
                mb.mem_funs('GetContextData').exclude()
                mb.mem_funs('GetScriptDesc').exclude()
                mb.mem_funs('GetScriptInstance').exclude()
                mb.mem_funs('GetScriptOwnerEntity').exclude()
                mb.mem_funs('GetScriptScope').exclude()
                mb.mem_funs('ScriptFirstMoveChild').exclude()
                mb.mem_funs('ScriptGetModelKeyValues').exclude()
                mb.mem_funs('ScriptGetMoveParent').exclude()
                mb.mem_funs('ScriptGetRootMoveParent').exclude()
                mb.mem_funs('ScriptNextMovePeer').exclude()
                mb.mem_funs('InputDispatchEffect').exclude() # No def?
            
            # Do not want the firebullets function with multiple arguments. Only the one with the struct.
            mb.mem_funs(name='FireBullets', function=calldef_withtypes('int')).exclude()

    def ParseBaseAnimating(self, mb):
        cls = mb.class_('C_BaseAnimating' if self.isclient else 'CBaseAnimating')
    
        # Transformations
        mb.mem_funs('GetPoseParameterRange').add_transformation(FT.output('minValue'), FT.output('maxValue'))
        
        # Give back a direct reference to CStudioHdr (not fully safe, but should be OK)
        studiohdr = mb.class_('CStudioHdr')
        mb.calldefs(matchers.calldef_matcher_t(return_type=pointer_t(declarated_t(studiohdr))), allow_empty=True).call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)  
        
        # Create properties for the following variables, since they are networked
        if self.isclient or self.settings.branch != 'source2013':
            cls.mem_fun('GetSkin').exclude()
        for entcls in self.entclasses:
            if entcls == cls or next((x for x in entcls.recursive_bases if x.related_class == cls), None):
                self.AddNetworkVarProperty('skin', 'm_nSkin', 'int', entcls)    

        # Exclude anything return CBoneCache
        bonecache = mb.class_('CBoneCache')
        mb.calldefs(matchers.calldef_matcher_t(return_type=pointer_t(declarated_t(bonecache))), allow_empty=True).exclude()
            
        if self.isclient:
            cls.vars('m_SequenceTransitioner').exclude()
            cls.vars('m_nHitboxSet').exclude()
            cls.vars('m_pClientsideRagdoll').exclude()
            cls.vars('m_pRagdoll').exclude()
            
            mb.mem_funs('PyOnNewModel').rename('OnNewModel')
            mb.mem_funs('PyOnNewModel').virtuality = 'virtual'
            
            # Exclude
            if self.settings.branch == 'swarm':
                mb.mem_funs('GetBoneArrayForWrite').exclude()
                mb.mem_funs('GetBoneForWrite').exclude()

            cls.add_property( 'customlightingoffset',
                               cls.mem_fun('GetCustomLightingOffset'),
                               cls.mem_fun('SetCustomLightingOffset')) 
            cls.mem_fun('GetCustomLightingOffset').exclude()
            cls.mem_fun('SetCustomLightingOffset').exclude()
        else:
            mb.mem_funs('PyOnNewModel').include()
            mb.mem_funs('PyOnNewModel').rename('OnNewModel')
            mb.mem_funs('PyOnNewModel').virtuality = 'virtual'
            mb.mem_funs('OnSequenceSet').virtuality = 'virtual'
            
            # excludes
            mb.mem_funs('GetPoseParameterArray').exclude()
            mb.mem_funs('GetEncodedControllerArray').exclude()

            if self.settings.branch == 'swarm':
                mb.mem_funs('GetBoneCache').exclude()
                mb.mem_funs('InputIgniteNumHitboxFires').exclude()
                mb.mem_funs('InputIgniteHitboxFireScale').exclude()
                mb.mem_funs( name='GetBonePosition', function=lambda decl: HasArgType(decl, 'char') ).exclude() 
                
            # Rename vars
            self.IncludeVarAndRename('m_OnIgnite', 'onignite')
            self.IncludeVarAndRename('m_flGroundSpeed', 'groundspeed')
            self.IncludeVarAndRename('m_flLastEventCheck', 'lastevencheck')
            
            # Transformations
            mb.mem_funs('GotoSequence').add_transformation(FT.output('iNextSequence'), FT.output('flCycle'), FT.output('iDir'))
            mb.mem_funs('LookupHitbox').add_transformation(FT.output('outSet'), FT.output('outBox'))
            mb.mem_funs('GetIntervalMovement').add_transformation(FT.output('bMoveSeqFinished'))
            
            # Enums
            mb.enums('LocalFlexController_t').include()
        
    def ParseBaseAnimatingOverlay(self, mb):
        cls = mb.class_('C_BaseAnimatingOverlay') if self.isclient else mb.class_('CBaseAnimatingOverlay')
    
        cls.mem_funs('GetAnimOverlay').exclude()

    def ParseBaseFlex(self, mb):
        cls = mb.class_('C_BaseFlex') if self.isclient else mb.class_('CBaseFlex')

        excludetypes = [
            pointer_t(declarated_t(mb.class_('CChoreoScene'))),
            pointer_t(declarated_t(mb.class_('CChoreoActor'))),
        ]
        mb.calldefs( calldef_withtypes( excludetypes ) ).exclude()
        
        if self.isserver:
            mb.mem_funs('FlexSettingLessFunc').exclude()
            cls.class_('FS_LocalToGlobal_t').exclude()
            mb.mem_funs( lambda decl: HasArgType(decl, 'AI_Response') ).exclude() 
            if self.settings.branch == 'swarm':
                mb.mem_funs('ScriptGetOldestScene').exclude()
                mb.mem_funs('ScriptGetSceneByIndex').exclude()
            
    def ParseBaseCombatWeapon(self, mb):
        cls_name = 'C_BaseCombatWeapon' if self.isclient else 'CBaseCombatWeapon'
        cls = mb.class_(cls_name)
        
        # Overridable
        mb.mem_funs('PrimaryAttack').virtuality = 'virtual'
        mb.mem_funs('SecondaryAttack').virtuality = 'virtual'
        
        # Shared Excludes
        mb.mem_funs('ActivityList').exclude()
        mb.mem_funs('GetConstraint').exclude()
        mb.mem_funs('GetDeathNoticeName').exclude()
        mb.mem_funs('GetEncryptionKey').exclude()
        mb.mem_funs('GetProficiencyValues').exclude()
        mb.mem_funs('GetControlPanelClassName').exclude()
        mb.mem_funs('GetControlPanelInfo').exclude()
        
        if self.isclient:
            # Exclude anything returning a pointer to CHudTexture (don't care for now)
            hudtexture = mb.class_('CHudTexture')
            mb.calldefs(matchers.calldef_matcher_t(return_type=pointer_t(declarated_t(hudtexture))), allow_empty=True).exclude()
            mb.calldefs(matchers.calldef_matcher_t(return_type=pointer_t(const_t(declarated_t(hudtexture)))), allow_empty=True).exclude()
            
            if self.settings.branch == 'swarm':
                mb.mem_funs('GetWeaponList').exclude() # Returns a CUtlLinkedList
        else:
            # Exclude anything returning a pointer to CHudTexture
            mb.calldefs(matchers.calldef_matcher_t(return_type='::CHudTexture const *'), allow_empty=True).exclude()
            mb.calldefs(matchers.calldef_matcher_t(return_type='::CHudTexture *'), allow_empty=True).exclude()
            
            # Server excludes
            mb.mem_funs('RepositionWeapon').exclude() # Declaration only...
            mb.mem_funs('IsInBadPosition').exclude() # Declaration only...
            if self.settings.branch == 'swarm':
                mb.mem_funs('IsCarrierAlive').exclude() # Declaration only..
            if self.settings.branch == 'source2013':
                mb.mem_funs('GetDmgAccumulator').exclude()
                
        # Rename variables
        self.IncludeVarAndRename('m_bAltFiresUnderwater', 'altfiresunderwater')
        self.IncludeVarAndRename('m_bFireOnEmpty', 'fireonempty')
        self.IncludeVarAndRename('m_bFiresUnderwater', 'firesunderwater')
        self.IncludeVarAndRename('m_bInReload', 'inreload')
        self.IncludeVarAndRename('m_bReloadsSingly', 'reloadssingly')
        self.IncludeVarAndRename('m_fFireDuration', 'fireduration')
        self.IncludeVarAndRename('m_fMaxRange1', 'maxrange1')
        self.IncludeVarAndRename('m_fMaxRange2', 'maxrange2')
        self.IncludeVarAndRename('m_fMinRange1', 'minrange1')
        self.IncludeVarAndRename('m_fMinRange2', 'minrange2')
        self.IncludeVarAndRename('m_flNextEmptySoundTime', 'nextemptysoundtime')
        self.IncludeVarAndRename('m_flUnlockTime', 'unlocktime')
        self.IncludeVarAndRename('m_hLocker', 'locker')
        self.IncludeVarAndRename('m_iSubType', 'subtype')
        self.IncludeVarAndRename('m_iViewModelIndex', 'viewmodelindex')
        self.IncludeVarAndRename('m_iWorldModelIndex', 'worldmodelindex')
        self.IncludeVarAndRename('m_iszName', 'name')
        self.IncludeVarAndRename('m_nViewModelIndex', 'viewmodelindex')
        
        # Create properties for the following variables, since they are networked
        for entcls in self.entclasses:
            if entcls == cls or next((x for x in entcls.recursive_bases if x.related_class == cls), None):
                self.AddNetworkVarProperty('nextprimaryattack', 'm_flNextPrimaryAttack', 'float', entcls)
                self.AddNetworkVarProperty('nextsecondaryattack', 'm_flNextSecondaryAttack', 'float', entcls)
                self.AddNetworkVarProperty('timeweaponidle', 'm_flTimeWeaponIdle', 'float', entcls)
                self.AddNetworkVarProperty('state', 'm_iState', 'int', entcls)
                self.AddNetworkVarProperty('primaryammotype', 'm_iPrimaryAmmoType', 'int', entcls)
                self.AddNetworkVarProperty('secondaryammotype', 'm_iSecondaryAmmoType', 'int', entcls)
                self.AddNetworkVarProperty('clip1', 'm_iClip1', 'int', entcls)
                self.AddNetworkVarProperty('clip2', 'm_iClip2', 'int', entcls)
            
        # Misc
        mb.enum('WeaponSound_t').include()
        mb.enum('WeaponSound_t').rename('WeaponSound')
                         
    def ParseBaseCombatCharacter(self, mb):
        cls = mb.class_('C_BaseCombatCharacter' if self.isclient else 'CBaseCombatCharacter')
        
        # enums
        mb.enum('Disposition_t').include()
        
        self.IncludeVarAndRename('m_HackedGunPos', 'hackedgunpos')
        
        mb.mem_funs('GetLastKnownArea').exclude()
        mb.mem_funs('IsAreaTraversable').exclude()
        mb.mem_funs('ClearLastKnownArea').exclude()
        mb.mem_funs('UpdateLastKnownArea').exclude()
        mb.mem_funs('OnNavAreaChanged').exclude()
        mb.mem_funs('OnNavAreaRemoved').exclude()
        
        if self.isclient:
            self.SetupProperty(cls, 'activeweapon', 'GetActiveWeapon')
                             
            # LIST OF CLIENT FUNCTIONS TO OVERRIDE
            mb.mem_funs('OnActiveWeaponChanged').virtuality = 'virtual'
        else:
            cls.member_function('SetActiveWeapon').exclude()
            self.SetupProperty(cls, 'activeweapon', 'GetActiveWeapon', 'SetActiveWeapon')
        
            # Exclude
            mb.mem_funs('RemoveWeapon').exclude() # Declaration only
            mb.mem_funs('CauseDeath').exclude()
            #mb.mem_funs('FInViewCone').exclude()
            mb.mem_funs('OnPursuedBy').exclude()

            if self.settings.branch == 'swarm':
                mb.mem_funs('GetEntitiesInFaction').exclude()
            mb.mem_funs('GetFogTrigger').exclude()
            mb.mem_funs('PlayFootstepSound').exclude()
            
            mb.free_function('RadiusDamage').include()

            self.IncludeVarAndRename('m_bForceServerRagdoll', 'forceserverragdoll')
            self.IncludeVarAndRename('m_bPreventWeaponPickup', 'preventweaponpickup')
            
            # LIST OF SERVER FUNCTIONS TO OVERRIDE
            mb.mem_funs('Weapon_Equip').virtuality = 'virtual'
            mb.mem_funs('Weapon_Switch').virtuality = 'virtual'
            mb.mem_funs('Weapon_Drop').virtuality = 'virtual'
            mb.mem_funs('Event_KilledOther').virtuality = 'virtual'
            
    def ParseBasePlayer(self, mb):
        cls = mb.class_('C_BasePlayer') if self.isclient else mb.class_('CBasePlayer')
 
        self.IncludeVarAndRename('m_nButtons', 'buttons')
        self.IncludeVarAndRename('m_afButtonLast', 'buttonslast')
        self.IncludeVarAndRename('m_afButtonPressed', 'buttonspressed')
        self.IncludeVarAndRename('m_afButtonReleased', 'buttonsreleased')
 
        cls.mem_fun('GetLadderSurface').exclude()
        cls.mem_fun('Hints').exclude()
        if self.settings.branch == 'source2013' or self.isserver:
            cls.mem_fun('GetSurfaceData').exclude()
        
        if self.isclient:
            # Exclude for now
            cls.mem_fun('GetFogParams').exclude()
            cls.mem_fun('OverrideView').exclude()
            cls.mem_fun('GetFootstepSurface').exclude()
            cls.mem_fun('GetHeadLabelMaterial').exclude()
            cls.mem_fun('GetRepresentativeRagdoll').exclude()
            cls.mem_fun('ShouldGoSouth').exclude() # No definition
            
            if self.settings.branch == 'swarm':
                mb.mem_funs('ActivePlayerCombatCharacter').exclude()
                mb.mem_funs('GetActiveColorCorrection').exclude()
                mb.mem_funs('GetActivePostProcessController').exclude()
                mb.mem_funs('GetPotentialUseEntity').exclude()
                mb.mem_funs('GetSoundscapeListener').exclude()
                mb.mem_funs('GetSplitScreenPlayers').exclude()
                mb.mem_funs('GetViewEntity').exclude()
                mb.mem_funs('IsReplay').exclude()

            mb.mem_funs('CalcView').add_transformation(FT.output('zNear'), FT.output('zFar'), FT.output('fov'))
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
            mb.mem_funs('GetLastKnownArea').exclude()
            mb.mem_funs('GetPhysicsController').exclude()
            mb.mem_funs('GetGroundVPhysics').exclude()
            mb.mem_funs('GetExpresser').exclude()
            mb.mem_funs('GetAudioParams').exclude()
            mb.mem_funs('SetupVPhysicsShadow').exclude()
            
            if self.settings.branch == 'swarm':
                mb.mem_funs('ActivePlayerCombatCharacter').exclude()
                mb.mem_funs('FindPickerAILink').exclude()
                mb.mem_funs('GetPlayerProxy').exclude()
                mb.mem_funs('GetSoundscapeListener').exclude()
                mb.mem_funs('GetSplitScreenPlayerOwner').exclude()
                mb.mem_funs('GetSplitScreenPlayers').exclude()
                mb.mem_funs('GetTonemapController').exclude()
                mb.mem_funs('FindPickerAINode').exclude()

    def ParseTriggers(self, mb):
        if self.isclient and self.settings.branch == 'source2013':
            return
            
        # CBaseTrigger
        cls_name = 'C_BaseTrigger' if self.isclient else 'CBaseTrigger'
        cls = mb.class_(cls_name)
        cls.no_init = False
        
        if self.isserver:
            cls.mem_funs('GetTouchingEntities').exclude()
            cls.mem_funs('GetClientSidePredicted').exclude() 
            cls.mem_funs('SetClientSidePredicted').exclude() 
            cls.add_property( 'clientsidepredicted'
                             , cls.mem_fun('GetClientSidePredicted')
                             , cls.mem_fun('SetClientSidePredicted') )
        else:
            self.IncludeVarAndRename('m_bClientSidePredicted', 'clientsidepredicted')

        # CTriggerMultiple
        if self.isserver:
            cls = mb.class_('CTriggerMultiple')
            mb.class_('CTriggerMultiple').no_init = False
            self.IncludeVarAndRename('m_bDisabled', 'disabled')
            self.IncludeVarAndRename('m_hFilter', 'filter')
            self.IncludeVarAndRename('m_iFilterName', 'filtername')
            
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
                         
    def ParseProps(self, mb):
        if self.isserver:
            cls = mb.class_('CBreakableProp')
            cls.mem_funs('GetRootPhysicsObjectForBreak').exclude()
            
            cls = mb.class_('CPhysicsProp')
                
            cls = mb.class_('CRagdollProp')
            cls.mem_funs('GetRagdoll').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        

    def ParseRemainingEntities(self, mb):
        # CBaseGrenade
        cls_name = 'C_BaseGrenade' if self.isclient else 'CBaseGrenade'
        cls = mb.class_(cls_name)
        
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

        if self.isserver:
            # Not sure where to put this
            mb.free_function('DoSpark').include()
        
            # CSoundEnt
            self.IncludeEmptyClass(mb, 'CSoundEnt')
            mb.mem_funs('InsertSound').include()
            
            # CGib
            mb.free_functions('CreateRagGib').include()
            mb.enum('GibType_e').include()
            
            # CSprite
            mb.free_functions('SpawnBlood').include()

            cls = mb.class_('SmokeTrail')
            self.IncludeVarAndRename('m_EndSize', 'endsize')
            self.IncludeVarAndRename('m_MaxSpeed', 'maxspeed')
            self.IncludeVarAndRename('m_MinSpeed', 'minspeed')
            self.IncludeVarAndRename('m_MaxDirectedSpeed', 'maxdirectedspeed')
            self.IncludeVarAndRename('m_MinDirectedSpeed', 'mindirectedspeed')
            self.IncludeVarAndRename('m_Opacity', 'opacity')
            self.IncludeVarAndRename('m_ParticleLifetime', 'particlelifetime')
            self.IncludeVarAndRename('m_SpawnRadius', 'spawnradius')
            self.IncludeVarAndRename('m_SpawnRate', 'spawnrate')
            self.IncludeVarAndRename('m_StartSize', 'startsize')
            self.IncludeVarAndRename('m_StopEmitTime', 'stopemittime')
            self.IncludeVarAndRename('m_bEmit', 'emit')
            self.IncludeVarAndRename('m_nAttachment', 'attachment')


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
            
            cls = mb.class_('RocketTrail')
            self.IncludeVarAndRename('m_EndSize', 'endsize')
            self.IncludeVarAndRename('m_MaxSpeed', 'maxspeed')
            self.IncludeVarAndRename('m_MinSpeed', 'minspeed')
            self.IncludeVarAndRename('m_Opacity', 'opacity')
            self.IncludeVarAndRename('m_ParticleLifetime', 'particlelifetime')
            self.IncludeVarAndRename('m_SpawnRadius', 'spawnradius')
            self.IncludeVarAndRename('m_SpawnRate', 'spawnrate')
            self.IncludeVarAndRename('m_StartSize', 'startsize')
            self.IncludeVarAndRename('m_StopEmitTime', 'stopemittime')
            self.IncludeVarAndRename('m_bEmit', 'emit')
            self.IncludeVarAndRename('m_nAttachment', 'attachment')
            self.IncludeVarAndRename('m_bDamaged', 'damaged')
            self.IncludeVarAndRename('m_flFlareScale', 'flarescale')
            
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
            
            # Base filter
            cls = mb.class_('CBaseFilter')
            cls.no_init = False
            cls.mem_funs('PassesFilterImpl').virtuality = 'virtual' 
            cls.mem_funs('PassesDamageFilterImpl').virtuality = 'virtual' 
            
            # CEntityFlame
            cls = mb.class_('CEntityFlame')
        else:
            # C_PlayerResource
            mb.add_declaration_code( "C_PlayerResource *wrap_PlayerResource( void )\r\n{\r\n\treturn g_PR;\r\n}\r\n" )
            mb.add_registration_code( 'bp::def( "PlayerResource", wrap_PlayerResource, bp::return_value_policy< bp::return_by_value >() );' )   
           
            # C_Sprite
            mb.mem_funs('GlowBlend').exclude()

    # Wars Entity Parsing
    def ParseWars(self, mb):
        ''' Parsing wars additions for base entity classes. '''
        baseentcls  = mb.class_('C_BaseEntity' if self.isclient else 'CBaseEntity')
        
        # Overridables
        mb.mem_funs('GetIMouse').virtuality = 'virtual'
        mb.mem_funs('OnChangeOwnerNumber').virtuality = 'virtual'
        
        # Call policies
        mb.mem_funs('GetIMouse').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
                
        if self.isclient:
            mb.mem_funs('GetTeamColor').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        
        # Properties
        if self.isclient:
            self.SetupProperty(baseentcls, 'viewdistance', 'GetViewDistance')
        else:
            self.SetupProperty(baseentcls, 'viewdistance', 'GetViewDistance', 'SetViewDistance')
        
        # Excludes
        mb.mem_funs('OnChangeOwnerNumberInternal').exclude()
        mb.mem_funs('MyUnitPointer').exclude() # Automatically done by converter
        mb.mem_funs('GetIUnit').exclude()
        
        mb.mem_funs('CanBeSeenBy').exclude()
        if self.isserver:
            mb.mem_funs('FOWShouldTransmit').exclude()
            mb.mem_funs('DensityMap').exclude() # Don't care for now.
            
        # Map boundary
        cls_name = 'C_BaseFuncMapBoundary' if self.isclient else 'CBaseFuncMapBoundary'
        cls = mb.class_(cls_name)
        cls.vars('m_pNext').exclude()
        mb.mem_funs('IsWithinAnyMapBoundary').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
    
    def ParseHL2WarsPlayer(self, mb):
        cls = mb.class_('C_HL2WarsPlayer') if self.isclient else mb.class_('CHL2WarsPlayer')

        cls.mem_funs('GetUnit').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        cls.mem_funs('GetGroupUnit').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        cls.mem_funs('GetControlledUnit').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        cls.mem_funs('GetMouseCapture').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        
        cls.mem_funs('OnLeftMouseButtonPressed').virtuality = 'virtual'
        cls.mem_funs('OnLeftMouseButtonDoublePressed').virtuality = 'virtual'
        cls.mem_funs('OnLeftMouseButtonReleased').virtuality = 'virtual'
        cls.mem_funs('OnRightMouseButtonPressed').virtuality = 'virtual'
        cls.mem_funs('OnRightMouseButtonDoublePressed').virtuality = 'virtual'
        cls.mem_funs('OnRightMouseButtonReleased').virtuality = 'virtual'
    
        if self.isclient:
            mb.mem_funs('GetLocalHL2WarsPlayer').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs('GetSelectedUnitTypeRange').add_transformation( FT.output('iMin'), FT.output('iMax') )
            
            cls.add_property( 'unit'
                             , cls.mem_fun('GetControlledUnit')) 
                             
            cls.mem_fun('CamFollowGroup').exclude()
            cls.mem_fun('PyCamFollowGroup').rename('CamFollowGroup')
            
            cls.mem_fun('MakeSelection').exclude()
            cls.mem_fun('PyMakeSelection').rename('MakeSelection')
        else:
            mb.mem_funs('EntSelectSpawnPoint').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            
            cls.add_property( 'unit'
                             , cls.mem_fun('GetControlledUnit')
                             , cls.mem_fun('SetControlledUnit')) 
                             
        mb.add_registration_code( "bp::scope().attr( \"PLAYER_MAX_GROUPS\" ) = PLAYER_MAX_GROUPS;" )
        
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
        
        if self.isclient:
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
        
        if self.isclient:
            self.AddProperty(cls, 'energy', 'GetEnergy')
            self.AddProperty(cls, 'maxenergy', 'GetMaxEnergy')
        else:
            self.AddProperty(cls, 'energy', 'GetEnergy', 'SetEnergy')
            self.AddProperty(cls, 'maxenergy', 'GetMaxEnergy', 'SetMaxEnergy')
        self.AddProperty(cls, 'kills', 'GetKills', 'SetKills')
        
    def ParseUnitBase(self, mb):
        cls_name = 'C_UnitBase' if self.isclient else 'CUnitBase'
        cls = mb.class_(cls_name)
        mb.free_function('SetPlayerRelationShip').include()
        mb.free_function('GetPlayerRelationShip').include()
        
        self.IncludeVarAndRename('m_bFOWFilterFriendly', 'fowfilterfriendly')
        
        self.IncludeVarAndRename('m_fEyePitch', 'eyepitch')
        self.IncludeVarAndRename('m_fEyeYaw', 'eyeyaw')
        
        self.IncludeVarAndRename('m_bNeverIgnoreAttacks', 'neverignoreattacks')
        self.IncludeVarAndRename('m_bBodyTargetOriginBased', 'bodytargetoriginbased')
        
        self.IncludeVarAndRename('m_fAccuracy', 'accuracy')
        
        # List of overridables
        mb.mem_funs('OnUnitTypeChanged').virtuality = 'virtual'
        mb.mem_funs('UserCmd').virtuality = 'virtual'
        mb.mem_funs('OnButtonsChanged').virtuality = 'virtual'
        mb.mem_funs('CustomCanBeSeen').virtuality = 'virtual'
        if self.isclient:
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
        
        mb.free_function('MapUnits').include()
        
        self.ParseUnitBaseShared(mb, cls_name)
        
        cls.mem_funs('PyGetAnimState').exclude() 
        cls.mem_funs('GetAnimState').exclude() 
        cls.mem_funs('SetAnimState').exclude() 
        cls.add_property( 'animstate'
                         , cls.mem_fun('PyGetAnimState')
                         , cls.mem_fun('SetAnimState') )
        
        #cls.mem_funs('GetEnemy').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        mb.mem_funs('GetEnemy').exclude() 
        if self.isclient:
            self.IncludeVarAndRename('m_iMaxHealth', 'maxhealth')
            cls.add_property( 'enemy'
                             , cls.mem_fun('GetEnemy'))
                             
            cls.add_property( 'crouching'
                             , cls.mem_fun('IsCrouching')) 
            cls.add_property( 'climbing'
                             , cls.mem_fun('IsClimbing'))    
            self.IncludeVarAndRename('m_bUpdateClientAnimations', 'updateclientanimations')
        else:
            cls.mem_funs('GetLastTakeDamageTime').exclude()
                 
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
        cls_name = 'CFuncUnit' if self.isserver else 'C_FuncUnit'
        cls = mb.class_(cls_name)
        cls.no_init = False
        self.ParseUnitBaseShared(mb, cls_name)
        if self.isclient:
            cls.vars('m_iMaxHealth').rename('maxhealth')
            
    def ParseWarsWeapon(self, mb):
        cls = mb.class_('C_WarsWeapon' if self.isclient else 'CWarsWeapon')
        cls.mem_funs( 'GetShootOriginAndDirection' ).add_transformation( FT.output('vShootOrigin'), FT.output('vShootDirection') )
        self.IncludeVarAndRename('m_fFireRate', 'firerate')
        self.IncludeVarAndRename('m_vBulletSpread', 'bulletspread')
        self.IncludeVarAndRename('m_fOverrideAmmoDamage', 'overrideammodamage')
        self.IncludeVarAndRename('m_fMaxBulletRange', 'maxbulletrange')
        self.IncludeVarAndRename('m_iMinBurst', 'minburst')
        self.IncludeVarAndRename('m_iMaxBurst', 'maxburst')
        self.IncludeVarAndRename('m_fMinRestTime', 'minresttime')
        self.IncludeVarAndRename('m_fMaxRestTime', 'maxresttime')
        self.IncludeVarAndRename('m_bEnableBurst', 'enableburst')
        self.IncludeVarAndRename('m_nBurstShotsRemaining', 'burstshotsremaining')
        
        if self.isclient:
            self.IncludeVarAndRename('m_vTracerColor', 'tracercolor')
        
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
        
    def ParseEntities(self, mb):
        self.ParseBaseEntityHandles(mb)
        
        self.IncludeEmptyClass(mb, 'IHandleEntity')
        self.AddEntityConverter(mb, 'IHandleEntity', True)
        
        mb.free_functions('CreateEntityByName').include()
        
        self.entclasses = []
        if self.isclient:
            self.ParseClientEntities(mb)
        else:
            self.ParseServerEntities(mb)
        
        self.ParseBaseEntity(mb)
        self.ParseBaseAnimating(mb)
        self.ParseBaseAnimatingOverlay(mb)
        self.ParseBaseFlex(mb)
        self.ParseBaseCombatWeapon(mb)
        self.ParseBaseCombatCharacter(mb)
        self.ParseBasePlayer(mb)
        self.ParseTriggers(mb)
        self.ParseProps(mb)
        self.ParseRemainingEntities(mb)
        
        self.ParseWars(mb)
        self.ParseHL2WarsPlayer(mb)
        self.ParseUnitBase(mb)
        self.ParseWarsWeapon(mb)

    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        self.ParseEntities(mb)
        
        # Protected functions we do want:
        if self.isserver:
            mb.mem_funs('TraceAttack').include()
            mb.mem_funs('PassesFilterImpl').include()
            mb.mem_funs('PassesDamageFilterImpl').include()
        
        # Finally apply common rules to all includes functions and classes, etc.
        self.ApplyCommonRules(mb)
        
