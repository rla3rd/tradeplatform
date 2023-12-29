#!/usr/bin/env python
import sys
import string
from datetime import date, datetime
import psycopg2
from pysqlite2 import dbapi2 as sqlite
import socket
#import psyco
#psyco.profile()

try:
    import pygtk
    pygtk.require('2.0')
except:
    pass
try:
    import gtk, gobject
    import gtk.glade
except:
    sys.exit(1)
    
gladefile = 'oms.glade'

class Application:
    def __init__(self):
        self.user = None
        self.password = None
#        self.fixsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#        self.fixsocket.connect( ("localhost", 40000) )

class DB:
    """This is the class that provides the application settings"""
    
    def __init__(self):
        self.local = sqlite
        self.server = psycopg2
        self.local.conn = sqlite.connect( 'tpclient.db' )
        self.local.cursor = self.local.conn.cursor()
        sql = "select host, port, database, user, password from serverconnection "
        self.local.cursor.execute(sql)
        results = self.local.cursor.fetchall()
        self.remote = {}
        for row in results:
            self.remote['host'] = row[0]
            self.remote['port'] = row[1]
            self.remote['database'] = row[2]
            self.remote['user'] = row[3]
            self.remote['password'] = row[4]
            self.server.conn = psycopg2.connect( host=self.remote['host'], port=self.remote['port'], database=self.remote['database'], user=self.remote['user'], password=self.remote['password'] )
            self.server.cursor = psycopg2.connect(host=self.remote['host'], port=self.remote['port'], database=self.remote['database'], user=self.remote['user'], password=self.remote['password']).cursor()
            sql = "select code, description from fix.exec_inst"
            self.server.cursor.execute(sql)
            results = self.server.cursor.fetchall()
        

class omsGTK:
    """This is the OMS Application"""
   
    def __init__(self):
                
        #import the main window and connect the destroy event
        
        self.wTree = gtk.glade.XML( gladefile )
        self.window = self.wTree.get_widget( 'MainWindow' )
        if ( self.window ):
            self.window.connect( 'destroy', gtk.main_quit )
            
        dic = { 
                'on_imagemenuitemOrderTable_activate' : self.on_imagemenuitemOrderTable_activate
                }
        self.wTree.signal_autoconnect(dic)
        self.datecheck()
        self.window.show()
        login = LoginDialogGTK()
        loginresults = login.run()
        
    def on_imagemenuitemOrderTable_activate(self, widget):
        self.orderTable = OrderTableGTK()
        
    def datecheck(self):
        select = 'select trade_date '
        fromtable = 'from orderids '
        limit = 'limit 1 '
        sql = select + fromtable + limit
        db.local.cursor.execute( sql )
        results = db.local.cursor.fetchone()

        if results != None:
            lastdate = results[0]
        else:
            lastdate = None
            
        today = date.today()
        print lastdate
        if lastdate != None:
           lastdate = date( int( lastdate[ :4 ] ), int( lastdate[ 5 : 7 ] ), int( lastdate[ 8 : ] ) )
        if today != lastdate and lastdate != None:
            db.local.cursor.execute('delete from orderids')  
            db.local.conn.commit()
        
