from srcpy.module_generators import SharedModuleGenerator
from pyplusplus import code_creators, function_transformers as FT
from pyplusplus.module_builder import call_policies

class SrcFilesystem(SharedModuleGenerator):
    module_name = '_filesystem'
    
    files = [
        'filesystem.h',
        'srcpy_filesystem.h',
    ]
    
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        # All functions named 'PyFS_' are part of the _filessytem module
        # Just strip the "PyFS_' suffix and include the function
        for decl in mb.free_functions():
            if decl.name.startswith('PyFS_'):
                decl.include()
                decl.rename(decl.name.split('PyFS_')[1])
        

