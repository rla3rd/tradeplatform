#!/usr/bin/env python
# xrc2wxg.py: Converts an XRC resource file (in a format wxGlade likes,
# i.e. all windows inside sizers, no widget unknown to wxGlade, ...) into a
# WXG file
# $Id: xrc2wxg.py,v 1.18 2005/05/06 21:48:24 agriggio Exp $
# 
# Copyright (c) 2002-2005 Alberto Griggio <agriggio@users.sourceforge.net>
# License: MIT (see license.txt)
# THIS PROGRAM COMES WITH NO WARRANTY

import xml.dom.minidom
import sys, getopt, os.path, time

__version__ = '0.0.3'
_name = 'xrc2wxg'

try: True, False
except NameError: True, False = 1, 0

def get_child_elems(node):
    def ok(n): return n.nodeType == n.ELEMENT_NODE
    return filter(ok, node.childNodes)

def get_text_elems(node):
    def ok(n): return n.nodeType == n.TEXT_NODE
    return filter(ok, node.childNodes)

_counter_name = 1


def convert(input, output):
    global _counter_name, _doc_encoding
    _counter_name = 1

    document = xml.dom.minidom.parse(input)
    fix_fake_panels(document)
    set_base_classes(document)
    fix_properties(document)
    fix_widgets(document)
    fix_encoding(input, document)
    if not hasattr(output, 'write'):
        output = open(output, 'w')
        write_output(document, output)
        output.close()
    else:
        write_output(document, output)

def write_output(document, output):
    """\
    This code has been adapted from XRCed 0.0.7-3.
    Many thanks to its author Roman Rolinsky.
    """
    dom_copy = xml.dom.minidom.Document()

    def indent(node, level=0):
        # Copy child list because it will change soon
        children = node.childNodes[:]
        # Main node doesn't need to be indented
        if level:
            text = dom_copy.createTextNode('\n' + '    ' * level)
            node.parentNode.insertBefore(text, node)
        if children:
            # Append newline after last child, except for text nodes
            if children[-1].nodeType == xml.dom.minidom.Node.ELEMENT_NODE:
                text = dom_copy.createTextNode('\n' + '    ' * level)
                node.appendChild(text)
            # Indent children which are elements
            for n in children:
                if n.nodeType == xml.dom.minidom.Node.ELEMENT_NODE:
                    indent(n, level + 1)

    comment = dom_copy.createComment(' generated by %s %s on %s '
                                     % (_name, __version__, time.asctime()))
    dom_copy.appendChild(comment)
    main_node = dom_copy.appendChild(document.documentElement)
    indent(main_node)
    
    comment_done = False
    for line in dom_copy.toxml().encode('utf-8').splitlines():
        if not comment_done and line.startswith('<!--'):
            line = line.replace('-->', '-->\n\n')
            comment_done = True
        if line.strip():
            output.write(line)
            output.write('\n')
    dom_copy.unlink()


def set_base_classes(document):
    for elem in document.getElementsByTagName('object'):
        klass = elem.getAttribute('class')
        if klass.startswith('wx'):
            elem.setAttribute('base', 'Edit' + klass[2:])
            name = elem.getAttribute('name')
            if not name:
                global _counter_name
                elem.setAttribute('name', 'object_%s' % _counter_name)
                _counter_name += 1


_props = {
    'bg': 'background',
    'fg': 'foreground',
    'content': 'choices',
    'item': 'choice',
    'growablerows': 'growable_rows',
    'growablecols': 'growable_cols',
    'enabled': 'disabled',
    'sashpos': 'sash_pos',
    }

def fix_properties(document):
    # special case...
    for elem in document.getElementsByTagName('disabled'):
        elem.tagName = 'disabled_bitmap'
    for prop in _props:
        for elem in document.getElementsByTagName(prop):
            elem.tagName = _props[prop]
    document.documentElement.tagName = 'application'
    if document.documentElement.hasAttribute('version'):
        document.documentElement.removeAttribute('version')


def fix_widgets(document):
    fix_menubars(document)
    fix_toolbars(document)
    fix_custom_widgets(document)
    fix_sizeritems(document)
    fix_notebooks(document)
    fix_splitters(document)
    fix_spacers(document)
    fix_sliders(document)
    fix_toplevel_names(document)


