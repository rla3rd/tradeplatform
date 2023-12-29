#!/usr/bin/python
import sys
import socket
import time
import datetime
import pytz
import smtplib
import psycopg2, psycopg2.extensions, select
from pysqlite2 import dbapi2 as sqlite
import psyco
psyco.profile()

gmt = pytz.timezone('GMT')


conn = psycopg2.connect( host='dbcrunch01', port=5432, database='tradeplatform', user='tradeplatform', password='tradeplatform')
conn.set_isolation_level(psycopg2.extensions.ISOLATION_LEVEL_AUTOCOMMIT)
cursor = conn.cursor()            

class DB:
    """This is the class that provides the application settings"""
    
    def __init__(self):
        self.local = sqlite
        self.local.conn = sqlite.connect( 'tpclient.db' )
        self.local.cursor = self.local.conn.cursor()
        
class QuoteClient:
    def __init__(self):
        self.quote = Quote()
        
    def reqMktData( self, symbol ):
        self.quote.symbol = symbol
        requestType = 'reqMktData'
        line = '%s|%s|%s' % ( requestType, symbol, '\0')
        self.quotesocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.quotesocket.connect( ("localhost", 8007) )
        self.quotesocket.sendall(line)
        #requestType = 'cancelMktData'
        #line = '%s|%s|%s' % ( requestType, symbol, delimiter)
        #self.sendLine(line)
        
    def reqHistMktData( self, symbol, htime ):
        self.quote.symbol = symbol
        htimestring = htime.strftime('%Y%m%d %H:%M:%S')
        requestType = 'reqHistMktData'
        line = '%s|%s|%s|%s' % ( requestType, symbol, htimestring, '\0')
        self.quotesocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.quotesocket.connect( ("localhost", 8007) )
        print line
        self.quotesocket.sendall(line)
        line = self.quotesocket.recv(1024)
        self.lineReceived(line)
        return self.quote
        
    def cxlMktData( self, symbol ):
        requestType = 'cancelMktData'
        line = '%s|%s|%s' % ( requestType, symbol, '\0')
        self.quotesocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.quotesocket.connect( ('localhost', 8007) )
        self.quotesocket.sendall(line)
        
    def insideQuote( self, symbol ):
        self.quote.symbol = symbol
        requestType = 'insideQuote'
        line = '%s|%s|%s' % ( requestType, symbol, '\0')
        print line
        self.quotesocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.quotesocket.connect( ('localhost', 8007) )
        self.quotesocket.sendall(line)
        line = self.quotesocket.recv(1024)
        self.lineReceived(line)
        return self.quote
        
    def lineReceived( self, line):
        line = line.replace('\0', '')
        data = line.split('|')
        print data
        if data[0] == 'insideQuote':
            try:
                self.quote.symbol = data[1]
                self.quote.bidSize = int(data[2])
                self.quote.bidPrice = float(data[3])
                self.quote.askPrice = float(data[4])
                self.quote.askSize = int(data[5])
            except ValueError:
                self.quote.symbol = data[1]
                self.quote.bidSize = 0
                self.quote.bidPrice = 0
                self.quote.askPrice = 0
                self.quote.askSize = 0
                
        elif data[0] == 'reqHistMktData':
            try:
                self.quote.symbol = data[1]
                self.quote.histBidSize = int(data[2])
                self.quote.histBidPrice = float(data[3])
                self.quote.histAskPrice = float(data[4])
                self.quote.histAskSize = int(data[5])
            except ValueError:
                self.quote.symbol = data[1]
                self.quote.histBidSize = 0
                self.quote.histBidPrice = 0
                self.quote.histAskPrice = 0
                self.quote.histAskSize = 0
       
##        print 'Symbol: %s' % self.quote.symbol
##        print 'Bid: %s %s' % ( self.quote.bidPrice, self.quote.bidSize )
##        print 'Ask: %s %s' % ( self.quote.askPrice, self.quote.askSize )
        
