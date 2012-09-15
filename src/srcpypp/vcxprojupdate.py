# TODO: rewrite this into something more clear

from xml.dom.minidom import parse, parseString, Node
import os

import settings
import vcxprojinfo

serverproject = '../game/server/wars_server.vcxproj'

testpaths = [
    '../shared/python/modules/',
    '../client/python/modules/',
    '../server/python/modules/',
    'python/modules/',
    'python/modules/',
]

if settings.addpythonfiles:
    testpaths.extend( [
        '../shared/python/',
        '../client/python/',
        '../server/python/',
        'python/',
    ])

def FindCLCompiles(dom, tagname='ClCompile'):
    project = dom.getElementsByTagName('Project')[0]
    itemgroups = project.getElementsByTagName('ItemGroup')
    clcompilesgroups = []
    for itemgroup in itemgroups:
        clcompiles = itemgroup.getElementsByTagName(tagname)
        if clcompiles:
            clcompilesgroups.append(itemgroup)
            
    assert(1 == len(clcompilesgroups))

    return clcompilesgroups[0]
    
'''
def FindCLIncludes(dom):
    project = dom.getElementsByTagName('Project')[0]
    itemgroups = project.getElementsByTagName('ItemGroup')
    clincludegroups = []
    for itemgroup in itemgroups:
        clinclude = itemgroup.getElementsByTagName('ClInclude')
        if clinclude:
            clincludegroups.append(itemgroup)
            
    assert(1 == len(clincludegroups))

    return clincludegroups[0]
'''

def FindFiltersNames(dom):
    project = dom.getElementsByTagName('Project')[0]
    itemgroups = project.getElementsByTagName('ItemGroup')
    filtergroups = []
    for itemgroup in itemgroups:
        #filters = itemgroup.getElementsByTagName('Filter')
        for child in itemgroup.childNodes:
            if child.nodeType == Node.ELEMENT_NODE and child.tagName == 'Filter':
                filtergroups.append(itemgroup)
                break
                
    assert(1 == len(filtergroups))

    return filtergroups[0]
    
def FindPythonNodes(itemgroup, tagname='ClCompile'):
    pythonincludes = []
    clcompiles = itemgroup.getElementsByTagName(tagname)
    for clcompile in clcompiles:
        includepath = os.path.normpath(clcompile.getAttribute('Include'))
        for tp in testpaths:
            tp = os.path.normpath(tp)
            if includepath.startswith(tp):
                pythonincludes.append(clcompile)
    return pythonincludes

# Compile/Include section updating
def RemoveCLCompile(itemgroup, filepath, tagname='ClCompile'):
    clcompiles = itemgroup.getElementsByTagName(tagname)
    for clcompile in clcompiles:
        if clcompile.getAttribute('Include') == filepath:
            itemgroup.removeChild(clcompile)
            break
        
def AddCLCompile(dom, itemgroup, filepath, tagname='ClCompile'):
    node = dom.createElement(tagname)
    node.setAttribute('Include', filepath)
    clcompiles = itemgroup.getElementsByTagName(tagname)
    itemgroup.insertBefore(node, clcompiles[0])
    
# Filter updating
def AddCLCompileFilter(dom, itemgroup, filepath, filter, tagname='ClCompile'):
    node = dom.createElement(tagname)
    node.setAttribute('Include', filepath)
    clcompiles = itemgroup.getElementsByTagName(tagname)
    itemgroup.insertBefore(node, clcompiles[0])
    
    filternode = dom.createElement('Filter')
    node.appendChild(filternode)
    
    filtertextnode = dom.createTextNode(filter)
    filternode.appendChild(filtertextnode)
    
def RemoveCLCompileFilter(itemgroup, filepath, tagname='ClCompile'):
    clcompiles = itemgroup.getElementsByTagName(tagname)
    for clcompile in clcompiles:
        if clcompile.getAttribute('Include') == filepath:
            itemgroup.removeChild(clcompile)
            break
            
