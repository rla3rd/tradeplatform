/* -*- C++ -*- */

/****************************************************************************
** Copyright (c) quickfixengine.org  All rights reserved.
**
** This file is part of the QuickFIX FIX Engine
**
** This file may be distributed under the terms of the quickfixengine.org
** license as defined by quickfixengine.org and appearing in the file
** LICENSE included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.quickfixengine.org/LICENSE for licensing information.
**
** Contact ask@quickfixengine.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

#ifndef TRADECLIENT_APPLICATION_H
#define TRADECLIENT_APPLICATION_H

#include "quickfix/Application.h"
#include "quickfix/MessageCracker.h"
#include "quickfix/Values.h"
#include "quickfix/Mutex.h"

#include "quickfix/fix40/NewOrderSingle.h"
#include "quickfix/fix40/ExecutionReport.h"
#include "quickfix/fix40/OrderCancelRequest.h"
#include "quickfix/fix40/OrderCancelReject.h"
#include "quickfix/fix40/OrderCancelReplaceRequest.h"

#include "quickfix/fix41/NewOrderSingle.h"
#include "quickfix/fix41/ExecutionReport.h"
#include "quickfix/fix41/OrderCancelRequest.h"
#include "quickfix/fix41/OrderCancelReject.h"
#include "quickfix/fix41/OrderCancelReplaceRequest.h"

#include "quickfix/fix42/NewOrderSingle.h"
#include "quickfix/fix42/ExecutionReport.h"
#include "quickfix/fix42/OrderCancelRequest.h"
#include "quickfix/fix42/OrderCancelReject.h"
#include "quickfix/fix42/OrderCancelReplaceRequest.h"

#include "quickfix/fix43/NewOrderSingle.h"
#include "quickfix/fix43/ExecutionReport.h"
#include "quickfix/fix43/OrderCancelRequest.h"
#include "quickfix/fix43/OrderCancelReject.h"
#include "quickfix/fix43/OrderCancelReplaceRequest.h"
#include "quickfix/fix43/MarketDataRequest.h"

#include "quickfix/fix44/NewOrderSingle.h"
#include "quickfix/fix44/ExecutionReport.h"
#include "quickfix/fix44/OrderCancelRequest.h"
#include "quickfix/fix44/OrderCancelReject.h"
#include "quickfix/fix44/OrderCancelReplaceRequest.h"
#include "quickfix/fix44/MarketDataRequest.h"

#include <queue>
#include <libpq-fe.h>
#include <string.h>

class Application :
      public FIX::Application,
      public FIX::MessageCracker
{
public:
  void run();

  void queryEnterOrder( std::string ParentID,
                        std::string MMID,
                        std::string ClOrdID,
                        std::string Symbol,
                        std::string ExDest,
                        std::string TIF,
                        std::string Side,
                        long Shares,
                        long MaxFloor,
                        long MinQty,
                        std::string OrdType,
                        double Price,
                        double StopPx,
                        double PegDiff,
                        std::string DiscInst,
                        double DiscOffset,
                        std::string ExecInst,
                        std::string AlgoInst );
  void queryCancelOrder( std::string ParentID,
                         std::string MMID,
                         std::string OrigClOrdID,
                         std::string ClOrdID,
                         std::string Symbol,
                         std::string ExDest,
                         std::string Side,
                         long Shares );
  void queryReplaceOrder( std::string ParentID,
                          std::string MMID,
                          std::string OrigClOrdID,
                          std::string ClOrdID,
                          std::string Symbol,
                          std::string ExDest,
                          std::string TIF,
                          std::string Side,
                          long Shares,
                          long MaxFloor,
                          long MinQty,
                          std::string OrdType,
                          double Price,
                          double StopPx,
                          double PegDiff,
                          std::string DiscInst,
                          double DiscOffset,
                          std::string ExecInst,
                          std::string AlgoInst );
  void queryMarketDataRequest( std::string MMID,
                               std::string Symbol );

private:
  void onCreate( const FIX::SessionID& ) {}
  void onLogon( const FIX::SessionID& sessionID );
  void onLogout( const FIX::SessionID& sessionID );
  void toAdmin( FIX::Message&, const FIX::SessionID& ) {}
  void toApp( FIX::Message&, const FIX::SessionID& )
  throw( FIX::DoNotSend );
  void fromAdmin( const FIX::Message&, const FIX::SessionID& )
  throw( FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon ) {}
  void fromApp( const FIX::Message& message, const FIX::SessionID& sessionID )
  throw( FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType );

  struct ConnectInfo {
    std::string SenderCompID;
    std::string TargetCompID;
    int Version;
    std::string DeskID;
  };

  void onMessage( const FIX40::ExecutionReport& message, const FIX::SessionID& SessionID );
  void onMessage( const FIX40::OrderCancelReject& message, const FIX::SessionID& SessionID );
  void onMessage( const FIX41::ExecutionReport& message, const FIX::SessionID& SessionID );
  void onMessage( const FIX41::OrderCancelReject& message, const FIX::SessionID& SessionID );
  void onMessage( const FIX42::ExecutionReport& message, const FIX::SessionID& SessionID );
  void onMessage( const FIX42::OrderCancelReject& message, const FIX::SessionID& SessionID );
  void onMessage( const FIX43::ExecutionReport& message, const FIX::SessionID& SessionID );
  void onMessage( const FIX43::OrderCancelReject& message, const FIX::SessionID& SessionID );
  void onMessage( const FIX44::ExecutionReport& message, const FIX::SessionID& SessionID );
  void onMessage( const FIX44::OrderCancelReject& message, const FIX::SessionID& SessionID );

  void queryNewOrderSingle40( std::string DeskID, std::string SenderCompID, std::string TargetCompID, std::string ClOrdID, std::string Symbol, std::string ExDest, std::string TIF, std::string Side, long Shares, long MaxFloor, long MinQty, std::string OrdType, double Price, double StopPx, double PegDiff, std::string DiscInst, double DiscOffset, std::string ExecInst, std::string AlgoInst );
  void queryNewOrderSingle41( std::string DeskID, std::string SenderCompID, std::string TargetCompID, std::string ClOrdID, std::string Symbol, std::string ExDest, std::string TIF, std::string Side, long Shares, long MaxFloor, long MinQty, std::string OrdType, double Price, double StopPx, double PegDiff, std::string DiscInst, double DiscOffset, std::string ExecInst, std::string AlgoInst );
  void queryNewOrderSingle42( std::string DeskID, std::string SenderCompID, std::string TargetCompID, std::string ClOrdID, std::string Symbol, std::string ExDest, std::string TIF, std::string Side, long Shares, long MaxFloor, long MinQty, std::string OrdType, double Price, double StopPx, double PegDiff, std::string DiscInst, double DiscOffset, std::string ExecInst, std::string AlgoInst );
  void queryNewOrderSingle43( std::string DeskID, std::string SenderCompID, std::string TargetCompID, std::string ClOrdID, std::string Symbol, std::string ExDest, std::string TIF, std::string Side, long Shares, long MaxFloor, long MinQty, std::string OrdType, double Price, double StopPx, double PegDiff, std::string DiscInst, double DiscOffset, std::string ExecInst, std::string AlgoInst );
  void queryNewOrderSingle44( std::string DeskID, std::string SenderCompID, std::string TargetCompID, std::string ClOrdID, std::string Symbol, std::string ExDest, std::string TIF, std::string Side, long Shares, long MaxFloor, long MinQty, std::string OrdType, double Price, double StopPx, double PegDiff, std::string DiscInst, double DiscOffset, std::string ExecInst, std::string AlgoInst );
  void queryOrderCancelRequest40( std::string DeskID, std::string SenderCompID, std::string TargetCompID, std::string OrigClOrdID, std::string ClOrdID, std::string Symbol,
       std::string ExDest, std::string Side, long Shares );
  void queryOrderCancelRequest41( std::string DeskID, std::string SenderCompID, std::string TargetCompID, std::string OrigClOrdID, std::string ClOrdID, std::string Symbol,
       std::string ExDest, std::string Side, long Shares );
  void queryOrderCancelRequest42( std::string DeskID, std::string SenderCompID, std::string TargetCompID, std::string OrigClOrdID, std::string ClOrdID, std::string Symbol,
       std::string ExDest, std::string Side, long Shares );
  void queryOrderCancelRequest43( std::string DeskID, std::string SenderCompID, std::string TargetCompID, std::string OrigClOrdID, std::string ClOrdID, std::string Symbol,
       std::string ExDest, std::string Side, long Shares );
  void queryOrderCancelRequest44( std::string DeskID, std::string SenderCompID, std::string TargetCompID, std::string OrigClOrdID, std::string ClOrdID, std::string Symbol,
       std::string ExDest, std::string Side, long Shares );
  void queryCancelReplaceRequest40( std::string DeskID, std::string SenderCompID, std::string TargetCompID, std::string OrigClOrdID, std::string ClOrdID, std::string Symbol, std::string ExDest, std::string TIF, std::string Side, long Shares, long MaxFloor, long MinQty, std::string OrdType, double Price, double StopPx, double PegDiff, std::string DiscInst, double DiscOffset, std::string ExecInst, std::string AlgoInst );
  void queryCancelReplaceRequest41( std::string DeskID, std::string SenderCompID, std::string TargetCompID, std::string OrigClOrdID, std::string ClOrdID, std::string Symbol, std::string ExDest, std::string TIF, std::string Side, long Shares, long MaxFloor, long MinQty, std::string OrdType, double Price, double StopPx, double PegDiff, std::string DiscInst, double DiscOffset, std::string ExecInst, std::string AlgoInst );
  void queryCancelReplaceRequest42( std::string DeskID, std::string SenderCompID, std::string TargetCompID, std::string OrigClOrdID, std::string ClOrdID, std::string Symbol, std::string ExDest, std::string TIF, std::string Side, long Shares, long MaxFloor, long MinQty, std::string OrdType, double Price, double StopPx, double PegDiff, std::string DiscInst, double DiscOffset, std::string ExecInst, std::string AlgoInst );
  void queryCancelReplaceRequest43( std::string DeskID, std::string SenderCompID, std::string TargetCompID, std::string OrigClOrdID, std::string ClOrdID, std::string Symbol, std::string ExDest, std::string TIF, std::string Side, long Shares, long MaxFloor, long MinQty, std::string OrdType, double Price, double StopPx, double PegDiff, std::string DiscInst, double DiscOffset, std::string ExecInst, std::string AlgoInst );
  void queryCancelReplaceRequest44( std::string DeskID, std::string SenderCompID, std::string TargetCompID, std::string OrigClOrdID, std::string ClOrdID, std::string Symbol, std::string ExDest, std::string TIF, std::string Side, long Shares, long MaxFloor, long MinQty, std::string OrdType, double Price, double StopPx, double PegDiff, std::string DiscInst, double DiscOffset, std::string ExecInst, std::string AlgoInst );
  void queryMarketDataRequest43( std::string SenderCompID, std::string TargetCompID, std::string Symbol );
  void queryMarketDataRequest44(std::string SenderCompID, std::string TargetCompID, std::string Symbol );

  void queryHeader( FIX::Header& header );
  char queryAction();
  bool queryConfirm( const std::string& query );

  FIX::SenderCompID querySenderCompID();
  FIX::TargetCompID queryTargetCompID();
  FIX::TargetSubID queryTargetSubID();
  FIX::Side querySide( std::string Side );
  FIX::OrderQty queryOrderQty();
  FIX::OrdType queryOrdType( std::string OrdType );
  FIX::Price queryPrice();
  FIX::StopPx queryStopPx();
  FIX::TimeInForce queryTimeInForce( std::string TIF );
  void parseAlgoInst(FIX::Message& message, std::string TargetCompID, std::string DeskID, std::string AlgoInst);

  ConnectInfo queryBroker( std::string MMID );
  void exit_nicely(PGconn *conn);
  const char* getConnection();
};

#endif

// Definition of the Socket class

#ifndef Socket_class
#define Socket_class


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>


const int MAXHOSTNAME = 200;
const int MAXCONNECTIONS = 5;
const int MAXRECV = 500;

class Socket
{
 public:
  Socket();
  virtual ~Socket();

  // Server initialization
  bool create();
  bool bind ( const int port );
  bool listen() const;
  bool accept ( Socket& ) const;

  // Client initialization
  bool connect ( const std::string host, const int port );

  // Data Transimission
  bool send ( const std::string ) const;
  int recv ( std::string& ) const;


  void set_non_blocking ( const bool );

  bool is_valid() const { return m_sock != -1; }

 private:

  int m_sock;
  sockaddr_in m_addr;


};


#endif

// Definition of the ServerSocket class

#ifndef ServerSocket_class
#define ServerSocket_class

#include "Socket.h"


class ServerSocket : private Socket
{
 public:

  ServerSocket ( int port );
  ServerSocket (){};
  virtual ~ServerSocket();

  const ServerSocket& operator << ( const std::string& ) const;
  const ServerSocket& operator >> ( std::string& ) const;

  void accept ( ServerSocket& );

};


#endif

#ifndef SocketException_class
#define SocketException_class

#include <string>

class SocketException
{
 public:
  SocketException ( std::string s ) : m_s ( s ) {};
  ~SocketException (){};

  std::string description() { return m_s; }

 private:

  std::string m_s;

};

#endif
