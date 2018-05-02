/*********************************************************************/
/* Copyright (c) 2013-2017, EPFL/Blue Brain Project                  */
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

#include "Server.h"

#include "FrameDispatcher.h"
#include "ServerWorker.h"
#include "deflect/NetworkProtocol.h"

#include <QNetworkProxy>
#include <QThread>

#include <stdexcept>

namespace deflect
{
namespace server
{
const int Server::defaultPortNumber = DEFAULT_PORT_NUMBER;

class Server::Impl
{
public:
    Impl(QObject* parent)
        : frameDispatcher(
              new FrameDispatcher(parent)) // will be deleted by parent
    {
    }

    FrameDispatcher* frameDispatcher;
};

Server::Server(const int port)
    : _impl(new Impl(this))
{
    setProxy(QNetworkProxy::NoProxy);
    if (!listen(QHostAddress::Any, port))
    {
        const auto err = QString("could not listen on port: %1. QTcpServer: %2")
                             .arg(port)
                             .arg(QTcpServer::errorString());
        throw std::runtime_error(err.toStdString());
    }

    // Forward FrameDispatcher signals
    connect(_impl->frameDispatcher, &FrameDispatcher::pixelStreamOpened, this,
            &Server::pixelStreamOpened);
    connect(_impl->frameDispatcher, &FrameDispatcher::pixelStreamClosed, this,
            &Server::pixelStreamClosed);
    connect(_impl->frameDispatcher, &FrameDispatcher::sendFrame, this,
            &Server::receivedFrame);
    connect(_impl->frameDispatcher, &FrameDispatcher::pixelStreamException,
            [this](const QString uri, const QString what) {
                emit pixelStreamException(uri, what);
                closePixelStream(uri);
            });
}

Server::~Server()
{
    for (QObject* child : children())
    {
        if (QThread* workerThread = qobject_cast<QThread*>(child))
        {
            workerThread->quit();
            workerThread->wait();
        }
    }
}

void Server::requestFrame(const QString uri)
{
    _impl->frameDispatcher->requestFrame(uri);
}

void Server::closePixelStream(const QString uri)
{
    emit _closePixelStream(uri);
    _impl->frameDispatcher->deleteStream(uri);
}

void Server::incomingConnection(const qintptr socketHandle)
{
    QThread* workerThread = new QThread(this);
    ServerWorker* worker = new ServerWorker(socketHandle);

    worker->moveToThread(workerThread);

    connect(workerThread, &QThread::started, worker,
            &ServerWorker::initConnection);
    connect(worker, &ServerWorker::connectionClosed, workerThread,
            &QThread::quit);

    // Make sure the thread will be deleted
    connect(workerThread, &QThread::finished, worker,
            &ServerWorker::deleteLater);
    connect(workerThread, &QThread::finished, workerThread,
            &QThread::deleteLater);

    // public signals/slots, forwarding from/to worker
    connect(worker, &ServerWorker::registerToEvents, this,
            &Server::registerToEvents);
    connect(worker, &ServerWorker::receivedSizeHints, this,
            &Server::receivedSizeHints);
    connect(worker, &ServerWorker::receivedData, this, &Server::receivedData);
    connect(this, &Server::_closePixelStream, worker,
            &ServerWorker::closeConnection);

    // FrameDispatcher
    connect(worker, &ServerWorker::addStreamSource, _impl->frameDispatcher,
            &FrameDispatcher::addSource);
    connect(worker, &ServerWorker::receivedTile, _impl->frameDispatcher,
            &FrameDispatcher::processTile);
    connect(worker, &ServerWorker::receivedFrameFinished,
            _impl->frameDispatcher, &FrameDispatcher::processFrameFinished);
    connect(worker, &ServerWorker::removeStreamSource, _impl->frameDispatcher,
            &FrameDispatcher::removeSource);
    connect(worker, &ServerWorker::addObserver, _impl->frameDispatcher,
            &FrameDispatcher::addObserver);
    connect(worker, &ServerWorker::removeObserver, _impl->frameDispatcher,
            &FrameDispatcher::removeObserver);

    workerThread->start();
}
}
}
