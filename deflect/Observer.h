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

#ifndef DEFLECT_OBSERVER_H
#define DEFLECT_OBSERVER_H

#include <deflect/Event.h>
#include <deflect/api.h>
#include <deflect/types.h>

#include <functional>
#include <memory>
#include <string>

namespace deflect
{
class StreamPrivate;

/**
 */
class Observer
{
public:
    /**
     * Open a new connection to the Server using environment variables.
     *
     * DEFLECT_HOST  The address of the target Server instance (required).
     * DEFLECT_ID    The identifier for the stream. If not provided, a random
     *               unique identifier will be used.
     * @throw std::runtime_error if DEFLECT_HOST was not provided.
     * @version 1.3
     */
    DEFLECT_API Observer();

    /**
     * Open a new connection to the Server.
     *
     * The user can check if the connection was successfully established with
     * isConnected().
     *
     * Different Streams can contribute to a single window by using the same
     * identifier. All the Streams which contribute to the same window should be
     * created before any of them starts sending images.
     *
     * @param id The identifier for the stream. If left empty, the environment
     *           variable DEFLECT_ID will be used. If both values are empty,
     *           a random unique identifier will be used.
     * @param host The address of the target Server instance. It can be a
     *             hostname like "localhost" or an IP in string format like
     *             "192.168.1.83". If left empty, the environment variable
     *             DEFLECT_HOST will be used instead.
     * @param port Port of the Server instance, default 1701.
     * @throw std::runtime_error if no host was provided.
     * @version 1.0
     */
    DEFLECT_API Observer(const std::string& id, const std::string& host,
                         unsigned short port = 1701);

    /** Destruct the Stream, closing the connection. @version 1.0 */
    DEFLECT_API virtual ~Observer();

    /** @return true if the stream is connected, false otherwise. @version 1.0*/
    DEFLECT_API bool isConnected() const;

    /** @return the identifier defined by the constructor. @version 1.3 */
    DEFLECT_API const std::string& getId() const;

    /** @return the host defined by the constructor. @version 1.3 */
    DEFLECT_API const std::string& getHost() const;

    /**
     * Register to receive Events.
     *
     * After registering, the Server application will send Events whenever a
     * user is interacting with this Stream's window.
     *
     * Events can be retrieved using hasEvent() and getEvent().
     *
     * The current registration status can be checked with
     * isRegisteredForEvents().
     *
     * This method is synchronous and waits for a registration reply from the
     * Server before returning.
     *
     * @param exclusive Binds only one stream source for the same identifier.
     * @return true if the registration could be or was already established.
     * @version 1.0
     */
    DEFLECT_API bool registerForEvents(bool exclusive = false);

    /**
     * Is this stream registered to receive events.
     *
     * Check if the stream has already successfully registered with
     * registerForEvents().
     *
     * @return true after the Server application has acknowledged the
     *         registration request, false otherwise
     * @version 1.0
     */
    DEFLECT_API bool isRegisteredForEvents() const;

    /**
     * Get the native descriptor for the data stream.
     *
     * This descriptor can for instance be used by poll() on UNIX systems.
     * Having this descriptor lets a Stream class user detect when the Stream
     * has received any data. The user can the use query the state of the
     * Stream, for example using hasEvent(), and process the events accordingly.
     *
     * @return The native descriptor if available; otherwise returns -1.
     * @version 1.0
     */
    DEFLECT_API int getDescriptor() const;

    /**
     * Check if a new Event is available.
     *
     * This method is non-blocking. Use this method prior to calling getEvent(),
     * for example as the condition for a while() loop to process all pending
     * events.
     *
     * @return True if an Event is available, false otherwise
     * @version 1.0
     */
    DEFLECT_API bool hasEvent() const;

    /**
     * Get the next Event.
     *
     * This method is synchronous and waits until an Event is available before
     * returning (or a 1 second timeout occurs).
     *
     * Check if an Event is available with hasEvent() before calling this
     * method.
     *
     * @return The next Event if available, otherwise an empty (default) Event.
     * @version 1.0
     */
    DEFLECT_API Event getEvent();

    /**
     * Set a function to be be called just after the stream gets disconnected.
     *
     * @param callback the function to call
     * @note replaces the previous disconnected signal
     * @version 1.5
     */
    DEFLECT_API void setDisconnectedCallback(std::function<void()> callback);

protected:
    Observer(const Stream&) = delete;
    const Observer& operator=(const Observer&) = delete;

    std::unique_ptr<StreamPrivate> _impl;

    Observer(StreamPrivate* impl);
};
}

#endif
