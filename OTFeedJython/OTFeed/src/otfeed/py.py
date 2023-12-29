#!/usr/bin/env jython

from com.opentick import *
import java.util.Date as Date
import java.sql.Timestamp as Timestamp
import java.util.Iterator as Iterator
import java.util.List as List
import java.lang.Thread as Thread
import java.lang.Throwable
import socket

class WatchList:
    requestID = {}
    
    def __getitem__(self, id):
        if not self.requestID.has_key(id):
            self.requestID[id] = Quote()
            
        return self.requestID[id].symbol
    
    def getRequestID(self, symbol):
        valuelist = [str(value) for key, value in self.requestID.items()]
        try:
            return valuelist.index(symbol)
        except ValueError:
            return -1

class Quote:
    def __init__(self):
        self.symbol = None
        self.bidPrice = None
        self.askPrice = None
        self.bidSize = None
        self.askSize = None
        self.populated = False
        self.requestID = None
        self.historyPopulated = False
        self.requestID = None
        self.barTime = None
        self.barOpen = None
        self.barHigh = None
        self.barLow = None
        self.barLast = None
        self.barVolume = None
        self.lastExchange = None
        self.histBidPrice = None
        self.histAskPrice = None
        self.histBidSize = None
        self.histAskSize = None
        self.histLastExchange = None

        
    def __str__(self):
        return str(self.symbol)
    
    def __repr__(self):
        return str(self)
            
    def insideQuote(self):
        return ( self.symbol, 
                self.bidSize, 
                self.bidPrice, 
                self.askPrice, 
                self.askSize )
                
    def histQuote(self):
        return ( self.symbol,
                self.histBidSize,
                self.histBidPrice,
                self.histAskPrice,
                self.histAskSize )

