/*********************************************************************/
/* Copyright (c) 2013-2016, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
/*                          Daniel.Nachbaur@epfl.ch                  */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#include "ServerWorker.h"

#include "NetworkProtocol.h"

#include <stdint.h>
#include <iostream>

#include <QDataStream>

#define RECEIVE_TIMEOUT_MS 3000

namespace deflect
{

ServerWorker::ServerWorker( const int socketDescriptor )
    // Ensure that tcpSocket_ parent is *this* so it gets moved to thread
    : _tcpSocket( new QTcpSocket( this ))
    , _sourceId( socketDescriptor )
    , _registeredToEvents( false )
{
    if( !_tcpSocket->setSocketDescriptor( socketDescriptor ))
    {
        std::cerr << "could not set socket descriptor: "
                  << _tcpSocket->errorString().toStdString() << std::endl;
        emit( connectionClosed( ));
        return;
    }

    connect( _tcpSocket, &QTcpSocket::disconnected,
             this, &ServerWorker::connectionClosed );

    connect( _tcpSocket, &QTcpSocket::readyRead,
             this, &ServerWorker::_processMessages, Qt::QueuedConnection );
    connect( this, &ServerWorker::_dataAvailable,
             this, &ServerWorker::_processMessages, Qt::QueuedConnection );
}

ServerWorker::~ServerWorker()
{
    // If the sender crashed, we may not recieve the quit message.
    // We still want to remove this source so that the stream does not get stuck
    // if other senders are still active / resp. the window gets closed if no
    // more senders contribute to it.
    if( !_streamId.isEmpty( ))
        emit removeStreamSource( _streamId, _sourceId );

    if( _tcpSocket->state() == QAbstractSocket::ConnectedState )
        _sendQuit();

    delete _tcpSocket;
}

void ServerWorker::processEvent( const Event evt )
{
    _events.enqueue( evt );
    emit _dataAvailable();
}

void ServerWorker::initConnection()
{
    _sendProtocolVersion();
}

void ServerWorker::closeConnection( const QString uri )
{
    if( uri != _streamId )
        return;

    Event closeEvent;
    closeEvent.type = Event::EVT_CLOSE;
    _send( closeEvent );

    emit( connectionClosed( ));
}

void ServerWorker::replyToEventRegistration( const QString uri,
                                             const bool success )
{
    if( uri != _streamId )
        return;

    _registeredToEvents = success;
    _sendBindReply( _registeredToEvents );
}

void ServerWorker::_processMessages()
{
    const qint64 headerSize( MessageHeader::serializedSize );

    if( _tcpSocket->bytesAvailable() >= headerSize )
        _receiveMessage();

    // Send all events
    foreach( const Event& evt, _events )
        _send( evt );
    _events.clear();

    _tcpSocket->flush();

    // Finish reading messages from the socket if connection closed
    if( _tcpSocket->state() != QAbstractSocket::ConnectedState )
    {
        while( _tcpSocket->bytesAvailable() >= headerSize )
            _receiveMessage();

        emit( connectionClosed( ));
    }
    else if( _tcpSocket->bytesAvailable() >= headerSize )
        emit _dataAvailable();
}

void ServerWorker::_receiveMessage()
{
    const MessageHeader mh = _receiveMessageHeader();
    const QByteArray messageByteArray = _receiveMessageBody( mh.size );
    _handleMessage( mh, messageByteArray );
}

MessageHeader ServerWorker::_receiveMessageHeader()
{
    MessageHeader messageHeader;

    QDataStream stream( _tcpSocket );
    stream >> messageHeader;

    return messageHeader;
}

QByteArray ServerWorker::_receiveMessageBody( const int size )
{
    QByteArray messageByteArray;

    if( size > 0 )
    {
        messageByteArray = _tcpSocket->read( size );

        while( messageByteArray.size() < size )
        {
            if( !_tcpSocket->waitForReadyRead( RECEIVE_TIMEOUT_MS ))
            {
                emit connectionClosed();
                return QByteArray();
            }

            messageByteArray.append(
                        _tcpSocket->read( size - messageByteArray.size( )));
        }
    }

    return messageByteArray;
}

void ServerWorker::_handleMessage( const MessageHeader& messageHeader,
                                   const QByteArray& byteArray )
{
    const QString uri( messageHeader.uri );
    if( uri.isEmpty( ))
    {
        std::cerr << "Warning: rejecting streamer with empty id" << std::endl;
        closeConnection( _streamId );
        return;
    }
    if( uri != _streamId &&
            messageHeader.type != MESSAGE_TYPE_PIXELSTREAM_OPEN )
    {
        std::cerr << "Warning: ingnoring message with incorrect stream id: '"
                  << messageHeader.uri << "', expected: '"
                  << _streamId.toStdString() << "'" <<  std::endl;
        return;
    }

    switch( messageHeader.type )
    {
    case MESSAGE_TYPE_QUIT:
        emit removeStreamSource( _streamId, _sourceId );
        _streamId = QString();
        break;

    case MESSAGE_TYPE_PIXELSTREAM_OPEN:
        if( !_streamId.isEmpty( ))
        {
            std::cerr << "Warning: PixelStream already opened!" << std::endl;
            return;
        }
        _streamId = uri;
        emit addStreamSource( _streamId, _sourceId );
        break;

    case MESSAGE_TYPE_PIXELSTREAM_FINISH_FRAME:
        emit receivedFrameFinished( _streamId, _sourceId );
        break;

    case MESSAGE_TYPE_PIXELSTREAM:
        _handlePixelStreamMessage( byteArray );
        break;

    case MESSAGE_TYPE_SIZE_HINTS:
    {
        const SizeHints* hints =
                reinterpret_cast< const SizeHints* >( byteArray.data( ));
        emit receivedSizeHints( _streamId, SizeHints( *hints ));
        break;
    }

    case MESSAGE_TYPE_BIND_EVENTS:
    case MESSAGE_TYPE_BIND_EVENTS_EX:
        if( _registeredToEvents )
            std::cerr << "We are already bound!!" << std::endl;
        else
        {
            const bool exclusive =
                    (messageHeader.type == MESSAGE_TYPE_BIND_EVENTS_EX);
            emit registerToEvents( _streamId, exclusive, this );
        }
        break;

    default:
        break;
    }
}

void ServerWorker::_handlePixelStreamMessage( const QByteArray& byteArray )
{
    const SegmentParameters* parameters =
            reinterpret_cast< const SegmentParameters* >( byteArray.data( ));

    Segment segment;
    segment.parameters = *parameters;

    QByteArray imageData =
            byteArray.right( byteArray.size() - sizeof( SegmentParameters ));
    segment.imageData = imageData;

    emit( receivedSegment( _streamId, _sourceId, segment ));
}

void ServerWorker::_sendProtocolVersion()
{
    const int32_t protocolVersion = NETWORK_PROTOCOL_VERSION;
    _tcpSocket->write( (char*)&protocolVersion, sizeof( int32_t ));
    _flushSocket();
}

void ServerWorker::_sendBindReply( const bool successful )
{
    MessageHeader mh( MESSAGE_TYPE_BIND_EVENTS_REPLY, sizeof( bool ));
    _send( mh );

    _tcpSocket->write( (const char *)&successful, sizeof( bool ));
    _flushSocket();
}

void ServerWorker::_send( const Event& evt )
{
    // send message header
    MessageHeader mh( MESSAGE_TYPE_EVENT, Event::serializedSize );
    _send( mh );

    {
        QDataStream stream( _tcpSocket );
        stream << evt;
    }
    _flushSocket();
}

void ServerWorker::_sendQuit()
{
    MessageHeader mh( MESSAGE_TYPE_QUIT, 0 );
    _send( mh );
    _flushSocket();
}

bool ServerWorker::_send( const MessageHeader& messageHeader )
{
    QDataStream stream( _tcpSocket );
    stream << messageHeader;

    return stream.status() == QDataStream::Ok;
}

void ServerWorker::_flushSocket()
{
    _tcpSocket->flush();
    while( _tcpSocket->bytesToWrite() > 0 )
        _tcpSocket->waitForBytesWritten();
}

}
