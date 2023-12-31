# codegen.py: code generator functions for wxRadioBox objects
# $Id: codegen.py,v 1.12 2005/05/06 21:48:19 agriggio Exp $
#
# Copyright (c) 2002-2005 Alberto Griggio <agriggio@users.sourceforge.net>
# License: MIT (see license.txt)
# THIS PROGRAM COMES WITH NO WARRANTY

import common
from ChoicesCodeHandler import *


class PythonCodeGenerator:
    def get_code(self, obj):
        pygen = common.code_writers['python']
        prop = obj.properties
        id_name, id = pygen.generate_code_id(obj) 
        label = pygen.quote_str(prop.get('label', ''))
        choices = prop.get('choices', [])
        major_dim = prop.get('dimension', '0')
        if not obj.parent.is_toplevel: parent = 'self.%s' % obj.parent.name
        else: parent = 'self'
        style = prop.get("style")
        if style: style = ", style=%s" % pygen.cn_f(style)
        else: style = ''
        init = []
        if id_name: init.append(id_name)
        choices = ', '.join([pygen.quote_str(c) for c in choices])
        klass = obj.klass
        if klass == obj.base: klass = pygen.cn(klass)
        init.append('self.%s = %s(%s, %s, %s, choices=[%s], '
                    'majorDimension=%s%s)\n' % (obj.name, klass,
                                                parent, id, label,
                                                choices, major_dim, style))
        props_buf = pygen.generate_common_properties(obj)
        selection = prop.get('selection')
        if selection is not None:
            props_buf.append('self.%s.SetSelection(%s)\n' %
                             (obj.name, selection))
        return init, props_buf, []

# end of class PythonCodeGenerator


def xrc_code_generator(obj):
    xrcgen = common.code_writers['XRC']
    class RadioBoxXrcObject(xrcgen.DefaultXrcObject):
        def write_property(self, name, val, outfile, tabs):
            if name == 'choices':
                xrc_write_choices_property(self, outfile, tabs)
            else:
                xrcgen.DefaultXrcObject.write_property(self, name, val,
                                                       outfile, tabs)

    # end of class RadioBoxXrcObject

    return RadioBoxXrcObject(obj)


class CppCodeGenerator:
    def get_code(self, obj):
        """\
        generates the C++ code for wxRadioBox objects
        """
        cppgen = common.code_writers['C++']
        prop = obj.properties
        id_name, id = cppgen.generate_code_id(obj)
        if id_name: ids = [ id_name ]
        else: ids = []
        choices = prop.get('choices', [])
        major_dim = prop.get('dimension', '0')
        if not obj.parent.is_toplevel: parent = '%s' % obj.parent.name
        else: parent = 'this'
        number = len(choices)
        ch_arr = '{\n        %s\n    };\n' % \
                 ',\n        '.join([cppgen.quote_str(c) for c in choices])
        label = cppgen.quote_str(prop.get('label', ''))
        style = prop.get("style", "0")
        init = []
        init.append('const wxString %s_choices[] = %s' % (obj.name, ch_arr))
        init.append('%s = new %s(%s, %s, %s, wxDefaultPosition, '
                    'wxDefaultSize, %s, %s_choices, %s, %s);\n' % \
                    (obj.name, obj.klass, parent, id, label, number, obj.name,
                     major_dim, style))
        props_buf = cppgen.generate_common_properties(obj)
        selection = prop.get('selection')
        if selection is not None:
            props_buf.append('%s->SetSelection(%s);\n' % (obj.name, selection))
        return init, ids, props_buf, []

# end of class CppCodeGenerator


def initialize():
    common.class_names['EditRadioBox'] = 'wxRadioBox'

    pygen = common.code_writers.get("python")
    if pygen:
        pygen.add_widget_handler('wxRadioBox', PythonCodeGenerator())
        pygen.add_property_handler('choices', ChoicesCodeHandler)
    xrcgen = common.code_writers.get("XRC")
    if xrcgen:
        xrcgen.add_widget_handler('wxRadioBox', xrc_code_generator)
        xrcgen.add_property_handler('choices', ChoicesCodeHandler)
    cppgen = common.code_writers.get('C++')
    if cppgen:
        cppgen.add_widget_handler('wxRadioBox', CppCodeGenerator())
        cppgen.add_property_handler('choices', ChoicesCodeHandler)