class FeedClient(OTClient):
    def __init__(self):
        self.exchanges = List
        self.exchange = OTExchange()
        self.symbol = OTSymbol()
        self.pause = 60000
        self.requestID = []
        self.ticks = 10
        
        self.OTConstants = OTConstants
        self.iterator = Iterator
        self.watchlist = WatchList()
        self.watchlist[0]
        self.watchlist.requestID[0].symbol = 'NONE'
        self.currentID = 0
        
       
    def onRestoreConnection(self):
        print 'Connection Restored'

    def onError(self, error):
        print 'Current ID: %s' % self.currentID
        if error.code == 1003:
            self.watchlist.requestID[self.currentID].bidPrice = 0
            self.watchlist.requestID[self.currentID].askPrice = 0
            self.watchlist.requestID[self.currentID].bidSize = 0
            self.watchlist.requestID[self.currentID].askSize = 0
            self.watchlist.requestID[self.currentID].populated = True
            self.watchlist.requestID[self.currentID].barTime = 0
            self.watchlist.requestID[self.currentID].barOpen = 0
            self.watchlist.requestID[self.currentID].barHigh = 0
            self.watchlist.requestID[self.currentID].barLow = 0
            self.watchlist.requestID[self.currentID].barLast = 0
            self.watchlist.requestID[self.currentID].barVolume = 0
            self.watchlist.requestID[self.currentID].historyPopulated = True
        print error.code

    def onStatusChanged(self, status ):
        print 'Status: %s' % ( status )

    def onLogin(self):
        print 'Logged in'

    def onEquityInit(self, equity):
        print 'onEquityInit: %s ' % equity

    def onMessage(self, message):
        print 'Current ID: %s' % self.currentID
        if message.code == 30:
            if self.watchlist.requestID[self.currentID].bidPrice == None:
                self.watchlist.requestID[self.currentID].bidPrice = 0
            if self.watchlist.requestID[self.currentID].bidSize == None:
                self.watchlist.requestID[self.currentID].bidSize= 0
            if self.watchlist.requestID[self.currentID].askPrice == None:
                self.watchlist.requestID[self.currentID].askPrice = 0
            if self.watchlist.requestID[self.currentID].askSize == None:
                self.watchlist.requestID[self.currentID].askSize= 0
            self.watchlist.requestID[self.currentID].populated = True
        elif message.code == 10:
            if self.watchlist.requestID[self.currentID].histBidPrice == None:
                self.watchlist.requestID[self.currentID].histBidPrice = 0
            if self.watchlist.requestID[self.currentID].histBidSize == None:
                self.watchlist.requestID[self.currentID].histBidSize= 0
            if self.watchlist.requestID[self.currentID].histAskPrice == None:
                self.watchlist.requestID[self.currentID].histAskPrice = 0
            if self.watchlist.requestID[self.currentID].histAskSize == None:
                self.watchlist.requestID[self.currentID].histAskSize= 0
            self.watchlist.requestID[self.currentID].historyPopulated = True
        print 'Message: %s' % message.code
    
    def onListExchanges(self, exchanges):
        self.exchanges = exchanges
        iterator = Iterator()
        iterator = exchanges.iterator()
        url = str( iterator.next() )
        print 'URL: %s' % ( url )
        while iterator.hasNext():
            exchange = iterator.next()
            print 'Exchange - Code: %s,  Title: %s, Avail: %s, Desc: %s' % ( exchange.getCode(), exchange.getTitle(), exchange.isAvailable(), exchange.getDesciption() )

    def onListSymbols(self, symbols):
        print 'Received a list of symbols'
        iterator = Iterator()
        iterator = symbols.Iterator()
        count = 0
        while iterator.hasNext():
            symbol = iterator.next()
            count += 1
            print '%s, ID: %s, Code: %s, Company: %s, Currency: %s, Type: %s ' % ( count, symbol.getRequestID(), symbol.getCompany(), symbol.getCurrency(), symbol.getType() )

    def onRealtimeQuote(self, quote):
        watchID = quote.requestID
        wl = self.watchlist.requestID[watchID]
        if wl.bidPrice == None and wl.askPrice == None and wl.bidSize == None and wl.askSize == None:
            wl.bidPrice = quote.bidPrice
            wl.askPrice = quote.askPrice
            wl.bidSize = quote.bidSize
            wl.askSize = quote.askSize
            wl.lastExchange = quote.bidExchange
        else:
            if wl.lastExchange != quote.bidExchange:
                if quote.bidPrice > wl.bidPrice:
                    wl.bidPrice = quote.bidPrice
                    wl.bidSize = quote.bidSize
                elif quote.bidPrice == wl.bidPrice:
                    wl.bidSize = quote.bidSize
                if quote.askPrice < wl.askPrice:
                    wl.askPrice = quote.askPrice
                    wl.askSize = quote.askSize
                elif quote.askPrice == wl.askPrice:
                    wl.askSize = quote.askSize
                    
            else:
                wl.bidPrice = quote.bidPrice
                wl.askPrice = quote.askPrice
                wl.bidSize = quote.bidSize
                wl.askSize = quote.askSize
            wl.lastExchange = quote.bidExchange
        print wl.insideQuote()
        print 'onRealTimeQuote %s' % quote

    def onRealtimeMMQuote(self, quote):
        print 'onRealtimeMMquote: %s ' % quote

    def onRealtimeBBO(self, quote):
        print 'onRealtimeBBO: %s ' % quote

    def onTodaysOHL(self, todayOHL):
        print 'onTodaysOHL: %s ' % todayOHL

    def onHistQuote(self, quote):
        watchID = quote.requestID
        wl = self.watchlist.requestID[watchID]
        if wl.histBidPrice == None and wl.histAskPrice == None and wl.histBidSize == None and wl.histAskSize == None:
            wl.histBidPrice = quote.bidPrice
            wl.histAskPrice = quote.askPrice
            wl.histBidSize = quote.bidSize
            wl.histAskSize = quote.askSize
            wl.histLastExchange = quote.bidExchange
        else:
            if wl.histLastExchange != quote.bidExchange:
                if quote.bidPrice > wl.histBidPrice:
                    wl.histBidPrice = quote.bidPrice
                    wl.histBidSize = quote.bidSize
                elif quote.bidPrice == wl.histBidPrice:
                    wl.histBidSize = quote.bidSize
                if quote.askPrice < wl.histAskPrice:
                    wl.histAskPrice = quote.askPrice
                    wl.histAskSize = quote.askSize
                elif quote.askPrice == wl.histAskPrice:
                    wl.histAskSize = quote.askSize

            else:
                wl.histBidPrice = quote.bidPrice
                wl.histAskPrice = quote.askPrice
                wl.histBidSize = quote.bidSize
                wl.histAskSize = quote.askSize
            wl.histLastExchange = quote.bidExchange
        wl.histQuote()
	#print 'onHistQuote %s ' % quote

    def onHistMMQuote(self, quote):
        print 'onHistMMQuote: %s ' % quote

    def onHistTrade(self, trade):
        #print 'onHistTrade: %s ' % trade
        pass

    def onHistBBO(self, bbo):
        print 'onHistbbo: %s ' % bbo

    def onHistOHLC(self, ohlc):
        watchID = ohlc.requestID
        wl = self.watchlist.requestID[watchID]
        Timestamp( long(str(ohlc.timestamp) + '000') )
        wl.barTime = Timestamp( long(str(ohlc.timestamp) + '000') )
        wl.barOpen = ohlc.openPrice
        wl.barHigh = ohlc.highPrice
        wl.barLow = ohlc.lowPrice
        wl.barLast = ohlc.closePrice
        wl.barVolume = ohlc.volume
        wl.histMktData()
        print 'onHistOHLC: %s' %  ohlc

    def onRealtimeBookOrder(self, order):
        print 'onRealtimeBookOrder: %s ' % order

    def onRealtimeBookExecute(self, execute):
        print 'onRealtimeBookExecute: %s ' % execute

    def onRealtimeBookCancel(self, cancel):
        print 'onRealtimeBookCancel: %s ' % cancel

    def onRealtimeBookDelete(self, delete):
        print delete

    def onRealtimeBookChange(self, change):
        print change

    def onRealtimeBookReplace(self, replace):
        print replace

    def onRealtimeBookPurge(self, purge):
        print purge

    def onRealtimeBookPriceLevel(self, level):
        print level

