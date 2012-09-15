#! /usr/bin/python
import sys
import getopt

import settings


# Main
if __name__ == "__main__":
    try:
        opts, args = getopt.getopt(sys.argv[1:], "m:as")
    except getopt.GetoptError:
        print('generate_mods: Invalid arguments')
        sys.exit(2)

    # parse settings
    specific_module = None
    generate_append_file_only = False
    for o, a in opts:
        if o == '-m':
            settings.SPECIFIC_MODULE = a
        elif o == '-a':
            settings.APPEND_FILE_ONLY = True
        elif o =='-o':
            settings.ASW_CODE_BASE = False
            
    # Import AFTER parsing the settings
    import generate_mods_helper
    generate_mods_helper.ParseModules()