_widgets_list = [
    'wxFrame', 'wxDialog', 'wxPanel', 'wxSplitterWindow', 'wxNotebook',
    'wxButton', 'wxToggleButton', 'wxBitmapButton', 'wxTextCtrl',
    'wxSpinCtrl', 'wxSlider', 'wxGauge', 'wxStaticText', 'wxCheckBox',
    'wxRadioButton', 'wxRadioBox', 'wxChoice', 'wxComboBox', 'wxListBox',
    'wxStaticLine', 'wxStaticBitmap', 'wxGrid', 'wxMenuBar', 'wxStatusBar',
    'wxBoxSizer', 'wxStaticBoxSizer', 'wxGridSizer', 'wxFlexGridSizer',
    'wxTreeCtrl', 'wxListCtrl', 'wxToolBar',
    ]
_widgets = dict(zip(_widgets_list, [1] * len(_widgets_list)))

_special_class_names = [
    'notebookpage', 'sizeritem', 'separator', 'tool', 'spacer',
    ]
_special_class_names = dict(zip(_special_class_names,
                                [1] * len(_special_class_names)))

def fix_custom_widgets(document):
    for elem in document.getElementsByTagName('object'):
        klass = elem.getAttribute('class')
        if klass not in _widgets and klass not in _special_class_names:
            elem.setAttribute('base', 'CustomWidget')
            args = document.createElement('arguments')
            for child in get_child_elems(elem):
                # if child is a 'simple' attribute, i.e
                # <child>value</child>, convert it to an 'argument'
                if len(child.childNodes) == 1 and \
                       child.firstChild.nodeType == child.TEXT_NODE:
                    arg = document.createElement('argument')
                    arg.appendChild(document.createTextNode(
                        child.tagName + ': ' + child.firstChild.data))
                    args.appendChild(arg)
                    # and remove it
                    elem.removeChild(child)
                # otherwise, leave it where it is (it shouldn't hurt)
            elem.appendChild(args)


def fix_sizeritems(document):
    def ok(node): return node.getAttribute('class') == 'sizeritem'
    def ok2(node): return node.tagName == 'object'
    for sitem in filter(ok, document.getElementsByTagName('object')):
        for child in filter(ok2, get_child_elems(sitem)):
            sitem.appendChild(sitem.removeChild(child))
    fix_flag_property(document)


def fix_flag_property(document):
    for elem in document.getElementsByTagName('flag'):
        tmp = elem.firstChild.data.replace('CENTRE', 'CENTER')
        elem.firstChild.data = tmp.replace('GROW', 'EXPAND')
        if elem.firstChild.data.find('wxALIGN_CENTER_HORIZONTAL') < 0 and \
               elem.firstChild.data.find('wxALIGN_CENTER_VERTICAL') < 0:
            elem.firstChild.data = elem.firstChild.data.replace(
                'wxALIGN_CENTER', 'wxALIGN_CENTER_HORIZONTAL|'
                'wxALIGN_CENTER_VERTICAL')


def fix_menubars(document):
    def ok(elem): return elem.getAttribute('class') == 'wxMenuBar'
    menubars = filter(ok, document.getElementsByTagName('object'))
    for mb in menubars:
        fix_menus(document, mb)
        if mb.parentNode is not document.documentElement:
            mb_prop = document.createElement('menubar')
            mb_prop.appendChild(document.createTextNode('1'))
            mb.parentNode.insertBefore(mb_prop, mb)


def fix_menus(document, menubar):
    def ok(elem): return elem.getAttribute('class') == 'wxMenu'
    menus = filter(ok, get_child_elems(menubar))
    menus_node = document.createElement('menus')
    for menu in menus:
        try:
            label = [ c for c in get_child_elems(menu)
                      if c.tagName == 'label' ][0]
            label = label.firstChild.data
        except IndexError: label = ''
        new_menu = document.createElement('menu')
        new_menu.setAttribute('name', menu.getAttribute('name'))
        new_menu.setAttribute('label', label)
        fix_sub_menus(document, menu, new_menu)
        menus = document.createElement('menus')
        menus.appendChild(new_menu)
        menubar.removeChild(menu).unlink()
        menubar.appendChild(menus)


def fix_sub_menus(document, menu, new_menu):
    for child in get_child_elems(menu):
        klass = child.getAttribute('class')
        elem = document.createElement('')
        if klass == 'wxMenuItem':
            elem.tagName = 'item'
            name = document.createElement('name')
            name.appendChild(document.createTextNode(
                child.getAttribute('name')))
            elem.appendChild(name)
            for c in get_child_elems(child):
                elem.appendChild(c)
        elif klass == 'separator':
            elem.tagName = 'item'
            for name in 'label', 'id', 'name':
                e = document.createElement(name)
                e.appendChild(document.createTextNode('---'))
                elem.appendChild(e)
        elif klass == 'wxMenu':
            elem.tagName = 'menu'
            elem.setAttribute('name', child.getAttribute('name'))
            try:
                label = [ c for c in get_child_elems(child) if
                          c.tagName == 'label' ][0]
                label = label.firstChild.data
            except IndexError: label = ''
            elem.setAttribute('label', label)
            fix_sub_menus(document, child, elem)
        if elem.tagName: new_menu.appendChild(elem)


