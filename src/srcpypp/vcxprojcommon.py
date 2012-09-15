from xml.dom.minidom import parse, parseString
import os

# Find include directories
def GetIncludes(dom):
    project = dom.getElementsByTagName('Project')[0]
    itemdefgroups = project.getElementsByTagName('ItemDefinitionGroup')
    for itemdefgroup in itemdefgroups:
        attribute = itemdefgroup.getAttribute('Condition')
        if attribute.find('Release|Win32') != -1:
            break
    includestext = itemdefgroup.getElementsByTagName('ClCompile')[0].getElementsByTagName('AdditionalIncludeDirectories')[0].childNodes[0].data
    return includestext.split(';')
    