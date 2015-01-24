from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies
#from pyplusplus import function_transformers as FT

class Recast(SemiSharedModuleGenerator):
    module_name = '_recast'

    files = [
        'cbase.h',
        'recast/recast_mgr.h',
        #'unit_base_shared.h',
    ]
 
    def Parse(self, mb):
        mb.decls().exclude() 
        
        cls = mb.class_('CRecastMgr')
        cls.include()
        cls.no_init = True
        cls.calldefs().exclude()
        cls.mem_funs().virtuality = 'not virtual'
        
        cls.mem_fun('AddEntRadiusObstacle').include()
        cls.mem_fun('AddEntBoxObstacle').include()
        cls.mem_fun('RemoveEntObstacles').include()
        
        mb.free_function('RecastMgr').include()
        mb.free_function('RecastMgr').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        