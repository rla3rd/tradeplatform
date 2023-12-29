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

#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 )
#else
#include "config.h"
#endif

#include "Application.h"
#include "quickfix/MessageCracker.h"
#include "quickfix/Session.h"
#include <iostream>
#include <libpq-fe.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>

#include "string.h"
#include <errno.h>
#include <fcntl.h>
#include <sstream>
#include <vector>
#include <map>

using namespace std;

template < typename T>
std::string string_format(string & val, const T & t)
{
  ostringstream oss; 
  oss << t; 
  return oss.str(); // extract string and assign it to val
}

void Application::onLogon( const FIX::SessionID& sessionID )
{
  cout << endl << "Logon - " << sessionID << endl;
}

void Application::onLogout( const FIX::SessionID& sessionID )
{
  cout << endl << "Logout - " << sessionID << endl;
}

void Application::fromApp( const FIX::Message& message, const FIX::SessionID& sessionID )
throw( FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType )
{
  crack( message, sessionID );
  cout << endl << "IN: " << message << endl;
}

void Application::toApp( FIX::Message& message, const FIX::SessionID& sessionID )
throw( FIX::DoNotSend )
{
  try
  {
    FIX::PossDupFlag possDupFlag;
    message.getHeader().getField( possDupFlag );
    if ( possDupFlag ) throw FIX::DoNotSend();
  }
  catch ( FIX::FieldNotFound& ) {}

  cout << endl
  << "OUT: " << message << endl;
}