def AddFilterNameIfNotExist(dom, filtergroup, filtername):
    exists = False
    filters = filtergroup.getElementsByTagName('Filter')
    for filter in filters:
        if filter.getAttribute('Include') == filtername:
            exists = True
            break
    if exists:
        return
        
    # TODO: UniqueIdentifier is missing now, but don't know what it is.
    filternode = dom.createElement('Filter')
    filternode.setAttribute('Include', filtername)
    filtergroup.appendChild(filternode)
        
def GetFilterName(filepath):
    s = filepath.split('python\\')
    parts = os.path.split(s[-1])
    if len(parts) == 2 and parts[0]:
        extrafilter = '\%s' % (parts[0][0].upper() + parts[0][1:])
    elif len(parts) == 3 and parts[0] and parts[1]:
        extrafilter = '\%s\\%s' % (parts[0][0].upper() + parts[0][1:], parts[1][0].upper() + parts[1][1:])
    else:
        extrafilter = ''
    return 'Source Files\Python' + extrafilter
    
def SplitIncludeCompiles(files):
    includes = set()
    compiles = set()
    
    for filename in files:
        if filename.endswith('cpp'):
            compiles.add(filename)
        else:
            includes.add(filename)
    return compiles, includes
    
# Debug settings
dryrun = False
printfiles = False
printincludes = False
printnewfiles = True
printremoveincludes = True
printfilters = True
    
def UpdateFiles(dom, domfilters, tagname='ClCompile', files=[]):
    # Get all groups we need to update
    itemgroup = FindCLCompiles(dom, tagname=tagname)
    clcompiles = FindPythonNodes(itemgroup, tagname=tagname)
    includepaths = set(map(lambda clcompile: os.path.normpath(clcompile.getAttribute('Include')), clcompiles))
    
    # Filters
    itemgroupfilters = FindCLCompiles(domfilters, tagname=tagname)
    clcompilesfilters = FindPythonNodes(itemgroupfilters, tagname=tagname)
    filternamegroup = FindFiltersNames(domfilters)
    
    # Report files and includes (debug)
    if printfiles and files:
        print('%s All files: ' % (tagname))
        for f in files:
            print('\t%s' % (f))
    if printincludes and includepaths:
        print('%s Current includes: ' % (tagname))
        for f in includepaths:
            print('\t%s' % (f))
            
    newfiles = files.difference(includepaths)
    removeincludes = includepaths.difference(files)
    
    # Report changes
    if printnewfiles and newfiles:
        print('%s New files: ' % (tagname))
        for f in newfiles:
            print('\t%s' % (f))
            
    if printremoveincludes and removeincludes:
        print('%s Includes to be removed: ' % (tagname))
        for f in removeincludes:
            print('\t%s' % (f))
            
    # Only need to update if something changed
    if not newfiles and not removeincludes:
        #print('vcxprojupdate: nothing to do for project file %s' % (vcxproj))
        return False
        
    # Start updating vcxproj project
    for f in removeincludes:
        RemoveCLCompile(itemgroup, f, tagname=tagname)
    for f in newfiles:
        AddCLCompile(dom, itemgroup, f, tagname=tagname)
        
    # Now update filters
    filternames = set()
    for f in newfiles:
        filtername = GetFilterName(f)
        filternames.add(filtername)
        AddCLCompileFilter(domfilters, itemgroupfilters, f, filtername, tagname=tagname)
    for f in removeincludes:
        RemoveCLCompileFilter(itemgroupfilters, f, tagname=tagname)
        
    # Add new filters
    for f in filternames:
        AddFilterNameIfNotExist(domfilters, filternamegroup, f)
        if printfilters:
            print 'Detected filter: %s' % (f)
            
    return True

