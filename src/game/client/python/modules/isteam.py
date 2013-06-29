from generate_mods_helper import GenerateModuleClient
import settings

from pyplusplus import function_transformers as FT
from src_helper import DisableKnownWarnings, HasArgType
from pyplusplus.module_builder import call_policies

class ISteam(GenerateModuleClient):
    module_name = 'isteam'
    
    files = [
        'videocfg/videocfg.h',

        'cbase.h',
        'steam\steam_api.h',
        'steam\isteamfriends.h',
        'steam\isteamutils.h',
        'steam\steamclientpublic.h',
        
        'vgui_avatarimage.h',
    ]

    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()  
        
        # Accessor class for all
        mb.add_registration_code( "bp::scope().attr( \"steamapicontext\" ) = boost::ref(steamapicontext);" )
        cls = mb.class_('CSteamAPIContext')
        cls.include()
        cls.mem_fun('Init').exclude()
        cls.mem_fun('Clear').exclude()
        cls.mem_fun('SteamUser').exclude()
        cls.mem_fun('SteamMatchmaking').exclude()
        cls.mem_fun('SteamUserStats').exclude()
        cls.mem_fun('SteamApps').exclude()
        cls.mem_fun('SteamMatchmakingServers').exclude()
        
        cls.mem_fun('SteamHTTP').exclude()
        cls.mem_fun('SteamScreenshots').exclude()
        cls.mem_fun('SteamUnifiedMessages').exclude()
        
        
        if settings.ASW_CODE_BASE:
            cls.mem_fun('SteamNetworking').exclude()
            cls.mem_fun('SteamRemoteStorage').exclude()
        
        mb.class_('CSteamAPIContext').mem_funs('SteamFriends').call_policies = call_policies.return_internal_reference() 
        mb.class_('CSteamAPIContext').mem_funs('SteamUtils').call_policies = call_policies.return_internal_reference() 
        
        # CSteamID
        mb.class_('CSteamID').include()
        mb.class_('CSteamID').mem_funs('Render').exclude()
        
        mb.class_('CSteamID').calldefs( lambda decl: HasArgType(decl, 'char') ).exclude()
        mb.class_('CSteamID').mem_funs('SetFromString').exclude()      # No definition...
        mb.class_('CSteamID').mem_funs('SetFromSteam2String').exclude()      # No definition...
        mb.class_('CSteamID').mem_funs('BValidExternalSteamID').exclude()      # No definition...
        
        mb.enum('EResult').include()
        mb.enum('EDenyReason').include()
        mb.enum('EUniverse').include()
        mb.enum('EAccountType').include()
        mb.enum('ESteamUserStatType').include()
        mb.enum('EChatEntryType').include()
        mb.enum('EChatRoomEnterResponse').include()
        
        mb.add_registration_code( "bp::scope().attr( \"QUERY_PORT_NOT_INITIALIZED\" ) = (int)QUERY_PORT_NOT_INITIALIZED;" )
        mb.add_registration_code( "bp::scope().attr( \"QUERY_PORT_ERROR\" ) = (int)QUERY_PORT_ERROR;" )
        
        # Friends
        cls = mb.class_('ISteamFriends')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        cls.mem_fun('GetFriendGamePlayed').exclude()
        
        mb.enum('EFriendRelationship').include()
        mb.enum('EPersonaState').include()
        mb.add_registration_code( "bp::scope().attr( \"k_cchPersonaNameMax\" ) = (int)k_cchPersonaNameMax;" )
        
        # Utils
        cls = mb.class_('ISteamUtils')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        cls.mem_fun('GetImageRGBA').exclude()
        cls.mem_fun('GetImageSize').exclude()
        
        #mb.class_('ISteamUtils').mem_funs('GetImageSize').add_transformation( FT.output('pnWidth'), FT.output('pnHeight'))
        #mb.class_('ISteamUtils').mem_funs('GetCSERIPPort').add_transformation( FT.output('unIP'), FT.output('usPort'))
        
        # Disable known warnings, but exposed in other modules
        DisableKnownWarnings(mb)