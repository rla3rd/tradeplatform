import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

s.connect( ("localhost", 30000) )

def printNewOrderString():
    print "MsgType|ParentID|MMID|ClOrdID|Symbol|ExDest|TIF|Side|Quantity|show_qty|min_qty|OrdType|Price|StopPx|PegDiff|DiscInst|DiscOffset|ExecInst"

#newordersingle
s.sendall("0|1|REDI|2007-02-13-34|GE|N|DAY|BUY|100|0|0|LMT|2|0|0|0|0|0")
#ordercancelrequest
s.sendall("1|1|REDI|2007-02-13-34|2007-13-35|GE|N|BUY|100")
#orderreplacerequest
s.sendall("1|1|REDI|2007-02-13-34|2007-13-35|GE|N|BUY|100|LMT|2|0")

