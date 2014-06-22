from srcpy.module_generators import ClientModuleGenerator

from pyplusplus.module_builder import call_policies
from pyplusplus import function_transformers as FT
from pyplusplus import code_creators
from pygccxml.declarations import matchers

class GameUI(ClientModuleGenerator):
    module_name = '_gameui'

    files = [
        'gameui/gameui_util.h',
        'videocfg/videocfg.h',
    ]

    def Parse(self, mb):
        mb.decls().exclude()
        
        # CGameUIConVarRef
        mb.class_('CGameUIConVarRef').include()
        
        mb.free_function('ReadCurrentVideoConfig').include()
        
        # Remove any protected function 
        mb.calldefs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