class FixClient:
    def __init__(self):
        self.fixsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.fixsocket.connect( ("localhost", 30000) )
        self.order = OrderDetails()
        
    def sendMessage(self):
        if self.order.requestType == 0:
            self.order.message = '%s|%s|%s|' % (self.order.requestType, self.order.parentID, self.order.broker )
            self.order.message += '%s|%s|%s|%s|' % ( self.order.clOrdID, self.order.symbol, self.order.exchange, self.order.TIF )
            self.order.message += '%s|%s|%s|%s|' % ( self.order.side, self.order.quantity, self.order.showQty, self.order.minQty )
            self.order.message += '%s|%s|%s|%s|' % ( self.order.ordType, self.order.limitPx, self.order.stopPx, self.order.pegOffset ) 
            self.order.message += '%s|%s|%s|%s' % ( self.order.discInst, self.order.discOffset, self.order.execInst, self.order.algoInst )
        elif self.order.requestType == 1:
            self.order.message = '%s|%s|%s|' % (self.order.requestType, self.order.parentID, self.order.broker )
            self.order.message += '%s|%s|%s|%s|' % (self.order.origClOrdID, self.order.clOrdID, self.order.symbol, self.order.exchange)
            self.order.message += '%s|%s|' % (self.order.side, self.order.quantity)
        elif self.order.requestType == 2:
            self.order.message = '%s|%s|%s|' % (self.order.requestType, self.order.parentID, self.order.broker )
            self.order.message += '%s|%s|%s|%s|%s|' % ( self.order.origClOrdID, self.order.clOrdID, self.order.symbol, self.order.exchange, self.order.TIF )
            self.order.message += '%s|%s|%s|%s|' % ( self.order.side, self.order.quantity, self.order.showQty, self.order.minQty )
            self.order.message += '%s|%s|%s|%s|' % ( self.order.ordType, self.order.limitPx, self.order.stopPx, self.order.pegOffset ) 
            self.order.message += '%s|%s|%s|%s' % ( self.order.discInst, self.order.discOffset, self.order.execInst, self.order.algoInst )
        self.fixsocket.sendall(self.order.message)
        print self.order.message
        return
        