void Application::onMessage
( const FIX40::ExecutionReport& message, const FIX::SessionID& sessionID )
{
  FIX::ClOrdID ClOrdID;
  FIX::OrderID OrderID;
  FIX::ExecID ExecID;
  FIX::ExecTransType ExecTransType;
  FIX::OrdStatus OrdStatus;
  FIX::Symbol Symbol;
  FIX::Side Side;
  FIX::LeavesQty LeavesQty;
  FIX::CumQty CumQty;
  FIX::AvgPx AvgPx;
  FIX::LastShares LastShares;
  FIX::LastPx LastPx;
  FIX::TransactTime TransactTime;
  FIX::Text Text;
  PGconn *conn;
  string update, set, where, sql, value;
  PGresult *res;

  message.get( ClOrdID );
  message.get( OrderID );
  message.get( ExecID );
  message.get( ExecTransType );
  message.get( OrdStatus );
  message.get( Symbol );
  message.get( Side );
  message.get( CumQty );
  message.get( AvgPx );

  conn = PQconnectdb( getConnection() );

  if (PQstatus(conn) != CONNECTION_OK)
  {
      fprintf(stderr, "libpq error: Connection to database failed: %s",
              PQerrorMessage(conn));
      exit_nicely(conn);
  }
  cout << "Order Status: " << OrdStatus;
  update = "update orders ";
  if ( OrdStatus == FIX::OrdStatus_NEW )
  {
    set = "set order_status='NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PARTIALLY_FILLED )
  {
    set = "set order_status='PARTIAL', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_FILLED )
  {
    set = "set order_status='FILLED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_DONE_FOR_DAY )
  {
    set = "set order_status='DONE', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CANCELED )
  {
    message.get( ClOrdID );
    set = "set order_status='CANCELED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REPLACED )
  {
    message.get( ClOrdID );
    set = "set order_status='REPLACED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_CANCEL )
  {
    message.get( ClOrdID );
    set = "set order_status='PEND_CXL', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_STOPPED )
  {
    set = "set order_status='STOPPED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REJECTED )
  {
    message.get( Text );
    set = "set order_status='REJECTED', ";
    set += "msg_text='" + string_format(value, Text) +"', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_SUSPENDED )
  {
    set = "set order_status='SUSPENDED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_NEW )
  {
    set = "set order_status='PEND_NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CALCULATED )
  {
    set = "set order_status='CALCULATED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_EXPIRED )
  {
    set = "set order_status='EXPIRED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_ACCEPTED_FOR_BIDDING )
  {
    set = "set order_status='BIDDING', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_REPLACE )
  {
    message.get( ClOrdID );
    set = "set order_status='PEND_REPL', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  set += "cum_qty=" + string_format(value, CumQty) + ", ";
  set += "avg_price=" + string_format(value, AvgPx) +", ";
  set += "order_id='" + string_format(value, OrderID) + "', ";
  set += "exec_trans_type='" + string_format(value, ExecTransType) + "', ";
  set += "msg_time='" + string_format(value, TransactTime) + "', ";
  set += "exec_id='" + string_format(value, ExecID) + "' ";

  sql = update + set + where;
  cout << sql;
  res = PQexec(conn, sql.c_str());
  if (!res) 
  {
    fprintf(stderr, "libpq error: order status not updated\n\n");
    exit_nicely(conn);
  }

  PQclear(res);
  PQfinish(conn);
}

void Application::onMessage
( const FIX40::OrderCancelReject& message, const FIX::SessionID& sessionID )
{
  FIX::OrderID OrderID;
  FIX::ClOrdID ClOrdID;
  FIX::OrigClOrdID OrigClOrdID;
  FIX::OrdStatus OrdStatus;
  FIX::CxlRejResponseTo CxlRejResponseTo;
  FIX::Text Text;
  PGconn *conn;
  string update, set, where, sql, value;
  PGresult *res;

  message.get( OrderID );
  message.get( ClOrdID );
  

  conn = PQconnectdb( getConnection() );

  if (PQstatus(conn) != CONNECTION_OK)
  {
      fprintf(stderr, "libpq error: Connection to database failed: %s",
              PQerrorMessage(conn));
      exit_nicely(conn);
  }
  cout << "Order Status: " << OrdStatus;
  update = "update orders ";
  set = "set order_status='REJECTED', ";
  where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  sql = update + set + where;
  cout << sql;
  res = PQexec(conn, sql.c_str());
  if (!res) 
  {
    fprintf(stderr, "libpq error: order status not updated\n\n");
    exit_nicely(conn);
  }

  PQclear(res);
  PQfinish(conn);
}

void Application::onMessage
( const FIX41::ExecutionReport& message, const FIX::SessionID& sessionID )
{
  FIX::ClOrdID ClOrdID;
  FIX::OrigClOrdID OrigClOrdID;
  FIX::OrderID OrderID;
  FIX::ExecID ExecID;
  FIX::ExecTransType ExecTransType;
  FIX::ExecType ExecType;
  FIX::OrdStatus OrdStatus;
  FIX::Symbol Symbol;
  FIX::Side Side;
  FIX::LeavesQty LeavesQty;
  FIX::CumQty CumQty;
  FIX::AvgPx AvgPx;
  FIX::LastShares LastShares;
  FIX::LastPx LastPx;
  FIX::TransactTime TransactTime;
  FIX::Text Text;
  PGconn *conn;
  string update, set, where, sql, value;
  PGresult *res;

  message.get( ClOrdID );
  message.get( OrderID );
  message.get( ExecID );
  message.get( ExecTransType );
  message.get( ExecType );
  message.get( OrdStatus );
  message.get( Symbol );
  message.get( Side );
  message.get( LeavesQty );
  message.get( CumQty );
  message.get( AvgPx );

  conn = PQconnectdb( getConnection() );

  if (PQstatus(conn) != CONNECTION_OK)
  {
      fprintf(stderr, "libpq error: Connection to database failed: %s",
              PQerrorMessage(conn));
      exit_nicely(conn);
  }
  cout << "Order Status: " << OrdStatus;
  update = "update orders ";
  if ( OrdStatus == FIX::OrdStatus_NEW )
  {
    set = "set order_status='NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PARTIALLY_FILLED )
  {
    set = "set order_status='PARTIAL', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_FILLED )
  {
    set = "set order_status='FILLED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_DONE_FOR_DAY )
  {
    set = "set order_status='DONE', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CANCELED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='CANCELED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REPLACED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='REPLACED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_CANCEL )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_CXL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_STOPPED )
  {
    set = "set order_status='STOPPED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REJECTED )
  {
    message.get( Text );
    set = "set order_status='REJECTED', ";
    set += "msg_text='" + string_format(value, Text) +"', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_SUSPENDED )
  {
    set = "set order_status='SUSPENDED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_NEW )
  {
    set = "set order_status='PEND_NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CALCULATED )
  {
    set = "set order_status='CALCULATED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_EXPIRED )
  {
    set = "set order_status='EXPIRED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_ACCEPTED_FOR_BIDDING )
  {
    set = "set order_status='BIDDING', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_REPLACE )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_REPL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  set += "cum_qty=" + string_format(value, CumQty) + ", ";
  set += "leaves_qty=" + string_format(value, LeavesQty) + ", ";
  set += "avg_price=" + string_format(value, AvgPx) +", ";
  set += "order_id='" + string_format(value, OrderID) + "', ";
  set += "exec_trans_type='" + string_format(value, ExecTransType) + "', ";
  set += "exec_type='" + string_format(value, ExecType) +"', ";
  set += "msg_time='" + string_format(value, TransactTime) + "', ";
  set += "exec_id='" + string_format(value, ExecID) + "' ";

  sql = update + set + where;
  cout << sql;
  res = PQexec(conn, sql.c_str());
  PQclear(res);
  if ( OrdStatus == FIX::OrdStatus_REPLACED )
  {
    message.get( ClOrdID );
    set = "set order_status='NEW' ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  sql = update + set + where;
  cout << sql;
  res = PQexec(conn, sql.c_str());
  if (!res) 
  {
    fprintf(stderr, "libpq error: order status not updated\n\n");
    exit_nicely(conn);
  }
  PQclear(res);
  PQfinish(conn);
}

void Application::onMessage
( const FIX41::OrderCancelReject& message, const FIX::SessionID& sessionID )
{
  FIX::OrderID OrderID;
  FIX::ClOrdID ClOrdID;
  FIX::OrigClOrdID OrigClOrdID;
  FIX::OrdStatus OrdStatus;
  FIX::Text Text;
  PGconn *conn;
  string update, set, where, sql, value;
  PGresult *res;

  message.get( OrderID );
  message.get( ClOrdID );
  message.get( OrigClOrdID );
  message.get( OrdStatus );
  message.get( Text );

  conn = PQconnectdb( getConnection() );

  if (PQstatus(conn) != CONNECTION_OK)
  {
      fprintf(stderr, "libpq error: Connection to database failed: %s",
              PQerrorMessage(conn));
      exit_nicely(conn);
  }
  cout << "Order Status: " << OrdStatus;
  update = "update orders ";
  if ( OrdStatus == FIX::OrdStatus_NEW )
  {
    set = "set order_status='NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PARTIALLY_FILLED )
  {
    set = "set order_status='PARTIAL', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_FILLED )
  {
    set = "set order_status='FILLED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_DONE_FOR_DAY )
  {
    set = "set order_status='DONE', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CANCELED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='CANCELED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REPLACED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='REPLACED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_CANCEL )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_CXL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_STOPPED )
  {
    set = "set order_status='STOPPED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REJECTED )
  {
    message.get( Text );
    set = "set order_status='REJECTED', ";
    set += "msg_text='" + string_format(value, Text) +"', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_SUSPENDED )
  {
    set = "set order_status='SUSPENDED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_NEW )
  {
    set = "set order_status='PEND_NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CALCULATED )
  {
    set = "set order_status='CALCULATED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_EXPIRED )
  {
    set = "set order_status='EXPIRED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_ACCEPTED_FOR_BIDDING )
  {
    set = "set order_status='BIDDING', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_REPLACE )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_REPL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  sql = update + set + where;
  cout << sql;
  res = PQexec(conn, sql.c_str());
  if (!res) 
  {
    fprintf(stderr, "libpq error: order status not updated\n\n");
    exit_nicely(conn);
  }

  PQclear(res);
  PQfinish(conn);
}

void Application::onMessage
( const FIX42::ExecutionReport& message, const FIX::SessionID& sessionID )
{

  FIX::ClOrdID ClOrdID;
  FIX::OrigClOrdID OrigClOrdID;
  FIX::OrderID OrderID;
  FIX::ExecID ExecID;
  FIX::ExecTransType ExecTransType;
  FIX::ExecType ExecType;
  FIX::OrdStatus OrdStatus;
  FIX::Symbol Symbol;
  FIX::Side Side;
  FIX::LeavesQty LeavesQty;
  FIX::CumQty CumQty;
  FIX::AvgPx AvgPx;
  FIX::LastShares LastShares;
  FIX::LastPx LastPx;
  FIX::TransactTime TransactTime;
  FIX::Text Text;
  PGconn *conn;
  string update, set, where, sql, value;
  PGresult *res;

  message.get( ClOrdID );
  message.get( OrderID );
  message.get( ExecID );
  message.get( ExecTransType );
  message.get( ExecType );
  message.get( OrdStatus );
  message.get( Symbol );
  message.get( Side );
  message.get( LeavesQty );
  message.get( CumQty );
  message.get( AvgPx );

  conn = PQconnectdb( getConnection() );

  if (PQstatus(conn) != CONNECTION_OK)
  {
      fprintf(stderr, "libpq error: Connection to database failed: %s",
              PQerrorMessage(conn));
      exit_nicely(conn);
  }
  cout << "Order Status: " << OrdStatus;
  update = "update orders ";
  if ( OrdStatus == FIX::OrdStatus_NEW )
  {
    set = "set order_status='NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PARTIALLY_FILLED )
  {
    set = "set order_status='PARTIAL', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_FILLED )
  {
    set = "set order_status='FILLED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_DONE_FOR_DAY )
  {
    set = "set order_status='DONE', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CANCELED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='CANCELED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REPLACED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='REPLACED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_CANCEL )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_CXL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_STOPPED )
  {
    set = "set order_status='STOPPED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REJECTED )
  {
    message.get( Text );
    set = "set order_status='REJECTED', ";
    set += "msg_text='" + string_format(value, Text) +"', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_SUSPENDED )
  {
    set = "set order_status='SUSPENDED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_NEW )
  {
    set = "set order_status='PEND_NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CALCULATED )
  {
    set = "set order_status='CALCULATED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_EXPIRED )
  {
    set = "set order_status='EXPIRED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_ACCEPTED_FOR_BIDDING )
  {
    set = "set order_status='BIDDING', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_REPLACE )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_REPL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  set += "cum_qty=" + string_format(value, CumQty) + ", ";
  set += "leaves_qty=" + string_format(value, LeavesQty) + ", ";
  set += "avg_price=" + string_format(value, AvgPx) +", ";
  set += "order_id='" + string_format(value, OrderID) + "', ";
  set += "exec_trans_type='" + string_format(value, ExecTransType) + "', ";
  set += "exec_type='" + string_format(value, ExecType) +"', ";
  set += "msg_time='" + string_format(value, TransactTime) + "', ";
  set += "exec_id='" + string_format(value, ExecID) + "' ";

  sql = update + set + where;
  cout << sql;
  res = PQexec(conn, sql.c_str());
  PQclear(res);
  if ( OrdStatus == FIX::OrdStatus_REPLACED )
  {
    message.get( ClOrdID );
    set = "set order_status='NEW' ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  sql = update + set + where;
  cout << sql;
  res = PQexec(conn, sql.c_str());
  if (!res) 
  {
    fprintf(stderr, "libpq error: order status not updated\n\n");
    exit_nicely(conn);
  }
  PQclear(res);
  PQfinish(conn);
}

void Application::onMessage
( const FIX42::OrderCancelReject& message, const FIX::SessionID& sessionID )
{
  FIX::OrderID OrderID;
  FIX::ClOrdID ClOrdID;
  FIX::OrigClOrdID OrigClOrdID;
  FIX::OrdStatus OrdStatus;
  FIX::Text Text;
  PGconn *conn;
  string update, set, where, sql, value;
  PGresult *res;

  message.get( OrderID );
  message.get( ClOrdID );
  message.get( OrigClOrdID );
  message.get( OrdStatus );
  message.get( Text );

  conn = PQconnectdb( getConnection() );

  if (PQstatus(conn) != CONNECTION_OK)
  {
      fprintf(stderr, "libpq error: Connection to database failed: %s",
              PQerrorMessage(conn));
      exit_nicely(conn);
  }
  cout << "Order Status: " << OrdStatus;
  update = "update orders ";
  if ( OrdStatus == FIX::OrdStatus_NEW )
  {
    set = "set order_status='NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PARTIALLY_FILLED )
  {
    set = "set order_status='PARTIAL', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_FILLED )
  {
    set = "set order_status='FILLED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_DONE_FOR_DAY )
  {
    set = "set order_status='DONE', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CANCELED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='CANCELED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REPLACED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='REPLACED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_CANCEL )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_CXL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_STOPPED )
  {
    set = "set order_status='STOPPED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REJECTED )
  {
    message.get( Text );
    set = "set order_status='REJECTED', ";
    set += "msg_text='" + string_format(value, Text) +"', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_SUSPENDED )
  {
    set = "set order_status='SUSPENDED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_NEW )
  {
    set = "set order_status='PEND_NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CALCULATED )
  {
    set = "set order_status='CALCULATED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_EXPIRED )
  {
    set = "set order_status='EXPIRED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_ACCEPTED_FOR_BIDDING )
  {
    set = "set order_status='BIDDING', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_REPLACE )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_REPL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  sql = update + set + where;
  cout << sql;
  res = PQexec(conn, sql.c_str());
  if (!res) 
  {
    fprintf(stderr, "libpq error: order status not updated\n\n");
    exit_nicely(conn);
  }
  PQclear(res);
  PQfinish(conn);
}

void Application::onMessage
( const FIX43::ExecutionReport& message, const FIX::SessionID& sessionID)
{
  FIX::ClOrdID ClOrdID;
  FIX::OrigClOrdID OrigClOrdID;
  FIX::OrderID OrderID;
  FIX::ExecID ExecID;
  FIX::ExecType ExecType;
  FIX::OrdStatus OrdStatus;
  FIX::Symbol Symbol;
  FIX::Side Side;
  FIX::LeavesQty LeavesQty;
  FIX::CumQty CumQty;
  FIX::AvgPx AvgPx;
  FIX::LastShares LastShares;
  FIX::LastPx LastPx;
  FIX::TransactTime TransactTime;
  FIX::Text Text;
  PGconn *conn;
  string update, set, where, sql, value;
  PGresult *res;

  message.get( ClOrdID );
  message.get( OrderID );
  message.get( ExecID );
  message.get( ExecType );
  message.get( OrdStatus );
  message.get( Symbol );
  message.get( Side );
  message.get( LeavesQty );
  message.get( CumQty );
  message.get( AvgPx );

  conn = PQconnectdb( getConnection() );

  if (PQstatus(conn) != CONNECTION_OK)
  {
      fprintf(stderr, "libpq error: Connection to database failed: %s",
              PQerrorMessage(conn));
      exit_nicely(conn);
  }
  cout << "Order Status: " << OrdStatus;
  update = "update orders ";
  if ( OrdStatus == FIX::OrdStatus_NEW )
  {
    set = "set order_status='NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PARTIALLY_FILLED )
  {
    set = "set order_status='PARTIAL', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_FILLED )
  {
    set = "set order_status='FILLED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_DONE_FOR_DAY )
  {
    set = "set order_status='DONE', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CANCELED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='CANCELED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REPLACED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='REPLACED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_CANCEL )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_CXL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_STOPPED )
  {
    set = "set order_status='STOPPED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REJECTED )
  {
    message.get( Text );
    set = "set order_status='REJECTED', ";
    set += "msg_text='" + string_format(value, Text) +"', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_SUSPENDED )
  {
    set = "set order_status='SUSPENDED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_NEW )
  {
    set = "set order_status='PEND_NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CALCULATED )
  {
    set = "set order_status='CALCULATED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_EXPIRED )
  {
    set = "set order_status='EXPIRED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_ACCEPTED_FOR_BIDDING )
  {
    set = "set order_status='BIDDING', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_REPLACE )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_REPL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  set += "cum_qty=" + string_format(value, CumQty) + ", ";
  set += "leaves_qty=" + string_format(value, LeavesQty) + ", ";
  set += "avg_price=" + string_format(value, AvgPx) +", ";
  set += "order_id='" + string_format(value, OrderID) + "', ";
  set += "exec_type='" + string_format(value, ExecType) +"', ";
  set += "msg_time='" + string_format(value, TransactTime) + "', ";
  set += "exec_id='" + string_format(value, ExecID) + "' ";

  sql = update + set + where;
  cout << sql;
  res = PQexec(conn, sql.c_str());
  PQclear(res);
  if ( OrdStatus == FIX::OrdStatus_REPLACED )
  {
    message.get( ClOrdID );
    set = "set order_status='NEW' ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  sql = update + set + where;
  cout << sql;
  res = PQexec(conn, sql.c_str());
  if (!res) 
  {
    fprintf(stderr, "libpq error: order status not updated\n\n");
    exit_nicely(conn);
  }
  PQclear(res);
  PQfinish(conn);
}

void Application::onMessage
( const FIX43::OrderCancelReject& message, const FIX::SessionID& sessionID)
{
  FIX::OrderID OrderID;
  FIX::ClOrdID ClOrdID;
  FIX::OrigClOrdID OrigClOrdID;
  FIX::OrdStatus OrdStatus;
  FIX::Text Text;
  PGconn *conn;
  string update, set, where, sql, value;
  PGresult *res;

  message.get( OrderID );
  message.get( ClOrdID );
  message.get( OrigClOrdID );
  message.get( OrdStatus );
  message.get( Text );

  conn = PQconnectdb( getConnection() );

  if (PQstatus(conn) != CONNECTION_OK)
  {
      fprintf(stderr, "libpq error: Connection to database failed: %s",
              PQerrorMessage(conn));
      exit_nicely(conn);
  }
  cout << "Order Status: " << OrdStatus;
  update = "update orders ";
  if ( OrdStatus == FIX::OrdStatus_NEW )
  {
    set = "set order_status='NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PARTIALLY_FILLED )
  {
    set = "set order_status='PARTIAL', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_FILLED )
  {
    set = "set order_status='FILLED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_DONE_FOR_DAY )
  {
    set = "set order_status='DONE', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CANCELED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='CANCELED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REPLACED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='REPLACED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_CANCEL )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_CXL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_STOPPED )
  {
    set = "set order_status='STOPPED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REJECTED )
  {
    message.get( Text );
    set = "set order_status='REJECTED', ";
    set += "msg_text='" + string_format(value, Text) +"', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_SUSPENDED )
  {
    set = "set order_status='SUSPENDED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_NEW )
  {
    set = "set order_status='PEND_NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CALCULATED )
  {
    set = "set order_status='CALCULATED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_EXPIRED )
  {
    set = "set order_status='EXPIRED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_ACCEPTED_FOR_BIDDING )
  {
    set = "set order_status='BIDDING', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_REPLACE )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_REPL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  sql = update + set + where;
  cout << sql;
  res = PQexec(conn, sql.c_str());
  if (!res) 
  {
    fprintf(stderr, "libpq error: order status not updated\n\n");
    exit_nicely(conn);
  }
  PQclear(res);
  PQfinish(conn);
}

void Application::onMessage
( const FIX44::ExecutionReport& message, const FIX::SessionID& sessionID)
{
  FIX::ClOrdID ClOrdID;
  FIX::OrigClOrdID OrigClOrdID;
  FIX::OrderID OrderID;
  FIX::ExecID ExecID;
  FIX::ExecType ExecType;
  FIX::OrdStatus OrdStatus;
  FIX::Symbol Symbol;
  FIX::Side Side;
  FIX::LeavesQty LeavesQty;
  FIX::CumQty CumQty;
  FIX::AvgPx AvgPx;
  FIX::LastShares LastShares;
  FIX::LastPx LastPx;
  FIX::TransactTime TransactTime;
  FIX::Text Text;
  PGconn *conn;
  string update, set, where, sql, value;
  PGresult *res;

  message.get( ClOrdID );
  message.get( OrderID );
  message.get( ExecID );
  message.get( ExecType );
  message.get( OrdStatus );
  message.get( Symbol );
  message.get( Side );
  message.get( LeavesQty );
  message.get( CumQty );
  message.get( AvgPx );

  conn = PQconnectdb( getConnection() );

  if (PQstatus(conn) != CONNECTION_OK)
  {
      fprintf(stderr, "libpq error: Connection to database failed: %s",
              PQerrorMessage(conn));
      exit_nicely(conn);
  }
  cout << "Order Status: " << OrdStatus;
  update = "update orders ";
  if ( OrdStatus == FIX::OrdStatus_NEW )
  {
    set = "set order_status='NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PARTIALLY_FILLED )
  {
    set = "set order_status='PARTIAL', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_FILLED )
  {
    set = "set order_status='FILLED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_DONE_FOR_DAY )
  {
    set = "set order_status='DONE', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CANCELED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='CANCELED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REPLACED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='REPLACED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_CANCEL )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_CXL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_STOPPED )
  {
    set = "set order_status='STOPPED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REJECTED )
  {
    message.get( Text );
    set = "set order_status='REJECTED', ";
    set += "msg_text='" + string_format(value, Text) +"', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_SUSPENDED )
  {
    set = "set order_status='SUSPENDED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_NEW )
  {
    set = "set order_status='PEND_NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CALCULATED )
  {
    set = "set order_status='CALCULATED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_EXPIRED )
  {
    set = "set order_status='EXPIRED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_ACCEPTED_FOR_BIDDING )
  {
    set = "set order_status='BIDDING', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_REPLACE )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_REPL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  set += "cum_qty=" + string_format(value, CumQty) + ", ";
  set += "leaves_qty=" + string_format(value, LeavesQty) + ", ";
  set += "avg_price=" + string_format(value, AvgPx) +", ";
  set += "order_id='" + string_format(value, OrderID) + "', ";
  set += "exec_type='" + string_format(value, ExecType) +"', ";
  set += "msg_time='" + string_format(value, TransactTime) + "', ";
  set += "exec_id='" + string_format(value, ExecID) + "' ";

  sql = update + set + where;
  cout << sql;
  res = PQexec(conn, sql.c_str());
  PQclear(res);
  if ( OrdStatus == FIX::OrdStatus_REPLACED )
  {
    message.get( ClOrdID );
    set = "set order_status='NEW' ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  sql = update + set + where;
  cout << sql;
  res = PQexec(conn, sql.c_str());
  if (!res) 
  {
    fprintf(stderr, "libpq error: order status not updated\n\n");
    exit_nicely(conn);
  }
  PQclear(res);
  PQfinish(conn);
}

void Application::onMessage
( const FIX44::OrderCancelReject& message, const FIX::SessionID& sessionID)
{
  FIX::OrderID OrderID;
  FIX::ClOrdID ClOrdID;
  FIX::OrigClOrdID OrigClOrdID;
  FIX::OrdStatus OrdStatus;
  FIX::Text Text;
  PGconn *conn;
  string update, set, where, sql, value;
  PGresult *res;

  message.get( OrderID );
  message.get( ClOrdID );
  message.get( OrigClOrdID );
  message.get( OrdStatus );
  message.get( Text );

  conn = PQconnectdb( getConnection() );

  if (PQstatus(conn) != CONNECTION_OK)
  {
      fprintf(stderr, "libpq error: Connection to database failed: %s",
              PQerrorMessage(conn));
      exit_nicely(conn);
  }
  cout << "Order Status: " << OrdStatus;
  update = "update orders ";
  if ( OrdStatus == FIX::OrdStatus_NEW )
  {
    set = "set order_status='NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PARTIALLY_FILLED )
  {
    set = "set order_status='PARTIAL', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_FILLED )
  {
    set = "set order_status='FILLED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_DONE_FOR_DAY )
  {
    set = "set order_status='DONE', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CANCELED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='CANCELED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REPLACED )
  {
    message.get( OrigClOrdID );
    set = "set order_status='REPLACED', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_CANCEL )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_CXL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_STOPPED )
  {
    set = "set order_status='STOPPED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_REJECTED )
  {
    message.get( Text );
    set = "set order_status='REJECTED', ";
    set += "msg_text='" + string_format(value, Text) +"', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_SUSPENDED )
  {
    set = "set order_status='SUSPENDED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_NEW )
  {
    set = "set order_status='PEND_NEW', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_CALCULATED )
  {
    set = "set order_status='CALCULATED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_EXPIRED )
  {
    set = "set order_status='EXPIRED', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_ACCEPTED_FOR_BIDDING )
  {
    set = "set order_status='BIDDING', ";
    where = " where clordid='" + string_format(value, ClOrdID) + "'; ";
  }
  else if ( OrdStatus == FIX::OrdStatus_PENDING_REPLACE )
  {
    message.get( OrigClOrdID );
    set = "set order_status='PEND_REPL', ";
    where = " where clordid='" + string_format(value, OrigClOrdID) + "'; ";
  }
  sql = update + set + where;
  cout << sql;
  res = PQexec(conn, sql.c_str());
  if (!res) 
  {
    fprintf(stderr, "libpq error: order status not updated\n\n");
    exit_nicely(conn);
  }

  PQclear(res);
  PQfinish(conn);
}

void Tokenize(const string& str,
                      vector<string>& tokens,
                      const string& delimiters = " ")
{
  // Skip delimiters at beginning.
  string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  string::size_type pos     = str.find_first_of(delimiters, lastPos);

  while (string::npos != pos || string::npos != lastPos)
  {
      // Found a token, add it to the vector.
      tokens.push_back(str.substr(lastPos, pos - lastPos));
      // Skip delimiters.  Note the "not_of"
      lastPos = str.find_first_not_of(delimiters, pos);
      // Find next "non-delimiter"
      pos = str.find_first_of(delimiters, lastPos);
  }
}

std::string quote(string & val)
{
  ostringstream oss; 
  oss << "'" << val << "'"; 
  return oss.str(); // extract string and assign it to val
}

void Application::run()
{
  try
  {
    // Create the socket
    ServerSocket server ( 30000 );

    while ( true )
    {

      ServerSocket new_sock;
      server.accept ( new_sock );

      try
      {
        while ( true )
        {
          vector<string> msgtokens, gsattokens;
          string data;
          string type, ParentID, MMID, OrigClOrdID, ClOrdID, ExDest, Symbol, TIF, Side, OrdType, strQuantity, strMaxFloor, strMinQty, strPrice, strStopPx, strPegDiff, strDiscOffset, ExecInst, DiscInst, AlgoInst;
          int inttype, Quantity, MaxFloor, MinQty;
          double Price, StopPx, PegDiff, DiscOffset;
          ostringstream os;
          new_sock >> data;
          cout << data;
          Tokenize(data, msgtokens, "|");
          type = (string) msgtokens.at(0);
          inttype = atoi( type.c_str() ) ;

          switch ( inttype ) {
             case 0:
               ParentID = ( string ) msgtokens.at(1);
               MMID = ( string ) msgtokens.at(2);
               ClOrdID = ( string ) msgtokens.at(3);
               Symbol = ( string ) msgtokens.at(4);
               ExDest = ( string ) msgtokens.at(5);
               TIF = ( string ) msgtokens.at(6);
               Side = ( string ) msgtokens.at(7);
               strQuantity = ( string ) msgtokens.at(8);
               Quantity = atoi( strQuantity.c_str() );
               strMaxFloor = ( string ) msgtokens.at(9);
               MaxFloor = atoi( strMaxFloor.c_str() );
               strMinQty = ( string ) msgtokens.at(10);
               MinQty =  atoi( strMinQty.c_str() );
               OrdType = ( string ) msgtokens.at(11);
               strPrice = ( string ) msgtokens.at(12);
               Price = strtod( strPrice.c_str(), NULL );
               strStopPx = ( string ) msgtokens.at(13);
               StopPx = strtod( strStopPx.c_str(), NULL );
               strPegDiff = ( string ) msgtokens.at(14);
               PegDiff = strtod(strPegDiff.c_str(), NULL );
               DiscInst = ( string ) msgtokens.at(15);
               strDiscOffset = ( string ) msgtokens.at(16);
               DiscOffset = strtod( strDiscOffset.c_str(), NULL );
               ExecInst = ( string ) msgtokens.at(17);
               AlgoInst = ( string ) msgtokens.at(18);
               queryEnterOrder( ParentID, MMID, ClOrdID, Symbol, ExDest, TIF, Side, Quantity, MaxFloor, MinQty, OrdType, Price, StopPx, PegDiff, DiscInst, DiscOffset, ExecInst, AlgoInst );
               os << inttype;
               new_sock << os.str();
               break;
             case 1:
               ParentID = ( string ) msgtokens.at(1);
               MMID = ( string ) msgtokens.at(2);
               OrigClOrdID = ( string ) msgtokens.at(3);
               ClOrdID = ( string ) msgtokens.at(4);
               Symbol = ( string ) msgtokens.at(5);
               ExDest = ( string ) msgtokens.at(6);
               Side = ( string ) msgtokens.at(7);
               strQuantity = ( string ) msgtokens.at(8);
               Quantity = atoi( strQuantity.c_str() );
               queryCancelOrder( ParentID, MMID, OrigClOrdID, ClOrdID, Symbol, ExDest, Side, Quantity );
               os << inttype;
               new_sock << os.str();
               break;
             case 2:
               ParentID = ( string ) msgtokens.at(1);
               MMID = ( string ) msgtokens.at(2);
               OrigClOrdID = ( string ) msgtokens.at(3);
               ClOrdID = ( string ) msgtokens.at(4);
               Symbol = ( string ) msgtokens.at(5);
               ExDest = (string ) msgtokens.at(6);
               TIF = ( string ) msgtokens.at(7);
               Side = ( string ) msgtokens.at(8);
               strQuantity = ( string ) msgtokens.at(9);
               Quantity = atoi( strQuantity.c_str() );
               strMaxFloor = ( string ) msgtokens.at(10);
               MaxFloor = atoi( strMaxFloor.c_str() );
               strMinQty = ( string ) msgtokens.at(11);
               MinQty =  atoi( strMinQty.c_str() );
               OrdType = ( string ) msgtokens.at(12);
               strPrice = ( string ) msgtokens.at(13);
               Price = strtod( strPrice.c_str(), NULL );
               strStopPx = ( string ) msgtokens.at(14);
               StopPx = strtod( strStopPx.c_str(), NULL );
               strPegDiff = ( string ) msgtokens.at(15);
               PegDiff = strtod( strPegDiff.c_str(), NULL );
               DiscInst = ( string ) msgtokens.at(16);
               strDiscOffset = ( string ) msgtokens.at(17);
               DiscOffset = strtod( strDiscOffset.c_str(), NULL );
               ExecInst = ( string ) msgtokens.at(18);
               AlgoInst = ( string ) msgtokens.at(19);
               queryReplaceOrder( ParentID, MMID, OrigClOrdID, ClOrdID, Symbol, ExDest, TIF, Side, Quantity, MaxFloor, MinQty, OrdType, Price, StopPx, PegDiff, DiscInst, DiscOffset, ExecInst, AlgoInst );
               os << inttype;
               new_sock << os.str();
               break;
             case 3:
               MMID = ( string ) msgtokens.at(1);
               Symbol = ( string ) msgtokens.at(2);
               queryMarketDataRequest( MMID, Symbol );
               os << inttype;
               new_sock << os.str();
               break;

//              case 4:
//                MMID = ( string ) msgtokens.at(1);
//                ExecID = ( string ) msgtokens.at(2);
//                Account = ( string ) msgtokens.at(3);
//                strLastShares = ( string ) msgtokens.at(4);
//                LastShares = atoi( strLastShares.c_str() );
//                ExecTransType = ( string ) msgtokens.at(5);
//                SenderSubID = ( string ) msgtokens.at(6);
//                strLastPx = ( string ) mstokens.at(7);
//                LastPx = strtod( strLastPx.c_str(), NULL );
//                TransactTime = ( string ) msgtokens.at(8);
//                ExecBroker = ( string ) msgtokens.at(9);
//                queryExecutionReport( MMID, ExecID, Account, LastShares, ExecTransType, SenderSubID, LastPx, TransactTime, ExecBroker );
             default:
               cerr << "No instruction for type: " << type << endl;
               os << -1;
               new_sock << os.str();
               break;
           }

          // cout << "\nSocket Data: "<< data << "\n";
        }
      }
      catch ( SocketException& ) {}
    }
  }
  catch ( SocketException& e )
  {
    std::cout << "Exception was caught:" << e.description() << "\nExiting.\n";
  }
}

void Application::queryEnterOrder( std::string ParentID,
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
                                   std::string AlgoInst )
{
  ConnectInfo info;
  info = queryBroker(MMID);
  PGconn *conn;
  string sql, value;
  PGresult *res;

  conn = PQconnectdb( getConnection() );

  if (PQstatus(conn) != CONNECTION_OK)
  {
      fprintf(stderr, "libpq error: Connection to database failed: %s",
              PQerrorMessage(conn));
      exit_nicely(conn);
  }

  sql = "insert into orders(parent_id, mmid, clordid, symbol, exchange, tif, side, quantity, show_qty, min_qty, ordtype, ";
  sql += "limit_price, stop_price, peg_offset, disc_inst, disc_offset, exec_inst, algo_inst, order_status ) ";
  sql += "values( ";
  sql += quote(ParentID) + ", ";
  sql += quote(MMID) + ", ";
  sql += quote(ClOrdID) + ", ";
  sql += quote(Symbol) + ", ";
  sql += quote(ExDest) + ", ";
  sql += quote(TIF) + ", ";
  sql += quote(Side) + ", ";
  sql += string_format(value, Shares) + ", ";
  sql += string_format(value, MaxFloor) + ", ";
  sql += string_format(value, MinQty) + ", ";
  sql += quote(OrdType) + ", ";
  sql += string_format(value, Price) + ", ";
  sql += string_format(value, StopPx) + ", ";
  sql += string_format(value, PegDiff) + ", ";
  sql += quote(DiscInst) + ", ";
  sql += string_format(value, DiscOffset) + ", ";
  sql += quote(ExecInst) + ", ";
  sql +=quote(AlgoInst) + ", ";
  sql += "'PEND_NEW' ); ";
   cout << "SQL: " << sql << "\n";;
   res = PQexec(conn, sql.c_str());
   if (!res)
   {
     fprintf(stderr, "libpq error: bad insert\n\n");
     exit_nicely(conn);
   }
   PQclear(res);
   PQfinish(conn);
  //cout << "\nFix Version Selected.\n" << info.Version;
  switch ( info.Version ) {
  case 40:
    queryNewOrderSingle40( info.DeskID, info.SenderCompID, info.TargetCompID, ClOrdID, Symbol, ExDest, TIF, Side, Shares, MaxFloor, MinQty, OrdType, Price, StopPx, PegDiff, DiscInst, DiscOffset, ExecInst, AlgoInst );
    break;
  case 41:
    queryNewOrderSingle41( info.DeskID, info.SenderCompID, info.TargetCompID, ClOrdID, Symbol, ExDest, TIF, Side, Shares, MaxFloor, MinQty, OrdType, Price, StopPx, PegDiff, DiscInst, DiscOffset, ExecInst, AlgoInst );
    break;
  case 42:
    queryNewOrderSingle42( info.DeskID, info.SenderCompID, info.TargetCompID, ClOrdID, Symbol, ExDest, TIF, Side, Shares, MaxFloor, MinQty, OrdType, Price, StopPx, PegDiff, DiscInst, DiscOffset, ExecInst, AlgoInst );
    break;
  case 43:
    queryNewOrderSingle43( info.DeskID, info.SenderCompID, info.TargetCompID, ClOrdID, Symbol, ExDest, TIF, Side, Shares, MaxFloor, MinQty, OrdType, Price, StopPx, PegDiff, DiscInst, DiscOffset, ExecInst, AlgoInst );
    break;
  case 44:
    queryNewOrderSingle44( info.DeskID, info.SenderCompID, info.TargetCompID, ClOrdID, Symbol, ExDest, TIF, Side, Shares, MaxFloor, MinQty, OrdType, Price, StopPx, PegDiff, DiscInst, DiscOffset, ExecInst, AlgoInst );
  default:
    cerr << "No test for version " << info.Version << endl;
    break;
  }
}

void Application::queryCancelOrder( std::string ParentID,
                                    std::string MMID,
                                    std::string OrigClOrdID,
                                    std::string ClOrdID,
                                    std::string Symbol,
                                    std::string ExDest,
                                    std::string Side,
                                    long Shares )
{
  cout << "\nOrderCancelRequest\n";
  ConnectInfo info;
  info = queryBroker(MMID);
  cout << "Version: " << info.Version << "\n";
  switch ( info.Version ) {
  case 40:
    queryOrderCancelRequest40( info.DeskID, info.SenderCompID, info.TargetCompID, OrigClOrdID, ClOrdID, Symbol, ExDest, Side, Shares );
    break;
  case 41:
    queryOrderCancelRequest41( info.DeskID, info.SenderCompID, info.TargetCompID, OrigClOrdID, ClOrdID, Symbol, ExDest, Side, Shares );
    break;
  case 42:
    queryOrderCancelRequest42( info.DeskID, info.SenderCompID, info.TargetCompID, OrigClOrdID, ClOrdID, Symbol, ExDest, Side, Shares );
    break;
  case 43:
    queryOrderCancelRequest43( info.DeskID, info.SenderCompID, info.TargetCompID, OrigClOrdID, ClOrdID, Symbol, ExDest, Side, Shares );
    break;
  case 44:
    queryOrderCancelRequest44( info.DeskID, info.SenderCompID, info.TargetCompID, OrigClOrdID, ClOrdID, Symbol, ExDest, Side, Shares );
    break;
  default:
    cerr << "No test for version " << info.Version << endl;
    break;
  }
}

void Application::queryReplaceOrder( std::string ParentID,
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
                                     std::string AlgoInst )
{
  cout << "\nCancelReplaceRequest\n";
  ConnectInfo info;
  info = queryBroker(MMID);
  cout << "Version: " << info.Version << "\n";
  PGconn *conn;
  string sql, value;
  PGresult *res;

  conn = PQconnectdb( getConnection() );

  if (PQstatus(conn) != CONNECTION_OK)
  {
      fprintf(stderr, "libpq error: Connection to database failed: %s",
              PQerrorMessage(conn));
      exit_nicely(conn);
  }

  sql = "insert into orders(parent_id, mmid, origclordid, clordid, symbol, exchange, tif, side, quantity, show_qty, min_qty, ordtype, ";
  sql += "limit_price, stop_price, peg_offset, disc_inst, disc_offset, exec_inst, algo_inst, order_status ) ";
  sql += "values( ";
  sql += quote(ParentID) + ", ";
  sql += quote(MMID) + ", ";
  sql += quote(OrigClOrdID) + ", ";
  sql += quote(ClOrdID) + ", ";
  sql += quote(Symbol) + ", ";
  sql += quote(ExDest) + ", ";
  sql += quote(TIF) + ", ";
  sql += quote(Side) + ", ";
  sql += string_format(value, Shares) + ", ";
  sql += string_format(value, MaxFloor) + ", ";
  sql += string_format(value, MinQty) + ", ";
  sql += quote(OrdType) + ", ";
  sql += string_format(value, Price) + ", ";
  sql += string_format(value, StopPx) + ", ";
  sql += string_format(value, PegDiff) + ", ";
  sql += quote(DiscInst) + ", ";
  sql += string_format(value, DiscOffset) + ", ";
  sql += quote(ExecInst) + ", ";
  sql +=quote(AlgoInst) + ", ";
  sql += "'PEND_NEW' ); ";
   cout << "SQL: " << sql << "\n";;
   res = PQexec(conn, sql.c_str());
   if (!res)
   {
     fprintf(stderr, "libpq error: bad insert\n\n");
     exit_nicely(conn);
   }
   PQclear(res);
   PQfinish(conn);
  switch ( info.Version ) {
  case 40:
    queryCancelReplaceRequest40( info.DeskID, info.SenderCompID, info.TargetCompID, OrigClOrdID, ClOrdID, Symbol, ExDest, TIF, Side, Shares, MaxFloor, MinQty, OrdType, Price, StopPx, PegDiff, DiscInst, DiscOffset, ExecInst, AlgoInst );
    break;
  case 41:
    queryCancelReplaceRequest41( info.DeskID, info.SenderCompID, info.TargetCompID, OrigClOrdID, ClOrdID, Symbol, ExDest, TIF, Side, Shares, MaxFloor, MinQty, OrdType, Price, StopPx, PegDiff, DiscInst, DiscOffset, ExecInst, AlgoInst );
    break;
  case 42:
    queryCancelReplaceRequest42( info.DeskID, info.SenderCompID, info.TargetCompID, OrigClOrdID, ClOrdID, Symbol, ExDest, TIF, Side, Shares, MaxFloor, MinQty, OrdType, Price, StopPx, PegDiff, DiscInst, DiscOffset, ExecInst, AlgoInst );
    break;
  case 43:
    queryCancelReplaceRequest43( info.DeskID, info.SenderCompID, info.TargetCompID, OrigClOrdID, ClOrdID, Symbol, ExDest, TIF, Side, Shares, MaxFloor, MinQty, OrdType, Price, StopPx, PegDiff, DiscInst, DiscOffset, ExecInst, AlgoInst );
    break;
  case 44:
    queryCancelReplaceRequest44( info.DeskID, info.SenderCompID, info.TargetCompID, OrigClOrdID, ClOrdID, Symbol, ExDest, TIF, Side, Shares, MaxFloor, MinQty, OrdType, Price, StopPx, PegDiff, DiscInst, DiscOffset, ExecInst, AlgoInst );
    break;
  default:
    cerr << "No test for version " << info.Version << endl;
    break;
  }
}

void Application::queryMarketDataRequest( std::string MMID,
                                          std::string Symbol )
{
  cout << "\nMarketDataRequest\n";
  FIX::Message md;
  ConnectInfo info;
  info = queryBroker(MMID);

  switch (info.Version) {
  case 43:
    queryMarketDataRequest43( info.SenderCompID, info.TargetCompID, Symbol );
    break;
  case 44:
    queryMarketDataRequest44( info.SenderCompID, info.TargetCompID, Symbol );
    break;
  default:
    cerr << "No test for version " << info.Version << endl;
    break;
  }
}

void Application::queryNewOrderSingle40( string DeskID, string SenderCompID, string TargetCompID, 
     string ClOrdID, string Symbol, string ExDest, string TIF, string Side, long Shares, long MaxFloor, long MinQty, string OrdType, double Price, double StopPx, double PegDiff, string DiscInst, double DiscOffset, string ExecInst, string AlgoInst )
{
  vector<string> tokens;
  FIX::OrdType ordType;
  string SymbolSfx;

  FIX40::NewOrderSingle newOrderSingle(
    FIX::ClOrdID( ClOrdID ), FIX::HandlInst( '1' ), FIX::Symbol( Symbol ), querySide( Side ),
    FIX::OrderQty( Shares ), queryOrdType( OrdType ) );

  FIX::Header& header = newOrderSingle.getHeader();
  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    newOrderSingle.set( FIX::Symbol( Symbol ) );
    newOrderSingle.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    newOrderSingle.set( FIX::Symbol( Symbol ) );
  }
  if ( MaxFloor != 0 )
  {
    newOrderSingle.set( FIX::MaxFloor( MaxFloor ) );
  }
  if ( MinQty != 0 )
  {
    newOrderSingle.set( FIX::MinQty( MinQty ) );
  }
  newOrderSingle.set( queryTimeInForce( TIF ) );
  if ( ordType == FIX::OrdType_LIMIT || ordType == FIX::OrdType_STOP_LIMIT )
    newOrderSingle.set( FIX::Price( Price ) );
  if ( ordType == FIX::OrdType_STOP || ordType == FIX::OrdType_STOP_LIMIT )
    newOrderSingle.set( FIX::StopPx( StopPx ) );
  if ( querySide( Side ) == FIX::Side_SELL_SHORT )
    newOrderSingle.set( FIX::LocateReqd( 'Y' ) );

  if ( ExDest==";" && TargetCompID=="REDI" )
  {
    newOrderSingle.setField( 100, ExDest );
    newOrderSingle.setField(11007, "1");
  }
  else
  {
    newOrderSingle.setField( 100, ExDest );
  }

  newOrderSingle.set( FIX::ExecInst( ExecInst ) );


  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );
  if ( DeskID != "0" )
  {
    header.setField( FIX::DeliverToLocationID( DeskID ) );
  }
  parseAlgoInst( newOrderSingle, TargetCompID, DeskID, AlgoInst );
  FIX::Session::sendToTarget( newOrderSingle );
}

void Application::queryNewOrderSingle41( string DeskID, string SenderCompID, string TargetCompID, 
     string ClOrdID, string Symbol, string ExDest, string TIF, string Side, long Shares, long MaxFloor, long MinQty, string OrdType, double Price, double StopPx, double PegDiff, string DiscInst, double DiscOffset, string ExecInst, string AlgoInst )
{
  vector<string> tokens;
  FIX::OrdType ordType;
  string SymbolSfx;

  FIX41::NewOrderSingle newOrderSingle(
    FIX::ClOrdID( ClOrdID ), FIX::HandlInst( '1' ), FIX::Symbol( Symbol ), querySide( Side ),
    queryOrdType( OrdType ) );

  FIX::Header& header = newOrderSingle.getHeader();

  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    newOrderSingle.set( FIX::Symbol( Symbol ) );
    newOrderSingle.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    newOrderSingle.set( FIX::Symbol( Symbol ) );
  }

  newOrderSingle.set( FIX::OrderQty( Shares ) );
  if ( MaxFloor != 0 )
  {
    newOrderSingle.set( FIX::MaxFloor( MaxFloor ) );
  }
  if ( MinQty != 0 )
  {
    newOrderSingle.set( FIX::MinQty( MinQty ) );
  }
  newOrderSingle.set( queryTimeInForce( TIF ) );
  if ( ordType == FIX::OrdType_LIMIT || ordType == FIX::OrdType_STOP_LIMIT )
    newOrderSingle.set( FIX::Price( Price ) );
  if ( ordType == FIX::OrdType_STOP || ordType == FIX::OrdType_STOP_LIMIT )
    newOrderSingle.set( FIX::StopPx( StopPx ) );
  if ( querySide( Side ) == FIX::Side_SELL_SHORT )
    newOrderSingle.set( FIX::LocateReqd( 'Y' ) );
  if (ExDest==";")
  {
    newOrderSingle.setField( 100, ExDest );
    newOrderSingle.setField(11007, "1");
  }
  else
  {
    newOrderSingle.setField( 100, ExDest );
  }

  newOrderSingle.set( FIX::ExecInst( ExecInst ) );

  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );
  if ( DeskID != "0" )
  {
    header.setField( FIX::DeliverToLocationID( DeskID ) );
  }
  parseAlgoInst( newOrderSingle, TargetCompID, DeskID, AlgoInst );
  FIX::Session::sendToTarget( newOrderSingle );
}

void  Application::queryNewOrderSingle42( string DeskID, string SenderCompID, string TargetCompID,
      string ClOrdID, string Symbol, string ExDest, string TIF, string Side, long Shares, long MaxFloor, long MinQty, string OrdType, double Price, double StopPx, double PegDiff, string DiscInst, double DiscOffset, string ExecInst, string AlgoInst )
{
  vector<string> tokens;
  FIX::OrdType ordType;
  string SymbolSfx;

  FIX42::NewOrderSingle newOrderSingle(
    FIX::ClOrdID( ClOrdID ), FIX::HandlInst( '1' ), FIX::Symbol( Symbol ), querySide( Side ),
    FIX::TransactTime(), ordType = queryOrdType( OrdType ) );

  FIX::Header& header = newOrderSingle.getHeader();

  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    newOrderSingle.set( FIX::Symbol( Symbol ) );
    newOrderSingle.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    newOrderSingle.set( FIX::Symbol( Symbol ) );
  }
  newOrderSingle.set( FIX::OrderQty( Shares ) );
  if ( MaxFloor != 0 )
  {
    newOrderSingle.set( FIX::MaxFloor( MaxFloor ) );
  }
  if ( MinQty != 0 )
  {
    newOrderSingle.set( FIX::MinQty( MinQty ) );
  }
  newOrderSingle.set( queryTimeInForce( TIF ) );
  if ( ordType == FIX::OrdType_LIMIT || ordType == FIX::OrdType_STOP_LIMIT )
    newOrderSingle.set( FIX::Price( Price ) );
  if ( ordType == FIX::OrdType_STOP || ordType == FIX::OrdType_STOP_LIMIT )
    newOrderSingle.set( FIX::StopPx( StopPx ) );
  if ( querySide( Side ) == FIX::Side_SELL_SHORT )
    newOrderSingle.set( FIX::LocateReqd( 'Y' ) );
  if ( PegDiff != 0 )
  {
    newOrderSingle.set( FIX::PegDifference( PegDiff ) );
  }
  if (DiscOffset != 0)
  {
    newOrderSingle.set( FIX::DiscretionInst( *DiscInst.c_str() ) );
    newOrderSingle.set( FIX::DiscretionOffset( DiscOffset ) );
  }
  if ( ExDest==";" && TargetCompID=="REDI" )
  {
    newOrderSingle.setField( 100, ExDest );
    newOrderSingle.setField(11007, "1");
  }
  else
  {
    newOrderSingle.setField( 100, ExDest );
  }

  newOrderSingle.set( FIX::ExecInst( ExecInst ) );

  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );
  if ( DeskID != "0" )
  {
    header.setField( FIX::DeliverToLocationID( DeskID ) );
  }
  parseAlgoInst( newOrderSingle, TargetCompID, DeskID, AlgoInst );
  FIX::Session::sendToTarget( newOrderSingle );
}

void Application::queryNewOrderSingle43( string DeskID, string SenderCompID, string TargetCompID,
     string ClOrdID, string Symbol, string ExDest, string TIF, string Side, long Shares, long MaxFloor, long MinQty, string OrdType, double Price, double StopPx, double PegDiff, string DiscInst, double DiscOffset, string ExecInst, string AlgoInst )
{
  vector<string> tokens;
  FIX::OrdType ordType;
  string SymbolSfx;

  FIX43::NewOrderSingle newOrderSingle(
    FIX::ClOrdID( ClOrdID ), FIX::HandlInst( '1' ), querySide( Side ),
    FIX::TransactTime(), ordType = queryOrdType( OrdType ) );

  FIX::Header& header = newOrderSingle.getHeader();

  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    newOrderSingle.set( FIX::Symbol( Symbol ) );
    newOrderSingle.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    newOrderSingle.set( FIX::Symbol( Symbol ) );
  }

  newOrderSingle.set( FIX::OrderQty( Shares ) );
  if ( MaxFloor != 0 )
  {
    newOrderSingle.set( FIX::MaxFloor( MaxFloor ) );
  }
  if ( MinQty != 0 )
  {
    newOrderSingle.set( FIX::MinQty( MinQty ) );
  }
  newOrderSingle.set( queryTimeInForce( TIF ) );
  if ( ordType == FIX::OrdType_LIMIT || ordType == FIX::OrdType_STOP_LIMIT )
    newOrderSingle.set( FIX::Price( Price ) );
  if ( ordType == FIX::OrdType_STOP || ordType == FIX::OrdType_STOP_LIMIT )
    newOrderSingle.set( FIX::StopPx( StopPx ) );
  if ( querySide( Side ) == FIX::Side_SELL_SHORT )
    newOrderSingle.set( FIX::LocateReqd( 'Y' ) );
  if ( PegDiff != 0 )
  {
    newOrderSingle.set( FIX::PegDifference( PegDiff ) );
  }
  if (DiscOffset != 0)
  {
    newOrderSingle.set( FIX::DiscretionInst( *DiscInst.c_str() ) );
    newOrderSingle.set( FIX::DiscretionOffset( DiscOffset ) );
  }
  if ( ExDest==";" && TargetCompID=="REDI" )
  {
    newOrderSingle.setField( 100, ExDest );
    newOrderSingle.setField(11007, "1");
  }
  else
  {
    newOrderSingle.setField( 100, ExDest );
  }

  newOrderSingle.set( FIX::ExecInst( ExecInst ) );

  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );
  if ( DeskID != "0" )
  {
    header.setField( FIX::DeliverToLocationID( DeskID ) );
  }
  parseAlgoInst( newOrderSingle, TargetCompID, DeskID, AlgoInst );
  FIX::Session::sendToTarget( newOrderSingle );
}

void Application::queryNewOrderSingle44( string DeskID, string SenderCompID, string TargetCompID,
     string ClOrdID, string Symbol, string ExDest, string TIF, string Side, long Shares, long MaxFloor, long MinQty, string OrdType, double Price, double StopPx, double PegDiff, string DiscInst, double DiscOffset, string ExecInst, string AlgoInst )
{
  vector<string> tokens;
  FIX::OrdType ordType;
  string SymbolSfx;

  FIX44::NewOrderSingle newOrderSingle(
    FIX::ClOrdID( ClOrdID ), querySide( Side ),
    FIX::TransactTime(), ordType = queryOrdType( OrdType ) );

  FIX::Header& header = newOrderSingle.getHeader();

  newOrderSingle.set( FIX::HandlInst('1') );

  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    newOrderSingle.set( FIX::Symbol( Symbol ) );
    newOrderSingle.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    newOrderSingle.set( FIX::Symbol( Symbol ) );
  }

  newOrderSingle.set( FIX::OrderQty( Shares ) );
  if ( MaxFloor != 0 )
  {
    newOrderSingle.set( FIX::MaxFloor( MaxFloor ) );
  }
  if ( MinQty != 0 )
  {
    newOrderSingle.set( FIX::MinQty( MinQty ) );
  }
  newOrderSingle.set( queryTimeInForce( TIF ) );
  if ( ordType == FIX::OrdType_LIMIT || ordType == FIX::OrdType_STOP_LIMIT )
    newOrderSingle.set( FIX::Price( Price ) );
  if ( ordType == FIX::OrdType_STOP || ordType == FIX::OrdType_STOP_LIMIT )
    newOrderSingle.set( FIX::StopPx( StopPx ) );
  if ( querySide( Side ) == FIX::Side_SELL_SHORT )
    newOrderSingle.set( FIX::LocateReqd( 'Y' ) );
  if ( PegDiff != 0 )
  {
    newOrderSingle.set( FIX::PegOffsetValue( PegDiff ) );
  }
  if (DiscOffset != 0)
  {
    newOrderSingle.set( FIX::DiscretionInst( *DiscInst.c_str() ) );
    newOrderSingle.set( FIX::DiscretionOffsetValue( DiscOffset ) );
  }
  if ( ExDest==";" && TargetCompID=="REDI" )
  {
    newOrderSingle.setField( 100, ExDest );
    newOrderSingle.setField(11007, "1");
  }
  else
  {
    newOrderSingle.setField( 100, ExDest );
  }

  newOrderSingle.set( FIX::ExecInst( ExecInst ) );

  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );
  if ( DeskID != "0" )
  {
    header.setField( FIX::DeliverToLocationID( DeskID ) );
  }
  parseAlgoInst( newOrderSingle, TargetCompID, DeskID, AlgoInst );
  FIX::Session::sendToTarget( newOrderSingle );
}

void Application::queryOrderCancelRequest40( string DeskID, string SenderCompID, string TargetCompID,
     string OrigClOrdID, string ClOrdID, string Symbol, string ExDest, string Side, long Shares )
{
  vector<string> tokens;
  string SymbolSfx;

  FIX40::OrderCancelRequest orderCancelRequest(
     FIX::OrigClOrdID( OrigClOrdID ), FIX::ClOrdID( ClOrdID ), FIX::CxlType( 'F' ),
    FIX::Symbol( Symbol ), querySide( Side ), FIX::OrderQty( Shares ) );

  FIX::Header& header = orderCancelRequest.getHeader();

  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    orderCancelRequest.set( FIX::Symbol( Symbol ) );
    orderCancelRequest.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    orderCancelRequest.set( FIX::Symbol( Symbol ) );
  }

  orderCancelRequest.set( FIX::OrderQty( Shares ) );

  if ( ExDest==";" && TargetCompID=="REDI" )
  {
    orderCancelRequest.setField( 100, ExDest );
    orderCancelRequest.setField(11007, "1");
  }
  else
  {
    orderCancelRequest.setField( 100, ExDest );
  }

  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );
  if ( DeskID != "0" )
  {
    header.setField( FIX::DeliverToLocationID( DeskID ) );
  }
  FIX::Session::sendToTarget( orderCancelRequest );
}

void Application::queryOrderCancelRequest41( string DeskID, string SenderCompID, string TargetCompID, 
     string OrigClOrdID, string ClOrdID, string Symbol, string ExDest, string Side, long Shares )
{
  vector<string> tokens;
  string SymbolSfx;

  FIX41::OrderCancelRequest orderCancelRequest(
    FIX::OrigClOrdID( OrigClOrdID ), FIX::ClOrdID( ClOrdID ), FIX::Symbol( Symbol ), querySide( Side ) );

  FIX::Header& header = orderCancelRequest.getHeader();

 Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    orderCancelRequest.set( FIX::Symbol( Symbol ) );
    orderCancelRequest.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    orderCancelRequest.set( FIX::Symbol( Symbol ) );
  }

  orderCancelRequest.set( FIX::OrderQty( Shares ) );

  if ( ExDest==";" && TargetCompID=="REDI" )
  {
    orderCancelRequest.setField( 100, ExDest );
    orderCancelRequest.setField(11007, "1");
  }
  else
  {
    orderCancelRequest.setField( 100, ExDest );
  }

  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );
  if ( DeskID != "0" )
  {
    header.setField( FIX::DeliverToLocationID( DeskID ) );
  }
  FIX::Session::sendToTarget( orderCancelRequest );
}

void Application::queryOrderCancelRequest42( string DeskID, string SenderCompID, string TargetCompID, string OrigClOrdID, string ClOrdID, string Symbol, string ExDest, string Side, long Shares )
{
  vector<string> tokens;
  string SymbolSfx;

  FIX42::OrderCancelRequest orderCancelRequest( FIX::OrigClOrdID( OrigClOrdID ),
      FIX::ClOrdID( ClOrdID ), FIX::Symbol( Symbol ), querySide( Side ), FIX::TransactTime() );

  FIX::Header& header = orderCancelRequest.getHeader();

  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    orderCancelRequest.set( FIX::Symbol( Symbol ) );
    orderCancelRequest.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    orderCancelRequest.set( FIX::Symbol( Symbol ) );
  }
  cout << "Cancel Symbol: " << Symbol << "\n";
  orderCancelRequest.set( FIX::OrderQty( Shares ) );
  if ( ExDest==";" && TargetCompID=="REDI" )
  {
    orderCancelRequest.setField( 100, ExDest );
    orderCancelRequest.setField(11007, "1");
  }
  else
  {
    orderCancelRequest.setField( 100, ExDest );
  }

  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );
  if ( DeskID != "0" )
  {
    header.setField( FIX::DeliverToLocationID( DeskID ) );
  }
  cout << "made it to outgoing msg stage\n";
  FIX::Session::sendToTarget( orderCancelRequest );
}

void Application::queryOrderCancelRequest43( string DeskID, string SenderCompID, string TargetCompID,
     string OrigClOrdID, string ClOrdID, string Symbol, string ExDest, string Side, long Shares )
{
  vector<string> tokens;
  string SymbolSfx;

  FIX43::OrderCancelRequest orderCancelRequest( FIX::OrigClOrdID( OrigClOrdID ),
      FIX::ClOrdID( ClOrdID ), querySide( Side ), FIX::TransactTime() );

  FIX::Header& header = orderCancelRequest.getHeader();

  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    orderCancelRequest.set( FIX::Symbol( Symbol ) );
    orderCancelRequest.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    orderCancelRequest.set( FIX::Symbol( Symbol ) );
  }

  orderCancelRequest.set( FIX::OrderQty( Shares ) );

  if ( ExDest==";" && TargetCompID=="REDI" )
  {
    orderCancelRequest.setField( 100, ExDest );
    orderCancelRequest.setField(11007, "1");
  }
  else
  {
    orderCancelRequest.setField( 100, ExDest );
  }

  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );
  if ( DeskID != "0" )
  {
    header.setField( FIX::DeliverToLocationID( DeskID ) );
  }
  FIX::Session::sendToTarget( orderCancelRequest );
}

void Application::queryOrderCancelRequest44( string DeskID, string SenderCompID, string TargetCompID,
     string OrigClOrdID, string ClOrdID, string Symbol, string ExDest, string Side, long Shares )
{
  vector<string> tokens;
  string SymbolSfx;

  FIX44::OrderCancelRequest orderCancelRequest( FIX::OrigClOrdID( OrigClOrdID ),
      FIX::ClOrdID( ClOrdID ), querySide( Side ), FIX::TransactTime() );

  FIX::Header& header = orderCancelRequest.getHeader();

  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    orderCancelRequest.set( FIX::Symbol( Symbol ) );
    orderCancelRequest.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    orderCancelRequest.set( FIX::Symbol( Symbol ) );
  }

  orderCancelRequest.set( FIX::OrderQty( Shares ) );

  if ( ExDest==";" && TargetCompID=="REDI" )
  {
    orderCancelRequest.setField( 100, ExDest );
    orderCancelRequest.setField(11007, "1");
  }
  else
  {
    orderCancelRequest.setField( 100, ExDest );
  }


  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );
  if ( DeskID != "0" )
  {
    header.setField( FIX::DeliverToLocationID( DeskID ) );
  }
  FIX::Session::sendToTarget( orderCancelRequest );
}

void Application::queryCancelReplaceRequest40( string DeskID, string SenderCompID, string TargetCompID, string OrigClOrdID, string ClOrdID, string Symbol, string ExDest, string TIF, string Side, long Shares, long MaxFloor, long MinQty, string OrdType, double Price, double StopPx, double PegDiff, string DiscInst, double DiscOffset, string ExecInst, string AlgoInst )
{
  vector<string> tokens;
  string SymbolSfx;

  FIX40::OrderCancelReplaceRequest cancelReplaceRequest(
    FIX::OrigClOrdID( OrigClOrdID ), FIX::ClOrdID( ClOrdID ), FIX::HandlInst( '1' ),
    FIX::Symbol( Symbol ), querySide( Side ), FIX::OrderQty( Shares ), queryOrdType( OrdType ) );

  FIX::Header& header = cancelReplaceRequest.getHeader();

  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    cancelReplaceRequest.set( FIX::Symbol( Symbol ) );
    cancelReplaceRequest.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    cancelReplaceRequest.set( FIX::Symbol( Symbol ) );
  }
  if ( MaxFloor != 0 )
  {
    cancelReplaceRequest.set( FIX::MaxFloor( MaxFloor ) );
  }
  if ( MinQty != 0 )
  {
    cancelReplaceRequest.set( FIX::MinQty( MinQty ) );
  }
  if ( Price != 0 )
    cancelReplaceRequest.set( FIX::Price( Price ) );
  if ( StopPx != 0 )
    cancelReplaceRequest.set( FIX::StopPx( StopPx ) );

  if ( ExDest==";" && TargetCompID=="REDI" )
  {
    cancelReplaceRequest.setField( 100, ExDest );
    cancelReplaceRequest.setField(11007, "1");
  }
  else
  {
    cancelReplaceRequest.setField( 100, ExDest );
  }
  cancelReplaceRequest.set( queryTimeInForce( TIF ) );
  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );
  if ( DeskID != "0" )
  {
    header.setField( FIX::DeliverToLocationID( DeskID ) );
  }
  cancelReplaceRequest.set( FIX::ExecInst( ExecInst ) );
  parseAlgoInst( cancelReplaceRequest, TargetCompID, DeskID, AlgoInst );
  FIX::Session::sendToTarget( cancelReplaceRequest );
}

void Application::queryCancelReplaceRequest41( string DeskID, string SenderCompID, string TargetCompID, string OrigClOrdID, string ClOrdID, string Symbol, string ExDest, string TIF, string Side, long Shares, long MaxFloor, long MinQty, string OrdType, double Price, double StopPx, double PegDiff, string DiscInst, double DiscOffset, string ExecInst, string AlgoInst )
{
  vector<string> tokens;
  string SymbolSfx;

  FIX41::OrderCancelReplaceRequest cancelReplaceRequest(
    FIX::OrigClOrdID( OrigClOrdID ), FIX::ClOrdID( ClOrdID ), FIX::HandlInst( '1' ),
    FIX::Symbol( Symbol ), querySide( Side ), queryOrdType( OrdType ) );

  FIX::Header& header = cancelReplaceRequest.getHeader();

  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    cancelReplaceRequest.set( FIX::Symbol( Symbol ) );
    cancelReplaceRequest.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    cancelReplaceRequest.set( FIX::Symbol( Symbol ) );
  }
  if ( MaxFloor != 0 )
  {
    cancelReplaceRequest.set( FIX::MaxFloor( MaxFloor ) );
  }
  if ( MinQty != 0 )
  {
    cancelReplaceRequest.set( FIX::MinQty( MinQty ) );
  }
  cancelReplaceRequest.set( FIX::OrderQty( Shares ) );
  if ( Price != 0 )
    cancelReplaceRequest.set( FIX::Price( Price ) );
  if ( StopPx != 0 )
    cancelReplaceRequest.set( FIX::StopPx( StopPx ) );
  if ( querySide( Side ) == FIX::Side_SELL_SHORT )
    cancelReplaceRequest.set( FIX::LocateReqd( 'Y' ) );
  if ( PegDiff != 0 )
  {
    cancelReplaceRequest.set( FIX::PegDifference( PegDiff ) );
  }
  if ( ExDest==";" && TargetCompID=="REDI" )
  {
    cancelReplaceRequest.setField( 100, ExDest );
    cancelReplaceRequest.setField(11007, "1");
  }
  else
  {
    cancelReplaceRequest.setField( 100, ExDest );
  }
  cancelReplaceRequest.set( queryTimeInForce( TIF ) );
  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );
  if ( DeskID != "0" )
  {
    header.setField( FIX::DeliverToLocationID( DeskID ) );
  }
  cancelReplaceRequest.set( FIX::ExecInst( ExecInst ) );
  parseAlgoInst( cancelReplaceRequest, TargetCompID, DeskID, AlgoInst );
  FIX::Session::sendToTarget( cancelReplaceRequest );
}

void Application::queryCancelReplaceRequest42( string DeskID, string SenderCompID, string TargetCompID, string OrigClOrdID, string ClOrdID, string Symbol, string ExDest, string TIF, string Side, long Shares, long MaxFloor, long MinQty, string OrdType, double Price, double StopPx, double PegDiff, string DiscInst, double DiscOffset, string ExecInst, string AlgoInst )
{
  vector<string> tokens;
  string SymbolSfx;

  FIX42::OrderCancelReplaceRequest cancelReplaceRequest(
    FIX::OrigClOrdID( OrigClOrdID ), FIX::ClOrdID( ClOrdID ), FIX::HandlInst( '1' ),
    FIX::Symbol( Symbol ), querySide( Side ), FIX::TransactTime(), queryOrdType( OrdType ) );

  FIX::Header& header = cancelReplaceRequest.getHeader();

  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    cancelReplaceRequest.set( FIX::Symbol( Symbol ) );
    cancelReplaceRequest.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    cancelReplaceRequest.set( FIX::Symbol( Symbol ) );
  }
  if ( MaxFloor != 0 )
  {
    cancelReplaceRequest.set( FIX::MaxFloor( MaxFloor ) );
  }
  if ( MinQty != 0 )
  {
    cancelReplaceRequest.set( FIX::MinQty( MinQty ) );
  }
  cancelReplaceRequest.set( FIX::OrderQty( Shares ) );
  if ( Price != 0 )
    cancelReplaceRequest.set( FIX::Price( Price ) );
  if ( StopPx != 0 )
    cancelReplaceRequest.set( FIX::StopPx( StopPx ) );
  if ( querySide( Side ) == FIX::Side_SELL_SHORT )
    cancelReplaceRequest.set( FIX::LocateReqd( 'Y' ) );
  if ( PegDiff != 0 )
  {
    cancelReplaceRequest.set( FIX::PegDifference( PegDiff ) );
  }
  if (DiscOffset != 0)
  {
    cancelReplaceRequest.set( FIX::DiscretionInst( *DiscInst.c_str() ) );
    cancelReplaceRequest.set( FIX::DiscretionOffset( DiscOffset ) );
  }
  cancelReplaceRequest.setField( 100, ExDest );
  cancelReplaceRequest.set( queryTimeInForce( TIF ) );
  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );
  if ( DeskID != "0" )
  {
    header.setField( FIX::DeliverToLocationID( DeskID ) );
  }
  cancelReplaceRequest.set( FIX::ExecInst( ExecInst ) );
  parseAlgoInst( cancelReplaceRequest, TargetCompID, DeskID, AlgoInst );
  FIX::Session::sendToTarget( cancelReplaceRequest );
}

void Application::queryCancelReplaceRequest43( string DeskID, string SenderCompID, string TargetCompID, string OrigClOrdID, string ClOrdID, string Symbol, string ExDest, string TIF, string Side, long Shares, long MaxFloor, long MinQty, string OrdType, double Price, double StopPx, double PegDiff, string DiscInst, double DiscOffset, string ExecInst, string AlgoInst )
{
  vector<string> tokens;
  string SymbolSfx;

  FIX43::OrderCancelReplaceRequest cancelReplaceRequest(
    FIX::OrigClOrdID( OrigClOrdID ), FIX::ClOrdID( ClOrdID ), FIX::HandlInst( '1' ),
    querySide( Side ), FIX::TransactTime(), queryOrdType( OrdType ) );

  FIX::Header& header = cancelReplaceRequest.getHeader();

  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    cancelReplaceRequest.set( FIX::Symbol( Symbol ) );
    cancelReplaceRequest.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    cancelReplaceRequest.set( FIX::Symbol( Symbol ) );
  }
  if ( MaxFloor != 0 )
  {
    cancelReplaceRequest.set( FIX::MaxFloor( MaxFloor ) );
  }
  if ( MinQty != 0 )
  {
    cancelReplaceRequest.set( FIX::MinQty( MinQty ) );
  }
  cancelReplaceRequest.set( FIX::OrderQty( Shares ) );
  if ( Price != 0 )
    cancelReplaceRequest.set( FIX::Price( Price ) );
  if ( StopPx != 0 )
    cancelReplaceRequest.set( FIX::StopPx( StopPx ) );
  if ( querySide( Side ) == FIX::Side_SELL_SHORT )
    cancelReplaceRequest.set( FIX::LocateReqd( 'Y' ) );
  if ( PegDiff != 0 )
  {
    cancelReplaceRequest.set( FIX::PegDifference( PegDiff ) );
  }
  if (DiscOffset != 0)
  {
    cancelReplaceRequest.set( FIX::DiscretionInst( *DiscInst.c_str() ) );
    cancelReplaceRequest.set( FIX::DiscretionOffset( DiscOffset ) );
  }

  if ( ExDest==";" && TargetCompID=="REDI" )
  {
    cancelReplaceRequest.setField( 100, ExDest );
    cancelReplaceRequest.setField(11007, "1");
  }
  else
  {
    cancelReplaceRequest.setField( 100, ExDest );
  }
  cancelReplaceRequest.set( queryTimeInForce( TIF ) );
  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );
  if ( DeskID != "0" )
  {
    header.setField( FIX::DeliverToLocationID( DeskID ) );
  }
  cancelReplaceRequest.set( FIX::ExecInst( ExecInst ) );
  parseAlgoInst( cancelReplaceRequest, TargetCompID, DeskID, AlgoInst );
  FIX::Session::sendToTarget( cancelReplaceRequest );
}

void Application::queryCancelReplaceRequest44( string DeskID, string SenderCompID, string TargetCompID, string OrigClOrdID, string ClOrdID, string Symbol, string ExDest, string TIF, string Side, long Shares, long MaxFloor, long MinQty, string OrdType, double Price, double StopPx, double PegDiff, string DiscInst, double DiscOffset, string ExecInst, string AlgoInst )
{
  vector<string> tokens;
  string SymbolSfx;

  FIX44::OrderCancelReplaceRequest cancelReplaceRequest(
    FIX::OrigClOrdID( OrigClOrdID ), FIX::ClOrdID( ClOrdID ),
    querySide( Side ), FIX::TransactTime(), queryOrdType( OrdType ) );

  FIX::Header& header = cancelReplaceRequest.getHeader();

  cancelReplaceRequest.set( FIX::HandlInst('1') );

  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    cancelReplaceRequest.set( FIX::Symbol( Symbol ) );
    cancelReplaceRequest.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    cancelReplaceRequest.set( FIX::Symbol( Symbol ) );
  }
  if ( MaxFloor != 0 )
  {
    cancelReplaceRequest.set( FIX::MaxFloor( MaxFloor ) );
  }
  if ( MinQty != 0 )
  {
    cancelReplaceRequest.set( FIX::MinQty( MinQty ) );
  }
  cancelReplaceRequest.set( FIX::OrderQty( Shares ) );
  if ( Price != 0 )
    cancelReplaceRequest.set( FIX::Price( Price ) );
  if ( StopPx != 0 )
    cancelReplaceRequest.set( FIX::StopPx( StopPx ) );
  if ( querySide( Side ) == FIX::Side_SELL_SHORT )
    cancelReplaceRequest.set( FIX::LocateReqd( 'Y' ) );
  if ( PegDiff != 0 )
  {
    cancelReplaceRequest.set( FIX::PegOffsetValue( PegDiff ) );
  }
  if (DiscOffset != 0)
  {
    cancelReplaceRequest.set( FIX::DiscretionInst( *DiscInst.c_str() ) );
    cancelReplaceRequest.set( FIX::DiscretionOffsetValue( DiscOffset ) );
  }

  if ( ExDest==";" && TargetCompID=="REDI" )
  {
    cancelReplaceRequest.setField( 100, ExDest );
    cancelReplaceRequest.setField(11007, "1");
  }
  else
  {
    cancelReplaceRequest.setField( 100, ExDest );
  }
  cancelReplaceRequest.set( queryTimeInForce( TIF ) );
  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );
  if ( DeskID != "0" )
  {
    header.setField( FIX::DeliverToLocationID( DeskID ) );
  }
  cancelReplaceRequest.set( FIX::ExecInst( ExecInst ) );
  parseAlgoInst( cancelReplaceRequest, TargetCompID, DeskID, AlgoInst );
  FIX::Session::sendToTarget( cancelReplaceRequest );
}

void Application::queryMarketDataRequest43( string SenderCompID, string TargetCompID, string Symbol )
{
  vector<string> tokens;
  string SymbolSfx;

  FIX::MDReqID mdReqID( "MARKETDATAID" );
  FIX::SubscriptionRequestType subType( FIX::SubscriptionRequestType_SNAPSHOT );
  FIX::MarketDepth marketDepth( 0 );

  FIX43::MarketDataRequest::NoMDEntryTypes marketDataEntryGroup;
  FIX::MDEntryType mdEntryType( FIX::MDEntryType_BID );
  marketDataEntryGroup.set( mdEntryType );

  FIX43::MarketDataRequest::NoRelatedSym symbolGroup;

  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    symbolGroup.set( FIX::Symbol( Symbol ) );
    symbolGroup.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    symbolGroup.set( FIX::Symbol( Symbol ) );
  }

  FIX43::MarketDataRequest message( mdReqID, subType, marketDepth );
  message.addGroup( marketDataEntryGroup );
  message.addGroup( symbolGroup );

  FIX::Header& header = message.getHeader();
  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );

  cout << message.toXML() << endl;
  cout << message.toString() << endl;

  FIX::Session::sendToTarget( message ); 
}

void Application::queryMarketDataRequest44( string SenderCompID, string TargetCompID, string Symbol )
{
  vector<string> tokens;
  string SymbolSfx;

  FIX::MDReqID mdReqID( "MARKETDATAID" );
  FIX::SubscriptionRequestType subType( FIX::SubscriptionRequestType_SNAPSHOT );
  FIX::MarketDepth marketDepth( 0 );

  FIX44::MarketDataRequest::NoMDEntryTypes marketDataEntryGroup;
  FIX::MDEntryType mdEntryType( FIX::MDEntryType_BID );
  marketDataEntryGroup.set( mdEntryType );

  FIX44::MarketDataRequest::NoRelatedSym symbolGroup;
  
  Tokenize(Symbol, tokens, ".");

  if (tokens.size() == 2)
  {
    Symbol = ( string ) tokens.at(0);
    SymbolSfx = ( string ) tokens.at(1);
    symbolGroup.set( FIX::Symbol( Symbol ) );
    symbolGroup.set( FIX::SymbolSfx( SymbolSfx ) );
  }
  else
  {
    symbolGroup.set( FIX::Symbol( Symbol ) );
  }

  FIX44::MarketDataRequest message( mdReqID, subType, marketDepth );
  message.addGroup( marketDataEntryGroup );
  message.addGroup( symbolGroup );

  FIX::Header& header = message.getHeader();
  header.setField( FIX::SenderCompID( SenderCompID ) );
  header.setField( FIX::TargetCompID( TargetCompID ) );

  cout << message.toXML() << endl;
  cout << message.toString() << endl;

  FIX::Session::sendToTarget( message );
}

void Application::exit_nicely(PGconn *conn)
{
    PQfinish(conn);
    exit(1);
}
const char* Application::getConnection()
{
  const char *conninfo;
  conninfo = "host='dbcrunch01' port='5432' dbname='tradeplatform' user='tradeplatform' password='tradeplatform'";
  return conninfo;
}

Application::ConnectInfo Application::queryBroker(string MMID)
{
  string SenderCompID;
  string TargetCompID;
  string DeskID;
  int Version;
  PGconn *conn;
  //const char *conninfo;
  string query;
  PGresult *res;
  int i, j;
  ConnectInfo Info;

  conn = PQconnectdb( getConnection() );
  /* Check to see that the backend connection was successfully made */    
  if (PQstatus(conn) != CONNECTION_OK)
  {
      fprintf(stderr, "libpq error: Connection to database failed: %s",
              PQerrorMessage(conn));
      exit_nicely(conn);
  }
  query = "select sendercompid, targetcompid, version, deskid from broker_connections where mmid = '" + MMID + "';\n";
  //cout << query;
  res = PQexec(conn, query.c_str());
  if (!res || !(j = PQntuples(res))) 
  {
    fprintf(stderr, "libpq error: no rows returned or bad result set in function queryBroker()\n\n");
    exit_nicely(conn);
  }
  for (i = 0; i < j; i++)
  {
    SenderCompID =  PQgetvalue(res, i, 0);
    TargetCompID =  PQgetvalue(res, i, 1);
    Version = atoi(PQgetvalue(res, i, 2));
    DeskID = PQgetvalue(res, i, 3);
    //cout << "Query Results: " << SenderCompID << " " << TargetCompID << " " << Version << "\n";
  }
  PQclear(res);

  Info.SenderCompID = SenderCompID;
  Info.TargetCompID = TargetCompID;
  Info.Version = Version;
  Info.DeskID = DeskID;
  //cout << Info.SenderCompID;
  //cout << Info.TargetCompID;
  //cout << Info.Version;
  PQfinish(conn);
  return Info;
}

FIX::Side Application::querySide( string Side )
{

  if (Side=="BUY")
  {
    return FIX::Side( FIX::Side_BUY );
  }
  else if (Side=="SELL")
  {
    return FIX::Side( FIX::Side_SELL );
  }
  else if (Side=="SHORT")
  {
    return FIX::Side( FIX::Side_SELL_SHORT );
  }
  else if (Side=="SELLPLUS")
  {
    return FIX::Side( FIX::Side_SELL_PLUS );
  }
  else if (Side=="BUYMINUS")
  {
    return FIX::Side( FIX::Side_BUY_MINUS );
  }
  else
  {
    throw exception();
  }
}

FIX::OrdType Application::queryOrdType( string OrdType )
{
  char sweep;
  sweep = 'S';
  if (OrdType=="MKT")
  {
    return FIX::OrdType( FIX::OrdType_MARKET );
  }
  else if (OrdType=="LMT")
  {
    return FIX::OrdType( FIX::OrdType_LIMIT );
  }
  else if (OrdType=="STP")
  {
    return FIX::OrdType( FIX::OrdType_STOP );
  }
  else if (OrdType=="SLMT")
  {
    return FIX::OrdType( FIX::OrdType_STOP_LIMIT );
  }
  else if (OrdType=="MOC")
  {
    return FIX::OrdType( FIX::OrdType_MARKET_ON_CLOSE );
  }
  else if (OrdType=="LOC")
  {
    return FIX::OrdType( FIX::OrdType_LIMIT_ON_CLOSE );
  }
  else if (OrdType=="PEG")
  {
    return FIX::OrdType( FIX::OrdType_PEGGED );
  }
  else if (OrdType=="SWP")
  {
    return FIX::OrdType( FIX::CharField( 40, sweep ) );
  }
  else
  {
    throw exception();
  }
}

FIX::TimeInForce Application::queryTimeInForce( string TIF )
{
  if (TIF=="DAY")
  {
    return FIX::TimeInForce( FIX::TimeInForce_DAY );
  }
  else if (TIF=="IOC")
  {
    return FIX::TimeInForce( FIX::TimeInForce_IMMEDIATE_OR_CANCEL );
  }
  else if (TIF=="FOK")
  {
    return FIX::TimeInForce( FIX::TimeInForce_FILL_OR_KILL );
  }
  else if (TIF=="OPG")
  {
    return FIX::TimeInForce( FIX::TimeInForce_AT_THE_OPENING );
  }
  else if (TIF=="GTC")
  {
    return FIX::TimeInForce( FIX::TimeInForce_GOOD_TILL_CANCEL );
  }
  else if (TIF=="GTX")
  {
    return FIX::TimeInForce( FIX::TimeInForce_GOOD_TILL_CROSSING );
  }
  else
  {
    throw exception();
  }
}

void Application::parseAlgoInst(FIX::Message& message, string TargetCompID, string DeskID, string AlgoInst)
{
  vector<string> algotokens;
  string type, startTime, endTime, orderDepth, volumeLimit, minExecTime, minReqComp, riskAvoidance, executionStyle, initPrate, minPrate, maxPrate, ignorePrintsBasis, ignorePrintsAbove, benchmarkID, tobPrice, tobQuantity, udPrice, udQuantity;
  if ( TargetCompID == "REDI" && ( DeskID=="1" || DeskID=="2") )
  {
    Tokenize(AlgoInst, algotokens, " ");
    type = ( string ) algotokens.at(0);
    message.setField( 8031, type );
    if ( type == "4CAST" )
    {
      startTime = ( string ) algotokens.at(1);
      endTime = ( string ) algotokens.at(2);
      volumeLimit = ( string ) algotokens.at(3);
      minExecTime = ( string ) algotokens.at(4);
      minReqComp = ( string ) algotokens.at(5);
      riskAvoidance = ( string ) algotokens.at(6);
      cout << "Start Time: " << startTime << "\n";
      cout << "End Time: " << endTime << "\n";
      cout << "Vol Lmt: " << volumeLimit << "\n";
      cout << "Min Exec Time: " << minExecTime << "\n";
      cout << "Min Req Complete: " << minReqComp << "\n";
      cout << "Risk Avoid: " << riskAvoidance << "\n";
      if ( startTime != "0" )
      {
        message.setField( 168, startTime );
      }
      if ( endTime != "0" )
      {
        message.setField( 126, endTime );
      }
      if ( volumeLimit != "0" )
      {
        message.setField( 111, volumeLimit );
      }
      if ( minExecTime != "0" )
      {
        message.setField( 8040, minExecTime );
      }
      if ( minReqComp != "0" )
      {
        message.setField( 110, minReqComp );
      }
      if ( riskAvoidance != "0" )
      {
        message.setField( 8035, riskAvoidance );
      }
    }
    else if ( type == "DScaling" )
    {
      executionStyle = ( string ) algotokens.at(1);
      startTime = ( string ) algotokens.at(2);
      endTime = ( string ) algotokens.at(3);
      initPrate = ( string ) algotokens.at(4);
      minPrate = ( string ) algotokens.at(5);
      maxPrate = ( string ) algotokens.at(6);
      message.setField( 8042, executionStyle );
      if ( startTime != "0" )
      {
        message.setField( 168, startTime );
      }
      if ( endTime != "0" )
      {
        message.setField( 126, endTime );
      }
      if ( initPrate != "0" )
      {
        message.setField( 849, initPrate );
      }
      if ( minPrate != "0" )
      {
        message.setField( 8047, minPrate );
      }
      if ( maxPrate !="0" )
      {
        message.setField( 8046, maxPrate );
      }
    }
    else if ( type == "IntelliShort" )
    {
      startTime = ( string ) algotokens.at(1);
      executionStyle = ( string ) algotokens.at(2);
      if ( startTime != "0" )
      {
        message.setField( 168, startTime );
      }
      if ( executionStyle != "0" )
      {
        message.setField( 8042, executionStyle );
      }
    }
    else if ( type == "Participate" )
    {
      volumeLimit = ( string ) algotokens.at(1);
      startTime = (string) algotokens.at(2);
      ignorePrintsBasis = ( string ) algotokens.at(3);
      ignorePrintsAbove = ( string ) algotokens.at(4);
      message.setField( 111, volumeLimit );
      if ( startTime != "0" )
      {
        message.setField( 168, startTime );
      }
      if ( ignorePrintsBasis != "0" )
      {
        message.setField( 8038, ignorePrintsBasis );
      }
      if ( ignorePrintsAbove != "0" )
      {
        message.setField( 8039, ignorePrintsAbove );
      }
    }
    else if ( type == "RScaling" )
    {
      executionStyle = ( string ) algotokens.at(1);
      startTime = ( string ) algotokens.at(2);
      endTime = ( string ) algotokens.at(3);
      benchmarkID = ( string ) algotokens.at(4);
      initPrate = ( string ) algotokens.at(5);
      minPrate = ( string ) algotokens.at(6);
      maxPrate = ( string ) algotokens.at(7);
      message.setField( 8042, executionStyle );
      if ( startTime != "0" )
      {
        message.setField( 168, startTime );
      }
      if ( endTime != "0" )
      {
        message.setField( 126, endTime );
      }
      if ( benchmarkID != "0" )
      {
        message.setField( 699, benchmarkID );
      }
      if ( initPrate != "0" ) 
      {
        message.setField( 849, initPrate );
      }
      if ( minPrate != "0" )
      {
        message.setField( 8047, minPrate );
      }
      if ( maxPrate !="0" )
      {
        message.setField( 8046, maxPrate );
      }
    }
    else if ( type == "Scale" )
    {
      tobQuantity = (string) algotokens.at(1);
      tobPrice = (string) algotokens.at(2);
      orderDepth = (string) algotokens.at(3);
      udPrice = (string) algotokens.at(4);
      udQuantity = (string) algotokens.at(5);
      message.setField( 8053, tobQuantity );
      if ( tobPrice !="0" )
      {
        message.setField( 8054, tobPrice );
      }
      if ( orderDepth != "0" )
      {
        message.setField( 8052, orderDepth );
      }
      if ( udPrice != "0" )
      {
        message.setField( 8056, udPrice );
      }
      if ( udQuantity !="0" )
      {
        message.setField( 8055, udQuantity );
      }
    }
    else if ( type == "Sonar" )
    {
      startTime = ( string ) algotokens.at(1);
      endTime = ( string ) algotokens.at(2);
      executionStyle = ( string ) algotokens.at(3);
      minReqComp = ( string ) algotokens.at(4);
      if ( startTime != "0" )
      {
        message.setField( 168, startTime );
      }
      if ( endTime != "0" )
      {
        message.setField( 126, endTime );
      }
      if ( executionStyle != "0" )
      {
        message.setField( 8042, executionStyle );
      }
      if ( minReqComp != "0" )
      {
        message.setField( 110, minReqComp );
      }
    }
    else if ( type == "TWAP" || type == "VWAP" )
    {
      volumeLimit = ( string ) algotokens.at(1);
      startTime = ( string ) algotokens.at(2);
      endTime = ( string ) algotokens.at(3);
      if ( volumeLimit != "0" )
      {
        message.setField( 111, volumeLimit );
      }
      if ( startTime != "0" )
      {
        message.setField( 168, startTime );
      }
      if ( endTime != "0" )
      {
        message.setField( 126, endTime );
      }
    }
    else if (type == "NONE" )
    {
    }
    else
    {
    }
  }
}

Socket::Socket() :
  m_sock ( -1 )
{

  memset ( &m_addr,
	   0,
	   sizeof ( m_addr ) );

}

Socket::~Socket()
{
  if ( is_valid() )
    ::close ( m_sock );
}

bool Socket::create()
{
  m_sock = socket ( AF_INET,
		    SOCK_STREAM,
		    0 );

  if ( ! is_valid() )
    return false;


  // TIME_WAIT - argh
  int on = 1;
  if ( setsockopt ( m_sock, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 )
    return false;


  return true;

}

bool Socket::bind ( const int port )
{

  if ( ! is_valid() )
    {
      return false;
    }



  m_addr.sin_family = AF_INET;
  m_addr.sin_addr.s_addr = INADDR_ANY;
  m_addr.sin_port = htons ( port );

  int bind_return = ::bind ( m_sock,
			     ( struct sockaddr * ) &m_addr,
			     sizeof ( m_addr ) );


  if ( bind_return == -1 )
    {
      return false;
    }

  return true;
}

bool Socket::listen() const
{
  if ( ! is_valid() )
    {
      return false;
    }

  int listen_return = ::listen ( m_sock, MAXCONNECTIONS );


  if ( listen_return == -1 )
    {
      return false;
    }

  return true;
}

bool Socket::accept ( Socket& new_socket ) const
{
  int addr_length = sizeof ( m_addr );
  new_socket.m_sock = ::accept ( m_sock, ( sockaddr * ) &m_addr, ( socklen_t * ) &addr_length );

  if ( new_socket.m_sock <= 0 )
    return false;
  else
    return true;
}

bool Socket::send ( const std::string s ) const
{
  int status = ::send ( m_sock, s.c_str(), s.size(), MSG_NOSIGNAL );
  if ( status == -1 )
    {
      return false;
    }
  else
    {
      return true;
    }
}

int Socket::recv ( std::string& s ) const
{
  char buf [ MAXRECV + 1 ];

  s = "";

  memset ( buf, 0, MAXRECV + 1 );

  int status = ::recv ( m_sock, buf, MAXRECV, 0 );

  if ( status == -1 )
    {
      std::cout << "status == -1   errno == " << errno << "  in Socket::recv\n";
      return 0;
    }
  else if ( status == 0 )
    {
      return 0;
    }
  else
    {
      s = buf;
      return status;
    }
}

bool Socket::connect ( const std::string host, const int port )
{
  if ( ! is_valid() ) return false;

  m_addr.sin_family = AF_INET;
  m_addr.sin_port = htons ( port );

  int status = inet_pton ( AF_INET, host.c_str(), &m_addr.sin_addr );

  if ( errno == EAFNOSUPPORT ) return false;

  status = ::connect ( m_sock, ( sockaddr * ) &m_addr, sizeof ( m_addr ) );

  if ( status == 0 )
    return true;
  else
    return false;
}

void Socket::set_non_blocking ( const bool b )
{

  int opts;

  opts = fcntl ( m_sock,
		 F_GETFL );

  if ( opts < 0 )
    {
      return;
    }

  if ( b )
    opts = ( opts | O_NONBLOCK );
  else
    opts = ( opts & ~O_NONBLOCK );

  fcntl ( m_sock,
	  F_SETFL,opts );

}

ServerSocket::ServerSocket ( int port )
{
  if ( ! Socket::create() )
    {
      throw SocketException ( "Could not create server socket." );
    }

  if ( ! Socket::bind ( port ) )
    {
      throw SocketException ( "Could not bind to port." );
    }

  if ( ! Socket::listen() )
    {
      throw SocketException ( "Could not listen to socket." );
    }

}

ServerSocket::~ServerSocket()
{
}

const ServerSocket& ServerSocket::operator << ( const std::string& s ) const
{
  if ( ! Socket::send ( s ) )
    {
      throw SocketException ( "Could not write to socket." );
    }

  return *this;

}

const ServerSocket& ServerSocket::operator >> ( std::string& s ) const
{
  if ( ! Socket::recv ( s ) )
    {
      throw SocketException ( "Could not read from socket." );
    }

  return *this;
}

void ServerSocket::accept ( ServerSocket& sock )
{
  if ( ! Socket::accept ( sock ) )
    {
      throw SocketException ( "Could not accept socket." );
    }
}

template <class out_type, class in_value>  out_type cast_stream(const in_value & t)
  {
    stringstream ss;
    ss << t; // first insert value to stream
    out_type result; // value will be converted to out_type
    ss >> result; // write value to result
    return result;
  }
