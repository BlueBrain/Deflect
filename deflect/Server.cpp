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

#include "Server.h"

#include "CommandHandler.h"
#include "FrameDispatcher.h"
#include "NetworkProtocol.h"
#include "ServerWorker.h"

#ifdef DEFLECT_USE_SERVUS
#  include <servus/servus.h>
#  include <boost/lexical_cast.hpp>
#endif

#include <QThread>
#include <stdexcept>

namespace deflect
{
const int Server::defaultPortNumber = DEFAULT_PORT_NUMBER;
const std::string Server::serviceName = SERVUS_SERVICE_NAME;

class Server::Impl
{
public:
    Impl()
#ifdef DEFLECT_USE_SERVUS
        : servus( Server::serviceName )
#endif
    {}

    FrameDispatcher pixelStreamDispatcher;
    CommandHandler commandHandler;
#ifdef DEFLECT_USE_SERVUS
    servus::Servus servus;
#endif
};

Server::Server( const int port )
    : _impl( new Impl )
{
    if( !listen( QHostAddress::Any, port ))
    {
        const QString err =
                QString( "could not listen on port: %1" ).arg( port );
        throw std::runtime_error( err.toStdString( ));
    }
#ifdef DEFLECT_USE_SERVUS
    _impl->servus.announce( port, boost::lexical_cast< std::string >( port ));
#endif
}

Server::~Server()
{
    delete _impl;
}

CommandHandler& Server::getCommandHandler()
{
    return _impl->commandHandler;
}

FrameDispatcher& Server::getPixelStreamDispatcher()
{
    return _impl->pixelStreamDispatcher;
}

void Server::onPixelStreamerClosed( const QString uri )
{
    emit _pixelStreamerClosed( uri );
}

void Server::onEventRegistrationReply( const QString uri, const bool success )
{
    emit _eventRegistrationReply( uri, success );
}

void Server::incomingConnection( const qintptr socketHandle )
{
    QThread* workerThread = new QThread( this );
    ServerWorker* worker = new ServerWorker( socketHandle );

    worker->moveToThread( workerThread );

    connect( workerThread, SIGNAL( started( )),
             worker, SLOT( initConnection( )));
    connect( worker, SIGNAL( connectionClosed( )),
             workerThread, SLOT( quit( )));

    // Make sure the thread will be deleted
    connect( workerThread, SIGNAL( finished( )), worker, SLOT( deleteLater( )));
    connect( workerThread, SIGNAL( finished( )),
             workerThread, SLOT( deleteLater( )));

    // public signals/slots, forwarding from/to worker
    connect( worker, SIGNAL( registerToEvents( QString, bool,
                                               deflect::EventReceiver* )),
             this, SIGNAL( registerToEvents( QString, bool,
                                             deflect::EventReceiver* )));
    connect( this, SIGNAL( _pixelStreamerClosed( QString )),
             worker, SLOT( closeConnection( QString )));
    connect( this, SIGNAL( _eventRegistrationReply( QString, bool )),
             worker, SLOT( replyToEventRegistration( QString, bool )));

    // Commands
    connect( worker, SIGNAL( receivedCommand( QString, QString )),
             &_impl->commandHandler, SLOT( process( QString, QString )));

    // PixelStreamDispatcher
    connect( worker, SIGNAL( addStreamSource( QString, size_t )),
             &_impl->pixelStreamDispatcher, SLOT(addSource( QString, size_t )));
    connect( worker,
             SIGNAL( receivedSegement( QString, size_t, deflect::Segment )),
             &_impl->pixelStreamDispatcher,
             SLOT( processSegment( QString, size_t, deflect::Segment )));
    connect( worker,
             SIGNAL( receivedFrameFinished( QString, size_t )),
             &_impl->pixelStreamDispatcher,
             SLOT( processFrameFinished( QString, size_t )));
    connect( worker,
             SIGNAL( removeStreamSource( QString, size_t )),
             &_impl->pixelStreamDispatcher,
             SLOT( removeSource( QString, size_t )));

    workerThread->start();
}

}