def fix_toolbars(document):
    def ok(elem): return elem.getAttribute('class') == 'wxToolBar'
    toolbars = filter(ok, document.getElementsByTagName('object'))
    for tb in toolbars:
        fix_tools(document, tb)
        if tb.parentNode is not document.documentElement:
            tb_prop = document.createElement('toolbar')
            tb_prop.appendChild(document.createTextNode('1'))
            tb.parentNode.insertBefore(tb_prop, tb)


def fix_tools(document, toolbar):
    tools = document.createElement('tools')
    for tool in [c for c in get_child_elems(toolbar) if c.tagName == 'object']:
        if tool.getAttribute('class') == 'tool':
            new_tool = document.createElement('tool')
            id = document.createElement('id')
            id.appendChild(document.createTextNode(
                tool.getAttribute('name')))
            new_tool.appendChild(id)
            for c in get_child_elems(tool):
                if c.tagName == 'bitmap':
                    c.tagName = 'bitmap1'
                elif c.tagName == 'tooltip':
                    c.tagName = 'short_help'
                elif c.tagName == 'longhelp':
                    c.tagName = 'long_help'
                elif c.tagName == 'toggle':
                    c.tagName = 'type'
                new_tool.appendChild(c)
            tools.appendChild(new_tool)
            toolbar.removeChild(tool).unlink()
        elif tool.getAttribute('class') == 'separator':
            new_tool = document.createElement('tool')
            id = document.createElement('id')
            id.appendChild(document.createTextNode('---'))
            new_tool.appendChild(id)
            tools.appendChild(new_tool)
            toolbar.removeChild(tool).unlink()
        else:
            # some kind of control, unsupported at the moment, just remove it
            toolbar.removeChild(tool).unlink()
    toolbar.appendChild(tools)


def fix_notebooks(document):
    def ispage(node): return node.getAttribute('class') == 'notebookpage'
    def isnotebook(node): return node.getAttribute('class') == 'wxNotebook'
    for nb in filter(isnotebook, document.getElementsByTagName('object')):
        pages = filter(ispage, get_child_elems(nb))
        tabs = document.createElement('tabs')
        try:
            us = filter(lambda n: n.tagName == 'usenotebooksizer',
                        get_child_elems(nb))[0]
            nb.removeChild(us).unlink()
        except IndexError:
            pass
        for page in pages:
            tab = document.createElement('tab')
            obj = None
            for c in get_child_elems(page):
                if c.tagName == 'label':
                    tab.appendChild(c.firstChild)
                elif c.tagName == 'object':
                    tab.setAttribute('window', c.getAttribute('name'))
                    c.setAttribute('base', 'NotebookPane')
                    obj = c
            tabs.appendChild(tab)
            nb.replaceChild(obj, page)
        nb.insertBefore(tabs, nb.firstChild)


def fix_splitters(document):
    def issplitter(node):
        return node.getAttribute('class') == 'wxSplitterWindow'
    def ispane(node): return node.tagName == 'object'
    for sp in filter(issplitter, document.getElementsByTagName('object')):
        panes = filter(ispane, get_child_elems(sp))
        assert len(panes) <= 2, "Splitter window with more than 2 panes!"
        for i in range(len(panes)):
            e = document.createElement('window_%s' % (i+1))
            e.appendChild(document.createTextNode(
                panes[i].getAttribute('name')))
            sp.insertBefore(e, sp.firstChild)
        for orient in filter(lambda n: n.tagName == 'orientation',
                             get_child_elems(sp)):
            if orient.firstChild.data == 'vertical':
                orient.firstChild.data = 'wxVERTICAL'
            elif orient.firstChild.data == 'horizontal':
                orient.firstChild.data = 'wxHORIZONTAL'


def fix_fake_panels(document):
    def isframe(node): return node.getAttribute('class') == 'wxFrame'
    for frame in filter(isframe, document.getElementsByTagName('object')):
        for c in get_child_elems(frame):
            if c.tagName == 'object' and c.getAttribute('class') == 'wxPanel' \
               and c.getAttribute('name') == '':
                elems = get_child_elems(c)
                if len(elems) == 1 and \
                       elems[0].getAttribute('class').find('Sizer') != -1:
                    frame.replaceChild(elems[0], c)