class LoginDialogGTK:
    """This is the OMS Login Window"""
    
    def __init__(self):
        self.wTree = gtk.glade.XML( gladefile )
        self.window = self.wTree.get_widget( 'LoginDialog' )
        self.window.set_title('OMS Login')
        self.entryUser = self.wTree.get_widget('entryUser')
        self.entryPassword = self.wTree.get_widget('entryPassword')
        
        dic = { 'on_entryUser_changed' : self.on_entryUser_changed, 
                'on_entryPassword_changed' : self.on_entryPassword_changed,
                'on_buttonQuit_clicked' : self.on_buttonQuit_clicked, 
                'on_buttonLogin_clicked' : self.on_buttonLogin_clicked
                }
        self.wTree.signal_autoconnect(dic)
        self.entryUser.set_text('')
        self.entryPassword.set_text('')
                
    def on_entryUser_changed(self, widget):
        app.user = self.entryUser.get_text()
        
    def on_entryPassword_changed(self, widget):
        app.password = self.entryPassword.get_text()
    
    def on_buttonQuit_clicked(self, widget):
        sys.exit(0)
        
    def on_buttonLogin_clicked(self, widget):
        
        if app.user == '':
            return
        
        select = 'select id '
        fromtable = 'from fix.client_ids '
        where = "where id = \'%s\' " % ( app.user )

        sql =  select + fromtable + where
        
        print sql
        
        db.server.cursor.execute( sql )
        results = db.server.cursor.fetchone()
        self.lastuser = results[0]

        if app.user != self.lastuser:
            sql = 'delete from fix.currentuser'
            db.local.cursor.execute( sql )
            sql = 'insert into fix.currentuser(user_id) values(\'%s\') ' % ( app.user )
            db.local.cursor.execute( sql )
            sql = 'delete from fix.orderids'
            db.local.cursor.execute( sql )
            db.local.conn.commit()
        
    def run(self):
        self.result = self.window.run()
        self.window.destroy()
        return self.result
        
