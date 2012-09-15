from generate_mods_helper import GenerateModuleSemiShared
from pyplusplus.module_builder import call_policies
import settings
#from src_helper import *

class NavMesh(GenerateModuleSemiShared):
    module_name = '_navmesh'
    
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
        'src_python_navmesh.h',
    ]
    
    def GetFiles(self):
        if self.isClient:
            return self.client_files + self.files 
        return self.files
        
    def Parse(self, mb):
        # Exclude everything, then add what we need
        # Otherwise we get very big source code and dll's
        mb.decls().exclude() 
        
        mb.free_function('NavMeshAvailable').include()
        mb.free_function('NavMeshTestHasArea').include()
        mb.free_function('NavMeshGetPathDistance').include()
        mb.free_function('NavMeshGetPositionNearestNavArea').include()
        mb.free_function('CreateNavArea').include()
        mb.free_function('CreateNavAreaByCorners').include()
        mb.free_function('DestroyNavArea').include()
        mb.free_function('RandomNavAreaPosition').include()
        mb.free_function('DestroyAllNavAreas').include()
        
        mb.free_function('GetNavAreaAt').include()
        mb.free_function('GetNavAreasAtBB').include()
        mb.free_function('SplitAreasAtBB').include()
        mb.free_function('SetAreasBlocked').include()
        mb.free_function('IsBBCoveredByNavAreas').include()
        
        mb.free_function('TryMergeSurrounding').include()
        
        mb.free_function('GetHidingSpotsInRadius').include()
