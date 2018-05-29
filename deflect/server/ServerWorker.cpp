/*********************************************************************/
/* Copyright (c) 2013-2018, EPFL/Blue Brain Project                  */
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

#include "deflect/NetworkProtocol.h"
#include "deflect/SegmentParameters.h"

#include <QDataStream>

#include <cstdint>
#include <stdexcept>

namespace
{
const int RECEIVE_TIMEOUT_MS = 3000;

class protocol_error : public std::runtime_error
{
    using runtime_error::runtime_error;
};

bool _isProtocolStart(const deflect::MessageType messageType)
{
    return messageType == deflect::MESSAGE_TYPE_PIXELSTREAM_OPEN ||
           messageType == deflect::MESSAGE_TYPE_OBSERVER_OPEN;
}
}

namespace deflect
{
namespace server
{
ServerWorker::ServerWorker(const int socketDescriptor)
    : _tcpSocket{new QTcpSocket(this)} // Ensure that _tcpSocket parent is
                                       // *this* so it gets moved to thread
    , _sourceId{socketDescriptor}
    , _clientProtocolVersion{NETWORK_PROTOCOL_VERSION}
{
    if (!_tcpSocket->setSocketDescriptor(socketDescriptor))
    {
        throw std::runtime_error("could not set socket descriptor: " +
                                 _tcpSocket->errorString().toStdString());
    }

    connect(_tcpSocket, &QTcpSocket::disconnected, this,
            &ServerWorker::connectionClosed);

    connect(_tcpSocket, &QTcpSocket::readyRead, this,
            &ServerWorker::_processMessages, Qt::QueuedConnection);
    connect(this, &ServerWorker::_dataAvailable, this,
            &ServerWorker::_processMessages, Qt::QueuedConnection);
}

ServerWorker::~ServerWorker()
{
    // If the sender crashed, we may not recieve the quit message.
    // We still want to remove this source so that the stream does not get stuck
    // if other senders are still active / resp. the window gets closed if no
    // more senders contribute to it.
    if (_isProtocolStarted())
        _notifyProtocolEnd();

    if (_isConnected())
        _sendQuit();
}

void ServerWorker::processEvent(const Event evt)
{
    _events.emplace_back(evt);
    emit _dataAvailable();
}

void ServerWorker::initConnection()
{
    _sendProtocolVersion();
}

void ServerWorker::closeConnections(const QString uri)
{
    if (uri == _streamId)
        _terminateConnection();
}

void ServerWorker::closeConnection(const QString uri, const size_t sourceIndex)
{
    if (uri == _streamId && sourceIndex == (size_t)_sourceId)
        _terminateConnection();
}

void ServerWorker::_terminateConnection()
{
    if (_registeredToEvents)
        _sendCloseEvent();

    emit connectionClosed();
}

void ServerWorker::_processMessages()
{
    if (_socketHasMessage())
        _receiveMessage();

    _sendPendingEvents();

    if (!_isConnected())
    {
        // Drain pending messages from closed socket
        while (_socketHasMessage())
            _receiveMessage();

        emit connectionClosed();
    }
    else if (_socketHasMessage())
        emit _dataAvailable();
}

void ServerWorker::_receiveMessage()
{
    try
    {
        const auto messageHeader = _receiveMessageHeader();
        const auto messageBody = _receiveMessageBody(messageHeader.size);
        _handleMessage(messageHeader, messageBody);
    }
    catch (const std::runtime_error& e)
    {
        emit connectionError(_streamId, e.what());
        _terminateConnection();
    }
}

MessageHeader ServerWorker::_receiveMessageHeader()
{
    MessageHeader messageHeader;

    QDataStream stream(_tcpSocket);
    stream >> messageHeader;

    return messageHeader;
}

QByteArray ServerWorker::_receiveMessageBody(const int size)
{
    QByteArray messageData;

    if (size > 0)
    {
        messageData = _tcpSocket->read(size);

        while (messageData.size() < size)
        {
            if (!_tcpSocket->waitForReadyRead(RECEIVE_TIMEOUT_MS))
                throw std::runtime_error("Timeout reading message data");

            messageData.append(_tcpSocket->read(size - messageData.size()));
        }
    }

    return messageData;
}

bool ServerWorker::_socketHasMessage() const
{
    return _tcpSocket->bytesAvailable() >=
           (qint64)MessageHeader::serializedSize;
}

void ServerWorker::_handleMessage(const MessageHeader& messageHeader,
                                  const QByteArray& byteArray)
{
    _validate(messageHeader.type);

    switch (messageHeader.type)
    {
    case MESSAGE_TYPE_PIXELSTREAM_OPEN:
        _startProtocol(messageHeader.uri, byteArray, false);
        break;

    case MESSAGE_TYPE_OBSERVER_OPEN:
        _startProtocol(messageHeader.uri, byteArray, true);
        break;

    case MESSAGE_TYPE_QUIT:
        _stopProtocol();
        break;

    case MESSAGE_TYPE_PIXELSTREAM_FINISH_FRAME:
        emit receivedFrameFinished(_streamId, _sourceId);
        break;

    case MESSAGE_TYPE_PIXELSTREAM:
        emit receivedTile(_streamId, _sourceId, _parseTile(byteArray));
        break;

    case MESSAGE_TYPE_SIZE_HINTS:
    {
        const auto hints = reinterpret_cast<const SizeHints*>(byteArray.data());
        emit receivedSizeHints(_streamId, *hints);
        break;
    }

    case MESSAGE_TYPE_DATA:
        emit receivedData(_streamId, byteArray);
        break;

    case MESSAGE_TYPE_IMAGE_VIEW:
    {
        const auto view = reinterpret_cast<const View*>(byteArray.data());
        if (*view >= View::mono && *view <= View::right_eye)
            _activeView = *view;
        break;
    }

    case MESSAGE_TYPE_IMAGE_ROW_ORDER:
    {
        const auto order = reinterpret_cast<const RowOrder*>(byteArray.data());
        if (*order >= RowOrder::top_down && *order <= RowOrder::bottom_up)
            _activeRowOrder = *order;
        break;
    }

    case MESSAGE_TYPE_IMAGE_CHANNEL:
    {
        const auto channel = reinterpret_cast<const uint8_t*>(byteArray.data());
        _activeChannel = *channel;
        break;
    }

    case MESSAGE_TYPE_BIND_EVENTS:
    case MESSAGE_TYPE_BIND_EVENTS_EX:
    {
        const auto excl = messageHeader.type == MESSAGE_TYPE_BIND_EVENTS_EX;
        _tryRegisteringForEvents(excl);
        _sendBindReply(_registeredToEvents);
        break;
    }

    default:
        break;
    }
}

void ServerWorker::_validate(const MessageType messageType) const
{
    if (!_isProtocolStarted() && !_isProtocolStart(messageType))
    {
        throw protocol_error(
            std::string("Unexpected message received before protocol start (") +
            std::to_string((int)messageType) + ")");
    }
}

void ServerWorker::_startProtocol(const QString& uri,
                                  const QByteArray& byteArray,
                                  const bool observer)
{
    if (_isProtocolStarted())
        throw protocol_error("Stream protocol was started already");

    if (uri.isEmpty())
        throw protocol_error("Can't init stream protocol with empty stream id");

    if (_protocolEnded)
        throw protocol_error("Stream protocol cannot be restarted once ended");

    _streamId = uri;
    _observer = observer;
    _parseClientProtocolVersion(byteArray);

    if (_observer)
        emit addObserver(_streamId);
    else
        emit addStreamSource(_streamId, _sourceId);
}

void ServerWorker::_stopProtocol()
{
    if (!_isProtocolStarted())
        throw protocol_error("Stream protocol had already ended");

    _notifyProtocolEnd();

    _streamId = QString();
    _protocolEnded = true;
}

void ServerWorker::_notifyProtocolEnd()
{
    if (_observer)
        emit removeObserver(_streamId);
    else
        emit removeStreamSource(_streamId, _sourceId);
}

bool ServerWorker::_isProtocolStarted() const
{
    return !_streamId.isEmpty();
}

void ServerWorker::_parseClientProtocolVersion(const QByteArray& message)
{
    // The version is only sent by deflect clients since release 0.12.1
    if (message.isEmpty())
        return;

    bool ok = false;
    const int version = message.toInt(&ok);
    if (ok)
        _clientProtocolVersion = version;
}

Tile ServerWorker::_parseTile(const QByteArray& message) const
{
    Tile tile;

    const auto data = message.data();
    const auto params = reinterpret_cast<const SegmentParameters*>(data);
    tile.format = params->format;
    tile.x = params->x;
    tile.y = params->y;
    tile.width = params->width;
    tile.height = params->height;
    tile.imageData = message.right(message.size() - sizeof(SegmentParameters));
    tile.view = _activeView;
    tile.rowOrder = _activeRowOrder;
    tile.channel = _activeChannel;

    return tile;
}

void ServerWorker::_tryRegisteringForEvents(const bool exclusive)
{
    if (_registeredToEvents)
        throw protocol_error("The stream has already registered for events");

    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();

    emit registerToEvents(_streamId, exclusive, this, std::move(promise));

    try
    {
        _registeredToEvents = future.get();
    }
    catch (...)
    {
    }
}

void ServerWorker::_sendProtocolVersion()
{
    const int32_t protocolVersion = NETWORK_PROTOCOL_VERSION;
    _tcpSocket->write((char*)&protocolVersion, sizeof(int32_t));
    _flushSocket();
}

void ServerWorker::_sendPendingEvents()
{
    for (const auto& evt : _events)
        _send(evt);
    _events.clear();
    _flushSocket();
}

void ServerWorker::_sendBindReply(const bool successful)
{
    MessageHeader mh(MESSAGE_TYPE_BIND_EVENTS_REPLY, sizeof(bool));
    _send(mh);

    _tcpSocket->write((const char*)&successful, sizeof(bool));
    _flushSocket();
}

void ServerWorker::_send(const Event& evt)
{
    // send message header
    MessageHeader mh(MESSAGE_TYPE_EVENT, Event::serializedSize);
    _send(mh);

    {
        QDataStream stream(_tcpSocket);
        stream << evt;
    }
    _flushSocket();
}

void ServerWorker::_sendCloseEvent()
{
    Event closeEvent;
    closeEvent.type = Event::EVT_CLOSE;
    _send(closeEvent);
}

void ServerWorker::_sendQuit()
{
    MessageHeader mh(MESSAGE_TYPE_QUIT, 0);
    _send(mh);
    _flushSocket();
}

bool ServerWorker::_send(const MessageHeader& messageHeader)
{
    QDataStream stream(_tcpSocket);
    stream << messageHeader;

    return stream.status() == QDataStream::Ok;
}

void ServerWorker::_flushSocket()
{
    _tcpSocket->flush();
    while (_tcpSocket->bytesToWrite() > 0 && _isConnected())
        _tcpSocket->waitForBytesWritten();
}

bool ServerWorker::_isConnected() const
{
    return _tcpSocket->state() == QTcpSocket::ConnectedState;
}
}
}
