import os

from . basesource import SourceModuleGenerator

class ServerModuleGenerator(SourceModuleGenerator):
    module_type = 'server'
    dll_name = 'Server'
    
    @property
    def path(self):
        return os.path.join(self.settings.srcpath, self.settings.server_path)