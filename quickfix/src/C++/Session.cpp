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
#include "stdafx.h"
#else
#include "config.h"
#endif
#include "CallStack.h"

#include "Session.h"
#include "Values.h"
#include <algorithm>
#include <iostream>

namespace FIX
{
Session::Sessions Session::s_sessions;
Session::SessionIDs Session::s_sessionIDs;
Session::Sessions Session::s_registered;
Mutex Session::s_mutex;

#define LOGEX( method ) try { method; } catch( std::exception& e ) \
  { m_state.onEvent( e.what() ); }

Session::Session( Application& application,
                  MessageStoreFactory& messageStoreFactory,
                  const SessionID& sessionID,
                  const DataDictionary& dataDictionary,
                  const SessionTime& sessionTime,
                  int heartBtInt, LogFactory* pLogFactory )
: m_application( application ),
  m_sessionID( sessionID ),
  m_sessionTime( sessionTime ),
  m_sendRedundantResendRequests( false ),
  m_checkCompId( true ),
  m_checkLatency( true ), 
  m_maxLatency( 120 ),
  m_resetOnLogon( false ),
  m_resetOnLogout( false ), 
  m_resetOnDisconnect( false ),
  m_millisecondsInTimeStamp( true ),
  m_persistMessages( true ),
  m_dataDictionary( dataDictionary ),
  m_messageStoreFactory( messageStoreFactory ),
  m_pLogFactory( pLogFactory ),
  m_pResponder( 0 )
{
  m_state.heartBtInt( heartBtInt );
  m_state.initiate( heartBtInt != 0 );
  m_state.store( m_messageStoreFactory.create( m_sessionID ) );
  if ( m_pLogFactory )
    m_state.log( m_pLogFactory->create( m_sessionID ) );

  if( !checkSessionTime( UtcTimeStamp() ) )
    reset();

  addSession( *this );
  m_application.onCreate( m_sessionID );
  m_state.onEvent( "Created session" );
}

Session::~Session()
{ QF_STACK_IGNORE_BEGIN
  removeSession( *this );
  m_messageStoreFactory.destroy( m_state.store() );
  if ( m_pLogFactory && m_state.log() )
    m_pLogFactory->destroy( m_state.log() );
  QF_STACK_IGNORE_END
}

void Session::insertSendingTime( Header& header )
{ QF_STACK_PUSH(Session::insertSendingTime)

  UtcTimeStamp now;
  bool showMilliseconds = m_sessionID.getBeginString() >= BeginString_FIX42;
  header.setField( SendingTime(now, showMilliseconds && m_millisecondsInTimeStamp) );

  QF_STACK_POP
}

void Session::insertOrigSendingTime( Header& header, const UtcTimeStamp& when )
{ QF_STACK_PUSH(Session::insertSendingTime)

  bool showMilliseconds = m_sessionID.getBeginString() >= BeginString_FIX42;
  header.setField( OrigSendingTime(when, showMilliseconds && m_millisecondsInTimeStamp) );

  QF_STACK_POP
}

void Session::fill( Header& header )
{ QF_STACK_PUSH(Session::fill)

  UtcTimeStamp now;
  m_state.lastSentTime( now );
  header.setField( m_sessionID.getBeginString() );
  header.setField( m_sessionID.getSenderCompID() );
  header.setField( m_sessionID.getTargetCompID() );
  header.setField( MsgSeqNum( getExpectedSenderNum() ) );
  insertSendingTime( header );

  QF_STACK_POP
}

void Session::next()
{ QF_STACK_PUSH(Session::next)

  try
  {
    if( !isEnabled() )
    {
      if( isLoggedOn() )
      {
        if( !m_state.sentLogout() )
        {
          m_state.onEvent( "Initiated logout request" );
          generateLogout( m_state.logoutReason() );
        }
      }
      else
        return;
    }

    UtcTimeStamp now;
    if ( !checkSessionTime( now ) )
      { reset(); return; }

    if ( !m_state.receivedLogon() )
    {
      if ( m_state.shouldSendLogon() )
      {
        generateLogon();
        m_state.onEvent( "Initiated logon request" );
      }
      else if ( m_state.alreadySentLogon() && m_state.logonTimedOut() )
      {
        m_state.onEvent( "Timed out waiting for logon response" );
        disconnect();
      }
      return ;
    }

    if ( m_state.heartBtInt() == 0 ) return ;

    if ( m_state.logoutTimedOut() )
    {
      m_state.onEvent( "Timed out waiting for logout response" );
      disconnect();
    }

    if ( m_state.withinHeartBeat() ) return ;

    if ( m_state.timedOut() )
    {
      m_state.onEvent( "Timed out waiting for heartbeat" );
      disconnect();
    }
    else
    {
      if ( m_state.needTestRequest() )
      {
        generateTestRequest( "TEST" );
        m_state.testRequest( m_state.testRequest() + 1 );
        m_state.onEvent( "Sent test request TEST" );
      }
      else if ( m_state.needHeartbeat() )
      {
        generateHeartbeat();
      }
    }
  }
  catch ( FIX::IOException& e )
  {
    m_state.onEvent( e.what() );
    disconnect();
  }

  QF_STACK_POP
}

void Session::nextLogon( const Message& logon )
{ QF_STACK_PUSH(Session::nextLogon)

  SenderCompID senderCompID;
  TargetCompID targetCompID;
  logon.getHeader().getField( senderCompID );
  logon.getHeader().getField( targetCompID );

  if( m_refreshOnLogon )
    refresh();

  ResetSeqNumFlag resetSeqNumFlag(false);
  if( logon.isSetField(resetSeqNumFlag) )
    logon.getField( resetSeqNumFlag );
  m_state.receivedReset( resetSeqNumFlag );

  if( m_state.receivedReset() )
  {
    m_state.onEvent( "Logon contains ResetSeqNumFlag=Y, reseting sequence numbers to 1" );
    if( !m_state.sentReset() ) m_state.reset();
  }

  if( m_state.shouldSendLogon() && !m_state.receivedReset() )
  {
    m_state.onEvent( "Received logon response before sending request" );
    disconnect();
    return ;
  }

  if( !m_state.initiate() && m_resetOnLogon )
    m_state.reset();

  if( !verify( logon, false, true ) )
    return;
  m_state.receivedLogon( true );

  if ( !m_state.initiate() 
       || (m_state.sentReset() && !m_state.receivedReset()) )
  {
    if( logon.isSetField(m_state.heartBtInt()) )
      logon.getField( m_state.heartBtInt() );
    m_state.onEvent( "Received logon request" );
    generateLogon( logon );
    m_state.onEvent( "Responding to logon request" );
  }
  else
    m_state.onEvent( "Received logon response" );

  m_state.sentReset( false );
  m_state.receivedReset( false );

  MsgSeqNum msgSeqNum;
  logon.getHeader().getField( msgSeqNum );
  if ( isTargetTooHigh( msgSeqNum ) && !resetSeqNumFlag )
  {
    doTargetTooHigh( logon );
  }
  else
  {
    m_state.incrNextTargetMsgSeqNum();
    nextQueued();
  }

  if ( isLoggedOn() )
    m_application.onLogon( m_sessionID );

  QF_STACK_POP
}

void Session::nextHeartbeat( const Message& heartbeat )
{ QF_STACK_PUSH(Session::nextHeartbeat)

  if ( !verify( heartbeat ) ) return ;
  m_state.incrNextTargetMsgSeqNum();
  nextQueued();

  QF_STACK_POP
}

void Session::nextTestRequest( const Message& testRequest )
{ QF_STACK_PUSH(Session::nextTestRequest)

  if ( !verify( testRequest ) ) return ;
  generateHeartbeat( testRequest );
  m_state.incrNextTargetMsgSeqNum();
  nextQueued();

  QF_STACK_POP
}

void Session::nextLogout( const Message& logout )
{ QF_STACK_PUSH(Session::nextLogout)

  if ( !verify( logout, false, false ) ) return ;
  if ( !m_state.sentLogout() )
  {
    m_state.onEvent( "Received logout request" );
    generateLogout();
    m_state.onEvent( "Sending logout response" );
  }
  else
    m_state.onEvent( "Received logout response" );

  m_state.incrNextTargetMsgSeqNum();
  if ( m_resetOnLogout ) m_state.reset();
  disconnect();

  QF_STACK_POP
}

void Session::nextReject( const Message& reject )
{ QF_STACK_PUSH(Session::nextReject)

  if ( !verify( reject, false, true ) ) return ;
  m_state.incrNextTargetMsgSeqNum();
  nextQueued();

  QF_STACK_POP
}

void Session::nextSequenceReset( const Message& sequenceReset )
{ QF_STACK_PUSH(Session::nextSequenceReset)

  bool isGapFill = false;
  GapFillFlag gapFillFlag;
  if ( sequenceReset.isSetField( gapFillFlag ) )
  {
    sequenceReset.getField( gapFillFlag );
    isGapFill = gapFillFlag;
  }

  if ( !verify( sequenceReset, isGapFill, isGapFill ) ) return ;

  NewSeqNo newSeqNo;
  if ( sequenceReset.isSetField( newSeqNo ) )
  {
    sequenceReset.getField( newSeqNo );

    m_state.onEvent( "Received SequenceReset FROM: "
                     + IntConvertor::convert( getExpectedTargetNum() ) +
                     " TO: " + IntConvertor::convert( newSeqNo ) );

    if ( newSeqNo > getExpectedTargetNum() )
      m_state.setNextTargetMsgSeqNum( MsgSeqNum( newSeqNo ) );
    else if ( newSeqNo < getExpectedTargetNum() )
      generateReject( sequenceReset, SessionRejectReason_VALUE_IS_INCORRECT );
  }

  QF_STACK_POP
}

void Session::nextResendRequest( const Message& resendRequest )
{ QF_STACK_PUSH(Session::nextResendRequest)

  if ( !verify( resendRequest, false, false ) ) return ;

  Locker l( m_mutex );

  BeginSeqNo beginSeqNo;
  EndSeqNo endSeqNo;
  resendRequest.getField( beginSeqNo );
  resendRequest.getField( endSeqNo );

  m_state.onEvent( "Received ResendRequest FROM: "
       + IntConvertor::convert( beginSeqNo ) +
                   " TO: " + IntConvertor::convert( endSeqNo ) );

  std::string beginString = m_sessionID.getBeginString();
  if ( beginString >= FIX::BeginString_FIX42 && endSeqNo == 0 ||
       beginString <= FIX::BeginString_FIX42 && endSeqNo == 999999 ||
       endSeqNo >= getExpectedSenderNum() )
  { endSeqNo = getExpectedSenderNum() - 1; }

  if ( !m_persistMessages )
  {
    endSeqNo = EndSeqNo(endSeqNo + 1);
    int next = m_state.getNextSenderMsgSeqNum();
    if( endSeqNo > next )
      endSeqNo = EndSeqNo(next);
    generateSequenceReset( beginSeqNo, endSeqNo );
    return;
  }

  std::vector < std::string > messages;
  m_state.get( beginSeqNo, endSeqNo, messages );

  std::vector < std::string > ::iterator i;
  MsgSeqNum msgSeqNum(0);
  MsgType msgType;
  int begin = 0;
  int current = beginSeqNo;
  std::string messageString;

  for ( i = messages.begin(); i != messages.end(); ++i )
  {
    Message msg( *i, m_dataDictionary );
    msg.getHeader().getField( msgSeqNum );
    msg.getHeader().getField( msgType );

    if( (current != msgSeqNum) && !begin )
      begin = current;

    if ( Message::isAdminMsgType( msgType ) )
    {
      if ( !begin ) begin = msgSeqNum;
    }
    else
    {
      if ( resend( msg ) )
      {
        if ( begin ) generateSequenceReset( begin, msgSeqNum );
        send( msg.toString(messageString) );
        m_state.onEvent( "Resending Message: "
                         + IntConvertor::convert( msgSeqNum ) );
        begin = 0;
      }
      else
      { if ( !begin ) begin = msgSeqNum; }
    }
    current = msgSeqNum + 1;
  }
  if ( begin )
  {
    generateSequenceReset( begin, msgSeqNum + 1 );
  }

  if ( endSeqNo > msgSeqNum )
  {
    endSeqNo = EndSeqNo(endSeqNo + 1);
    int next = m_state.getNextSenderMsgSeqNum();
    if( endSeqNo > next )
      endSeqNo = EndSeqNo(next);
    generateSequenceReset( beginSeqNo, endSeqNo );
  }

  resendRequest.getHeader().getField( msgSeqNum );
  if( !isTargetTooHigh(msgSeqNum) && !isTargetTooLow(msgSeqNum) )
    m_state.incrNextTargetMsgSeqNum();

  QF_STACK_POP
}

bool Session::send( Message& message )
{ QF_STACK_PUSH(Session::send)

  message.getHeader().removeField( FIELD::PossDupFlag );
  message.getHeader().removeField( FIELD::OrigSendingTime );
  return sendRaw( message );

  QF_STACK_POP
}

bool Session::sendRaw( Message& message, int num )
{ QF_STACK_PUSH(Session::sendRaw)

  Locker l( m_mutex );

  try
  {
    bool result = false;
    Header& header = message.getHeader();

    MsgType msgType;
    if( header.isSetField(msgType) )
      header.getField( msgType );

    fill( header );
    std::string messageString;

    if ( num )
      header.setField( MsgSeqNum( num ) );

    if ( Message::isAdminMsgType( msgType ) )
    {
      m_application.toAdmin( message, m_sessionID );

      if( msgType == "A" && !m_state.receivedReset() )
      {
        ResetSeqNumFlag resetSeqNumFlag( false );
        if( message.isSetField(resetSeqNumFlag) )
          message.getField( resetSeqNumFlag );
        if( resetSeqNumFlag )
        {
          m_state.reset();
          message.getHeader().setField( MsgSeqNum(getExpectedSenderNum()) );
        }
        m_state.sentReset( resetSeqNumFlag );
      }

      message.toString( messageString );

      if (
        msgType == "A" || msgType == "5"
        || msgType == "2" || msgType == "4"
        || isLoggedOn() )
      {
        result = send( messageString );
      }
    }
    else
    {
      // do not send application messages if they will just be cleared
      if( !isLoggedOn() && shouldSendReset() )
        return false;

      try
      {
        m_application.toApp( message, m_sessionID );
        message.toString( messageString );
        if ( isLoggedOn() )
          result = send( messageString );
      }
      catch ( DoNotSend& ) { return false; }
    }

    if ( !num )
    {
      MsgSeqNum msgSeqNum;
      header.getField( msgSeqNum );
      if( m_persistMessages )
        m_state.set( msgSeqNum, messageString );
      m_state.incrNextSenderMsgSeqNum();
    }
    return result;
  }
  catch ( IOException& e )
  {
    m_state.onEvent( e.what() );
    return false;
  }

  QF_STACK_POP
}

bool Session::send( const std::string& string )
{ QF_STACK_PUSH(Session::send)

  if ( !m_pResponder ) return false;
  m_state.onOutgoing( string );
  return m_pResponder->send( string );

  QF_STACK_POP
}

void Session::disconnect()
{ QF_STACK_PUSH(Session::disconnect)

  Locker l(m_mutex);

  if ( m_pResponder )
  {
    m_state.onEvent( "Disconnecting" );

    m_pResponder->disconnect();
    m_pResponder = 0;
  }

  if ( m_state.receivedLogon() || m_state.sentLogon() )
  {
    m_state.receivedLogon( false );
    m_state.sentLogon( false );
    m_application.onLogout( m_sessionID );
  }

  m_state.sentLogout( false );
  m_state.receivedReset( false );
  m_state.sentReset( false );
  m_state.clearQueue();
  m_state.logoutReason();
  if ( m_resetOnDisconnect )
    m_state.reset();

  m_state.resendRange( 0, 0 );

  QF_STACK_POP
}

bool Session::resend( Message& message )
{ QF_STACK_PUSH(Session::resend)

  SendingTime sendingTime;
  MsgSeqNum msgSeqNum;
  Header& header = message.getHeader();
  header.getField( sendingTime );
  header.getField( msgSeqNum );
  insertOrigSendingTime( header, sendingTime );
  header.setField( PossDupFlag( true ) );
  insertSendingTime( header );

  try
  {
    m_application.toApp( message, m_sessionID );
    return true;
  }
  catch ( DoNotSend& )
  { return false; }

  QF_STACK_POP
}

void Session::generateLogon()
{ QF_STACK_PUSH(Session::generateLogon)

  Message logon;
  logon.getHeader().setField( MsgType( "A" ) );
  logon.setField( EncryptMethod( 0 ) );
  logon.setField( m_state.heartBtInt() );

  if( m_refreshOnLogon )
    refresh();
  if( m_resetOnLogon )
    m_state.reset();
  if( shouldSendReset() )
    logon.setField( ResetSeqNumFlag(true) );

  fill( logon.getHeader() );
  UtcTimeStamp now;
  m_state.lastReceivedTime( now );
  m_state.testRequest( 0 );
  m_state.sentLogon( true );
  sendRaw( logon );

  QF_STACK_POP
}

void Session::generateLogon( const Message& aLogon )
{ QF_STACK_PUSH(Session::generateLogon)

  Message logon;
  EncryptMethod encryptMethod;
  HeartBtInt heartBtInt;
  logon.setField( EncryptMethod( 0 ) );
  if( m_state.receivedReset() )
    logon.setField( ResetSeqNumFlag(true) );
  aLogon.getField( heartBtInt );
  logon.getHeader().setField( MsgType( "A" ) );
  logon.setField( heartBtInt );
  fill( logon.getHeader() );
  sendRaw( logon );
  m_state.sentLogon( true );

  QF_STACK_POP
}

void Session::generateResendRequest( const BeginString& beginString, const MsgSeqNum& msgSeqNum )
{ QF_STACK_PUSH(Session::generateResendRequest)

  Message resendRequest;
  BeginSeqNo beginSeqNo( ( int ) getExpectedTargetNum() );
  EndSeqNo endSeqNo( msgSeqNum - 1 );
  if ( beginString >= FIX::BeginString_FIX42 )
    endSeqNo = 0;
  else if( beginString <= FIX::BeginString_FIX41 )
    endSeqNo = 999999;
  resendRequest.getHeader().setField( MsgType( "2" ) );
  resendRequest.setField( beginSeqNo );
  resendRequest.setField( endSeqNo );
  fill( resendRequest.getHeader() );
  sendRaw( resendRequest );

  m_state.onEvent( "Sent ResendRequest FROM: "
                   + IntConvertor::convert( beginSeqNo ) +
                   " TO: " + IntConvertor::convert( endSeqNo ) );

  m_state.resendRange( beginSeqNo, msgSeqNum - 1 );

  QF_STACK_POP
}

void Session::generateSequenceReset
( int beginSeqNo, int endSeqNo )
{ QF_STACK_PUSH(Session::generateSequenceReset)

  Message sequenceReset;
  NewSeqNo newSeqNo( endSeqNo );
  sequenceReset.getHeader().setField( MsgType( "4" ) );
  sequenceReset.getHeader().setField( PossDupFlag( true ) );
  sequenceReset.setField( newSeqNo );
  fill( sequenceReset.getHeader() );

  SendingTime sendingTime;
  sequenceReset.getHeader().getField( sendingTime );
  insertOrigSendingTime( sequenceReset.getHeader(), sendingTime );
  sequenceReset.getHeader().setField( MsgSeqNum( beginSeqNo ) );
  sequenceReset.setField( GapFillFlag( true ) );
  sendRaw( sequenceReset, beginSeqNo );
  m_state.onEvent( "Sent SequenceReset TO: "
                   + IntConvertor::convert( newSeqNo ) );

  QF_STACK_POP
}

void Session::generateHeartbeat()
{ QF_STACK_PUSH(Session::generateHeartbeat)

  Message heartbeat;
  heartbeat.getHeader().setField( MsgType( "0" ) );
  fill( heartbeat.getHeader() );
  sendRaw( heartbeat );

  QF_STACK_POP
}

void Session::generateHeartbeat( const Message& testRequest )
{ QF_STACK_PUSH(Session::generateHeartbeat)

  Message heartbeat;
  heartbeat.getHeader().setField( MsgType( "0" ) );
  fill( heartbeat.getHeader() );
  try
  {
    TestReqID testReqID;
    testRequest.getField( testReqID );
    heartbeat.setField( testReqID );
  }
  catch ( FieldNotFound& ) {}

  sendRaw( heartbeat );

  QF_STACK_POP
}

void Session::generateTestRequest( const std::string& id )
{ QF_STACK_PUSH(Session::generateTestRequest)

  Message testRequest;
  testRequest.getHeader().setField( MsgType( "1" ) );
  fill( testRequest.getHeader() );
  TestReqID testReqID( id );
  testRequest.setField( testReqID );

  sendRaw( testRequest );

  QF_STACK_POP
}

void Session::generateReject( const Message& message, int err, int field )
{ QF_STACK_PUSH(Session::generateReject)

  std::string beginString = m_sessionID.getBeginString();

  Message reject;
  reject.getHeader().setField( MsgType( "3" ) );
  reject.reverseRoute( message.getHeader() );
  fill( reject.getHeader() );

  MsgSeqNum msgSeqNum;
  MsgType msgType;

  message.getHeader().getField( msgType );
  if( message.getHeader().isSetField( msgSeqNum ) )
  {
    message.getHeader().getField( msgSeqNum );
    if( msgSeqNum.getString() != "" )
      reject.setField( RefSeqNum( msgSeqNum ) );
  }

  if ( beginString >= FIX::BeginString_FIX42 )
  {
    if( msgType.getString() != "" )
      reject.setField( RefMsgType( msgType ) );
    if ( (beginString == FIX::BeginString_FIX42
          && err <= SessionRejectReason_INVALID_MSGTYPE)
          || beginString > FIX::BeginString_FIX42 )
    {
      reject.setField( SessionRejectReason( err ) );
    }
  }
  if ( msgType != MsgType_Logon && msgType != MsgType_SequenceReset
       && msgSeqNum == getExpectedTargetNum() )
  { m_state.incrNextTargetMsgSeqNum(); }

  const char* reason = 0;
  switch ( err )
  {
    case SessionRejectReason_INVALID_TAG_NUMBER:
    reason = SessionRejectReason_INVALID_TAG_NUMBER_TEXT;
    break;
    case SessionRejectReason_REQUIRED_TAG_MISSING:
    reason = SessionRejectReason_REQUIRED_TAG_MISSING_TEXT;
    break;
    case SessionRejectReason_TAG_NOT_DEFINED_FOR_THIS_MESSAGE_TYPE:
    reason = SessionRejectReason_TAG_NOT_DEFINED_FOR_THIS_MESSAGE_TYPE_TEXT;
    break;
    case SessionRejectReason_TAG_SPECIFIED_WITHOUT_A_VALUE:
    reason = SessionRejectReason_TAG_SPECIFIED_WITHOUT_A_VALUE_TEXT;
    break;
    case SessionRejectReason_VALUE_IS_INCORRECT:
    reason = SessionRejectReason_VALUE_IS_INCORRECT_TEXT;
    break;
    case SessionRejectReason_INCORRECT_DATA_FORMAT_FOR_VALUE:
    reason = SessionRejectReason_INCORRECT_DATA_FORMAT_FOR_VALUE_TEXT;
    break;
    case SessionRejectReason_COMPID_PROBLEM:
    reason = SessionRejectReason_COMPID_PROBLEM_TEXT;
    break;
    case SessionRejectReason_SENDINGTIME_ACCURACY_PROBLEM:
    reason = SessionRejectReason_SENDINGTIME_ACCURACY_PROBLEM_TEXT;
    break;
    case SessionRejectReason_INVALID_MSGTYPE:
    reason = SessionRejectReason_INVALID_MSGTYPE_TEXT;
    break;
    case SessionRejectReason_TAG_APPEARS_MORE_THAN_ONCE:
    reason = SessionRejectReason_TAG_APPEARS_MORE_THAN_ONCE_TEXT;
    break;
    case SessionRejectReason_TAG_SPECIFIED_OUT_OF_REQUIRED_ORDER:
    reason = SessionRejectReason_TAG_SPECIFIED_OUT_OF_REQUIRED_ORDER_TEXT;
    break;
    case SessionRejectReason_INCORRECT_NUMINGROUP_COUNT_FOR_REPEATING_GROUP:
    reason = SessionRejectReason_INCORRECT_NUMINGROUP_COUNT_FOR_REPEATING_GROUP_TEXT;
  };

  if ( reason && ( field || err == SessionRejectReason_INVALID_TAG_NUMBER ) )
  {
    populateRejectReason( reject, field, reason );
    m_state.onEvent( "Message " + msgSeqNum.getString() + " Rejected: "
                     + reason + ":" + IntConvertor::convert( field ) );
  }
  else if ( reason )
  {
    populateRejectReason( reject, reason );
    m_state.onEvent( "Message " + msgSeqNum.getString()
         + " Rejected: " + reason );
  }
  else
    m_state.onEvent( "Message " + msgSeqNum.getString() + " Rejected" );

  if ( !m_state.receivedLogon() )
    throw std::runtime_error( "Tried to send a reject while not logged on" );

  sendRaw( reject );

  QF_STACK_POP
}

void Session::generateReject( const Message& message, const std::string& str )
{ QF_STACK_PUSH(Session::generateReject)

  std::string beginString = m_sessionID.getBeginString();

  Message reject;
  reject.getHeader().setField( MsgType( "3" ) );
  reject.reverseRoute( message.getHeader() );
  fill( reject.getHeader() );

  MsgType msgType;
  MsgSeqNum msgSeqNum;

  message.getHeader().getField( msgType );
  message.getHeader().getField( msgSeqNum );
  if ( beginString >= FIX::BeginString_FIX42 )
    reject.setField( RefMsgType( msgType ) );
  reject.setField( RefSeqNum( msgSeqNum ) );

  if ( msgType != MsgType_Logon && msgType != MsgType_SequenceReset )
    m_state.incrNextTargetMsgSeqNum();

  reject.setField( Text( str ) );
  sendRaw( reject );
  m_state.onEvent( "Message " + msgSeqNum.getString()
                   + " Rejected: " + str );

  QF_STACK_POP
}

void Session::generateBusinessReject( const Message& message, int err, int field )
{ QF_STACK_PUSH(Session::generateBusinessReject)

  Message reject;
  reject.getHeader().setField( MsgType( MsgType_BusinessMessageReject ) );
  fill( reject.getHeader() );
  MsgType msgType;
  MsgSeqNum msgSeqNum;
  message.getHeader().getField( msgType );
  message.getHeader().getField( msgSeqNum );
  reject.setField( RefMsgType( msgType ) );
  reject.setField( RefSeqNum( msgSeqNum ) );
  reject.setField( BusinessRejectReason( err ) );
  m_state.incrNextTargetMsgSeqNum();

  const char* reason = 0;
  switch ( err )
  {
    case BusinessRejectReason_OTHER:
    reason = BusinessRejectReason_OTHER_TEXT;
    break;
    case BusinessRejectReason_UNKOWN_ID:
    reason = BusinessRejectReason_UNKNOWN_ID_TEXT;
    break;
    case BusinessRejectReason_UNKNOWN_SECURITY:
    reason = BusinessRejectReason_UNKNOWN_SECURITY_TEXT;
    break;
    case BusinessRejectReason_UNSUPPORTED_MESSAGE_TYPE:
    reason = BusinessRejectReason_UNSUPPORTED_MESSAGE_TYPE_TEXT;
    break;
    case BusinessRejectReason_APPLICATION_NOT_AVAILABLE:
    reason = BusinessRejectReason_APPLICATION_NOT_AVAILABLE_TEXT;
    break;
    case BusinessRejectReason_CONDITIONALLY_REQUIRED_FIELD_MISSING:
    reason = BusinessRejectReason_CONDITIONALLY_REQUIRED_FIELD_MISSING_TEXT;
    break;
    case BusinessRejectReason_NOT_AUTHORIZED:
    reason = BusinessRejectReason_NOT_AUTHORIZED_TEXT;
    break;
    case BusinessRejectReason_DELIVERTO_FIRM_NOT_AVAILABLE_AT_THIS_TIME:
    reason = BusinessRejectReason_DELIVERTO_FIRM_NOT_AVAILABLE_AT_THIS_TIME_TEXT;
    break;
  };

  if ( reason && field )
  {
    populateRejectReason( reject, field, reason );
    m_state.onEvent( "Message " + msgSeqNum.getString() + " Rejected: "
                     + reason + ":" + IntConvertor::convert( field ) );
  }
  else if ( reason )
  {
    populateRejectReason( reject, reason );
    m_state.onEvent( "Message " + msgSeqNum.getString()
         + " Rejected: " + reason );
  }
  else
    m_state.onEvent( "Message " + msgSeqNum.getString() + " Rejected" );

  sendRaw( reject );

  QF_STACK_POP
}

void Session::generateLogout( const std::string& text )
{ QF_STACK_PUSH(Session::generateLogout)

  Message logout;
  logout.getHeader().setField( MsgType( MsgType_Logout ) );
  fill( logout.getHeader() );
  if ( text.length() )
    logout.setField( Text( text ) );
  sendRaw( logout );
  m_state.sentLogout( true );

  QF_STACK_POP
}

void Session::populateRejectReason( Message& reject, int field,
                                    const std::string& text )
{ QF_STACK_PUSH(Session::populateRejectReason)

  MsgType msgType;
   reject.getHeader().getField( msgType );

  if ( msgType == MsgType_REJECT 
       && m_sessionID.getBeginString() >= FIX::BeginString_FIX42 )
  {
    reject.setField( RefTagID( field ) );
    reject.setField( Text( text ) );
  }
  else
  {
    std::stringstream stream;
    stream << text << " (" << field << ")";
    reject.setField( Text( stream.str() ) );
  }

  QF_STACK_POP
}

void Session::populateRejectReason( Message& reject, const std::string& text )
{ QF_STACK_PUSH(Session::populateRejectReason)
  reject.setField( Text( text ) );
  QF_STACK_POP
}

bool Session::verify( const Message& msg, bool checkTooHigh,
                      bool checkTooLow )
{ QF_STACK_PUSH(Session::verify)

  const MsgType* pMsgType = 0;
  const MsgSeqNum* pMsgSeqNum = 0;

  try
  {
    const Header& header = msg.getHeader();

    pMsgType = FIELD_GET_PTR( header, MsgType );
    const SenderCompID& senderCompID = FIELD_GET_REF( header, SenderCompID );
    const TargetCompID& targetCompID = FIELD_GET_REF( header, TargetCompID );
    const SendingTime& sendingTime = FIELD_GET_REF( header, SendingTime );

    if( checkTooHigh || checkTooLow )
      pMsgSeqNum = FIELD_GET_PTR( header, MsgSeqNum );

    if ( !validLogonState( *pMsgType ) )
      throw std::logic_error( "Logon state is not valid for message" );

    if ( !isGoodTime( sendingTime ) )
    {
      doBadTime( msg );
      return false;
    }
    if ( !isCorrectCompID( senderCompID, targetCompID ) )
    {
      doBadCompID( msg );
      return false;
    }

    if ( checkTooHigh && isTargetTooHigh( *pMsgSeqNum ) )
    {
      doTargetTooHigh( msg );
      return false;
    }
    else if ( checkTooLow && isTargetTooLow( *pMsgSeqNum ) )
    {
      doTargetTooLow( msg );
      return false;
    }

    if ( (checkTooHigh || checkTooLow) && m_state.resendRequested() )
    {
      SessionState::ResendRange range = m_state.resendRange();
 
      if ( *pMsgSeqNum >= range.second )
      {
        m_state.onEvent ("ResendRequest for messages FROM: " +
                         IntConvertor::convert (range.first) + " TO: " +
                         IntConvertor::convert (range.second) +
                         " has been satisfied.");
        m_state.resendRange (0, 0);
      }
    }
  }
  catch ( std::exception& e )
  {
    m_state.onEvent( e.what() );
    disconnect();
    return false;
  }

  UtcTimeStamp now;
  m_state.lastReceivedTime( now );
  m_state.testRequest( 0 );

  fromCallback( pMsgType ? *pMsgType : MsgType(), msg, m_sessionID );
  return true;

  QF_STACK_POP
}

bool Session::shouldSendReset()
{ QF_STACK_PUSH(Session::shouldSendReset)

  std::string beginString = m_sessionID.getBeginString();
  return beginString >= FIX::BeginString_FIX41
    && ( m_resetOnLogon || 
         m_resetOnLogout || 
         m_resetOnDisconnect )
    && ( getExpectedSenderNum() == 1 )
    && ( getExpectedTargetNum() == 1 );

  QF_STACK_POP
}

bool Session::validLogonState( const MsgType& msgType )
{ QF_STACK_PUSH(Session::validLogonState)

  if ( msgType == MsgType_Logon && m_state.sentReset() || m_state.receivedReset() )
    return true;
  if ( msgType == MsgType_Logon && !m_state.receivedLogon()
       || msgType != MsgType_Logon && m_state.receivedLogon() )
    return true;
  if ( msgType == MsgType_Logout && m_state.sentLogon() )
    return true;
  if ( msgType != MsgType_Logout && m_state.sentLogout() )
    return true;
  if ( msgType == MsgType_SequenceReset ) 
    return true;
  if ( msgType == MsgType_Reject )
    return true;

  return false;

  QF_STACK_POP
}

void Session::fromCallback( const MsgType& msgType, const Message& msg,
                            const SessionID& sessionID )
{ QF_STACK_PUSH(Session::fromCallback)

  if ( Message::isAdminMsgType( msgType ) )
    m_application.fromAdmin( msg, m_sessionID );
  else
    m_application.fromApp( msg, m_sessionID );

  QF_STACK_POP
}

void Session::doBadTime( const Message& msg )
{ QF_STACK_PUSH(Session::doBadTime)

  generateReject( msg, SessionRejectReason_SENDINGTIME_ACCURACY_PROBLEM );
  generateLogout();

  QF_STACK_POP
}

void Session::doBadCompID( const Message& msg )
{ QF_STACK_PUSH(Session::doBadCompID)

  generateReject( msg, SessionRejectReason_COMPID_PROBLEM );
  generateLogout();

  QF_STACK_POP
}

bool Session::doPossDup( const Message& msg )
{ QF_STACK_PUSH(Session::doPossDup)

  const Header & header = msg.getHeader();
  OrigSendingTime origSendingTime;
  SendingTime sendingTime;
  MsgType msgType;

  header.getField( msgType );
  header.getField( sendingTime );

  if ( msgType != MsgType_SequenceReset )
  {
    if ( !header.isSetField( origSendingTime ) )
    {
      generateReject( msg, SessionRejectReason_REQUIRED_TAG_MISSING, origSendingTime.getField() );
      return false;
    }
    header.getField( origSendingTime );

    if ( origSendingTime > sendingTime )
    {
      generateReject( msg, SessionRejectReason_SENDINGTIME_ACCURACY_PROBLEM );
      generateLogout();
      return false;
    }
  }
  return true;

  QF_STACK_POP
}

bool Session::doTargetTooLow( const Message& msg )
{ QF_STACK_PUSH(Session::doTargetTooLow)

  const Header & header = msg.getHeader();
  PossDupFlag possDupFlag(false);
  MsgSeqNum msgSeqNum;
  if( header.isSetField(possDupFlag) )
    header.getField( possDupFlag );
  header.getField( msgSeqNum );

  if ( !possDupFlag )
  {
    std::stringstream stream;
    stream << "MsgSeqNum too low, expecting " << getExpectedTargetNum()
           << " but received " << msgSeqNum;
    generateLogout( stream.str() );
    throw std::logic_error( stream.str() );
  }

  return doPossDup( msg );

  QF_STACK_POP
}

void Session::doTargetTooHigh( const Message& msg )
{ QF_STACK_PUSH(Session::doTargetTooHigh)

  const Header & header = msg.getHeader();
  BeginString beginString;
  MsgSeqNum msgSeqNum;
  header.getField( beginString );
  header.getField( msgSeqNum );

  m_state.onEvent( "MsgSeqNum too high, expecting "
                   + IntConvertor::convert( getExpectedTargetNum() )
                   + " but received "
                   + IntConvertor::convert( msgSeqNum ) );

  m_state.queue( msgSeqNum, msg );

  if( m_state.resendRequested() )
  {
    SessionState::ResendRange range = m_state.resendRange();

    if( !m_sendRedundantResendRequests && msgSeqNum >= range.first )
    {
          m_state.onEvent ("Already sent ResendRequest FROM: " +
                           IntConvertor::convert (range.first) + " TO: " +
                           IntConvertor::convert (range.second) +
                           ".  Not sending another.");
          return;
    }
  }

  generateResendRequest( beginString, msgSeqNum );

  QF_STACK_POP
}

void Session::nextQueued()
{ QF_STACK_PUSH(Session::nextQueued)
  while ( nextQueued( getExpectedTargetNum() ) ) {}
  QF_STACK_POP
}

bool Session::nextQueued( int num )
{ QF_STACK_PUSH(Session::nextQueued)

  Message msg;
  MsgType msgType;

  if( m_state.retreive( num, msg ) )
  {
    m_state.onEvent( "Processing QUEUED message: "
                     + IntConvertor::convert( num ) );
    msg.getHeader().getField( msgType );
    if( msgType == MsgType_Logon
        || msgType == MsgType_ResendRequest )
    {
      m_state.incrNextTargetMsgSeqNum();
    }
    else
    {
      std::string msgString;
      next( msg.toString(msgString), true );
    }
    return true;
  }
  return false;

  QF_STACK_POP
}

void Session::next( const std::string& msg, bool queued )
{ QF_STACK_PUSH(Session::next)

  try
  {
    m_state.onIncoming( msg );
    next( Message( msg, m_dataDictionary ), queued );
  }
  catch( InvalidMessage& e )
  {
    m_state.onEvent( e.what() );

    try
    {
      if( identifyType(msg) == MsgType_Logon )
      {
        m_state.onEvent( "Logon message is not valid" );
        disconnect();
      }
    } catch( MessageParseError& ) {}
    throw e;
  }

  QF_STACK_POP
}

void Session::next( const Message& message, bool queued )
{ QF_STACK_PUSH(Session::next)

  const Header& header = message.getHeader();

  try
  {
    UtcTimeStamp now;
    if ( !checkSessionTime(now) )
      { reset(); return; }

    const MsgType& msgType = FIELD_GET_REF( header, MsgType );
    const BeginString& beginString = FIELD_GET_REF( header, BeginString );
    FIELD_GET_REF( header, SenderCompID );
    FIELD_GET_REF( header, TargetCompID );

    if ( beginString != m_sessionID.getBeginString() )
      throw UnsupportedVersion();

    m_dataDictionary.validate( message );

    if ( msgType == MsgType_Logon )
      nextLogon( message );
    else if ( msgType == MsgType_Heartbeat )
      nextHeartbeat( message );
    else if ( msgType == MsgType_TestRequest )
      nextTestRequest( message );
    else if ( msgType == MsgType_SequenceReset )
      nextSequenceReset( message );
    else if ( msgType == MsgType_Logout )
      nextLogout( message );
    else if ( msgType == MsgType_ResendRequest )
      nextResendRequest( message );
    else if ( msgType == MsgType_Reject )
      nextReject( message );
    else
    {
      if ( !verify( message ) ) return ;
      m_state.incrNextTargetMsgSeqNum();
    }
  }
  catch ( MessageParseError& e )
  { m_state.onEvent( e.what() ); }
  catch ( RequiredTagMissing & e )
  { LOGEX( generateReject( message, SessionRejectReason_REQUIRED_TAG_MISSING, e.field ) ); }
  catch ( FieldNotFound & e )
  {
    if( header.getField(FIELD::BeginString) >= FIX::BeginString_FIX42 && message.isApp() )
    {
      LOGEX( generateBusinessReject( message, BusinessRejectReason_CONDITIONALLY_REQUIRED_FIELD_MISSING, e.field ) );
    }
    else
    {
      LOGEX( generateReject( message, SessionRejectReason_REQUIRED_TAG_MISSING, e.field ) );
      if ( header.getField(FIELD::MsgType) == MsgType_Logon )
      {
        m_state.onEvent( "Required field missing from logon" );
        disconnect();
      }
    }
  }
  catch ( InvalidTagNumber & e )
  { LOGEX( generateReject( message, SessionRejectReason_INVALID_TAG_NUMBER, e.field ) ); }
  catch ( NoTagValue & e )
  { LOGEX( generateReject( message, SessionRejectReason_TAG_SPECIFIED_WITHOUT_A_VALUE, e.field ) ); }
  catch ( TagNotDefinedForMessage & e )
  { LOGEX( generateReject( message, SessionRejectReason_TAG_NOT_DEFINED_FOR_THIS_MESSAGE_TYPE, e.field ) ); }
  catch ( InvalidMessageType& )
  { LOGEX( generateReject( message, SessionRejectReason_INVALID_MSGTYPE ) ); }
  catch ( UnsupportedMessageType& )
  {
    if ( header.getField(FIELD::BeginString) >= FIX::BeginString_FIX42 )
      { LOGEX( generateBusinessReject( message, BusinessRejectReason_UNSUPPORTED_MESSAGE_TYPE ) ); }
    else
      { LOGEX( generateReject( message, "Unsupported message type" ) ); }
  }
  catch ( TagOutOfOrder & e )
  { LOGEX( generateReject( message, SessionRejectReason_TAG_SPECIFIED_OUT_OF_REQUIRED_ORDER, e.field ) ); }
  catch ( IncorrectDataFormat & e )
  { LOGEX( generateReject( message, SessionRejectReason_INCORRECT_DATA_FORMAT_FOR_VALUE, e.field ) ); }
  catch ( IncorrectTagValue & e )
  { LOGEX( generateReject( message, SessionRejectReason_VALUE_IS_INCORRECT, e.field ) ); }
  catch ( RepeatedTag & e )
  { LOGEX( generateReject( message, SessionRejectReason_TAG_APPEARS_MORE_THAN_ONCE, e.field ) ); }
  catch ( RepeatingGroupCountMismatch & e )
  { LOGEX( generateReject( message, SessionRejectReason_INCORRECT_NUMINGROUP_COUNT_FOR_REPEATING_GROUP, e.field ) ); }
  catch ( InvalidMessage& e )
  { m_state.onEvent( e.what() ); }
  catch ( RejectLogon& e )
  {
    m_state.onEvent( e.what() );
    generateLogout( e.what() );
    disconnect();
  }
  catch ( UnsupportedVersion& )
  {
    if ( header.getField(FIELD::MsgType) == MsgType_Logout )
      nextLogout( message );
    else
    {
      generateLogout( "Incorrect BeginString" );
      m_state.incrNextTargetMsgSeqNum();
    }
  }
  catch ( IOException& e )
  {
    m_state.onEvent( e.what() );
    disconnect();
  }

  if( !queued )
    nextQueued();

  if( isLoggedOn() )
    next();

  QF_STACK_POP
}

bool Session::sendToTarget( Message& message, const std::string& qualifier )
throw( SessionNotFound )
{ QF_STACK_PUSH(Session::sendToTarget)

  try
  {
    SessionID sessionID = message.getSessionID( qualifier );
    return sendToTarget( message, sessionID );
  }
  catch ( FieldNotFound& ) { throw SessionNotFound(); }

  QF_STACK_POP
}

bool Session::sendToTarget( Message& message, const SessionID& sessionID )
throw( SessionNotFound )
{ QF_STACK_PUSH(Session::sendToTarget)

  message.setSessionID( sessionID );
  Session* pSession = lookupSession( sessionID );
  if ( !pSession ) throw SessionNotFound();
  return pSession->send( message );

  QF_STACK_POP
}

bool Session::sendToTarget
( Message& message,
  const SenderCompID& senderCompID,
  const TargetCompID& targetCompID,
  const std::string& qualifier )
throw( SessionNotFound )
{ QF_STACK_PUSH(Session::sendToTarget)

  message.getHeader().setField( senderCompID );
  message.getHeader().setField( targetCompID );
  return sendToTarget( message, qualifier );

  QF_STACK_POP
}

bool Session::sendToTarget
( Message& message, const std::string& sender, const std::string& target,
  const std::string& qualifier )
throw( SessionNotFound )
{ QF_STACK_PUSH(Session::sendToTarget)

  return sendToTarget( message, SenderCompID( sender ),
                       TargetCompID( target ), qualifier );

  QF_STACK_POP
}

std::set<SessionID> Session::getSessions()
{
  return s_sessionIDs;
}

bool Session::doesSessionExist( const SessionID& sessionID )
{ QF_STACK_PUSH(Session::doesSessionExist)

  Locker locker( s_mutex );
  return s_sessions.end() != s_sessions.find( sessionID );

  QF_STACK_POP
}

Session* Session::lookupSession( const SessionID& sessionID )
{ QF_STACK_PUSH(Session::lookupSession)

  Locker locker( s_mutex );
  Sessions::iterator find = s_sessions.find( sessionID );
  if ( find != s_sessions.end() )
    return find->second;
  else
    return 0;

  QF_STACK_POP
}

Session* Session::lookupSession( const std::string& string, bool reverse )
{ QF_STACK_PUSH(Session::lookupSession)

  Message message;
  if ( !message.setStringHeader( string ) )
    return 0;

  try
  {
    const Header& header = message.getHeader();
    const BeginString& beginString = FIELD_GET_REF( header, BeginString );
    const SenderCompID& senderCompID = FIELD_GET_REF( header, SenderCompID );
    const TargetCompID& targetCompID = FIELD_GET_REF( header, TargetCompID );

    if ( reverse )
    {
      return lookupSession( SessionID( beginString, SenderCompID( targetCompID ),
                                     TargetCompID( senderCompID ) ) );
    }

    return lookupSession( SessionID( beginString, senderCompID,
                          targetCompID ) );
  }
  catch ( FieldNotFound& ) { return 0; }

  QF_STACK_POP
}

bool Session::isSessionRegistered( const SessionID& sessionID )
{ QF_STACK_PUSH(Session::isSessionRegistered)

  Locker locker( s_mutex );
  return s_registered.end() != s_registered.find( sessionID );

  QF_STACK_POP
}

Session* Session::registerSession( const SessionID& sessionID )
{ QF_STACK_PUSH(Session::registerSession)

  Locker locker( s_mutex );
  Session* pSession = lookupSession( sessionID );
  if ( pSession == 0 ) return 0;
  if ( isSessionRegistered( sessionID ) ) return 0;
  s_registered[ sessionID ] = pSession;
  return pSession;

  QF_STACK_POP
}

void Session::unregisterSession( const SessionID& sessionID )
{ QF_STACK_PUSH(Session::unregisterSession)
  Locker locker( s_mutex );
  s_registered.erase( sessionID );
  QF_STACK_POP
}

int Session::numSessions()
{ QF_STACK_PUSH(Session::numSessions)
  Locker locker( s_mutex );
  return s_sessions.size();
  QF_STACK_POP
}

bool Session::addSession( Session& s )
{ QF_STACK_PUSH(Session::addSession)

  Locker locker( s_mutex );
  Sessions::iterator it = s_sessions.find( s.m_sessionID );
  if ( it == s_sessions.end() )
  {
    s_sessions[ s.m_sessionID ] = &s;
    s_sessionIDs.insert( s.m_sessionID );
    return true;
  }
  else
    return false;

  QF_STACK_POP
}

void Session::removeSession( Session& s )
{ QF_STACK_PUSH(Session::removeSession)

  Locker locker( s_mutex );
  s_sessions.erase( s.m_sessionID );
  s_sessionIDs.erase( s.m_sessionID );
  s_registered.erase( s.m_sessionID );

  QF_STACK_POP
}
}
