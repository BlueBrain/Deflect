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

#ifndef DEFLECT_SERVER_SERVERWORKER_H
#define DEFLECT_SERVER_SERVERWORKER_H

#include <deflect/Event.h>
#include <deflect/MessageHeader.h>
#include <deflect/SizeHints.h>
#include <deflect/server/EventReceiver.h>
#include <deflect/server/Tile.h>

#include <QtNetwork/QTcpSocket>

namespace deflect
{
namespace server
{
class ServerWorker : public EventReceiver
{
    Q_OBJECT

public:
    explicit ServerWorker(int socketDescriptor);
    ~ServerWorker();

public slots:
    void processEvent(Event evt) final;

    void initConnection();
    void closeConnections(QString uri);
    void closeConnection(QString uri, size_t sourceIndex);

signals:
    void addStreamSource(QString uri, size_t sourceIndex);
    void removeStreamSource(QString uri, size_t sourceIndex);

    void addObserver(QString uri);
    void removeObserver(QString uri);

    void receivedTile(QString uri, size_t sourceIndex,
                      deflect::server::Tile tile);
    void receivedFrameFinished(QString uri, size_t sourceIndex);
    void registerToEvents(QString uri, bool exclusive,
                          deflect::server::EventReceiver* receiver,
                          deflect::server::BoolPromisePtr success);
    void receivedSizeHints(QString uri, deflect::SizeHints hints);

    void receivedData(QString uri, QByteArray data);

    void connectionClosed();

    void connectionError(QString uri, QString what);

    /** @internal */
    void _dataAvailable();

private slots:
    void _processMessages();

private:
    QTcpSocket* _tcpSocket = nullptr; // child QObject
    const int _sourceId;

    QString _streamId;
    int _clientProtocolVersion;
    bool _observer = false;

    bool _registeredToEvents = false;
    std::vector<Event> _events;

    View _activeView = View::mono;
    RowOrder _activeRowOrder = RowOrder::top_down;
    uint8_t _activeChannel = 0;

    bool _protocolEnded = false;

    void _terminateConnection();

    void _receiveMessage();
    MessageHeader _receiveMessageHeader();
    QByteArray _receiveMessageBody(int size);

    bool _socketHasMessage() const;
    void _handleMessage(const MessageHeader& messageHeader,
                        const QByteArray& message);
    void _validate(MessageType messageType) const;
    void _startProtocol(const QString& uri, const QByteArray& byteArray,
                        bool observer);
    void _stopProtocol();
    void _notifyProtocolEnd();
    bool _isProtocolStarted() const;

    void _parseClientProtocolVersion(const QByteArray& message);
    Tile _parseTile(const QByteArray& message) const;

    void _tryRegisteringForEvents(bool exclusive);

    void _sendProtocolVersion();
    void _sendPendingEvents();
    void _sendBindReply(bool successful);
    void _send(const Event& evt);
    void _sendCloseEvent();
    void _sendQuit();
    bool _send(const MessageHeader& messageHeader);
    void _flushSocket();
    bool _isConnected() const;
};
}
}

#endif
