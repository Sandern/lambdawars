from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies
import settings

class FOW(SemiSharedModuleGenerator):
    module_name = '_fow'
    
    if settings.ASW_CODE_BASE:
        client_files = [
            'videocfg/videocfg.h',
        ]
    else:
        client_files = [
            'wchartypes.h',
            'shake.h',
        ]
        
    files = [
        'cbase.h',
        'hl2wars/fowmgr.h',
    ]
    
    def GetFiles(self):
        if self.isclient:
            return self.client_files + self.files 
        return self.files
        
    def Parse(self, mb):
        # Exclude everything, then add what we need
        # Otherwise we get very big source code and dll's
        mb.decls().exclude() 
        
        cls = mb.class_('CFogOfWarMgr')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        cls.no_init = True
        
        cls.mem_funs().exclude()
        
        cls.mem_fun('ComputeFOWPosition').include()
        cls.mem_fun('ComputeWorldPosition').include()
        cls.mem_fun('DebugPrintEntities').include()
        cls.mem_fun('ModifyHeightAtTile').include()
        cls.mem_fun('ModifyHeightAtPoint').include()
        cls.mem_fun('ModifyHeightAtExtent').include()
        cls.mem_fun('GetHeightAtTile').include()
        cls.mem_fun('GetHeightAtPoint').include()
        
        if self.isserver:
            cls.mem_fun('PointInFOW').include()
        if self.isclient:
            cls.mem_fun('ResetExplored').include()
                
        mb.free_function('FogOfWarMgr').include()
        mb.free_function('FogOfWarMgr').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object )
