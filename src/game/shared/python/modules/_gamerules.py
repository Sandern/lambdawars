from generate_mods_helper import GenerateModuleSemiShared

from pyplusplus.module_builder import call_policies
from pyplusplus import function_transformers as FT
from pygccxml.declarations import matchers

from src_helper import *

import settings

class GameRules(GenerateModuleSemiShared):
    module_name = '_gamerules'
    
    if settings.ASW_CODE_BASE:
        client_files = [
            'videocfg/videocfg.h',
            'cbase.h',
        ]
    else:
        client_files = [
            'wchartypes.h',
            'shake.h',
            'cbase.h',
        ]
    
    server_files = [
        'cbase.h',
        'items.h',
    ]
    
    files = [
        'gamerules.h',
        'multiplay_gamerules.h',
        'teamplay_gamerules.h',
        'hl2wars_gamerules.h',
        'src_python_gamerules.h',
        'ammodef.h',
    ]
    
    def GetFiles(self):
        if self.isClient:
            return self.client_files + self.files 
        return self.server_files + self.files 

    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        cls_name = 'CHL2WarsGameRules' if self.isServer else 'C_HL2WarsGameRules'
        cls = mb.class_(cls_name)
        cls.include()
        
        cls.mem_fun('ShouldCollide').virtuality = 'not virtual'
        cls.mem_fun('GetAmmoDamage').virtuality = 'not virtual' # Just modify the ammo table when needed...

        # Gamerules class
        if self.isServer:
            cls.mem_fun('GetIntermissionEndTime').exclude()
            cls.mem_fun('SetintermissionEndTime').exclude()
            cls.mem_fun('GetGameOver').exclude()
            cls.mem_fun('SetGameOver').exclude()
            cls.add_property( 'intermissionendtime'
                             , cls.member_function( 'GetIntermissionEndTime' )
                             , cls.member_function( 'SetintermissionEndTime' ) )
            cls.add_property( 'gameover'
                             , cls.member_function( 'GetGameOver' )
                             , cls.member_function( 'SetGameOver' ) )
                             
            # Call policies
            mb.mem_funs('GetPlayerSpawnSpot').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            
            mb.mem_funs('GetDeathScorer').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            mb.mem_funs('VoiceCommand').call_policies = call_policies.return_value_policy( call_policies.return_by_value )     
            
            # ..
            cls.mem_funs('GetPySelf').exclude()   
            cls.mem_fun('FrameUpdatePostEntityThink').exclude()
            cls.add_wrapper_code(
                'virtual PyObject *GetPySelf() const { return boost::python::detail::wrapper_base_::get_owner(*this); }'
            )
            
            # Exlude and replace
            cls.mem_fun('ClientConnected').exclude()
            cls.mem_fun('PyClientConnected').rename('ClientConnected')
            cls.mem_fun('ClientDisconnected').exclude()
            cls.mem_fun('PyClientDisconnected').exclude()
            cls.mem_fun('PyClientDisconnected').rename('ClientDisconnected')
            AddWrapReg( mb, 'CHL2WarsGameRules', GetTopDeclaration( mb, 'CHL2WarsGameRules', 'PyClientDisconnected' ), ['*client'] )
            cls.mem_fun('ClientActive').exclude()
            AddWrapReg( mb, 'CHL2WarsGameRules', GetTopDeclaration( mb, 'CHL2WarsGameRules', 'ClientActive' ), ['*client'] )
            
            cls.mem_fun('PlayerSpawn').exclude()
            cls.mem_fun('PlayerThink').exclude()
            cls.mem_fun('FPlayerCanRespawn').exclude()    
            cls.mem_fun('FlPlayerSpawnTime').exclude()
            cls.mem_fun('GetPlayerSpawnSpot').exclude()
            cls.mem_fun('IsSpawnPointValid').exclude()  
            
            AddWrapReg( mb, 'CHL2WarsGameRules', GetTopDeclaration( mb, 'CHL2WarsGameRules', 'PlayerSpawn' ), ['*player'] )
            AddWrapReg( mb, 'CHL2WarsGameRules', GetTopDeclaration( mb, 'CHL2WarsGameRules', 'PlayerThink' ), ['*player'] )
            AddWrapReg( mb, 'CHL2WarsGameRules', GetTopDeclaration( mb, 'CHL2WarsGameRules', 'FPlayerCanRespawn' ), ['*player'] )
            AddWrapReg( mb, 'CHL2WarsGameRules', GetTopDeclaration( mb, 'CHL2WarsGameRules', 'FlPlayerSpawnTime' ), ['*player'] )
            AddWrapReg( mb, 'CHL2WarsGameRules', GetTopDeclaration( mb, 'CHL2WarsGameRules', 'GetPlayerSpawnSpot' ), ['*player'] )
            AddWrapReg( mb, 'CHL2WarsGameRules', GetTopDeclaration( mb, 'CHL2WarsGameRules', 'IsSpawnPointValid' ), ['*spot', '*player'] )
            
            cls.mem_fun('ClientCommand').exclude()  
            cls.mem_fun('ClientSettingsChanged').exclude() 
            AddWrapReg( mb, 'CHL2WarsGameRules', GetTopDeclaration( mb, 'CHL2WarsGameRules', 'ClientCommand' ), ['*edict', 'args'] )
            AddWrapReg( mb, 'CHL2WarsGameRules', GetTopDeclaration( mb, 'CHL2WarsGameRules', 'ClientSettingsChanged' ), ['*player'] )
            
            cls.mem_fun('PlayerChangedOwnerNumber').exclude() 
            AddWrapReg( mb, 'CHL2WarsGameRules', GetTopDeclaration( mb, 'CHL2WarsGameRules', 'PlayerChangedOwnerNumber' ), ['*player', 'oldownernumber', 'newownernumber'] )

            # Excludes
            mb.mem_funs('VoiceCommand').exclude()
        else:
            cls.rename('CHL2WarsGameRules')
            
            
            
        # Temp excludes
        mb.mem_funs('GetEncryptionKey').exclude()
        mb.mem_funs('GetViewVectors').exclude()

        # Call policies
        mb.mem_funs('GetNextBestWeapon').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  
        
        # Hide instance
        mb.vars('m_pyInstance').exclude()
        
        mb.free_function('PyGameRules').include()
        mb.free_function('PyGameRules').rename('GameRules')
        
        # Installing the gamerules
        mb.free_function('PyInstallGameRules').include()
        mb.free_function('PyInstallGameRules').rename('InstallGameRules')
        
        # CAmmoDef
        mb.class_('CAmmoDef').include()
        mb.class_('CAmmoDef').mem_fun('GetAmmoOfIndex').exclude()
        mb.class_('CAmmoDef').vars('m_AmmoType').exclude()
        
        mb.free_function('GetAmmoDef').include()
        mb.free_function('GetAmmoDef').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object )  
        
        # Remove any protected function 
        mb.calldefs( matchers.access_type_matcher_t( 'protected' ) ).exclude()  
    