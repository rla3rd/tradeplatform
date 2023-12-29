import os,sys

PLATFORM                    = sys.platform
WIN                         = PLATFORM.startswith('win')
DARWIN                      = PLATFORM.startswith('darwin')
LINUX                       = not (WIN or DARWIN)

if WIN:
    windowsVer = sys.getwindowsversion()
    if (windowsVer[3] == 1 ):
        WIN98   = True
    else:
        WIN98   = False
else:
    WIN98       = False

PYTHON_EXEC                 = sys.executable
if WIN:
    if PYTHON_EXEC.endswith('w.exe'):
        PYTHON_EXEC = PYTHON_EXEC[:-5] + '.exe'
    try:
        import win32api
        PYTHON_EXEC         = win32api.GetShortPathName(PYTHON_EXEC)
        PYTHON_COM          = True
    except ImportError:
        PYTHON_EXEC         = (r'%s'%PYTHON_EXEC).replace('Program Files','progra~1')
        PYTHON_COM          = False
    if ' ' in PYTHON_EXEC:
        PYTHON_EXEC         = '"%s"'%PYTHON_EXEC
elif DARWIN:
    PYTHON_EXEC.replace('ython','ythonw')
    PYTHON_COM              = False
else:
    PYTHON_COM              = False

PATH                        = os.path.dirname(__file__)
_PATH                       = os.path.dirname(PATH)

def path(p):
    if WIN and ' ' in p:
        return '"%s"'%p
    else:
        return p

#---Append sm
if PATH not in sys.path:
    sys.path.append(PATH)
if _PATH not in sys.path:
    sys.path.append(_PATH)

import sm.osx

INFO={
    'author'            : "www.stani.be",
    'author_email'      : 'spe.stani.be@gmail.com',
    'blenderVersion'    : "2.35",
    'date'              : "27-10-2005",
    'donate'            : "If you enjoy SPE, consider a (small) donation.",
    'doc'               : "%(titleFull)s\n\n%(description)s\n\n%(links)s\n\n%(requirements)s\n\n%(copyright)s",
    'forums'            : '',
    'license'           : 'GPL',
    'location'          : PATH,
    'pyVersion'         : "2.3",
    'pyVersionC'        : sys.version.split(' ')[0],
    'scripts'           : ['spe','spe_wininst.py'],
    'skinLocation'      : os.path.join(PATH,'skins','default'),
    'smLocation'        : os.path.join(PATH,'sm'),
    'title'             : "SPE",
    'url'               : 'http://pythonide.stani.be',
    'userPath'          : sm.osx.userPath('.spe'),
    'version'           : "0.8.4.b",
    'wxVersion'         : "2.6.1.0.",
}


INFO['defaults']            = os.path.join(INFO['location'],'defaults.cfg')
INFO['defaultsUser']        = os.path.join(INFO['userPath'],'defaults.cfg')
INFO['defaultWorkspace']    = os.path.join(INFO['userPath'],'defaults.sws')

INFO['titleFull']           = "%(title)s %(version)s"%INFO

if DARWIN:
    INFO['python']          = 'pythonw'
else:
    INFO['python']          = 'python'

INFO['links']               =\
"""Homepage : %s
Donwloads: http://www.stani.be/python/spe/page_blender
Forum    : http://www.stani.be/python/spe/page_forum
Lists    : http://www.stani.be/python/spe/page_mailman"""%INFO['url']

INFO['description']         =\
"""Stani's Python Editor

Spe is a python IDE with auto-indentation, auto completion, call tips, syntax
coloring, syntax highlighting, class explorer, source index, auto todo list,
sticky notes, integrated pycrust shell, python file browser, recent file
browser, drag&drop, context help, ... Special is its blender support with a
blender 3d object browser and its ability to run interactively inside blender.
Spe is extensible with boa.

Wanted: wxpython programmers to extend spe's features, feel free to do a proposal.

For more information, see spe/doc/manual.html"""%INFO

INFO['requirements']        =\
"""Python   v%(pyVersion)s      required
wxPython v%(wxVersion)s required
Blender  v%(blenderVersion)s     optional"""%INFO

INFO['copyright']           =\
"""Copyright (C)%(author)s (%(date)s)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
"""%INFO

INFO['contribute']          = """There are different ways to contribute:

- %s (or convince your boss)
- Let your company sponsor SPE (see manual)
- SPE needs documentation writers and screenshot takers for manual
- Contribute code (eg wxPython panels) & patches
- Promote SPE in newsgroups, press, blogs, ...
- Subscribe to mailing lists
- Translate manual
- Report bugs in the bug tracker"""%INFO['donate']

WILDCARD = "Python (*.py;*.pyw;*.tpy)|*.py;*.pyw;*.tpy|Backup files (*.bak)|*.bak|All files (*)|*"
WORKSPACE_WILDCARD = "SPE Workspace (*.sws)|*.sws|All files (*)|*"
WILDCARD_EXTENDED = WILDCARD+'|Python All(*.py;*.pyw;*.tpy;*.pyc;*.pyd;*.pyo)|*.py;*.pyw;*.tpy;*.pyc;*.pyd;*.pyo|Text (*.txt;*.rtf;*.htm;*.html;*.pdf;*.cfg)|*.txt;*.rtf;*.htm;*.html;*.pdf;*.cfg|Bitmap (*.jpg;*.jpeg;*.bmp;*.tif;*.tiff;*.png;*.pic)|*.jpg;*.jpeg;*.bmp;*.tif;*.tiff;*.png;*.pic|Vector (*.dxf;*.dwg;*.svg;*.swf;*.vrml;*.wrl)|*.dxf;*.dwg;*.svg;*.swf;*.vrml;*.wrl'


__doc__=INFO['doc']%INFO

def copy():
    return INFO.copy()

def dirname(fileName):
    fileName    = os.path.dirname(fileName)
    if PYTHON_COM:
        return win32api.GetShortPathName(fileName)
    else:
        return fileName

def imageFile(fileName):
    return os.path.join(INFO['skinLocation'],fileName)

#---wx
try:
    import wx
    INFO['wxVersionC']  = wx.VERSION_STRING
##    if INFO['wxVersionC']!=INFO['wxVersion']:
##        print '\nSpe Warning: Spe was developped on wxPython v%s, but v%s was found.'%(INFO['wxVersion'],INFO['wxVersionC'])
##        print 'If you experience any problems please install wxPython v%s\n'%INFO['wxVersion']
    INFO['encoding']    = wx.GetDefaultPyEncoding()
    WX_ERROR = False
except ImportError, message:
    print "Spe Error: Please install the right version of wxPython: %s"%INFO['wxVersion']
    print message
    WX_ERROR = True
