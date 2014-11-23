## Lambda Wars (based on Alien Swarm SDK).
Source code of Lambda Wars (http://lambdawars.com/).

Game code can be found at: http://svn.hl2wars.com/

## Compiling
Tested with Visual Studio 2013:

1. Open src/python/srcbuid/pcbuild.pln and build the pythoncore project
2. Open src/hl2wars.sln and compile the full project

## Generating Python bindings
Bindings are automatically generated from python files.
You can find the list of modules and other settings in "src/srcpypp/settings.py".
The actually modules can be found in "game/(client|server|shared)/python/modules".

Py++ is used to generate the actually bindings. You can find more information at:
http://sourceforge.net/projects/pygccxml/

Generating the modules (requires Python 3):
Open a command prompt and go to the srcpypp folder.
From here you can run a few commands:
- Generate bindings for all modules: python generate_mods
- Generate bindings for a single module: python generate_mods -m module_name

## Compiling shaders
- Follow: https://developer.valvesoftware.com/wiki/Shader_Authoring
- Update paths buildsdkshaders_wars.bat

## License
Creative Commons Attribution-NonCommercial 3.0 Unported
Only applies to the new code in the sdk (in particular the code in game/(.*)/(lambdawars|python|cef) folders).

See license.txt