#!/usr/bin/env python
# -*- coding: ISO-8859-1 -*-
# generated by wxGlade 0.4 on Sat Jan 21 01:25:28 2006

import wx

def _(x): return x

class RunTerminalDialog(wx.Dialog):
    def __init__(self, fileName, runPreviousArguments, runPreviousInspect, runPreviousExit, *args, **kwds):
        #todo: replace choices = [] with choices = runPreviousArguments
        # begin wxGlade: RunTerminalDialog.__init__
        kwds["style"] = wx.DEFAULT_DIALOG_STYLE
        wx.Dialog.__init__(self, *args, **kwds)
        self.fileName = wx.StaticText(self, -1, _("File"))
        self.argumentsLabel = wx.StaticText(self, -1, _("Arguments"))
        self.arguments = wx.ComboBox(self, -1, choices=[], style=wx.CB_DROPDOWN)
        self.inspect = wx.CheckBox(self, -1, _("Inspect interactively after running script"))
        self.exit = wx.CheckBox(self, -1, _("Exit terminal after running script"))
        self.label = wx.StaticText(self, -1, _("Use Edit>Execute to run code snippets in shell"))
        self.cancel = wx.Button(self, wx.ID_CANCEL, _("Cancel"))
        self.ok = wx.Button(self, wx.ID_OK, _("Run"))

        self.__set_properties()
        self.__do_layout()
        # end wxGlade
        self.fileName.SetLabel("File name: "+fileName)
        self.inspect.SetValue(runPreviousInspect)
        self.exit.SetValue(runPreviousExit)

    def __set_properties(self):
        #todo: comment out self.arguments.SetSelection(-1)
        # begin wxGlade: RunTerminalDialog.__set_properties
        self.SetTitle(_("Stani's Python Editor - Run"))
        self.label.Enable(False)
        self.ok.SetDefault()
        # end wxGlade

    def __do_layout(self):
        # begin wxGlade: RunTerminalDialog.__do_layout
        sizer_1 = wx.BoxSizer(wx.VERTICAL)
        sizer_3 = wx.BoxSizer(wx.HORIZONTAL)
        sizer_2 = wx.BoxSizer(wx.HORIZONTAL)
        sizer_1.Add(self.fileName, 1, wx.ALL, 4)
        sizer_2.Add(self.argumentsLabel, 0, wx.ALL|wx.ALIGN_CENTER_VERTICAL, 4)
        sizer_2.Add(self.arguments, 1, wx.ALL|wx.ALIGN_CENTER_VERTICAL, 4)
        sizer_1.Add(sizer_2, 1, wx.EXPAND, 0)
        sizer_1.Add(self.inspect, 0, wx.ALL, 4)
        sizer_1.Add(self.exit, 1, wx.ALL, 4)
        sizer_3.Add(self.label, 1, wx.ALL|wx.ALIGN_CENTER_VERTICAL, 4)
        sizer_3.Add(self.cancel, 0, wx.ALL|wx.ALIGN_CENTER_VERTICAL, 4)
        sizer_3.Add(self.ok, 0, wx.ALL|wx.ALIGN_CENTER_VERTICAL, 4)
        sizer_1.Add(sizer_3, 1, wx.EXPAND, 0)
        self.SetSizer(sizer_1)
        sizer_1.Fit(self)
        self.Layout()
        # end wxGlade

# end of class RunTerminalDialog


class MyApp(wx.App):
    def OnInit(self):
        wx.InitAllImageHandlers()
        dialog = RunTerminalDialog(None, -1, "")
        self.SetTopWindow(dialog)
        dialog.Show()
        return 1

# end of class MyApp

if __name__ == "__main__":
    import gettext
    gettext.install("app") # replace with the appropriate catalog name

    app = MyApp(0)
    app.MainLoop()