def fix_spacers(document):
    def isspacer(node): return node.getAttribute('class') == 'spacer'
    for spacer in filter(isspacer, document.getElementsByTagName('object')):
        spacer.setAttribute('name', 'spacer')
        spacer.setAttribute('base', 'EditSpacer')
        sizeritem = document.createElement('object')
        sizeritem.setAttribute('class', 'sizeritem')
        for child in get_child_elems(spacer):
            if child.tagName == 'size':
                w, h = [s.strip() for s in child.firstChild.data.split(',')]
                width = document.createElement('width')
                width.appendChild(document.createTextNode(w))
                height = document.createElement('height')
                height.appendChild(document.createTextNode(h))
                spacer.removeChild(child).unlink()
                spacer.appendChild(width)
                spacer.appendChild(height)
            else:
                sizeritem.appendChild(spacer.removeChild(child))
        spacer.parentNode.replaceChild(sizeritem, spacer)
        sizeritem.appendChild(spacer)


def fix_toplevel_names(document):
    names = {}
    for widget in get_child_elems(document.documentElement):
        klass = widget.getAttribute('class')
        if not klass:
            continue # don't add a new 'class' attribute if it doesn't exist 
        if klass == 'wxPanel':
            widget.setAttribute('base', 'EditTopLevelPanel')
        klass_name = kn = klass.replace('wx', 'My')
        name = widget.getAttribute('name')
        i = 1
        while names.has_key(klass_name) or klass_name == name:
            klass_name = kn + str(i)
            i += 1
        widget.setAttribute('class', klass_name)


def fix_sliders(document):
    def isslider(node):
        klass = node.getAttribute('class')
        return klass == 'wxSlider' or klass == 'wxSpinCtrl'
    for slider in filter(isslider, document.getElementsByTagName('object')):
        v1, v2 = 0, 100
        for child in get_child_elems(slider):
            if child.tagName == 'min':
                v1 = child.firstChild.data.strip()
                slider.removeChild(child).unlink()
            elif child.tagName == 'max':
                v2 = child.firstChild.data.strip()
                slider.removeChild(child).unlink()
        rng = document.createElement('range')
        rng.appendChild(document.createTextNode('%s, %s' % (v1, v2)))
        slider.appendChild(rng)


def fix_encoding(filename, document):
    # first try to find the encoding of the xml doc
    import re
    enc = re.compile(r'^\s*<\?xml\s+.*(encoding\s*=\s*"(.*?)").*\?>')
    tag = re.compile(r'<.+?>')
    for line in open(filename):
        match = re.match(enc, line)
        if match is not None:
            document.documentElement.setAttribute('encoding', match.group(2))
            return
        elif re.match(tag, line) is not None:
            break
    # if it's not specified, try to find a child of the root called 'encoding':
    # I don't know why, but XRCed does this
    for child in document.documentElement.childNodes:
        if child.nodeType == child.ELEMENT_NODE and \
               child.tagName == 'encoding':
            if child.firstChild is not None and \
               child.firstChild.nodeType == child.TEXT_NODE:
                document.documentElement.setAttribute(
                    'encoding', child.firstChild.data)
            document.documentElement.removeChild(child)


def usage():
    msg = """\
usage: python %s OPTIONS <INPUT_FILE.xrc> [WXG_FILE]

OPTIONS:
  -d, --debug: debug mode, i.e. you can see the whole traceback of each error
  
If WXG_FILE is not given, it defaults to INPUT_FILE.wxg
    """ % _name
    print msg
    sys.exit(1)


def print_exception(exc):
    msg = """\
An error occurred while trying to convert the XRC file. Here's the short error
message:
\t%s\n
If you think this is a bug, or if you want to know more about the cause of the
error, run this script again in debug mode (-d switch). If you find a bug,
please report it to the mailing list (wxglade-general@lists.sourceforge.net),
or enter a bug report at the SourceForge bug tracker.

Please note that this doesn't handle ALL XRC files correctly, but only those
which already are in a format which wxGlade likes (this basically means that
every non-toplevel widget must be inside sizers, but there might be other
cases).
""" % str(exc)
    print >> sys.stderr, msg
    sys.exit(1)


def main():
    try: options, args = getopt.getopt(sys.argv[1:], "d", ['debug'])
    except getopt.GetoptError: usage()
    if not args: usage()
    input = args[0]
    try: output = args[1]
    except IndexError: output = os.path.splitext(input)[0] + '.wxg'
    if not options:
        try: convert(input, output)
        except Exception, e: # catch the exception and print a nice message
            print_exception(e)
    else: # if in debug mode, let the traceback be printed
        convert(input, output)


if __name__ == '__main__':
    _name = os.path.basename(sys.argv[0])
    main()

