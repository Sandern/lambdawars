from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies
#from pyplusplus import function_transformers as FT
from pygccxml.declarations import matchers, pointer_t, declarated_t

class Recast(SemiSharedModuleGenerator):
    module_name = '_recast'

    files = [
        'cbase.h',
        'recast/recast_mgr.h',
        'recast/recast_mesh.h',
        '#recast/recast_mapmesh.h',
        #'unit_base_shared.h',
    ]
 
    def Parse(self, mb):
        mb.decls().exclude() 
        
        meshcls = mb.class_('CRecastMesh')
        meshcls.include()
        meshcls.no_init = True
        meshcls.calldefs().exclude()
        meshcls.mem_funs().virtuality = 'not virtual'
        
        self.AddProperty(meshcls, 'agentradius', 'GetAgentRadius')
        self.AddProperty(meshcls, 'agentheight', 'GetAgentHeight')
        
        self.AddProperty(meshcls, 'cellsize', 'GetCellSize', 'SetCellSize')
        self.AddProperty(meshcls, 'cellheight', 'GetCellHeight', 'SetCellHeight')
        self.AddProperty(meshcls, 'tilesize', 'GetTileSize', 'SetTileSize')
        
        cls = mb.class_('CRecastMgr')
        cls.include()
        cls.no_init = True
        cls.calldefs().exclude()
        cls.mem_funs().virtuality = 'not virtual'
        
        if self.isserver:
            cls.mem_fun('Build').include()
            cls.mem_fun('Save').include()
            cls.mem_fun('RebuildPartial').include()
        
        cls.mem_fun('AddEntRadiusObstacle').include()
        cls.mem_fun('AddEntBoxObstacle').include()
        cls.mem_fun('RemoveEntObstacles').include()
        
        cls.mem_funs('GetMesh').include()
        cls.mem_funs('GetMesh', matchers.calldef_matcher_t(return_type=pointer_t(declarated_t(meshcls)))).call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        
        mb.free_function('RecastMgr').include()
        mb.free_function('RecastMgr').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        
        if self.isserver:
            cls = mb.class_('CMapMesh')
            cls.include()
            cls.no_init = True
            cls.calldefs().exclude()
            cls.mem_funs().virtuality = 'not virtual'
            
            cls.mem_fun('AddEntity').include()
        
        