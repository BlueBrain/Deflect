/*********************************************************************/
/* Copyright (c) 2013-2017, EPFL/Blue Brain Project                  */
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

#include "Observer.h"
#include "NetworkProtocol.h"
#include "StreamPrivate.h"

#include <QDataStream>

#include <cassert>
#include <iostream>

namespace deflect
{
const unsigned short Observer::defaultPortNumber = DEFAULT_PORT_NUMBER;

Observer::Observer()
    : _impl(new StreamPrivate("", "", 0, true))
{
}

Observer::Observer(const unsigned short port)
    : _impl(new StreamPrivate("", "", port, true))
{
}

Observer::Observer(const std::string& id, const std::string& host,
                   const unsigned short port)
    : _impl(new StreamPrivate(id, host, port, true))
{
}

Observer::Observer(StreamPrivate* impl)
    : _impl(impl)
{
}

Observer::~Observer()
{
}

bool Observer::isConnected() const
{
    return _impl->socket.isConnected();
}

const std::string& Observer::getId() const
{
    return _impl->id;
}

const std::string& Observer::getHost() const
{
    return _impl->socket.getHost();
}

unsigned short Observer::getPort() const
{
    return _impl->socket.getPort();
}

bool Observer::registerForEvents(const bool exclusive)
{
    if (!isConnected())
    {
        std::cerr << "deflect::Stream::registerForEvents: Stream is not "
                  << "connected, operation failed" << std::endl;
        return false;
    }

    if (isRegisteredForEvents())
        return true;

    // Send the bind message
    if (!_impl->bindEvents(exclusive).get())
    {
        std::cerr << "deflect::Stream::registerForEvents: sending bind message "
                  << "failed" << std::endl;
        return false;
    }

    // Wait for bind reply
    MessageHeader mh;
    QByteArray message;
    if (!_impl->socket.receive(mh, message))
    {
        std::cerr << "deflect::Stream::registerForEvents: receive bind reply "
                  << "failed" << std::endl;
        return false;
    }
    if (mh.type != MESSAGE_TYPE_BIND_EVENTS_REPLY)
    {
        std::cerr << "deflect::Stream::registerForEvents: received unexpected "
                  << "message type (" << int(mh.type) << ")" << std::endl;
        return false;
    }
    _impl->registeredForEvents = *(bool*)(message.data());

    return isRegisteredForEvents();
}

bool Observer::isRegisteredForEvents() const
{
    return _impl->registeredForEvents;
}

int Observer::getDescriptor() const
{
    return _impl->socket.getFileDescriptor();
}

bool Observer::hasEvent() const
{
    return _impl->socket.hasMessage(Event::serializedSize);
}

Event Observer::getEvent()
{
    MessageHeader mh;
    QByteArray message;
    if (!_impl->socket.receive(mh, message))
    {
        std::cerr << "deflect::Stream::getEvent: receive failed" << std::endl;
        return Event();
    }
    if (mh.type != MESSAGE_TYPE_EVENT)
    {
        std::cerr << "deflect::Stream::getEvent: received unexpected message "
                  << "type (" << int(mh.type) << ")" << std::endl;
        return Event();
    }

    assert((size_t)message.size() == Event::serializedSize);

    Event event;
    {
        QDataStream stream(message);
        stream >> event;
    }
    return event;
}

void Observer::setDisconnectedCallback(const std::function<void()> callback)
{
    _impl->disconnectedCallback = callback;
}

void Observer::sendSizeHints(const SizeHints& hints)
{
    _impl->send(hints);
}

bool Observer::sendData(const char* data, const size_t count)
{
    return _impl->send(QByteArray::fromRawData(data, int(count))).get();
}
}
