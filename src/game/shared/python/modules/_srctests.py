from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies

class _SrcTests(SemiSharedModuleGenerator):
    module_name = '_srctests'

    files = [
        'cbase.h',
        'srcpy_tests.h'
    ]
    
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude() 

        mb.free_function('SrcPyTest_EntityArg').include()
        mb.free_function('SrcPyTest_NCrossProducts').include()