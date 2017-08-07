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
 * Connect to a deflect::Server and register for events.
 *
 * In case a dedicated event handling w/o the need for streaming images is
 * required, the Observer class can be used, in contrast to the deflect::Stream.
 *
 * On the server side the observer also opens and closes the stream as regular
 * deflect::Streams would do.
 *
 * This class is new since version 1.7 while sharing the same API as
 * deflect::Stream prior 1.7.
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
     * Different observers and streams can share the same window by using the
     * same identifier.
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

    /** Destruct the Observer, closing the connection. @version 1.0 */
    DEFLECT_API virtual ~Observer();

    /**
     * @return true if the observer is connected, false otherwise.
     * @version 1.0
     */
    DEFLECT_API bool isConnected() const;

    /** @return the identifier defined by the constructor. @version 1.3 */
    DEFLECT_API const std::string& getId() const;

    /** @return the host defined by the constructor. @version 1.3 */
    DEFLECT_API const std::string& getHost() const;

    /**
     * Register to receive Events.
     *
     * After registering, the Server application will send Events whenever a
     * user is interacting with this Observers's window.
     *
     * Events can be retrieved using hasEvent() and getEvent().
     *
     * The current registration status can be checked with
     * isRegisteredForEvents().
     *
     * This method is synchronous and waits for a registration reply from the
     * Server before returning.
     *
     * @param exclusive Binds only one observer source for the same identifier.
     * @return true if the registration could be or was already established.
     * @version 1.0
     */
    DEFLECT_API bool registerForEvents(bool exclusive = false);

    /**
     * Is this observer registered to receive events.
     *
     * Check if the observer has already successfully registered with
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
     * Having this descriptor lets a Observer class user detect when the Stream
     * has received any data. The user can the use query the state of the
     * Observer, for example using hasEvent(), and process the events
     * accordingly.
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
     * Set a function to be be called just after the observer gets disconnected.
     *
     * @param callback the function to call
     * @note replaces the previous disconnected signal
     * @version 1.5
     */
    DEFLECT_API void setDisconnectedCallback(std::function<void()> callback);

    /**
     * Send size hints to the stream server to indicate sizes that should be
     * respected by resize operations on the server side.
     *
     * @note blocks until all pending asynchonous send operations are finished.
     * @param hints the new size hints for the server
     * @version 1.2
     */
    DEFLECT_API void sendSizeHints(const SizeHints& hints);

    /**
     * Send data to the Server.
     *
     * @note blocks until all pending asynchonous send operations are finished.
     * @param data the pointer to the data buffer.
     * @param count the number of bytes to send.
     * @return true if the data could be sent, false otherwise
     * @version 1.3
     */
    DEFLECT_API bool sendData(const char* data, size_t count);

protected:
    Observer(const Observer&) = delete;
    const Observer& operator=(const Observer&) = delete;

    std::unique_ptr<StreamPrivate> _impl;

    Observer(StreamPrivate* impl);
};
}

#endif
