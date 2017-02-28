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

#include "Server.h"

#include "FrameDispatcher.h"
#include "NetworkProtocol.h"
#include "ServerWorker.h"

#include <QThread>
#include <stdexcept>

namespace deflect
{
const int Server::defaultPortNumber = DEFAULT_PORT_NUMBER;

class Server::Impl
{
public:
    FrameDispatcher frameDispatcher;
};

Server::Server(const int port)
    : _impl(new Impl)
{
    if (!listen(QHostAddress::Any, port))
    {
        const auto err = QString("could not listen on port: %1").arg(port);
        throw std::runtime_error(err.toStdString());
    }

    // Forward FrameDispatcher signals
    connect(&_impl->frameDispatcher, &FrameDispatcher::pixelStreamOpened, this,
            &Server::pixelStreamOpened);
    connect(&_impl->frameDispatcher, &FrameDispatcher::pixelStreamClosed, this,
            &Server::pixelStreamClosed);
    connect(&_impl->frameDispatcher, &FrameDispatcher::sendFrame, this,
            &Server::receivedFrame);
    connect(&_impl->frameDispatcher, &FrameDispatcher::bufferSizeExceeded, this,
            &Server::closePixelStream);
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
    _impl->frameDispatcher.requestFrame(uri);
}

void Server::closePixelStream(const QString uri)
{
    emit _closePixelStream(uri);
    _impl->frameDispatcher.deleteStream(uri);
}

void Server::replyToEventRegistration(const QString uri, const bool success)
{
    emit _eventRegistrationReply(uri, success);
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
    connect(this, &Server::_eventRegistrationReply, worker,
            &ServerWorker::replyToEventRegistration);

    // FrameDispatcher
    connect(worker, &ServerWorker::addStreamSource, &_impl->frameDispatcher,
            &FrameDispatcher::addSource);
    connect(worker, &ServerWorker::receivedSegment, &_impl->frameDispatcher,
            &FrameDispatcher::processSegment);
    connect(worker, &ServerWorker::receivedFrameFinished,
            &_impl->frameDispatcher, &FrameDispatcher::processFrameFinished);
    connect(worker, &ServerWorker::removeStreamSource, &_impl->frameDispatcher,
            &FrameDispatcher::removeSource);

    workerThread->start();
}
}
