## Lambda Wars (based on Alien Swarm SDK).
Source code of Lambda Wars (https://lambdawars.com/), available on the Steam store: https://steamcommunity.com/app/270370

Game code can be found at: https://svn.lambdawars.com/ (svn checkout https://svn.lambdawars.com/lambdawars/trunk/ lambdawarsdev)

## Overview

### Recast Navigation
[Recast Navigation mesh](https://github.com/recastnavigation/recastnavigation) is integrated for path finding. 
In Half-Life 2, NPCs use manual places for path finding. However in a game like
Lambda Wars this would quite tedious. Another option earlier used by Lambda Wars was the navigation supplied by Valve (used by Counter-Strike
and Left 4 Dead). The down-side of that navigation mesh implementation is long generation times, only one mesh for all unit types and being restricted to
world aligned polygons.
The [recast navigation meshes](https://www.youtube.com/watch?v=oTYREslZu2s) do not have these restrictions (LW uses 5 different 
generated meshes for different unit types) and meshes are generated within a minute for most of our maps (usually much faster).

### Chromium Embedded Framework
[Chromium Embedded Framework](https://bitbucket.org/chromiumembedded/cef) is integrated to easily develop new UI for Lambda Wars.
The Chrome developer tools can be used to reload a view ingame and debug parts of the UI.
CEF is used for the majority of the 
[main menu](https://svn.lambdawars.com/listing.php?repname=Lambda+Wars&path=%2Ftrunk%2Fui%2Fmenu_next%2F&#a02c1dfbdceec971815a7fcae4eb0e8f1) 
and smaller parts of the ingame UI.

### Python bindings
Lambda Wars contains extensive (automatically generated) Python bindings. These bindings are used to extend the game code into Python, allowing
to extend entity classes and define new game modes among others. These bindings are also available as a separate project, 
[PySource SDK 2013](https://github.com/Sandern/py-source-sdk-2013).

### Unit Navigator
The [unit navigator](https://github.com/Sandern/lambdawars/blob/develop/src/game/server/lambdawars/unit_navigator.cpp) is responsible for managing the paths, local obstacle avoidance and event dispatching to the AI (e.g. goal completion). The path finding
uses recast navigation mesh as described above. The local avoidance for small objects is loosely based on [Crowd Flows](http://grail.cs.washington.edu/projects/crowd-flows/), also
[demo'ed here](https://www.youtube.com/watch?v=4FrqW_DiugI) in Lambda Wars. Bigger or semi-static objects like buildings are registered on navigation mesh as obstacles, so pathing
takes them into account.

### Unit AI
The unit AI is loosely based on the [AI systems concepts of Left 4 Dead](http://www.valvesoftware.com/publications/2009/ai_systems_of_l4d_mike_booth.pdf).
Units can have one or more behaviors and each behavior defines a set of actions of which one action can be active within the behavior. An action can change 
or suspend to another action and is fully event driven. The most common actions are defined in 
[behavior_generic.py](https://svn.lambdawars.com/filedetails.php?repname=Lambda+Wars&path=%2Ftrunk%2Fpython%2Fcore%2Funits%2Fbehavior_generic.py) for Lambda Wars.

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
The mod comes with a set of custom shaders to support the Fog of War, as well as to integrate the [deferred lighting project](https://www.youtube.com/watch?v=bMjXx-KweIo). The deferred lighting shaders are modified to blend with regular lightmaps. If you don't plan on modifying the shaders, you don't need to follow these steps. The compiled shaders are already included in the repository.
- Follow: https://developer.valvesoftware.com/wiki/Shader_Authoring
- Update paths buildsdkshaders_wars.bat
