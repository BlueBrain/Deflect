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
/*    THIS  SOFTWARE  IS  PROVIDED  BY  THE  ECOLE  POLYTECHNIQUE    */
/*    FEDERALE DE LAUSANNE  ''AS IS''  AND ANY EXPRESS OR IMPLIED    */
/*    WARRANTIES, INCLUDING, BUT  NOT  LIMITED  TO,  THE  IMPLIED    */
/*    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  A PARTICULAR    */
/*    PURPOSE  ARE  DISCLAIMED.  IN  NO  EVENT  SHALL  THE  ECOLE    */
/*    POLYTECHNIQUE  FEDERALE  DE  LAUSANNE  OR  CONTRIBUTORS  BE    */
/*    LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,    */
/*    EXEMPLARY,  OR  CONSEQUENTIAL  DAMAGES  (INCLUDING, BUT NOT    */
/*    LIMITED TO,  PROCUREMENT  OF  SUBSTITUTE GOODS OR SERVICES;    */
/*    LOSS OF USE, DATA, OR  PROFITS;  OR  BUSINESS INTERRUPTION)    */
/*    HOWEVER CAUSED AND  ON ANY THEORY OF LIABILITY,  WHETHER IN    */
/*    CONTRACT, STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE    */
/*    OR OTHERWISE) ARISING  IN ANY WAY  OUT OF  THE USE OF  THIS    */
/*    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.   */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of Ecole polytechnique federale de Lausanne.          */
/*********************************************************************/

#include "Server.h"

#include "FrameDispatcher.h"
#include "ServerWorker.h"
#include "deflect/NetworkProtocol.h"

#include <QThread>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QTcpServer>

#include <stdexcept>

namespace deflect
{
namespace server
{
const int Server::defaultPortNumber = DEFAULT_PORT_NUMBER;

class Server::Impl : public QTcpServer
{
public:
    Impl(const int port, Server* parent_)
        : QTcpServer(parent_)
        , server{parent_}
        , frameDispatcher{new FrameDispatcher{parent_}}
    {
        setProxy(QNetworkProxy::NoProxy);
        if (!listen(QHostAddress::Any, port))
        {
            const auto err =
                QString("could not listen on port: %1. QTcpServer: %2")
                    .arg(port)
                    .arg(QTcpServer::errorString());
            throw std::runtime_error(err.toStdString());
        }
    }

    ~Impl()
    {
        for (auto child : children())
        {
            if (auto workerThread = qobject_cast<QThread*>(child))
            {
                workerThread->quit();
                workerThread->wait();
            }
        }
    }

    /** Re-implemented handling of connections from QTCPSocket. */
    void incomingConnection(const qintptr socketHandle) final
    {
        try
        {
            auto worker = new ServerWorker(socketHandle);
            auto workerThread = new QThread(this);
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
            connect(worker, &ServerWorker::registerToEvents, server,
                    &Server::registerToEvents);
            connect(worker, &ServerWorker::receivedSizeHints, server,
                    &Server::receivedSizeHints);
            connect(worker, &ServerWorker::receivedData, server,
                    &Server::receivedData);
            connect(worker, &ServerWorker::connectionError, server,
                    &Server::pixelStreamException);
            connect(server, &Server::_closePixelStream, worker,
                    &ServerWorker::closeConnections);

            // FrameDispatcher
            connect(worker, &ServerWorker::addStreamSource, frameDispatcher,
                    &FrameDispatcher::addSource);
            connect(frameDispatcher, &FrameDispatcher::sourceRejected, worker,
                    &ServerWorker::closeConnection);
            connect(worker, &ServerWorker::receivedTile, frameDispatcher,
                    &FrameDispatcher::processTile);
            connect(worker, &ServerWorker::receivedFrameFinished,
                    frameDispatcher, &FrameDispatcher::processFrameFinished);
            connect(worker, &ServerWorker::removeStreamSource, frameDispatcher,
                    &FrameDispatcher::removeSource);
            connect(worker, &ServerWorker::addObserver, frameDispatcher,
                    &FrameDispatcher::addObserver);
            connect(worker, &ServerWorker::removeObserver, frameDispatcher,
                    &FrameDispatcher::removeObserver);

            workerThread->start();
        }
        catch (const std::runtime_error& e)
        {
            emit server->pixelStreamException("", e.what());
        }
    }

    Server* server = nullptr;
    FrameDispatcher* frameDispatcher = nullptr; // owned by QObject's parent
};

Server::Server(const int port)
    : _impl(new Impl(port, this))
{
    // Forward FrameDispatcher signals
    connect(_impl->frameDispatcher, &FrameDispatcher::pixelStreamOpened, this,
            &Server::pixelStreamOpened);
    connect(_impl->frameDispatcher, &FrameDispatcher::pixelStreamClosed, this,
            &Server::pixelStreamClosed);
    connect(_impl->frameDispatcher, &FrameDispatcher::sendFrame, this,
            &Server::receivedFrame);
    connect(_impl->frameDispatcher, &FrameDispatcher::pixelStreamWarning, this,
            &Server::pixelStreamException);
    connect(_impl->frameDispatcher, &FrameDispatcher::pixelStreamError,
            [this](const QString uri, const QString what) {
                emit pixelStreamException(uri, what);
                closePixelStream(uri);
            });
}

Server::~Server()
{
    _impl.release(); // avoid double-deletion of child QObject
}

quint16 Server::getPort() const
{
    return _impl->serverPort();
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
}
}
