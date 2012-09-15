from xml.dom.minidom import parse, parseString
import os

import settings
from vcxprojcommon import *

# Read project files
serverdom = parse(settings.vcxprojserver)
clientdom = parse(settings.vcxprojclient)

# Get includes
serverincludes = GetIncludes(serverdom)
clientincludes = GetIncludes(clientdom)