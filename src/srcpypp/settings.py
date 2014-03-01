import os

# srcpath is the folder containing the game directory with source code (i.e. server, client and shared)
srcpath = '..'

# Paths to be searched by Python for modules containing the binding code
searchpaths = [
    os.path.normpath('game/client/python/modules/'),
    os.path.normpath('game/server/python/modules/'),
    os.path.normpath('game/shared/python/modules/'),
]

# Output folder name, used for creating filters by VPC (see srcpypp.genvpc)
autogenfoldername = 'autogenerated'

# Output paths for generated binding code (must use autogenfoldername)
client_path = os.path.normpath('game/client/python/modules/%s/' % (autogenfoldername))
server_path = os.path.normpath('game/server/python/modules/%s/' % (autogenfoldername))
shared_path = os.path.normpath('game/shared/python/modules/%s/' % (autogenfoldername))

# Branch (currently either "swarm" or "source2013")
branch = "swarm"
# VPC Project name
vpcgamename = 'pysource'
# VPC Project file paths
vpcserverpath = os.path.join(srcpath, 'game/server/server_pysource.vpc')
vpcclientpath = os.path.join(srcpath, 'game/client/client_pysource.vpc')
# Output paths for generated VPC file, to be included in the game VPC files
vpcserverautopath = os.path.join(os.path.dirname(vpcserverpath), '%s_autogen.vpc' % (os.path.splitext(os.path.basename(vpcserverpath))[0]))
vpcclientautopath = os.path.join(os.path.dirname(vpcclientpath), '%s_autogen.vpc' % (os.path.splitext(os.path.basename(vpcclientpath))[0]))

# Project project paths (see vxprojinfo.py for the available info)
vcxprojserver = '../game/server/wars_server.vcxproj'
vcxprojclient = '../game/client/wars_client.vcxproj'

# Automatically add all python module files to the project files
# Backup before using!
autoupdatevxproj = False
addpythonfiles = True

# The list of modules
# The parse code looks in the above paths
modules = [
    # Base
    ('srcbuiltins', 'SrcBuiltins'),
    ('_srcbase', 'SrcBase'),
    ('_vmath', 'VMath'),
    ('_filesystem', 'SrcFilesystem'),
    
    # Game
    ('_animation', 'Animation'),
    ('_entities', 'Entities'),
    ('_entitiesmisc', 'EntitiesMisc'),
    ('_gameinterface', 'GameInterface'),
    ('_utils', 'Utils'),
    ('_physics', 'Physics'),
    ('_sound', 'Sound'),
    ('_particles', 'Particles'),
    ('materials', 'Materials'),
    ('_gamerules', 'GameRules'),
    
    ('_te', 'TE'),
    ('_fow', 'FOW'),
    
    # Client
    ('_input', 'Input'),
    ('_vgui', 'VGUI'),
    ('_vguicontrols', 'VGUIControls'),
    ('_cef', 'CEF'),
    
    # Wars
    ('unit_helper', 'UnitHelper'),
    
    # Misc
    ('_steam', 'Steam'),
    ('_navmesh', 'NavMesh'),
    ('_ndebugoverlay', 'NDebugOverlay'),
    ('vprof', 'VProf'),
    ('_srctests', '_SrcTests'),
    ('matchmaking', 'MatchMaking'),
]

