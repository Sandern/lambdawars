from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies
from pygccxml.declarations import matchers
from pyplusplus import code_creators

class EntitiesMisc(SemiSharedModuleGenerator):
    module_name = '_entitiesmisc'
    
    files = [
        'cbase.h',
        'takedamageinfo.h',
        '$cliententitylist.h',
        '$soundinfo.h',
        
        '#mathlib/vmatrix.h', 
        '#utlvector.h', 
        '#shareddefs.h', 
        '#util.h',
        
        '#networkvar.h',
        '#eventqueue.h',
        '#entitylist.h',
        '#takedamageinfo.h',
        '#srcpy_networkvar.h',
        '#soundent.h',
        '#entityoutput.h',
        '#SkyCamera.h',
        '#world.h',
        '#globals.h',
        '#physics_prop_ragdoll.h',

        '#srcpy_srcbuiltins.h',
        
        'shared_classnames.h',
        'npcevent.h',
        'studio.h',
        'srcpy_entities.h',
        'isaverestore.h',
        'saverestore.h',
        'mapentities_shared.h',
        'vcollide_parse.h',
        'props_shared.h',
        'beam_shared.h',
        'shot_manipulator.h',
        
        'imouse.h',
        '$c_hl2wars_player.h',
        '#hl2wars_player.h',
    ]
        
    def ParseClientEntityRelated(self, mb):    
        # Creating a client class
        cls =  mb.class_('NetworkedClass')
        cls.include()
        cls.vars('m_pClientClass').exclude()
        
        cls = self.IncludeEmptyClass(mb, 'ClientClass')
        cls.no_init = True
        cls.calldefs('ClientClass').exclude()
        cls.mem_fun('GetName').include() # For debugging purposes
        
        # Client Entity List
        cls = mb.class_('CClientEntityList')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
        cls.no_init = True

        cls.mem_funs().exclude()
        
        cls.mem_fun('NumberOfEntities').include()
        cls.mem_fun('GetHighestEntityIndex').include()
        cls.mem_fun('GetBaseEntity').include()
        cls.mem_fun('GetBaseEntity').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_fun('GetBaseEntityFromHandle').include()
        cls.mem_fun('GetBaseEntityFromHandle').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_fun('EntIndexToHandle').include()
        cls.mem_fun('HandleToEntIndex').include()
        cls.mem_fun('IsHandleValid').include()
        cls.mem_fun('FirstBaseEntity').include()
        cls.mem_fun('FirstBaseEntity').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_fun('NextBaseEntity').include()
        cls.mem_fun('NextBaseEntity').call_policies = call_policies.return_value_policy(call_policies.return_by_value)

        mb.free_function('ClientEntityList').include()
        mb.free_function('ClientEntityList').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object )
        
        # PyEntityFactory    
        self.IncludeEmptyClass(mb, 'PyEntityFactory')
        mb.class_('PyEntityFactory').rename('EntityFactory')
        mb.free_function('PyGetClassByClassname').include()
        mb.free_function('PyGetClassByClassname').rename('GetClassByClassname')
        mb.free_function('PyGetAllClassnames').include()
        mb.free_function('PyGetAllClassnames').rename('GetAllClassnames')
        if self.isserver or self.settings.branch == 'swarm':
            mb.free_function('PyReadDataDesc').include()
            mb.free_function('PyReadDataDesc').rename('ReadDataDesc')
        
        # Client only structs
        cls = mb.class_('SpatializationInfo_t')
        cls.include()
        cls.var('pOrigin').exclude()
        cls.var('pAngles').exclude()
        cls.var('pflRadius').exclude()
        
    def ParseServerEntityRelated(self, mb):
        # PyEntityFactory    
        self.IncludeEmptyClass(mb, 'PyEntityFactory')
        mb.class_('PyEntityFactory').rename('EntityFactory')
        mb.free_function('PyGetClassByClassname').include()
        mb.free_function('PyGetClassByClassname').rename('GetClassByClassname')
        mb.free_function('PyGetAllClassnames').include()
        mb.free_function('PyGetAllClassnames').rename('GetAllClassnames')
        
        # Creating a server class
        cls = mb.class_('NetworkedClass')
        cls.include()
        cls.vars('m_pServerClass').exclude()
        
        cls = self.IncludeEmptyClass(mb, 'ServerClass')
        cls.no_init = True
        cls.calldefs('ServerClass').exclude()
        cls.mem_fun('GetName').include() # For debugging purposes

        # Creating a networked variable
        cls = mb.class_('CPythonNetworkVarBase')
        cls.include()
        cls.no_init = True
        cls.calldefs().exclude()
        cls.vars().exclude()
        cls.mem_fun('NetworkStateChanged').include()
        
        cls = mb.class_('CPythonNetworkVar')
        cls.include()
        cls.rename('NetworkVarInternal') 
        cls.mem_funs('Str').rename('__str__')
        cls.mem_funs('NetworkVarsUpdateClient').exclude()

        cls = mb.class_('CPythonNetworkArray')
        cls.include()
        cls.rename('NetworkArrayInternal') 
        cls.mem_funs('NetworkVarsUpdateClient').exclude()
        cls.mem_funs('GetItem').rename('__getitem__')
        cls.mem_funs('SetItem').rename('__setitem__')
        cls.mem_funs('DelItem').rename('__delitem__')
        cls.mem_funs('Str').rename('__str__')
        
        cls = mb.class_('CPythonNetworkDict')
        cls.include()
        cls.rename('NetworkDictInternal') 
        cls.mem_funs('NetworkVarsUpdateClient').exclude()
        cls.mem_funs('GetItem').rename('__getitem__')
        cls.mem_funs('SetItem').rename('__setitem__')
        cls.mem_funs('Str').rename('__str__')
        
        # Send proxies for python network variables
        cls = mb.class_('CPythonSendProxyBase')
        cls.rename('SendProxyBase') 
        cls.include()
        
        # Ugly globals
        mb.add_registration_code( "bp::scope().attr( \"g_vecAttackDir\" ) = boost::ref(g_vecAttackDir);" )
    
        # Event queue
        cls = mb.class_('CEventQueue')
        cls.include()
        mb.vars('g_EventQueue').include()
        cls.mem_funs('Init').exclude()
        cls.mem_funs('Restore').exclude()
        cls.mem_funs('Save').exclude()
        cls.mem_funs('ValidateQueue').exclude()
        cls.mem_funs('GetBaseMap').exclude()    
        
        # Global entity list
        cls = mb.class_('IEntityFindFilter')
        cls.include()
        cls.mem_funs('GetFilterResult').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        
        cls = mb.class_('CGlobalEntityList')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
        cls.no_init = True
        cls.mem_funs('GetServerNetworkable').exclude()
        cls.mem_funs('GetBaseNetworkable').exclude()
        cls.mem_funs('GetBaseEntity').exclude()
        cls.mem_funs('GetEdict').exclude()    
        cls.mem_funs('AddToDeleteList').exclude()
        #cls.mem_funs('CleanupDeleteList').exclude() # Keep
        #cls.mem_funs('ResetDeleteList').exclude() # Keep
        cls.mem_funs('Clear').exclude()
        cls.mem_funs('NotifyCreateEntity').exclude()
        cls.mem_funs('NotifySpawn').exclude()
        cls.mem_funs('NotifyRemoveEntity').exclude()
        cls.mem_funs('AddListenerEntity').exclude() # Would require some kind of python version
        cls.mem_funs('RemoveListenerEntity').exclude()
        
        cls.mem_funs('NextEnt').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FirstEnt').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FindEntityByClassname').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FindEntityByName').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FindEntityInSphere').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FindEntityByTarget').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FindEntityByModel').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FindEntityByNameNearest').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FindEntityByNameWithin').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FindEntityByClassnameNearest').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FindEntityByClassnameWithin').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FindEntityGeneric').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FindEntityGenericWithin').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FindEntityGenericNearest').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FindEntityNearestFacing').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FindEntityClassNearestFacing').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('FindEntityProcedural').call_policies = call_policies.return_value_policy(call_policies.return_by_value)

        if self.settings.branch == 'swarm':
            cls.mem_funs('FindEntityByClassnameFast').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
            cls.mem_funs('FindEntityByClassnameNearest2D').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
            cls.mem_funs('FindEntityByClassnameNearestFast').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
            cls.mem_funs('FindEntityByNameFast').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
            cls.mem_funs('FindEntityByOutputTarget').call_policies = call_policies.return_value_policy(call_policies.return_by_value)

        mb.vars('gEntList').include()
        mb.vars('gEntList').rename('entlist')
        
        # Respawning the player using a python entity class
        mb.free_function('PyRespawnPlayer').include()
        mb.free_function('PyRespawnPlayer').rename('RespawnPlayer')
        
        # Getting the world
        mb.free_function('PyGetWorldEntity').include()
        mb.free_function('PyGetWorldEntity').rename('GetWorldEntity')
        
        # Entity input/output
        cls = mb.class_('CBaseEntityOutput')
        cls.include()
        cls.mem_funs('GetBaseMap').exclude()
        cls.mem_funs('Save').exclude()
        cls.mem_funs('Restore').exclude()
        
        if self.settings.branch == 'swarm':
            cls.mem_funs('GetActionForTarget').exclude()
            cls.mem_funs('GetFirstAction').exclude()
            
        # Note: PyOutputEvent covers all the other types
        # There is no need for COutputInt, COutputFloat, etc.
        cls = mb.class_('PyOutputEvent')
        cls.include()
        cls.rename('COutputEvent') 
        
        # Inputdata_t and variant_t
        cls = mb.class_('inputdata_t')
        cls.include()
        cls.var('pActivator').getter_call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.var('pCaller').getter_call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.var('nOutputID').rename('outputid')
        cls.var('pActivator').rename('activator')
        cls.var('pCaller').rename('caller')
        
        cls = mb.class_('variant_t')
        cls.include()
        cls.vars( lambda decl: 'm_Save' in decl.name ).exclude()
        cls.vars('vecVal').exclude()
        cls.mem_fun('Entity').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        
        cls.mem_fun('Entity').exclude()
        cls.add_wrapper_code(   'static bp::object PyEntity( variant_t &inst )\r\n' + \
                                '{\r\n' + \
                                '   return inst.Entity() ? inst.Entity()->GetPyHandle() : bp::object();\r\n' + \
                                '}\r\n' )
        cls.add_registration_code( 'def("Entity", &::variant_t_wrapper::PyEntity)')
        
        # Collision
        cls = mb.class_('vcollisionevent_t')
        cls.include()
        cls.vars('pObjects').exclude()
        
        # Wrap internal data functions
        cls.vars('pInternalData').exclude()
        cls.add_wrapper_code(   'static void GetSurfaceNormal( vcollisionevent_t &inst, Vector &out )\r\n' + \
                                '{\r\n' + \
                                '   inst.pInternalData->GetSurfaceNormal(out);\r\n' + \
                                '}\r\n' )
        cls.add_registration_code( 'def("GetSurfaceNormal", &::vcollisionevent_t_wrapper::GetSurfaceNormal)')
        cls.add_wrapper_code(   'static void GetContactPoint( vcollisionevent_t &inst, Vector &out )\r\n' + \
                                '{\r\n' + \
                                '   inst.pInternalData->GetContactPoint(out);\r\n' + \
                                '}\r\n' )
        cls.add_registration_code( 'def("GetContactPoint", &::vcollisionevent_t_wrapper::GetContactPoint)')
        cls.add_wrapper_code(   'static void GetContactSpeed( vcollisionevent_t &inst, Vector &out )\r\n' + \
                                '{\r\n' + \
                                '   inst.pInternalData->GetContactSpeed(out);\r\n' + \
                                '}\r\n' )
        cls.add_registration_code( 'def("GetContactSpeed", &::vcollisionevent_t_wrapper::GetContactSpeed)')
        
        cls = mb.class_('gamevcollisionevent_t')
        cls.include()
        cls.mem_funs('Init').exclude()
        cls.vars('pEntities').exclude()
        
        cls.add_wrapper_code(   'static bp::object GetEnt( gamevcollisionevent_t &inst, int index )\r\n' + \
                                '{\r\n' + \
                                '   if( index < 0 || index > 1 )\r\n' + \
                                '       return bp::object();\r\n' + \
                                '   return inst.pEntities[index] ? inst.pEntities[index]->GetPyHandle() : bp::object();\r\n' + \
                                '}\r\n' )
        cls.add_registration_code( 'def("GetEnt", &::gamevcollisionevent_t_wrapper::GetEnt)')
            
    def ParseMisc(self, mb):
        # CUserCmd, player commands
        cls = mb.class_('CUserCmd')
        cls.include()
        cls.calldefs().exclude()
        self.PythonfyVariables(cls)
    
        # Save/restore interfaces. Just empty.
        cls = mb.class_('IRestore')
        cls.include()
        cls.calldefs().exclude()
        cls = mb.class_('ISave')
        cls.include()
        cls.calldefs().exclude()
    
        if self.isserver:
            # Sky camera
            mb.free_function('GetCurrentSkyCamera').include()
            mb.free_function('GetCurrentSkyCamera').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        
        # CTakeDamageInfo
        cls = mb.class_('CTakeDamageInfo')
        cls.include()
        cls.calldefs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
        
        cls.mem_funs('GetInflictor').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        cls.mem_funs('GetWeapon').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        cls.mem_funs('GetAttacker').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        
        self.SetupProperty(cls, 'attributes', 'GetAttributes', 'SetAttributes')
        
        cls = mb.class_('CMultiDamage')
        cls.include()
        cls.calldefs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
        cls.mem_funs('GetTarget').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        
        mb.free_functions('ClearMultiDamage').include()
        mb.free_functions('ApplyMultiDamage').include()
        mb.free_functions('AddMultiDamage').include()
        mb.free_functions('ImpulseScale').include()
        mb.free_functions('CalculateExplosiveDamageForce').include()
        mb.free_functions('CalculateBulletDamageForce').include()
        mb.free_functions('CalculateMeleeDamageForce').include()
        mb.free_functions('GuessDamageForce').include()
        
        # FireBulletsInfo_t
        cls = mb.class_('FireBulletsInfo_t')
        cls.include()
        cls.var('m_iShots').rename('shots')
        cls.var('m_vecSrc').rename('vecsrc')
        cls.var('m_vecDirShooting').rename('vecdirshooting')
        cls.var('m_vecSpread').rename('vecspread')
        cls.var('m_flDistance').rename('distance')
        cls.var('m_iAmmoType').rename('ammotype')
        cls.var('m_iTracerFreq').rename('tracerfreq')
        cls.var('m_nFlags').rename('flags')
        cls.var('m_flDamageForceScale').rename('damageforcescale')
        cls.var('m_pAttacker').rename('attacker')
        cls.var('m_pAdditionalIgnoreEnt').rename('additionalignoreent')
        cls.var('m_bPrimaryAttack').rename('primaryattack')
        cls.var('m_flDamage').rename('damage')
        cls.var('m_iPlayerDamage' if self.settings.branch == 'source2013' else 'm_flPlayerDamage').rename('playerdamage')
        cls.var('m_Attributes').rename('attributes')
        
        mb.enum('FireBulletsFlags_t').include()
        
        # CShotManipulator
        cls = mb.class_('CShotManipulator')
        cls.include()
        
        if self.isserver:
            # Bone follower
            cls = mb.class_('pyphysfollower_t')
            cls.include()
            cls.rename('physfollower')
            
            cls = mb.class_('PyBoneFollowerManager')
            cls.include()
            cls.rename('BoneFollowerManager')
            
            mb.free_function('GetAttachmentPositionInSpaceOfBone').include()
            
            # Ragdoll functions
            mb.free_function('CreateServerRagdoll').include()
            mb.free_function('CreateServerRagdoll').call_policies = call_policies.return_value_policy(call_policies.return_by_value)

            if self.settings.branch == 'swarm':
                mb.free_function('PyCreateServerRagdollAttached').include()
                mb.free_function('PyCreateServerRagdollAttached').rename('CreateServerRagdollAttached')
                mb.free_function('PyCreateServerRagdollAttached').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
            mb.free_function('DetachAttachedRagdoll').include()
            mb.free_function('DetachAttachedRagdollsForEntity').include()
        
        # Shared Props
        mb.class_('breakablepropparams_t').include()
        mb.free_functions('GetMassEquivalent').include()
        mb.free_functions('GetAutoMultiplayerPhysicsMode').include()
        mb.free_functions('PropBreakableCreateAll').include()
        mb.free_functions('PrecacheGibsForModel').include()
    
        # Enums
        mb.enums('MoveType_t').include()
        mb.enums('MoveCollide_t').include()
        mb.enums('SolidType_t').include()
        mb.enums('SolidFlags_t').include()
        mb.enums('Collision_Group_t').include()
        mb.enums('RenderMode_t').include()
        mb.enums('RenderFx_t').include()
        
        if self.isclient:
            mb.enums('ShadowType_t').include()
            mb.enums('RenderGroup_t').include()
            mb.enums('DataUpdateType_t').include()
            mb.enums('CollideType_t').include()
            mb.enums('ShouldTransmitState_t').include()
            mb.enums('OverrideType_t').include()
        else:
            mb.enums('EntityEvent_t').include()
            mb.enums('Class_T').include()
        mb.enums('_fieldtypes').include()
            
        mb.enums('USE_TYPE').include()
        mb.enums('Activity').include()
        
        # Anim event
        cls = mb.class_('animevent_t')
        cls.include()
        
        cls.add_wrapper_code(   'static bp::object get_options(animevent_t const & inst){\r\n' + \
                                '    return bp::object(inst.options);\r\n' + \
                                '}\r\n'
                            )
                            
        cls.add_registration_code( 'add_property("options"\r\n' + \
                                                "    , bp::make_function( (bp::object (*)( ::animevent_t & ))(&animevent_t_wrapper::get_options) ) )\r\n"
                                              )
        
        # Solid_t class
        mb.classes('solid_t').include()
        
        # Types/Contants
        if self.isserver:
            mb.add_registration_code( "bp::scope().attr( \"SOUND_NONE\" ) = (int)SOUND_NONE;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_COMBAT\" ) = (int)SOUND_COMBAT;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_WORLD\" ) = (int)SOUND_WORLD;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_PLAYER\" ) = (int)SOUND_PLAYER;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_DANGER\" ) = (int)SOUND_DANGER;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_BULLET_IMPACT\" ) = (int)SOUND_BULLET_IMPACT;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_CARCASS\" ) = (int)SOUND_CARCASS;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_GARBAGE\" ) = (int)SOUND_GARBAGE;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_THUMPER\" ) = (int)SOUND_THUMPER;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_BUGBAIT\" ) = (int)SOUND_BUGBAIT;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_PHYSICS_DANGER\" ) = (int)SOUND_PHYSICS_DANGER;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_DANGER_SNIPERONLY\" ) = (int)SOUND_DANGER_SNIPERONLY;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_MOVE_AWAY\" ) = (int)SOUND_MOVE_AWAY;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_PLAYER_VEHICLE\" ) = (int)SOUND_PLAYER_VEHICLE;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_READINESS_LOW\" ) = (int)SOUND_READINESS_LOW;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_READINESS_MEDIUM\" ) = (int)SOUND_READINESS_MEDIUM;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_READINESS_HIGH\" ) = (int)SOUND_READINESS_HIGH;" )
            
            mb.add_registration_code( "bp::scope().attr( \"SOUND_CONTEXT_FROM_SNIPER\" ) = (int)SOUND_CONTEXT_FROM_SNIPER;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_CONTEXT_GUNFIRE\" ) = (int)SOUND_CONTEXT_GUNFIRE;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_CONTEXT_MORTAR\" ) = (int)SOUND_CONTEXT_MORTAR;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_CONTEXT_COMBINE_ONLY\" ) = (int)SOUND_CONTEXT_COMBINE_ONLY;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_CONTEXT_REACT_TO_SOURCE\" ) = (int)SOUND_CONTEXT_REACT_TO_SOURCE;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_CONTEXT_EXPLOSION\" ) = (int)SOUND_CONTEXT_EXPLOSION;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_CONTEXT_EXCLUDE_COMBINE\" ) = (int)SOUND_CONTEXT_EXCLUDE_COMBINE;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_CONTEXT_DANGER_APPROACH\" ) = (int)SOUND_CONTEXT_DANGER_APPROACH;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_CONTEXT_ALLIES_ONLY\" ) = (int)SOUND_CONTEXT_ALLIES_ONLY;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUND_CONTEXT_PLAYER_VEHICLE\" ) = (int)SOUND_CONTEXT_PLAYER_VEHICLE;" )
            
            mb.add_registration_code( "bp::scope().attr( \"ALL_CONTEXTS\" ) = (int)ALL_CONTEXTS;" )
            mb.add_registration_code( "bp::scope().attr( \"ALL_SCENTS\" ) = (int)ALL_SCENTS;" )
            mb.add_registration_code( "bp::scope().attr( \"ALL_SOUNDS\" ) = (int)ALL_SOUNDS;" )
            
            # Channels
            mb.add_registration_code( "bp::scope().attr( \"SOUNDENT_CHANNEL_UNSPECIFIED\" ) = (int)SOUNDENT_CHANNEL_UNSPECIFIED;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUNDENT_CHANNEL_REPEATING\" ) = (int)SOUNDENT_CHANNEL_REPEATING;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUNDENT_CHANNEL_REPEATED_DANGER\" ) = (int)SOUNDENT_CHANNEL_REPEATED_DANGER;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUNDENT_CHANNEL_REPEATED_PHYSICS_DANGER\" ) = (int)SOUNDENT_CHANNEL_REPEATED_PHYSICS_DANGER;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUNDENT_CHANNEL_WEAPON\" ) = (int)SOUNDENT_CHANNEL_WEAPON;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUNDENT_CHANNEL_INJURY\" ) = (int)SOUNDENT_CHANNEL_INJURY;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUNDENT_CHANNEL_BULLET_IMPACT\" ) = (int)SOUNDENT_CHANNEL_BULLET_IMPACT;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUNDENT_CHANNEL_NPC_FOOTSTEP\" ) = (int)SOUNDENT_CHANNEL_NPC_FOOTSTEP;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUNDENT_CHANNEL_SPOOKY_NOISE\" ) = (int)SOUNDENT_CHANNEL_SPOOKY_NOISE;" )
            mb.add_registration_code( "bp::scope().attr( \"SOUNDENT_CHANNEL_ZOMBINE_GRENADE\" ) = (int)SOUNDENT_CHANNEL_ZOMBINE_GRENADE;" )
            
            # Transmit state
            mb.add_registration_code( "bp::scope().attr( \"FL_EDICT_CHANGED\" ) = (int)FL_EDICT_CHANGED;" )
            mb.add_registration_code( "bp::scope().attr( \"FL_EDICT_FREE\" ) = (int)FL_EDICT_FREE;" )
            mb.add_registration_code( "bp::scope().attr( \"FL_EDICT_FULL\" ) = (int)FL_EDICT_FULL;" )
            mb.add_registration_code( "bp::scope().attr( \"FL_EDICT_FULLCHECK\" ) = (int)FL_EDICT_FULLCHECK;" )
            mb.add_registration_code( "bp::scope().attr( \"FL_EDICT_ALWAYS\" ) = (int)FL_EDICT_ALWAYS;" )
            mb.add_registration_code( "bp::scope().attr( \"FL_EDICT_DONTSEND\" ) = (int)FL_EDICT_DONTSEND;" )
            mb.add_registration_code( "bp::scope().attr( \"FL_EDICT_PVSCHECK\" ) = (int)FL_EDICT_PVSCHECK;" )
            mb.add_registration_code( "bp::scope().attr( \"FL_EDICT_PENDING_DORMANT_CHECK\" ) = (int)FL_EDICT_PENDING_DORMANT_CHECK;" )
            mb.add_registration_code( "bp::scope().attr( \"FL_EDICT_DIRTY_PVS_INFORMATION\" ) = (int)FL_EDICT_DIRTY_PVS_INFORMATION;" )
            mb.add_registration_code( "bp::scope().attr( \"FL_FULL_EDICT_CHANGED\" ) = (int)FL_FULL_EDICT_CHANGED;" )
    
    def ParseWars(self, mb):
        if self.isserver:
            # Send proxies
            cls = mb.class_('CPythonSendProxyOwnerOnly')
            cls.rename('SendProxyOwnerOnly') 
            cls.include()
            cls.calldefs().virtuality = 'not virtual'
            
            cls = mb.class_('CPythonSendProxyAlliesOnly')
            cls.rename('SendProxyAlliesOnly') 
            cls.include()
            cls.calldefs().virtuality = 'not virtual'
        
        # Enums
        mb.enums('WarsCollision_Group_t').include()
    
        # Mouse trace data (HL2Wars player)
        cls = mb.class_('MouseTraceData_t')
        cls.include()
        cls.rename('MouseTraceData')
        cls.vars('m_vStartPos').rename('startpos')
        cls.vars('m_vEndPos').rename('endpos')
        cls.vars('m_vNormal').rename('normal')
        cls.vars('m_hEnt').exclude()
        cls.mem_funs().exclude()
        cls.mem_funs('GetEnt').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_fun('Clear').include()
        cls.vars('m_vWorldOnlyEndPos').rename('groundendpos')
        cls.vars('m_vWorldOnlyNormal').rename('groundnormal')    
        
        self.SetupProperty(cls, 'ent', 'GetEnt', 'SetEnt')
                         
        if self.isclient:
            cls.vars('m_iX').rename('x')
            cls.vars('m_iY').rename('y')  
            
        # IMouse 
        mb.class_('IMouse').include()
        mb.class_('IMouse').rename('IMouseDoNotUse')
        mb.class_('IMouse').mem_funs('GetIMouse').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        mb.class_('PyMouse').include()
        mb.class_('PyMouse').rename('IMouse')
        mb.add_registration_code( "ptr_imouse_to_py_imouse();" )
    
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()

        if self.isclient:
            self.ParseClientEntityRelated(mb)
        else:
            self.ParseServerEntityRelated(mb)
        self.ParseMisc(mb)
        self.ParseWars(mb)
        
        # Finally apply common rules to all includes functions and classes, etc.
        self.ApplyCommonRules(mb)
        
    def AddAdditionalCode(self, mb):
        header = code_creators.include_t( 'srcpy_converters_ents.h' )
        mb.code_creator.adopt_include(header)
        super(EntitiesMisc, self).AddAdditionalCode(mb)