class OrderTableGTK:
    """This is the Order Table Window"""
    
    def __init__(self):
        
        #import the main window and connect the destroy event
        self.wTree = gtk.glade.XML( gladefile )
        self.window = self.wTree.get_widget( 'OrderTableWindow' )
        self.viewOrderTable = self.wTree.get_widget('treeviewOrderTable')
        self.viewOrderTableColumn = gtk.TreeViewColumn('Parent ID', gtk.CellRendererText(), text=0)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('MMID', gtk.CellRendererText(), text=1)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Clordid', gtk.CellRendererText(), text=2)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Symbol', gtk.CellRendererText(), text=3)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Exchange', gtk.CellRendererText(), text=4)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('TIF', gtk.CellRendererText(), text=5)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Side', gtk.CellRendererText(), text=6)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Qty', gtk.CellRendererText(), text=7)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Show Qty', gtk.CellRendererText(), text=8)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Min Qty', gtk.CellRendererText(), text=9)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Ord Type', gtk.CellRendererText(), text=10)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Lmt Px', gtk.CellRendererText(), text=11)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Stop Px', gtk.CellRendererText(), text=12)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Peg Offset', gtk.CellRendererText(), text=13)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Disc Inst', gtk.CellRendererText(), text=14)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Disc Offset', gtk.CellRendererText(), text=15)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Exec Inst', gtk.CellRendererText(), text=16)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Algo Inst', gtk.CellRendererText(), text=17)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Status', gtk.CellRendererText(), text=18)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Msg Text', gtk.CellRendererText(), text=19)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Cum Qty', gtk.CellRendererText(), text=20)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Avg Px', gtk.CellRendererText(), text=21)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Order Type', gtk.CellRendererText(), text=22)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Exec Type', gtk.CellRendererText(), text=23)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Msg Time', gtk.CellRendererText(), text=24)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Exec ID', gtk.CellRendererText(), text=25)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Child ID', gtk.CellRendererText(), text=26)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.viewOrderTableColumn = gtk.TreeViewColumn('Order Date', gtk.CellRendererText(), text=27)
        self.viewOrderTable.append_column(self.viewOrderTableColumn)
        self.refreshOrderTable()
        
                    
        dic = { 'on_MainWindow_destroy' : gtk.main_quit,
                'on_newOrder_clicked': self.on_newOrder_clicked, 
                'on_cxlOrder_clicked': self.on_cxlOrder_clicked,
                'on_cxlReplaceOrder_clicked': self.on_cxlReplaceOrder_clicked, 
                'on_treeviewOrderTable_cursor_changed' : self.on_treeviewOrderTable_cursor_changed,
                'on_statusOrder_clicked': self.on_statusOrder_clicked }
                
        self.wTree.signal_autoconnect(dic)
            
        self.window.show()
        
    def refreshOrderTable(self):
        self.viewOrderTableValues = gtk.ListStore(str, str, str, str, str, 
                                                  str, str, int, int, int, 
                                                  str, float, float, float, str, 
                                                  float, str, str, str, str,
                                                  int, float, str, str, str,
                                                  str, int, str)
        self.viewOrderTableValues.clear()
        select = "select parent_id, "
        select += "mmid, "
        select += "clordid, "
        select += "symbol, "
        select += "exchange, "
        select += "tif, "
        select += "side, "
        select += "quantity, "
        select += "show_qty, "
        select += "min_qty, "
        select += "ordtype, "
        select += "limit_price, "
        select += "stop_price, "
        select += "peg_offset, "
        select += "disc_inst, "
        select += "disc_offset, "
        select += "exec_inst, "
        select += "algo_inst, "
        select += "order_status, "
        select += "msg_text, "
        select += "cum_qty, "
        select += "avg_price, "
        select += "order_id, "
        select += "exec_trans_type, "
        select += "msg_time, "
        select += "exec_id, "
        select += "child_id, "
        select += "order_date "
        fromtable = "from fix.orders "
        where = "where clordid like \'%s\' " % ( app.user+'%' )
        
        sql = select + fromtable + where
        
        print sql
        
        db.server.cursor.execute(sql)
        
        results = db.server.cursor.fetchall()
        
        for row in results:
            for i in range( len(row) ):
                if row[i] == None:
                    row[i] = ''
            self.viewOrderTableValues.append( (row[0], row[1], row[2], row[3], row[4],
                                               row[5], row[6], row[7], row[8], row[9],
                                               row[10], row[11], row[12], row[13], row[14],
                                               row[15], row[16], row[17], row[18], row[19],
                                               row[20], row[21], row[22], row[23], row[24],
                                              row[25], row[26], row[27]) )
                                            
        self.viewOrderTable.set_model(self.viewOrderTableValues)
        self.viewOrderTable.show()
        
    def on_treeviewOrderTable_cursor_changed(self, widget):
        model, iter = self.viewOrderTable.get_selection().get_selected()
        if not iter: return
        self.order = OrderDetails()
        self.order.broker = model.get_value(iter, 1)
        self.order.origClOrdID = model.get_value(iter, 2)
        self.order.clOrdID = None
        self.order.symbol = model.get_value(iter, 3)
        self.order.exchange = model.get_value(iter, 4)
        self.order.TIF = model.get_value(iter, 5)
        self.order.side = model.get_value(iter, 6)
        self.order.quantity = model.get_value(iter, 7)
        self.order.showQty = model.get_value(iter, 8)
        self.order.minQty = model.get_value(iter, 9)
        self.order.orderType = model.get_value(iter, 10)
        self.order.limitPx = model.get_value(iter, 11)
        self.order.stopPx = model.get_value(iter, 12)
        self.order.pegOffset = model.get_value(iter, 13)
        self.order.discInst = model.get_value(iter, 14)
        self.order.discOffset = model.get_value(iter, 15)
        self.order.execInst = model.get_value(iter, 16)
        self.order.algoInst = model.get_value(iter, 17)
        print model.get_value(iter, 2)
        
    def on_newOrder_clicked(self, widget):
        widget = OrderDialogGTK()
        result, newOrder = widget.run()
    def on_cxlOrder_clicked(self, widget):
        widget = OrderDialogGTK(requestType=1, order=self.order)
        result, newOrder = widget.run()
    def on_cxlReplaceOrder_clicked(self, widget):
        widget = OrderDialogGTK(requestType=2, order=self.order)
        result, newOrder = widget.run()
            
    def on_statusOrder_clicked(self, widget):
        pass
        
