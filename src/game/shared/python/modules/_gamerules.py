from srcpy.module_generators import SemiSharedModuleGenerator

from pyplusplus.module_builder import call_policies
from pyplusplus import function_transformers as FT
from pygccxml.declarations import matchers

class GameRules(SemiSharedModuleGenerator):
    module_name = '_gamerules'
    split = True
    
    @property
    def files(self):
        files = [
            'cbase.h',
            'gamerules.h',
            'multiplay_gamerules.h',
            'singleplay_gamerules.h',
            'teamplay_gamerules.h',
            'srcpy_gamerules.h',
            'ammodef.h',
            '#items.h',
            '$takedamageinfo.h',
        ]
        if 'HL2MP' in self.symbols:
            files.extend([
                '#hl2mp/hl2mp_player.h',
                'hl2mp/hl2mp_gamerules.h',
            ])
        if 'HL2WARS_DLL' in self.symbols:
            files.extend([
                'hl2wars_gamerules.h',
            ])
        return files
        
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        gamerules = [
            ('CGameRules', 'C_GameRules', True),
            ('CMultiplayRules', 'C_MultiplayRules', True),
            ('CSingleplayRules', 'C_SingleplayRules', True),
            ('CTeamplayRules', 'C_TeamplayRules', False),
        ]
        if 'HL2MP' in self.symbols:
            gamerules.append(('CHL2MPRules', 'C_HL2MPRules', False))
        if 'HL2WARS_DLL' in self.symbols:
            gamerules.append(('CHL2WarsGameRules', 'C_HL2WarsGameRules', False))
            
        for gamerulename, clientgamerulename, baseonly in gamerules:
            cls_name = gamerulename if self.isserver else clientgamerulename
            cls = mb.class_(cls_name)
            cls.include()
            
            if not baseonly:
                # Used internally
                cls.mem_funs('GetPySelf', allow_empty=True).exclude()
                cls.add_wrapper_code(
                    'virtual PyObject *GetPySelf() const { return boost::python::detail::wrapper_base_::get_owner(*this); }'
                )
            else:
                cls.no_init = True
            
            # Always use server class name
            cls.rename(gamerulename)
            
        mb.mem_funs('ShouldCollide').virtuality = 'not virtual'
        mb.mem_funs('GetAmmoDamage').virtuality = 'not virtual' # Just modify the ammo table when needed...
        
        mb.mem_funs('NetworkStateChanged').exclude()
        
        if self.isserver:
            if self.settings.branch == 'source2013':
                mb.mem_fun('TacticalMissionManagerFactory').exclude()
            if self.settings.branch == 'swarm':
                mb.mem_funs('DoFindClientInPVS').exclude()
                
                mb.mem_funs('IsTopDown').virtuality = 'not virtual'
                mb.mem_funs('ForceSplitScreenPlayersOnToSameTeam').virtuality = 'not virtual'

            # Excludes
            mb.mem_funs('VoiceCommand').exclude()
            
            # Remove virtuality from  or include some unneeded functions (would just slow down things)
            mb.mem_funs('FrameUpdatePostEntityThink').virtuality = 'not virtual' # Calls Think
            mb.mem_funs('EndGameFrame').virtuality = 'not virtual'
            mb.mem_funs('FAllowFlashlight', lambda d: d.virtuality == 'virtual').virtuality = 'not virtual'

        if 'HL2MP' in self.symbols:
            mb.mem_funs('GetHL2MPViewVectors').exclude()
            
            if self.isclient:
                mb.mem_funs('RestartGame').exclude()
                mb.mem_funs('CheckAllPlayersReady').exclude()
                mb.mem_funs('CheckRestartGame').exclude()
                mb.mem_funs('CleanUpMap').exclude()
                
        if 'HL2WARS_DLL' in self.symbols:
            if self.isserver:
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
                                 
                # Remove virtuality because not needed
                mb.mem_funs('Damage_IsTimeBased', lambda d: d.virtuality == 'virtual').virtuality = 'not virtual'
                mb.mem_funs('Damage_ShouldGibCorpse', lambda d: d.virtuality == 'virtual').virtuality = 'not virtual'
                mb.mem_funs('Damage_ShowOnHUD', lambda d: d.virtuality == 'virtual').virtuality = 'not virtual'
                mb.mem_funs('Damage_NoPhysicsForce', lambda d: d.virtuality == 'virtual').virtuality = 'not virtual'
                mb.mem_funs('Damage_ShouldNotBleed', lambda d: d.virtuality == 'virtual').virtuality = 'not virtual'
                mb.mem_funs('Damage_GetTimeBased', lambda d: d.virtuality == 'virtual').virtuality = 'not virtual'
                mb.mem_funs('Damage_GetShouldGibCorpse', lambda d: d.virtuality == 'virtual').virtuality = 'not virtual'
                mb.mem_funs('Damage_GetShowOnHud', lambda d: d.virtuality == 'virtual').virtuality = 'not virtual'
                mb.mem_funs('Damage_GetNoPhysicsForce', lambda d: d.virtuality == 'virtual').virtuality = 'not virtual'
                mb.mem_funs('Damage_GetShouldNotBleed', lambda d: d.virtuality == 'virtual').virtuality = 'not virtual'
                                 
                # Exlude and replace
                cls.mem_fun('ClientConnected').exclude()
                cls.mem_fun('PyClientConnected').rename('ClientConnected')
                cls.mem_fun('ClientDisconnected').exclude()
                cls.mem_fun('PyClientDisconnected').rename('ClientDisconnected')
            
        # Temp excludes
        mb.mem_funs('GetEncryptionKey').exclude()
        mb.mem_funs('GetViewVectors').exclude()

        # Call policies
        mb.mem_funs('GetNextBestWeapon').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        
        # Rename PyGameRules to just GameRules
        mb.free_function('PyGameRules').include()
        mb.free_function('PyGameRules').rename('GameRules')
        
        # Installing the gamerules
        mb.free_function('PyInstallGameRules').include()
        mb.free_function('PyInstallGameRules').rename('InstallGameRules')
        
        # CAmmoDef
        cls = mb.class_('CAmmoDef')
        cls.include()
        cls.mem_fun('GetAmmoOfIndex').exclude()
        cls.var('m_AmmoType').exclude()
        cls.var('m_nAmmoIndex').rename('ammoindex')
        
        mb.free_function('GetAmmoDef').include()
        mb.free_function('GetAmmoDef').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        
        # Remove any protected function 
        mb.calldefs( matchers.access_type_matcher_t('protected') ).exclude()
        
        # Finally apply common rules to all includes functions and classes, etc.
        self.ApplyCommonRules(mb)
    