/*********************************************************************/
/* Copyright (c) 2013-2014, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
/*                          Stefan.Eilemann@epfl.ch                  */
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
#include "Socket.h"
#include "ImageWrapper.h"
#include "PixelStreamSegment.h"
#include "PixelStreamSegmentParameters.h"

#include <QDataStream>
#include <boost/bind.hpp>

#include <iostream>

namespace deflect
{

Stream::Stream(const std::string& name, const std::string& address)
    : impl_( new StreamPrivate( this, name, address ))
{

}

Stream::~Stream()
{
    delete impl_;
}

bool Stream::isConnected() const
{
    return impl_->socket_.isConnected();
}

bool Stream::send( const ImageWrapper& image )
{
    return impl_->send( image );
}

bool Stream::finishFrame()
{
    return impl_->finishFrame();
}

Stream::Future Stream::asyncSend( const ImageWrapper& image )
{
    return impl_->asyncSend( image );
}

bool Stream::registerForEvents(const bool exclusive)
{
    if(!isConnected())
    {
        std::cerr << "Stream is not connected, registerForEvents failed"
                  << std::endl;
        return false;
    }

    if (isRegisteredForEvents())
        return true;

    MessageType type = exclusive ? MESSAGE_TYPE_BIND_EVENTS_EX :
                                    MESSAGE_TYPE_BIND_EVENTS;
    MessageHeader mh(type, 0, impl_->name_);

    // Send the bind message
    if( !impl_->socket_.send(mh, QByteArray()) )
    {
        std::cerr << "Could not send bind message" << std::endl;
        return false;
    }

    // Wait for bind reply
    QByteArray message;
    bool success = impl_->socket_.receive(mh, message);
    if(!success || mh.type != MESSAGE_TYPE_BIND_EVENTS_REPLY)
    {
        std::cerr << "Invalid reply from host" << std::endl;
        return false;
    }

    impl_->registeredForEvents_= *(bool*)(message.data());

    return isRegisteredForEvents();
}

bool Stream::isRegisteredForEvents() const
{
    return impl_->registeredForEvents_;
}

int Stream::getDescriptor() const
{
    return impl_->socket_.getFileDescriptor();
}

bool Stream::hasEvent() const
{
    QMutexLocker locker( &impl_->sendLock_ );
    return impl_->socket_.hasMessage(Event::serializedSize);
}

Event Stream::getEvent()
{
    MessageHeader mh;
    QByteArray message;

    QMutexLocker locker( &impl_->sendLock_ );
    bool success = impl_->socket_.receive(mh, message);

    if(!success || mh.type != MESSAGE_TYPE_EVENT)
    {
        std::cerr << "Invalid reply from host" << std::endl;
        return Event();
    }

    assert ((size_t)message.size() == Event::serializedSize);

    Event event;
    {
        QDataStream stream(message);
        stream >> event;
    }
    return event;
}

void Stream::sendCommand( const std::string& command )
{
    impl_->sendCommand( QString::fromStdString( command ));
}

}