class OrderDialogGTK:
    """This is the Order Entry Dialog Window"""
    
    def __init__(self, requestType=0, order=None):
        
        self.order = order
        if self.order == None:
            self.clOrdID = None
            self.order = OrderDetails()
        
        self.order.requestType = requestType
        self.order.parentID = 1
        self.wTree = gtk.glade.XML( gladefile )
        dic = { 'on_entrySymbol_changed': self.on_entrySymbol_changed,
                'on_buttonCxl_clicked' : self.on_buttonCxl_clicked, 
                'on_buttonSend_clicked' : self.on_buttonSend_clicked,
                'on_comboboxBroker_changed' : self.on_comboboxBroker_changed,
                'on_comboboxAlgoInst_changed' : self.on_comboboxAlgoInst_changed
                }
        self.wTree.signal_autoconnect(dic)
        
        self.window = self.wTree.get_widget( 'OrderDialog' )
        if self.order.requestType == 1:
            self.window.set_title('Cancel Order')
        elif self.order.requestType == 2:
            self.window.set_title('Replace Order')
        self.entrySymbol = self.wTree.get_widget('entrySymbol')
        self.entryTIF = self.wTree.get_widget('comboboxTIF')
        self.entrySide = self.wTree.get_widget('comboboxSide')
        self.entryExchange = self.wTree.get_widget('comboboxExchange')
        self.entryQuantity = self.wTree.get_widget('spinbuttonQuantity')
        self.entryShowQty = self.wTree.get_widget('spinbuttonShowQuantity')
        self.entryMinQty = self.wTree.get_widget('spinbuttonMinimumQuantity')
        self.entryOrderType = self.wTree.get_widget('comboboxOrderType')
        self.entryLimitPx = self.wTree.get_widget('spinbuttonLimitPrice')
        self.entryStopPx = self.wTree.get_widget('spinbuttonStopPrice')
        self.entryPegOffset = self.wTree.get_widget('spinbuttonPegOffset')
        self.entryDiscInst = self.wTree.get_widget('comboboxDiscInst')
        self.entryDiscOffset = self.wTree.get_widget('spinbuttonDiscOffset')
        self.entryBroker = self.wTree.get_widget('comboboxBroker')
        self.entryExecInst = self.wTree.get_widget('treeviewExecInst')
        self.entryExecInstValues = gtk.ListStore(bool, str, str)
        sql = "select code, description from exec_inst"
        db.local.cursor.execute(sql)
        results = db.local.cursor.fetchall()
        for row in results:
            self.entryExecInstValues.append( (False, row[0], row[1]) )
        self.entryExecInst.set_model(self.entryExecInstValues)    
        self.entryExecInstRenderer = gtk.CellRendererToggle()
        self.entryExecInstRenderer.set_property('activatable', True)
        self.entryExecInstRenderer.connect('toggled', self.on_entryExecInst_Toggled, self.entryExecInstValues)
        self.entryExecInstColumn = gtk.TreeViewColumn('Selected', self.entryExecInstRenderer)
        self.entryExecInstColumn.add_attribute( self.entryExecInstRenderer, "active", 0)
        self.entryExecInst.append_column(self.entryExecInstColumn)
        self.entryExecInstColumn = gtk.TreeViewColumn('Description', gtk.CellRendererText(), text=2)
        self.entryExecInst.append_column(self.entryExecInstColumn)
        
        self.entryAlgoInst = self.wTree.get_widget('comboboxAlgoInst')
        self.entryAlgoParams = self.wTree.get_widget('treeviewAlgoParams')
        self.entryAlgoParamsValues = gtk.ListStore(str, str)
        self.entryAlgoParams.set_model(self.entryAlgoParamsValues)
        self.entryAlgoParamsColumn = gtk.TreeViewColumn('Parameter', gtk.CellRendererText(), text=0)
        self.entryAlgoParams.append_column(self.entryAlgoParamsColumn)
        self.entryClOrdID = None
        
        if self.order.origClOrdID == None:
            self.entrySymbol.set_text(self.order.symbol)
            self.entryTIF.set_active(self.order.TIF)
            self.entrySide.set_active(self.order.side)
            self.entryExchange.set_active(self.order.exchange)
            self.entryQuantity.set_value(self.order.quantity)
            self.entryShowQty.set_value(self.order.showQty)
            self.entryMinQty.set_value(self.order.minQty)
            self.entryOrderType.set_active(self.order.orderType)
            self.entryLimitPx.set_value(self.order.limitPx)
            self.entryStopPx.set_value(self.order.stopPx)
            self.entryPegOffset.set_value(self.order.pegOffset)
            self.entryDiscInst.set_active(self.order.discInst)
            self.entryDiscOffset.set_value(self.order.discOffset)
            self.entryBroker.set_active(self.order.broker)
            self.entryAlgoInst.set_active(self.order.algoInst)
            
        else:
            print self.order.broker
            self.entryOrigClOrdID = self.order.origClOrdID
            self.entrySymbol.set_text(self.order.symbol)
            self.setActiveEntryTIF(self.order.TIF)
            self.setActiveEntrySide(self.order.side)
            self.setActiveEntryExchange(self.order.broker, self.order.exchange)
            self.entryQuantity.set_value(self.order.quantity)
            self.entryShowQty.set_value(self.order.showQty)
            self.entryMinQty.set_value(self.order.minQty)
            self.setActiveEntryOrderType(self.order.orderType)
            self.entryLimitPx.set_value(self.order.limitPx)
            self.entryStopPx.set_value(self.order.stopPx)
            self.entryPegOffset.set_value(self.order.pegOffset)
            self.entryDiscInst.set_active(int(self.order.discInst))
            self.entryDiscOffset.set_value(self.order.discOffset)
            self.setActiveEntryBroker(self.order.broker)
            self.entryAlgoInst.set_active(0)
            
    def setActiveEntryTIF(self, text):
        if text == 'DAY':
            self.entryTIF.set_active(0)
        elif text == 'IOC':
            self.entryTIF.set_active(1)
        elif text == 'FOK':
            self.entryTIF.set_active(2)
        elif text == 'OPG':
            self.entryTIF.set_active(3)
        elif text == 'GTC':
            self.entryTIF.set_active(4)
            
    def setActiveEntrySide(self, text):
        if text == 'BUY':
            self.entrySide.set_active(0)
        elif text == 'SELL':
            self.entrySide.set_active(1)
        elif text == 'SHORT':
            self.entrySide.set_active(2)
        elif text == 'BUYMINUS':
            self.entrySide.set_active(3)
        elif text == 'SELLPLUS':
            self.entrySide.set_active(4)
            
    def setActiveEntryOrderType(self, text):
        if text == 'SWP':
            self.entryOrderType.set_active(0)
        elif text == 'MKT':
            self.entryOrderType.set_active(1)
        elif text == 'LMT':
            self.entryOrderType.set_active(2)
        elif text == 'SLMT':
            self.entryOrderType.set_active(3)
        elif text == 'PEG':
            self.entryOrderType.set_active(4)
        elif text == 'MOC':
            self.entryOrderType.set_active(5)
        elif text == 'LOC':
            self.entryOrderType.set_active(6)
            
    def setActiveEntryBroker(self, text):
        if text == 'REDI':
            self.entryBroker.set_active(0)
        elif text == 'GSAT':
            self.entryBroker.set_active(1)
        
    def newOrderID(self):
        db.local.cursor.execute('insert into fix.orderids(trade_date) select date(\'now\')')
        db.local.conn.commit()
        db.local.cursor.execute('select trade_date, max(id) from fix.orderids limit 1')
        db.local.conn.commit()
        results = db.local.cursor.fetchone()
        self.date = results[0]
        self.id = results[1]
        orderID = '%s-%s-%s' % (app.user, self.date, self.id)
        db.local.cursor.execute('update fix.orderids set clordid = \'%s\' where id=(select max(id) from orderids) ' % ( orderID ))
        db.local.conn.commit()
        return orderID    
            
    def on_entryAlgoParamsValues_edited(self, cell, path, new_text, user_data):
        liststore, column = user_data
        print liststore, column, path, new_text
        self.order.algoInst = '%s ' % ( self.activeAlgoInstString )
        liststore[path][column] = new_text
        for i in range( len(liststore) ):
            if liststore[i][column] =='':
                liststore[i][column] = '0'
            print liststore[i][column]    
            self.order.algoInst += '%s ' % ( liststore[i][column] )
        self.order.algoInst = string.strip( self.order.algoInst )
        print self.order.algoInst
        
    def on_entryExecInst_Toggled(self, cell, path, model):
        model[path][0] = not model[path][0]
        self.order.execInst = ''
        for i in range(len(model)):
            if model[i][0]==True:
                self.order.execInst += '%s ' % ( model[i][1] )
        self.order.execInst = string.strip( self.order.execInst )
        
    def get_active_text(self, combobox):
      model = combobox.get_model()
      active = combobox.get_active()
      if active < 0:
          return None
      return model[active][0]

    def set_model_from_list (self, cb, items):
        """Setup a ComboBox or ComboBoxEntry based on a list of strings."""           
        model = gtk.ListStore(str)
        for row in items:
            model.append([row])
        cb.set_model(model)
        if type(cb) == gtk.ComboBoxEntry:
            cb.set_text_column(0)
        elif type(cb) == gtk.ComboBox:
            cell = gtk.CellRendererText()
            cb.pack_start(cell, True)
            cb.add_attribute(cell, 'text', 0)
        
    def on_entrySymbol_changed(self, widget):
        new_text = widget.get_text().upper()
        widget.set_text(new_text)
    
    def on_buttonCxl_clicked(self, widget):
        self.window.destroy()
        self.order = None
        print self.order
        
    def on_comboboxBroker_changed(self, widget):
        self.activeBrokerString = self.get_active_text(self.entryBroker)
        self.entryExchange.clear()
        self.entryExchangeValues = []
        sql = "select exchange, broker_code, orderby_id from exchange_codes where broker=\'%s\' order by orderby_id" % ( self.activeBrokerString )
        db.local.cursor.execute(sql)
        results = db.local.cursor.fetchall()
        for row in results:
            self.entryExchangeValues.append( row[0] )
        self.set_model_from_list( self.entryExchange, self.entryExchangeValues )
        self.entryExchange.set_active(0)
        
    def getExchangeCode(self, broker, exchangeText):
        sql = "select broker_code from exchange_codes where broker=\'%s\' and exchange=\'%s\' " % (broker, exchangeText)
        db.local.cursor.execute(sql)
        results = db.local.cursor.fetchone()
        return results[0][0]
    
    def setActiveEntryExchange(self, broker, exchangeCode):
        sql = "select orderby_id from exchange_codes where broker=\'%s\' and broker_code=\'%s\' " % (broker, exchangeCode)
        db.local.cursor.execute(sql)
        results = db.local.cursor.fetchone()
        return results[0][0]
    
    def on_comboboxAlgoInst_changed(self, widget):
        self.activeAlgoInstString = self.get_active_text(self.entryAlgoInst)
        self.entryAlgoParamsValues = gtk.ListStore(str, str)
        self.entryAlgoParamsValues.clear()
        sql = "select parameter, required from algo_parameters where algo=\'%s\' and algo_broker=\'%s\' order by param_id " % ( self.activeAlgoInstString, self.activeBrokerString )
        db.local.cursor.execute(sql)
        results = db.local.cursor.fetchall()
        for row in results:
            self.entryAlgoParamsValues.append( (row[0], '') )
        self.entryAlgoParams.set_model(self.entryAlgoParamsValues)
        try:
            self.entryAlgoParamsColumn = self.entryAlgoParams.get_column(1)
            self.entryAlgoParams.remove_column(self.entryAlgoParamsColumn)
        except:
            pass
        self.entryParamsValuesRenderer = gtk.CellRendererText()
        self.entryParamsValuesRenderer.set_property('editable', True)
        self.entryParamsValuesRenderer.connect('edited', self.on_entryAlgoParamsValues_edited, (self.entryAlgoParamsValues, 1) )
        self.entryAlgoParamsColumn = gtk.TreeViewColumn('Value', self.entryParamsValuesRenderer, text=1, editable=2)
        self.entryAlgoParamsColumn.set_flags(gtk.CAN_FOCUS)
        self.entryAlgoParams.append_column(self.entryAlgoParamsColumn)
        
                   
    def on_buttonSend_clicked(self, widget):
        self.entryClOrdID = self.newOrderID()
        self.order.clOrdID = self.entryClOrdID
        self.order.symbol = self.entrySymbol.get_text()
        if self.order.symbol == '':
            return
        self.order.side = self.get_active_text(self.entrySide).upper()
        self.order.broker = self.get_active_text(self.entryBroker)
        self.order.exchange = self.getExchangeCode( self.order.broker, self.get_active_text(self.entryExchange) )
        self.order.quantity = self.entryQuantity.get_value()
        self.order.showQty = self.entryShowQty.get_value()
        self.order.minQty = self.entryMinQty.get_value()
        self.order.orderType = self.get_active_text(self.entryOrderType)
        self.order.limitPx = self.entryLimitPx.get_value()
        self.order.stopPx = self.entryStopPx.get_value()
        self.order.pegOffset = self.entryPegOffset.get_value()
        self.order.discInst = self.entryDiscInst.get_active()
        self.order.discOffset = self.entryDiscOffset.get_value()
        self.order.TIF = self.get_active_text(self.entryTIF)
        if self.order.execInst == '':
            self.order.execInst = '5'
    
        self.window.destroy()
        if self.order.requestType == 0:
            self.order.message = '%s|%s|%s|' % (self.order.requestType, self.order.parentID, self.order.broker )
            self.order.message += '%s|%s|%s|%s|' % ( self.order.clOrdID, self.order.symbol, self.order.exchange, self.order.TIF )
            self.order.message += '%s|%s|%s|%s|' % ( self.order.side, self.order.quantity, self.order.showQty, self.order.minQty )
            self.order.message += '%s|%s|%s|%s|' % ( self.order.orderType, self.order.limitPx, self.order.stopPx, self.order.pegOffset ) 
            self.order.message += '%s|%s|%s|%s' % ( self.order.discInst, self.order.discOffset, self.order.execInst, self.order.algoInst )
        elif self.order.requestType == 1:
            self.order.message = '%s|%s|%s|' % (self.order.requestType, self.order.parentID, self.order.broker )
            self.order.message += '%s|%s|%s|%s|' % (self.order.origClOrdID, self.order.clOrdID, self.order.symbol, self.order.exchange)
            self.order.message += '%s|%s|' % (self.order.side, self.order.quantity)
        elif self.order.requestType == 2:
            self.order.message = '%s|%s|%s|' % (self.order.requestType, self.order.parentID, self.order.broker )
            self.order.message += '%s|%s|%s|%s|%s|' % ( self.order.origClOrdID, self.order.clOrdID, self.order.symbol, self.order.exchange, self.order.TIF )
            self.order.message += '%s|%s|%s|%s|' % ( self.order.side, self.order.quantity, self.order.showQty, self.order.minQty )
            self.order.message += '%s|%s|%s|%s|' % ( self.order.orderType, self.order.limitPx, self.order.stopPx, self.order.pegOffset ) 
            self.order.message += '%s|%s|%s|%s' % ( self.order.discInst, self.order.discOffset, self.order.execInst, self.order.algoInst )
        app.fixsocket.sendall(self.order.message)
        print self.order.message
        return
           
    def run(self):
        self.result = self.window.run()
        print self.order
        return self.result, self.order
           
