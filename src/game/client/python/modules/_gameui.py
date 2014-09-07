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
        
        'cbase.h',
        'srcpy_gameui.h',
    ]

    def Parse(self, mb):
        mb.decls().exclude()
        
        # CGameUIConVarRef
        mb.class_('CGameUIConVarRef').include()
        
        mb.free_function('ReadCurrentVideoConfig').include()
        
        # Sending gameui commands
        mb.enum('WINDOW_TYPE').include()
        
        mb.free_function('PyGameUICommand').include() # Temporary function
        mb.free_function('PyGameUICommand').rename('GameUICommand')
        mb.free_function('PyGameUIOpenWindow').include() # Temporary function
        mb.free_function('PyGameUIOpenWindow').rename('GameUIOpenWindow')
        
        mb.free_function('OpenGammaDialog').include()
        
        # Remove any protected function 
        mb.calldefs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
