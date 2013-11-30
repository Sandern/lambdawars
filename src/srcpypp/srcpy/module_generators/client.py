import os

from . basesource import SourceModuleGenerator

class ClientModuleGenerator(SourceModuleGenerator):
    module_type = 'client'
    dll_name = 'Client'
    isclient = True
    isserver = False
    
    @property
    def path(self):
        return os.path.join('../../', self.settings.client_path)

