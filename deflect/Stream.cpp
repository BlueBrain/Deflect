/*********************************************************************/
/* Copyright (c) 2013-2016, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
/*                          Stefan.Eilemann@epfl.ch                  */
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

#include "Stream.h"
#include "StreamPrivate.h"

#include "Segment.h"
#include "SegmentParameters.h"
#include "Socket.h"
#include "ImageWrapper.h"

#include <QDataStream>

#include <cassert>
#include <iostream>

namespace deflect
{

Stream::Stream()
    : _impl( new StreamPrivate( "", "", Socket::defaultPortNumber ))
{}

Stream::Stream( const std::string& id, const std::string& host,
                const unsigned short port )
    : _impl( new StreamPrivate( id, host, port ))
{}

Stream::~Stream()
{
}

bool Stream::isConnected() const
{
    return _impl->socket.isConnected();
}

const std::string& Stream::getId() const
{
    return _impl->id;
}

const std::string& Stream::getHost() const
{
    return _impl->socket.getHost();
}

bool Stream::send( const ImageWrapper& image )
{
    return _impl->send( image );
}

bool Stream::finishFrame()
{
    return _impl->finishFrame();
}

Stream::Future Stream::asyncSend( const ImageWrapper& image )
{
    return _impl->asyncSend( image );
}

bool Stream::registerForEvents( const bool exclusive )
{
    if( !isConnected( ))
    {
        std::cerr << "deflect::Stream::registerForEvents: Stream is not "
                  << "connected, operation failed" << std::endl;
        return false;
    }

    if( isRegisteredForEvents( ))
        return true;

    const MessageType type = exclusive ? MESSAGE_TYPE_BIND_EVENTS_EX :
                                         MESSAGE_TYPE_BIND_EVENTS;
    MessageHeader mh( type, 0, _impl->id );

    // Send the bind message
    if( !_impl->socket.send( mh, QByteArray( )))
    {
        std::cerr << "deflect::Stream::registerForEvents: sending bind message "
                  << "failed" << std::endl;
        return false;
    }

    // Wait for bind reply
    QByteArray message;
    if( !_impl->socket.receive( mh, message ))
    {
        std::cerr << "deflect::Stream::registerForEvents: receive bind reply "
                  << "failed" << std::endl;
        return false;
    }
    if( mh.type != MESSAGE_TYPE_BIND_EVENTS_REPLY )
    {
        std::cerr << "deflect::Stream::registerForEvents: received unexpected "
                  << "message type (" << int(mh.type) << ")" << std::endl;
        return false;
    }
    _impl->registeredForEvents = *(bool*)(message.data());

    return isRegisteredForEvents();
}

bool Stream::isRegisteredForEvents() const
{
    return _impl->registeredForEvents;
}

int Stream::getDescriptor() const
{
    return _impl->socket.getFileDescriptor();
}

bool Stream::hasEvent() const
{
    return _impl->socket.hasMessage( Event::serializedSize );
}

Event Stream::getEvent()
{
    MessageHeader mh;
    QByteArray message;
    if( !_impl->socket.receive( mh, message ))
    {
        std::cerr << "deflect::Stream::getEvent: receive failed" << std::endl;
        return Event();
    }
    if( mh.type != MESSAGE_TYPE_EVENT )
    {
        std::cerr << "deflect::Stream::getEvent: received unexpected message "
                  << "type (" << int(mh.type) << ")" << std::endl;
        return Event();
    }

    assert( (size_t)message.size() == Event::serializedSize );

    Event event;
    {
        QDataStream stream( message );
        stream >> event;
    }
    return event;
}

void Stream::sendSizeHints( const SizeHints& hints )
{
    _impl->sendSizeHints( hints );
}

void Stream::setDisconnectedCallback( const std::function<void()> callback )
{
    _impl->disconnectedCallback = callback;
}

bool Stream::sendData( const char* data, const size_t count )
{
    return _impl->send( QByteArray::fromRawData( data, count ));
}

}