class TradeSignalsClient:
    
    def __init__(self):
        self.quoteClient = QuoteClient()
        self.client = Client()
        self.fixClient = FixClient()
        self.fixBuyOrders = []
        self.fixSellOrders = []
        
    def priceActionCheck(self, dailyvalue, cp, hp):
        if dailyvalue <= 500000:
            threshhold = 0.03
        elif dailyvalue <= 2000000:
            threshhold = 0.03
        elif dailyvalue <= 10000000:
            threshhold = 0.02
        else:
            threshhold = 0.01
        
        pctchg = (cp - hp) / hp
        if pctchg < threshhold:
            return cp + ( cp * threshhold )
        else:
            return 0

    def mail(self, subject, msg):
        serverURL = 'mail01.xxxx.com'
        sender = 'tradingplatform@xxxx.com'
        receiver = 'tradingupdate@xxxx.com'
        header = "From: %s\r\nTo: %s\r\nSubject: %s\r\n\r\n" % (sender, receiver, subject)
        message = header + msg
        mailServer = smtplib.SMTP(serverURL)
        mailServer.sendmail(sender, receiver, message)
        mailServer.quit()

    def listen(self):
        buyOrders, sellOrders = [], []
        while 1:
            sql = 'listen NEWSTMSCRBUYTXN'
            cursor.execute(sql)
            select.select([cursor],[],[], 0)==([],[],[])
            if cursor.isready():
                try:
                    notify = cursor.connection.notifies.pop()
                    print 'Notify Event: %s' % notify[1]
                    if notify[1] == 'newstmscrbuytxn':
                        self.processEntrySignal()
                    else:
                        self.processExits()
                except IndexError:
                    sql = 'listen NEWSTMOPENPOSITION'
                    cursor.execute(sql)
                    select.select([cursor],[],[], 0)==([],[],[])
                    if cursor.isready():
                        try:
                            notify = cursor.connection.notifies.pop()
                            print 'Notify Event: %s' % notify[1]
                            if notify[1] == 'newstmopenposition':
                                self.processExits()
                            else:
                                self.processEntrySignal()
                        except IndexError:
                            self.processCancels('BUY')
                        
            self.processCancels('BUY')
            self.updateSells()
            
    def updateSells(self):
        sql = 'select symbol from positions where position > 0'
        cursor.execute(sql)
        results = cursor.fetchall()
        if cursor.rowcount > 0:
            symbols = []
            for row in results:
                symbols.append( row[0] )
            
            for symbol in symbols:
                sql = "select (select position from positions where symbol='%s' and position > 0) - " % symbol
                sql += "(select sum(leaves_qty) from orders where symbol='%s' and clordid like 'STM%%' " % symbol
                sql += "and side='SELL' and order_status in ('NEW', 'PARTIAL'))" 
                cursor.execute(sql)
                results = cursor.fetchone()
                if cursor.rowcount > 0:
                    shares = results[0]
                    if shares > 0:
                        sql = "select max(parent_id) from orders where symbol = '%s'" % symbol
                        cursor.execute(sql)
                        results = cursor.fetchall()
                        starttime = datetime.datetime.now(gmt) + datetime.timedelta(minutes=10)
                        endtime = datetime.datetime.now(gmt) + datetime.timedelta(hours=3)
                        timenow = int( datetime.datetime.now().strftime('%H%M%S') )
                        timenowgmt = int( datetime.datetime.now(gmt).strftime('%H%M%S') )
                        if timenow < 130000:
                            endtime = datetime.datetime.now(gmt) + datetime.timedelta(hours=3)
                            algotype = 'TWAP'
                        else:
                            today = datetime.date.today()
                            offset = ( timenowgmt - timenow )/10000
                            endtime = datetime.datetime(today.year, today.month, today.day, offset, 0, 0) + datetime.timedelta(hours=16)
                            if timenow < 140000:
                                algotype = 'TWAP'
                            else:
                                algotype = 'VWAP'
                        starttimestr = starttime.strftime('%Y%m%d-%H:%M:%S')
                        endtimestr = endtime.strftime('%Y%m%d-%H:%M:%S')
                        
                        self.fixClient.order.requestType = 0
                        self.fixClient.order.parentID = id
                        self.fixClient.order.broker = 'GSAT'
                        self.fixClient.order.clOrdID = self.client.newOrderID()
                        self.fixClient.order.symbol = symbol
                        self.fixClient.order.exchange = '6'
                        self.fixClient.order.TIF = 'DAY'
                        self.fixClient.order.side = 'SELL'
                        self.fixClient.order.quantity = shares
                        self.fixClient.order.showQty = 0
                        self.fixClient.order.minQty = 0
                        self.fixClient.order.ordType = 'MKT'
                        self.fixClient.order.limitPx = 0
                        self.fixClient.order.stopPx = 0
                        self.fixClient.order.pegOffset = 0
                        self.fixClient.order.discInst = 0
                        self.fixClient.order.discOffset = 0
                        self.fixClient.order.execInst = '5'
                        self.fixClient.order.algoInst = '%s 0 %s %s' % ( algotype, starttimestr, endtimestr )
                        self.fixClient.order.orderTime = datetime.datetime.now(gmt)
                        msg = 'Sending Order: %s SELL %s SHS with %s @ %s ' % ( symbol, shares, algotype, self.fixClient.order.orderTime )
                        print msg
                        self.mail('Clean up Sell Order', msg)
                        self.fixClient.sendMessage()
                        rowcount = 0
                        while rowcount == 0:
                            sql = "select clordid from orders where clordid = '%s' and order_status = 'NEW' " % self.fixClient.order.clOrdID
                            cursor.execute(sql)
                            results = cursor.fetchall()
                            rowcount = cursor.rowcount
                            print 'Waiting for order status new state.'
                    
    def sweepIOC(self, ticker, side, txnid, shares, lmtPrice):
        attempts = ( 0, 1, 2 )
        leaves = shares
        for attempt in attempts:
            currentQuote = self.quoteClient.insideQuote( ticker )
            if leaves > 0 and currentQuote.askPrice <= lmtPrice:
                
                #buy order
                
                self.fixClient.order.requestType = 0
                self.fixClient.order.parentID = txnid
                self.fixClient.order.broker = 'REDI'
                self.fixClient.order.clOrdID = self.client.newOrderID()
                self.fixClient.order.symbol = ticker
                self.fixClient.order.exchange = ';'
                self.fixClient.order.TIF = 'IOC'
                self.fixClient.order.side = side
                self.fixClient.order.quantity = leaves
                self.fixClient.order.showQty = 0
                self.fixClient.order.minQty = 0
                self.fixClient.order.ordType = 'LMT'
                self.fixClient.order.limitPx = currentQuote.askPrice
                self.fixClient.order.stopPx = 0
                self.fixClient.order.pegOffset = 0
                self.fixClient.order.discInst = 0
                self.fixClient.order.discOffset = 0
                self.fixClient.order.execInst = '5'
                self.fixClient.order.algoInst = 'NONE'
                self.fixClient.order.orderTime = datetime.datetime.now(gmt)
                self.fixClient.sendMessage()
                
                #wait time before cxl msg sent
                time.sleep( 0.2 )
                #buy order cancel to ensure leaves is correct
                self.fixClient.order.requestType = 1
                self.fixClient.order.parentID = txnid
                self.fixClient.order.origClOrdID = self.fixClient.order.clOrdID
                self.fixClient.order.clOrdID = self.client.newOrderID()
                self.fixClient.order.symbol = ticker
                self.fixClient.order.exchange = ';'
                self.fixClient.order.side = 'BUY'
                self.fixClient.order.quantity = shares
                self.fixClient.order.orderTime = datetime.datetime.now(gmt)
                self.fixClient.sendMessage()
                
                update = 'update orders '
                set = 'set cxlreqsent=True where clordid = \'%s\' ' % self.fixClient.order.origClOrdID
                sql = update + set
                print sql
                cursor.execute(sql)
                
                leavescurrent = False
                while leavescurrent == False:
                    sel = ' select quantity - cum_qty as leaves '
                    fromtable = 'from orders '
                    where = "where clordid = '%s' and ( order_status = 'CANCELED' or order_status = 'FILLED' ) " % self.fixClient.order.origClOrdID
                    sql = sel + fromtable + where
                    cursor.execute(sql)
                    results = cursor.fetchone()
                    if results != None:
                        leaves = results[0]
                        leavescurrent = True
                    else:
                        leavescurrent = False
                        
        return leaves    
    
    def fourCast(self, ticker, side, txnid, shares, lmtPrice, endTime):
        self.fixClient.order.requestType = 0
        self.fixClient.order.parentID = txnid
        self.fixClient.order.broker = 'GSAT'
        self.fixClient.order.clOrdID = self.client.newOrderID()
        self.fixClient.order.symbol = ticker
        self.fixClient.order.exchange = '6'
        self.fixClient.order.TIF = 'DAY'
        self.fixClient.order.side = side
        self.fixClient.order.quantity = shares
        self.fixClient.order.showQty = 0
        self.fixClient.order.minQty = 0
        self.fixClient.order.ordType = 'LMT'
        self.fixClient.order.limitPx = round(lmtPrice, 2)
        self.fixClient.order.stopPx = 0
        self.fixClient.order.pegOffset = 0
        self.fixClient.order.discInst = 0
        self.fixClient.order.discOffset = 0
        self.fixClient.order.execInst = '5'
        self.fixClient.order.algoInst = '4CAST 0 %s 0 0 0 10 ' % ( endTime )
        self.fixClient.order.orderTime = datetime.datetime.now(gmt)
        self.fixClient.sendMessage()
        
    def twap(self, ticker, side, txnid, shares, startTime, endTime):
        pass
        
    def processEntrySignal(self):
        sel = 'select txnid '
        fromtable = 'from lasttxnid'
        sql = sel + fromtable
        #print sql
        db.local.cursor.execute(sql)
        results = db.local.cursor.fetchone()
        print 'Analyzing Entry Signals since Txnid: %s ' % results[0]
        lasttxnid = results[0]
        
        sel = 'select t.ticker, max(s.txnid), max(gettargetpositionshares(s.txnid)), getdailytradingvalue(t.ticker, 4) '
        fromtable = 'from txns t, stm_screened_txns s '
        where = 'where t.txnid = s.txnid  '
        where += 'and s.txnid > %s ' % ( lasttxnid )
        groupby = 'group by t.ticker, getdailytradingvalue(t.ticker, 4) '
        orderby = 'order by max(s.txnid) asc '
        sql = sel + fromtable + where + groupby + orderby
        cursor.execute(sql)
        print sql
        tickers, txnids, shares, dailyval = [], [], [], []
        results = cursor.fetchall()
        
        for row in results:
            tickers.append(row[0])
            txnids.append(row[1])
            shares.append(row[2])
            dailyval.append(row[3])
            
        for i in range( len( tickers ) ):
            #print tickers[i]
            #self.quoteClient.reqMktData( tickers[i] )
            currentQuote = self.quoteClient.insideQuote( tickers[i] )
            htime = datetime.datetime.now() - datetime.timedelta(minutes=30)
            print int( datetime.datetime.now().strftime('%H%M%S') )
            if int( datetime.datetime.now().strftime('%H%M%S') ) < 93000:
                sel = 'select close '
                fromtable = 'from quotedaily '
                where = "where ticker = '%s' " % tickers[i]
                where += 'and trade_date = getlastvalidtradedate(now()::date) '
                sql = sel + fromtable + where
                cursor.execute(sql)
                results = cursor.fetchone(sql)
                histQuote.histAskPrice = results[0][0]
            else:
                histQuote = self.quoteClient.reqHistMktData( tickers[i], htime )
            print 'Quote Check - Curr:%s, Hist:%s ' % ( currentQuote.askPrice, histQuote.histAskPrice )
            if histQuote.histAskPrice == 0:
                maxBuyPrice = 0
            else:
                maxBuyPrice = self.priceActionCheck( dailyval[i], currentQuote.askPrice, histQuote.histAskPrice )
            if maxBuyPrice != 0:
                timenow = int( datetime.datetime.now().strftime('%H%M%S') )
                endtime = datetime.datetime.now(gmt) + datetime.timedelta(minutes=5)
                endtimestr = endtime.strftime('%Y%m%d-%H:%M:%S')
                print 'EntryTime: %s ' % ( timenow )
                if  timenow > 93000  and timenow < 153000:
                    msg = 'Sending Order: %s BUY %s @ %s %s ' % ( tickers[i], shares[i], maxBuyPrice, timenow )
                    print msg
                    self.mail('Buy Order', msg)
                    leaves = self.sweepIOC(tickers[i], 'BUY', txnids[i], shares[i], round(maxBuyPrice, 2))
                    if leaves > 0:
                        self.fourCast(tickers[i], 'BUY', txnids[i], leaves, round(maxBuyPrice, 2), endtimestr)
                
            update = 'update lasttxnid '
            set = 'set txnid = %s ' % ( txnids[-1] )    
            sql = update + set
            db.local.cursor.execute(sql)
            db.local.conn.commit()
                            
    def processExits(self):
        sql = "select max(o.parent_id), p.symbol, p.position from orders o, positions p where o.symbol = p.symbol and o.parent_id not in "
        sql += "(select parent_id from orders where side='SELL' and order_status not in ('REJECTED')) "
        sql += "group by p.symbol, p.position "
        cursor.execute(sql)
        print sql
        orderids, tickers, positions = [], [], []
        results = cursor.fetchall()  
        for row in results:
            if row[2] > 0:
                orderids.append( row[0] )
                tickers.append( row[1] )
                positions.append( row[2] )
        
        for i in range( len( orderids ) ):    
            starttime = datetime.datetime.now(gmt) + datetime.timedelta(minutes=15)
            endtime = datetime.datetime.now(gmt) + datetime.timedelta(hours=3)
            timenow = int( datetime.datetime.now().strftime('%H%M%S') )
            timenowgmt = int( datetime.datetime.now(gmt).strftime('%H%M%S') )
            print 'ExitTime: %s ' % ( timenow )
            print orderids[i], tickers[i], positions[i]
            if timenow < 130000:
                endtime = datetime.datetime.now(gmt) + datetime.timedelta(hours=3)
                algotype = 'TWAP'
            else:
                today = datetime.date.today()
                offset = ( timenowgmt - timenow )/10000
                endtime = datetime.datetime(today.year, today.month, today.day, offset, 0, 0) + datetime.timedelta(hours=16)
                if timenow < 140000:
                    algotype = 'TWAP'
                else:
                    algotype = 'VWAP'
            starttimestr = starttime.strftime('%Y%m%d-%H:%M:%S')
            endtimestr = endtime.strftime('%Y%m%d-%H:%M:%S')
                        
            self.fixClient.order.requestType = 0
            self.fixClient.order.parentID = orderids[i]
            self.fixClient.order.broker = 'GSAT'
            self.fixClient.order.clOrdID = self.client.newOrderID()
            self.fixClient.order.symbol = tickers[i]
            self.fixClient.order.exchange = '6'
            self.fixClient.order.TIF = 'DAY'
            self.fixClient.order.side = 'SELL'
            self.fixClient.order.quantity = positions[i]
            self.fixClient.order.showQty = 0
            self.fixClient.order.minQty = 0
            self.fixClient.order.ordType = 'MKT'
            self.fixClient.order.limitPx = 0
            self.fixClient.order.stopPx = 0
            self.fixClient.order.pegOffset = 0
            self.fixClient.order.discInst = 0
            self.fixClient.order.discOffset = 0
            self.fixClient.order.execInst = '5'
            self.fixClient.order.algoInst = '%s 0 %s %s' % ( algotype, starttimestr, endtimestr )
            self.fixClient.order.orderTime = datetime.datetime.now(gmt)
            msg = 'Sending Order: %s SELL %s SHS with %s @ %s ' % ( tickers[i], positions[i], algotype, self.fixClient.order.orderTime )
            print msg
            self.mail('Sell Order', msg)
            self.fixClient.sendMessage()
            
    def processCancels(self, side):
        pendingCxls = []
        sel = 'select parent_id, clordid, symbol, exchange, side, quantity, order_date '
        fromtable = 'from orders '
        where = 'where order_status not in (\'CANCELED\', \'REPLACED\', \'REJECTED\', \'FILLED\', \'PEND_CXL\', \'PEND_REPL\', \'EXPIRED\') '
        where += 'and side = \'%s\' ' % side
        where += 'and cxlreqsent=False '
        sql = sel + fromtable + where
        cursor.execute(sql)
        results = cursor.fetchall()
        if cursor.rowcount > 0:
            orderTime = datetime.datetime.now(gmt)
            for row in results:
                #print datetime.datetime.now(gmt)
                #print row
                if datetime.datetime.now(gmt) - row[6] > datetime.timedelta(minutes=5):
                    self.fixClient.order.requestType = 1
                    self.fixClient.order.parentID = row[0]
                    self.fixClient.order.origClOrdID = row[1]
                    self.fixClient.order.clOrdID = self.client.newOrderID()
                    self.fixClient.order.symbol = row[2]
                    self.fixClient.order.exchange = row[3]
                    self.fixClient.order.side = row[4]
                    self.fixClient.order.quantity = row[5]
                    self.fixClient.order.orderTime = datetime.datetime.now(gmt)
                    msg = 'Sending CXL %s: %s %s %s SHS @ %s' % ( self.fixClient.order.origClOrdID, self.fixClient.order.symbol, self.fixClient.order.side, self.fixClient.order.quantity, self.fixClient.order.orderTime )
                    print msg
                    update = 'update orders '
                    set = 'set cxlreqsent=True where clordid = \'%s\' ' % row[1]
                    sql = update + set
                    print sql
                    cursor.execute(sql)
                    self.mail('Buy Cancel', msg)
                    self.fixClient.sendMessage()
                    
    def eodcheck():
        timenow = int( datetime.datetime.now().strftime('%H%M%S') )
        if timenow > 170000:
            sys.exit(0)
        
