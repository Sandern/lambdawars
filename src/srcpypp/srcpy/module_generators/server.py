import os

from . basesource import SourceModuleGenerator

class ServerModuleGenerator(SourceModuleGenerator):
    module_type = 'server'
    dll_name = 'Server'
    isclient = False
    isserver = True
    
    @property
    def path(self):
        return os.path.join('../../', self.settings.server_path)