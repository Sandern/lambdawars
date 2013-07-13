from generate_mods_helper import GenerateModulePureShared
from pyplusplus import code_creators
from pyplusplus import function_transformers as FT
from src_helper import *
from pyplusplus.module_builder import call_policies

class SrcBuiltins(GenerateModulePureShared):
    module_name = 'srcbuiltins'
    
    files = [
        #'cbase.h',
        #'tier0\dbg.h',
        
        'srcpy_srcbuiltins.h',
    ]
    
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()      
        
        mb.free_function('SrcPyMsg').include()
        mb.free_function('SrcPyWarning').include()
        mb.free_function('SrcPyDevMsg').include()

        mb.class_('SrcPyStdOut').include()
        mb.class_('SrcPyStdErr').include()