class QuoteServer:
    def __init__(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind( ('localhost', 8007) )
        self.socket.listen(5)
        self.feedclient = FeedClient()
        self.feedclient.addHost('feed1.opentick.com', 10010)
        print 'Logging into OpenTick Server'
        self.feedclient.login('geniva', 'geniva')
        self.reqSymbol = None
        
    def listen(self):
        while 1:
            clientconn,  clientaddr = self.socket.accept()
            line = clientconn.recv(1024)
            request = line.split('|')
            symbol = request[1]
            if request[0] == 'insideQuote':
                res = self.insideQuote(symbol)
                self.response = request[0] + '|' + res 
                print self.response
                clientconn.sendall(self.response)
            elif request[0] == 'reqHistMktData':
                htime = request[2]
                res = self.histMktData(symbol, htime)
                self.response = request[0] + '|' + res
                print self.response
                clientconn.sendall(self.response)
            else:
                print request
            clientconn.close()
    def wait(self, id):
        wait = 0
        while not self.feedclient.watchlist.requestID[id].populated:
            wait+=1
            
    def historicalWait(self, id):
        wait = 0
        while not self.feedclient.watchlist.requestID[id].historyPopulated:
            wait+=1 

    def insideQuote(self, symbol):
        for exchange in ('Q', 'S', 'U', 'V', 'N', 'B', 'C', 'M', 'N', 'P', 'X', 'A', 'D', 'T'):
            id = self.feedclient.requestTickSnapshot(OTDataEntity(exchange, symbol), OTConstants.OT_TICK_TYPE_LEVEL1)
            self.feedclient.watchlist[id]
            self.feedclient.watchlist.requestID[id].symbol = symbol
            self.feedclient.currentID=id
            self.wait(id)
            res = self.feedclient.watchlist.requestID[id].insideQuote()
            if res[3] != 0 and res[3] != None:
                break
        response = '%s|%s|%s|%s|%s\0' % (res[0], res[1], res[2], res[3], res[4])
        print response
        return response

    def histMktData(self, symbol, htime):
        print 'histMktData func'
        yr = int(htime[0:4])
        yrUTC = yr - 1900 
        mon = int(htime[4:6]) - 1
        day = int(htime[6:8])
        hr = int(htime[9:11]) 
        min = int(htime[12:14])
        sec = int(htime[15:17])
        start = Date(yrUTC, mon, day, 0, 0)
        end = Date(yrUTC, mon, day, hr, min)
        start_int = int(start.getTime()/1000)
        end_int = int(end.getTime()/1000)
        print start, start_int, end, end_int
        for exchange in ('Q', 'S', 'U', 'V', 'N', 'B', 'C', 'M', 'N', 'P', 'X', 'A', 'D', 'T'):
            id = self.feedclient.requestHistTicks(OTDataEntity(exchange, symbol), start_int, end_int, OTConstants.OT_TICK_TYPE_LEVEL1)
            self.feedclient.watchlist[id]
            self.feedclient.watchlist.requestID[id].symbol = symbol
            self.feedclient.currentID=id
            self.historicalWait(id)
            res = self.feedclient.watchlist.requestID[id].histQuote()
            if res[3] != 0 and res[3] != None:
                break
        print res
        response = '%s|%s|%s|%s|%s\0' % (res[0], res[1], res[2], res[3], res[4])
        print response
        return response
    
if __name__ == '__main__':
    try:
        qs = QuoteServer()
        qs.listen()
        
    except java.lang.Throwable, error:
        print 'java.Lang.Throwable error: %s ' % error