def UpdateChanged(files, vcxproj):
    print('Detecting changes vcxproj %s...' % (vcxproj))
    
    # Load/parse the project files
    vcxprojfilters = '%s.filters' % (vcxproj)
    try:
        dom = parse(vcxproj)
    except IOError:
        print('Invalid vcxproj %s specified (check settings)!' % (vcxproj))
        return
        
    try:
        domfilters = parse(vcxprojfilters)
    except IOError:
        print('Invalid vcxproj filter %s specified (check settings)!' % (vcxprojfilters))
        return
        
    # Split into "compiles" and "includes"
    files, clincludes = SplitIncludeCompiles(files)

    updatedcompiles = UpdateFiles(dom, domfilters, tagname='ClCompile', files=files)
    updatedincludes = UpdateFiles(dom, domfilters, tagname='ClInclude', files=clincludes) 
    if not updatedcompiles and not updatedincludes:
        print('vcxprojupdate: nothing to do for project file %s' % (vcxproj))
        return
    '''
    # Project
    itemgroup = FindCLCompiles(dom)
    clcompiles = FindPythonNodes(itemgroup)
    clincludesgroup = FindCLIncludes(dom)
    filtergroup = FindCLCompiles(domfilters)
    filternamegroup = FindFiltersNames(domfilters)
    includepaths = set(map(lambda clcompile: os.path.normpath(clcompile.getAttribute('Include')), clcompiles))
    
    # Filters
    itemgroupfilters = FindCLCompiles(domfilters)
    clcompilesfilters = FindPythonNodes(itemgroupfilters)

    # Report files and includes (debug)
    if printfiles:
        print('All files: ')
        for f in files:
            print('\t%s' % (f))
    if printincludes:
        print('Current includes: ')
        for f in includepaths:
            print('\t%s' % (f))
    
    newfiles = files.difference(includepaths)
    removeincludes = includepaths.difference(files)
    
    # Report changes
    if printnewfiles:
        print('New files: ')
        for f in newfiles:
            print('\t%s' % (f))
            
    if printremoveincludes:
        print('Includes to be removed: ')
        for f in removeincludes:
            print('\t%s' % (f))
            
    # Only need to update if something changed
    if not newfiles and not removeincludes:
        print('vcxprojupdate: nothing to do for project file %s' % (vcxproj))
        return
        
    # Start updating vcxproj project
    for f in removeincludes:
        RemoveCLCompile(itemgroup, f)
    for f in newfiles:
        AddCLCompile(dom, itemgroup, f)
        
    # Now update filters
    filternames = set()
    for f in newfiles:
        filtername = GetFilterName(f)
        filternames.add(filtername)
        AddCLCompileFilter(domfilters, itemgroupfilters, f, filtername)
    for f in removeincludes:
        RemoveCLCompileFilter(itemgroupfilters, f)
        
    # Add new filters
    for f in filternames:
        AddFilterNameIfNotExist(domfilters, filternamegroup, f)
        if printfilters:
            print 'Detected filter: %s' % (f)
    '''
    
    if dryrun:
        print('Not updated vcxproj. Dry run')
        return
        
    raw_input("Required changes detected in vcxproj %s. Press enter to continue (backup!)" % (vcxproj))
    
    # Finally write to file
    fp = open(vcxproj, 'wb')
    dom.writexml(fp)
    fp.close()
    
    fp = open(vcxprojfilters, 'wb')
    domfilters.writexml(fp)
    fp.close()
    
    print('Updated vcxproj')
            
if __name__ == '__main__':
    #serverdom = parse(serverproject) # parse an XML file by name
    serverdom = parse('test.xml')
    
    itemgroup = FindCLCompiles(serverdom)
    #RemoveCLCompile(itemgroup, '..\shared\python\modules\matchmaking.cpp')
    clcompiles = FindPythonNodes(itemgroup)

    AddCLCompile(serverdom, itemgroup, os.path.normpath('../shared/python/modules/matchmaking.cpp'))
    
    #fp = open(serverproject, 'wb')
    fp = open('test2.xml', 'wb')
    serverdom.writexml(fp)
    fp.close()