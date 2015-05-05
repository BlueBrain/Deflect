/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
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

#include "NetworkListener.h"

#include "NetworkListenerWorker.h"
#include "PixelStreamDispatcher.h"
#include "CommandHandler.h"

#ifdef DEFLECT_USE_LUNCHBOX
#  include <lunchbox/servus.h>
#  include <boost/lexical_cast.hpp>
#endif

#include <QThread>
#include <stdexcept>

namespace deflect
{
const int NetworkListener::defaultPortNumber = 1701;
const std::string NetworkListener::serviceName = "_displaycluster._tcp";

namespace detail
{
class NetworkListener
{
public:
    NetworkListener()
#ifdef DEFLECT_USE_LUNCHBOX
        : servus( deflect::NetworkListener::serviceName )
#endif
    {}

    PixelStreamDispatcher pixelStreamDispatcher;
    CommandHandler commandHandler;
#ifdef DEFLECT_USE_LUNCHBOX
    lunchbox::Servus servus;
#endif
};
}


NetworkListener::NetworkListener( const int port )
    : _impl( new detail::NetworkListener )
{
    if( !listen( QHostAddress::Any, port ))
    {
        const QString err = QString("could not listen on port: %1").arg(port);
        throw std::runtime_error(err.toStdString());
    }
#ifdef DEFLECT_USE_LUNCHBOX
    _impl->servus.announce( port, boost::lexical_cast< std::string >( port ));
#endif
}

NetworkListener::~NetworkListener()
{
    delete _impl;
}

CommandHandler& NetworkListener::getCommandHandler()
{
    return _impl->commandHandler;
}

PixelStreamDispatcher& NetworkListener::getPixelStreamDispatcher()
{
    return _impl->pixelStreamDispatcher;
}

void NetworkListener::incomingConnection(int socketHandle)
{
    QThread* workerThread = new QThread(this);
    NetworkListenerWorker* worker = new NetworkListenerWorker(socketHandle);

    worker->moveToThread(workerThread);

    connect(workerThread, SIGNAL(started()), worker, SLOT(initialize()));
    connect(worker, SIGNAL(finished()), workerThread, SLOT(quit()));

    // Make sure the thread will be deleted
    connect(workerThread, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));

    // public signals/slots, forwarding from/to worker
    connect(worker,
            SIGNAL(registerToEvents(QString, bool, deflect::EventReceiver*)),
            this,
            SIGNAL(registerToEvents(QString, bool, deflect::EventReceiver*)));
    connect(this, SIGNAL(pixelStreamerClosed(QString)),
            worker, SLOT(pixelStreamerClosed(QString)));
    connect(this, SIGNAL(eventRegistrationReply(QString, bool)),
            worker, SLOT(eventRegistrationReply(QString, bool)));

    // Commands
    connect(worker, SIGNAL(receivedCommand(QString,QString)),
            &_impl->commandHandler, SLOT(process(QString,QString)));

    // PixelStreamDispatcher
    connect(worker, SIGNAL(receivedAddPixelStreamSource(QString,size_t)),
            &_impl->pixelStreamDispatcher, SLOT(addSource(QString,size_t)));
    connect(worker,
            SIGNAL(receivedPixelStreamSegement(QString,size_t,
                                               PixelStreamSegment)),
            &_impl->pixelStreamDispatcher,
            SLOT(processSegment(QString,size_t,PixelStreamSegment)));
    connect(worker,
            SIGNAL(receivedPixelStreamFinishFrame(QString,size_t)),
            &_impl->pixelStreamDispatcher,
            SLOT(processFrameFinished(QString,size_t)));
    connect(worker, SIGNAL(receivedRemovePixelStreamSource(QString,size_t)),
            &_impl->pixelStreamDispatcher,
            SLOT(removeSource(QString,size_t)));

    workerThread->start();
}

void NetworkListener::onPixelStreamerClosed( QString uri )
{
    emit pixelStreamerClosed( uri );
}

void NetworkListener::onEventRegistrationReply( QString uri, bool success )
{
    emit eventRegistrationReply( uri, success );
}

}