class Quote:
    def __init__(self):
        self.symbol = None
        self.bidPrice = None
        self.askPrice = None
        self.lastPrice = None
        self.bidSize = None
        self.askSize = None
        self.lastSize = None
        self.barTime = None
        self.barOpen = None
        self.barHigh = None
        self.barLow = None
        self.barLast = None
        self.barVolume = None
        self.barWAP = None
        self.barHasGaps = None
        self.histBidPrice = None
        self.histAskPrice = None
        self.histBidSize = None
        self.histAskSize = None
class Client:
    ''' This is the client class of the TP.
            It is responsible for setting the date and strategy codes to create
            custom fix order ids. Please set the self.user = strategy code in TP 
            server DB that this client will be executing '''
    def __init__(self):
        self.user = 'STM'
        self.datecheck()
        self.login()

    def datecheck(self):
        sel = 'select trade_date '
        fromtable = 'from orderids '
        limit = 'limit 1 '
        sql = sel + fromtable + limit
        db.local.cursor.execute( sql )
        results = db.local.cursor.fetchone()

        if results != None:
            lastdate = results[0]
        else:
            lastdate = None
            
        today = datetime.date.today()
        print lastdate
        if lastdate != None:
           lastdate = datetime.date( int( lastdate[ :4 ] ), int( lastdate[ 5 : 7 ] ), int( lastdate[ 8 : ] ) )
        if today != lastdate and lastdate != None:
            db.local.cursor.execute('delete from orderids')  
            db.local.conn.commit()
            
    def login(self):
        
        sel = 'select id '
        fromtable = 'from client_ids '
        where = "where id = \'%s\' " % ( self.user )

        sql =  sel + fromtable + where
        
        cursor.execute( sql )
        results = cursor.fetchone()
        self.lastuser = results[0]

        if self.user != self.lastuser:
            sql = 'delete from currentuser'
            db.local.cursor.execute( sql )
            sql = 'insert into currentuser(user_id) values(\'%s\') ' % ( self.user )
            db.local.cursor.execute( sql )
            sql = 'delete from orderids'
            db.local.cursor.execute( sql )
            db.local.conn.commit()
        cursor.execute('insert into history_stm_screened_txns select * from stm_screened_txns')
        cursor.execute( 'delete from stm_screened_txns' )
        print 'CLIENT NOTICE: pending txn screens reset'
        
    
    def newOrderID(self):
        db.local.cursor.execute('insert into orderids(trade_date) select date(\'now\')')
        db.local.conn.commit()
        db.local.cursor.execute('select trade_date, max(id) from orderids limit 1')
        db.local.conn.commit()
        results = db.local.cursor.fetchone()
        self.date = results[0]
        self.id = results[1]
        orderID = '%s-%s-%s' % (self.user, self.date, self.id)
        db.local.cursor.execute('update orderids set clordid = \'%s\' where id=(select max(id) from orderids) ' % ( orderID ))
        db.local.conn.commit()
        return orderID    
    
class OrderDetails:
    """This class stores all new order information"""
    
    def __init__(self):
        self.requestType = None
        self.origClOrdID = None
        self.clOrdID = None
        self.symbol = ''
        self.TIF = 'DAY'
        self.side = 'BUY'
        self.exchange = ';'
        self.quantity = 100
        self.showQty = 0
        self.minQty = 0
        self.ordType = 'LMT'
        self.limitPx = 0
        self.stopPx = 0
        self.pegOffset = 0
        self.discInst = 0
        self.discOffset = 0
        self.broker = 'GSAT'
        self.execInst = ''
        self.algoInst = ''
        self.orderTime = None
    
if __name__ == '__main__':
    db = DB()
    tradeSignals = TradeSignalsClient()
    tradeSignals.listen()
