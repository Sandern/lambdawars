## Lambda Wars (based on Alien Swarm SDK).
Source code of Lambda Wars (http://lambdawars.com/), available on Steam: http://steamcommunity.com/app/270370

Game code can be found at: http://svn.lambdawars.com/

## Compiling
Tested with Visual Studio 2013:

1. Open src/thirdparty/python/srcbuid/pcbuild.pln and build the pythoncore project
2. Open src/lambdawars.sln and compile the full project

## Generating Python bindings (optional)
Bindings are automatically generated from python files.
You can find the list of modules and other settings in "src/srcpypp/settings.py".
The actually modules can be found in "game/(client|server|shared)/python/modules".

Py++ is used to generate the actually bindings. You can find more information at:
https://github.com/gccxml/pygccxml

Generating the modules (requires Python 3):
Open a command prompt and go to the srcpypp folder.
From here you can run a few commands:
- Generate bindings for all modules: python generate_mods
- Generate bindings for a single module: python generate_mods -m module_name

## Compiling shaders (optional)
- Follow: https://developer.valvesoftware.com/wiki/Shader_Authoring
- Update paths buildsdkshaders_wars.bat

## License
Creative Commons Attribution-NonCommercial 3.0 Unported
Only applies to the new code in the sdk (in particular the code in game/(.*)/(lambdawars|python|cef) folders).

See license.txt