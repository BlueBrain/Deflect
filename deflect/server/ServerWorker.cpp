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
#include <iostream>

namespace
{
const int RECEIVE_TIMEOUT_MS = 3000;
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
        std::cerr << "could not set socket descriptor: "
                  << _tcpSocket->errorString().toStdString() << std::endl;
        emit(connectionClosed());
        return;
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
    if (!_streamId.isEmpty())
    {
        if (_observer)
            emit removeObserver(_streamId);
        else
            emit removeStreamSource(_streamId, _sourceId);
    }

    if (_isConnected())
        _sendQuit();

    delete _tcpSocket;
}

void ServerWorker::processEvent(const Event evt)
{
    _events.enqueue(evt);
    emit _dataAvailable();
}

void ServerWorker::initConnection()
{
    _sendProtocolVersion();
}

void ServerWorker::closeConnection(const QString uri)
{
    if (uri != _streamId)
        return;

    if (_registeredToEvents)
        _sendCloseEvent();

    emit(connectionClosed());
}

void ServerWorker::closeSource(const QString uri, const size_t sourceIndex)
{
    if (sourceIndex == (size_t)_sourceId)
        closeConnection(uri);
}

void ServerWorker::_processMessages()
{
    const qint64 headerSize(MessageHeader::serializedSize);

    if (_tcpSocket->bytesAvailable() >= headerSize)
        _receiveMessage();

    // Send all events
    foreach (const Event& evt, _events)
        _send(evt);
    _events.clear();

    _tcpSocket->flush();

    // Finish reading messages from the socket if connection closed
    if (!_isConnected())
    {
        while (_tcpSocket->bytesAvailable() >= headerSize)
            _receiveMessage();

        emit(connectionClosed());
    }
    else if (_tcpSocket->bytesAvailable() >= headerSize)
        emit _dataAvailable();
}

void ServerWorker::_receiveMessage()
{
    const auto messageHeader = _receiveMessageHeader();
    const auto messageBody = _receiveMessageBody(messageHeader.size);
    _handleMessage(messageHeader, messageBody);
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
    QByteArray messageByteArray;

    if (size > 0)
    {
        messageByteArray = _tcpSocket->read(size);

        while (messageByteArray.size() < size)
        {
            if (!_tcpSocket->waitForReadyRead(RECEIVE_TIMEOUT_MS))
            {
                emit connectionClosed();
                return QByteArray();
            }

            messageByteArray.append(
                _tcpSocket->read(size - messageByteArray.size()));
        }
    }

    return messageByteArray;
}

void ServerWorker::_handleMessage(const MessageHeader& messageHeader,
                                  const QByteArray& byteArray)
{
    const QString uri(messageHeader.uri);
    if (uri.isEmpty())
    {
        std::cerr << "Warning: rejecting streamer with empty id" << std::endl;
        closeConnection(_streamId);
        return;
    }
    if (uri != _streamId &&
        messageHeader.type != MESSAGE_TYPE_PIXELSTREAM_OPEN &&
        messageHeader.type != MESSAGE_TYPE_OBSERVER_OPEN)
    {
        std::cerr << "Warning: ignoring message with incorrect stream id: '"
                  << messageHeader.uri << "', expected: '"
                  << _streamId.toStdString() << "'" << std::endl;
        return;
    }

    switch (messageHeader.type)
    {
    case MESSAGE_TYPE_QUIT:
        if (_observer)
            emit removeObserver(_streamId);
        else
            emit removeStreamSource(_streamId, _sourceId);
        _streamId = QString();
        break;

    case MESSAGE_TYPE_PIXELSTREAM_OPEN:
        if (!_streamId.isEmpty())
        {
            std::cerr << "Warning: PixelStream already opened!" << std::endl;
            return;
        }
        _streamId = uri;
        // The version is only sent by deflect clients since v. 0.12.1
        if (!byteArray.isEmpty())
            _parseClientProtocolVersion(byteArray);
        emit addStreamSource(_streamId, _sourceId);
        break;

    case MESSAGE_TYPE_OBSERVER_OPEN:
        _streamId = uri;
        emit addObserver(_streamId);
        _observer = true;
        break;

    case MESSAGE_TYPE_PIXELSTREAM_FINISH_FRAME:
        emit receivedFrameFinished(_streamId, _sourceId);
        break;

    case MESSAGE_TYPE_PIXELSTREAM:
        _handlePixelStreamMessage(byteArray);
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
        if (_registeredToEvents)
            std::cerr << "We are already bound!!" << std::endl;
        else
        {
            const bool exclusive =
                (messageHeader.type == MESSAGE_TYPE_BIND_EVENTS_EX);
            auto promise = std::make_shared<std::promise<bool>>();
            auto future = promise->get_future();
            emit registerToEvents(_streamId, exclusive, this,
                                  std::move(promise));
            try
            {
                _registeredToEvents = future.get();
            }
            catch (...)
            {
            }
            _sendBindReply(_registeredToEvents);
        }
        break;

    default:
        break;
    }
}

void ServerWorker::_parseClientProtocolVersion(const QByteArray& message)
{
    bool ok = false;
    const int version = message.toInt(&ok);
    if (ok)
        _clientProtocolVersion = version;
}

void ServerWorker::_handlePixelStreamMessage(const QByteArray& message)
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

    emit(receivedTile(_streamId, _sourceId, tile));
}

void ServerWorker::_sendProtocolVersion()
{
    const int32_t protocolVersion = NETWORK_PROTOCOL_VERSION;
    _tcpSocket->write((char*)&protocolVersion, sizeof(int32_t));
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
