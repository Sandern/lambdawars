## Usage
The modules can be generated using generatemods.py. 

All modules can be parsed by running:

        py generatemods.py

A single module can be generated a follows:

        py generatemods.py -m _entities

## Added a new module
To create a new module you must do two things:
1. Add a new python files with parse code to either one of the following folders 
(depending on whether it is intended for the server, client or is shared code):
- [mp/src/game/shared/python/modules](/mp/src/game/shared/python/modules)
- [mp/src/game/client/python/modules](/mp/src/game/client/python/modules)
- [mp/src/game/server/python/modules](/mp/src/game/server/python/modules)

2. Open settings.py and add the new binding code to the module list.

## Updating GCCXML library
1. Go to https://github.com/gccxml/gccxml and clone latest
2. Use CMake to generate project files and build
3. Update Support folder
	- Create a virtual machine containing a 32 bit installation of a linux distribution (e.g. Ubuntu)
	- Install GCC and G++
	- Copy usr/lib and usr/include to the Support folder when parsing on other platforms than Linux

## Updating PyPlusPlus library
Merge changes from https://bitbucket.org/ompl/pyplusplus

## Updating src_module_builder Settings
1. Open srcpy/src_module_builder.py
2. Update include paths
	- `gcc -print-prog-name=cc1plus` -v
	- `gcc -print-prog-name=cc1` -v
3. Update symbols gccxml_gcc_options file
	- gcc -dM -E - < /dev/null

## Modifying importlib/_bootstrap.py
Contains modified importlib._bootstrap, to allow loading modules from vpk files. This module must be frozen and included in the server/client dlls.

Steps:
1. Compile _freeze_importlib project (thirdparty/python/pcbuild/_freeze_importlib)
2. Use it on _bootstrap.py: .\_freeze_importlib.exe _bootstrap.py srcpy_importlib.h
3. Update shared/python/srcpy_importlib.h and recompile game