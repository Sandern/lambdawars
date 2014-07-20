from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies
from pyplusplus import function_transformers as FT

class NavMesh(SemiSharedModuleGenerator):
    module_name = '_navmesh'

    files = [
        'cbase.h',
        'srcpy_navmesh.h',
        'unit_base_shared.h',
    ]
 
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
        mb.free_function('RandomNavAreaPositionWithin').include()
        mb.free_function('DestroyAllNavAreas').include()
        
        mb.free_function('GetActiveNavMesh').include()
        mb.free_function('GetEditingCursor').include()
        
        mb.free_function('GetNavAreaAt').include()
        mb.free_function('GetNavAreasAtBB').include()
        mb.free_function('SplitAreasAtBB').include()
        mb.free_function('SetAreasBlocked').include()
        mb.free_function('IsAreaBlocked').include()
        mb.free_function('IsBBCoveredByNavAreas').include()
        
        mb.free_function('TryMergeSurrounding').include()
        
        mb.free_function('GetHidingSpotsInRadius').include()
        mb.free_function('CreateHidingSpot').include()
        mb.free_function('CreateHidingSpot').add_transformation( FT.output('navareaid') )
        mb.free_function('DestroyHidingSpot').include()
        mb.free_function('DestroyHidingSpotByID').include()
        