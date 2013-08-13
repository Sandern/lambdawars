import os

from . basesource import SourceModuleGenerator
from .. src_module_builder import src_module_builder_t

class ClientModuleGenerator(SourceModuleGenerator):
    module_type = 'client'
    dll_name = 'Client'
    
    @property
    def path(self):
        return os.path.join(self.settings.srcpath, self.settings.client_path)
    
    # Create builder
    def CreateBuilder(self, files):
        return src_module_builder_t(files, is_client=True)    
    

    