class CxlOrderDialogGTK:
    """This is the Order Entry Dialog Window"""
    
    def __init__(self):
        
        #import the main window and connect the destroy event
        self.wTree = gtk.glade.XML( gladefile )
        self.window = self.wTree.get_widget( 'OrderDialog' )
        if ( self.window ):
            self.window.connect( 'destroy', gtk.main_quit )
            
class CxlReplaceOrderDialogGTK:
    """This is the Order Entry Dialog Window"""
    
    def __init__(self):
        
        #import the main window and connect the destroy event
        self.wTree = gtk.glade.XML( gladefile )
        self.window = self.wTree.get_widget( 'OrderDialog' )
        if ( self.window ):
            self.window.connect( 'destroy', gtk.main_quit )
            
class OrderDetails:
    """This class stores all new order information"""
    
    def __init__(self):
        self.requestType = None
        self.origClOrdID = None
        self.clOrdID = None
        self.symbol = ''
        self.TIF = 0
        self.side = 0
        self.exchange = 0
        self.quantity = 100
        self.showQty = 100
        self.minQty = 0
        self.orderType = 2
        self.limitPx = 0
        self.stopPx = 0
        self.pegOffset = 0
        self.discInst = 0
        self.discOffset = 0
        self.broker = 0
        self.execInst = ''
        self.algoInst = 0
            
if __name__ == '__main__':
    app = Application()
    db = DB()
    oms = omsGTK()
    gtk.main()
