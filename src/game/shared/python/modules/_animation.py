from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies

class Animation(SemiSharedModuleGenerator):
    module_name = '_animation'
    
    files = [
        'bone_setup.h',
        'eventlist.h',
        'animation.h',
        'ai_activity.h',
        'activitylist.h',
        'npcevent.h',
        'srcpy_animation.h',
    ]
    
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        # CStudioHdr
        cls = mb.class_('CStudioHdr')
        cls.include()
        cls.calldefs('CStudioHdr').exclude()
        cls.no_init = True
        cls.mem_funs('pszName').rename('name')
        
        cls.mem_fun('pSeqdesc').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        cls.mem_fun('pSeqdesc').rename('Seqdesc')
        
        # Excludes
        cls.mem_fun('Init').exclude()
        if self.settings.branch == 'swarm':
            cls.mem_fun('FindMapping').exclude()
        cls.mem_fun('GetSequences').exclude()
        cls.mem_fun('BoneFlexDriver').exclude()
        cls.mem_fun('GetBoneTableSortedByName').exclude()
        cls.mem_fun('GetRenderHdr').exclude()
        cls.mem_fun('GetVirtualModel').exclude()
        cls.mem_fun('pAnimStudioHdr').exclude()
        cls.mem_fun('pAnimdesc').exclude()
        cls.mem_fun('pBodypart').exclude()
        cls.mem_fun('pBone').exclude()
        cls.mem_fun('pBonecontroller').exclude()
        cls.mem_fun('pFlexRule').exclude()
        cls.mem_fun('pFlexcontroller').exclude()
        cls.mem_fun('pFlexcontrollerUI').exclude()
        cls.mem_fun('pFlexdesc').exclude()
        cls.mem_fun('pHitbox').exclude()
        cls.mem_fun('pHitboxSet').exclude()
        cls.mem_fun('pIKChain').exclude()
        cls.mem_fun('pLinearBones').exclude()
        cls.mem_fun('pSeqStudioHdr').exclude()
        #cls.mem_fun('pSeqdesc').exclude()
        cls.mem_fun('pszNodeName').exclude()
        cls.mem_fun('pszSurfaceProp').exclude()
        cls.mem_fun('SetSequenceActivity').exclude() # Declared, but no def
        cls.mem_fun('IsSequenceLooping').exclude() # Declared, but no def
        cls.mem_fun('GetSequenceCycleRate').exclude() # Declared, but no def
        cls.mem_fun('GetSequenceActivity').exclude() # Declared, but no def
        
        # mstudioseqdesc_t
        cls = mb.class_('mstudioseqdesc_t')
        cls.include()
        cls.no_init = True
        
        # Excludes
        cls.mem_fun('GetBaseMap').exclude()
        cls.var('m_DataMap').exclude()
        if self.settings.branch == 'swarm':
            cls.mem_fun('pActivityModifier').exclude()
        cls.mem_fun('pAutolayer').exclude()
        cls.mem_fun('pBoneweight').exclude()
        cls.mem_fun('pEvent').exclude()
        cls.mem_fun('pIKLock').exclude()
        cls.mem_fun('pPoseKey').exclude()
        cls.mem_fun('pStudiohdr').exclude()
        cls.mem_fun('pszActivityName').exclude()
        cls.mem_fun('pszLabel').exclude()
        
        mb.free_function('Py_GetSeqdescActivityName').include()
        mb.free_function('Py_GetSeqdescActivityName').rename('GetSeqdescActivityName')
        mb.free_function('Py_GetSeqdescLabel').include()
        mb.free_function('Py_GetSeqdescLabel').rename('GetSeqdescLabel')
        
        # Useful bone_setup functions
        mb.free_function('Py_Studio_GetMass').include()
        mb.free_function('Py_Studio_GetMass').rename('Studio_GetMass')
        
        # Event list
        mb.free_function('EventList_RegisterPrivateEvent').include()
        mb.free_function('EventList_IndexForName').include()
        mb.free_function('EventList_NameForIndex').include()
        mb.free_function('ResetEventIndexes').include()
        
        # Animation
        mb.free_function('BuildAllAnimationEventIndexes').include()
        mb.free_function('LookupActivity').include()
        mb.free_function('LookupSequence').include()
        mb.free_function('GetSequenceName').include()
        mb.free_function('GetSequenceActivityName').include()
        mb.free_function('GetSequenceFlags').include()
        mb.free_function('GetAnimationEvent').include()
        mb.free_function('HasAnimationEventOfType').include()
        
        # Activity
        mb.free_function('ActivityList_RegisterPrivateActivity').include()
        mb.free_function('ActivityList_IndexForName').include()
        mb.free_function('ActivityList_NameForIndex').include()
        mb.free_function('IndexModelSequences').include()
        mb.free_function('ResetActivityIndexes').include()
        mb.free_function('VerifySequenceIndex').include()
        
        # Enums
        mb.enum('Animevent').include()