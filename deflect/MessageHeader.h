/*********************************************************************/
/* Copyright (c) 2013-2016, EPFL/Blue Brain Project                  */
/*                          Raphael.Dumusc@epfl.ch                   */
/*                          Daniel.Nachbaur@epfl.ch                  */
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

#ifndef DEFLECT_MESSAGE_HEADER_H
#define DEFLECT_MESSAGE_HEADER_H

#include <deflect/api.h>

#ifdef _WIN32
    typedef unsigned __int32 uint32_t;
#else
    #include <stdint.h>
#endif

#include <string>

class QDataStream;

namespace deflect
{

/** The message types. */
enum MessageType
{
    MESSAGE_TYPE_NONE = 0,
    MESSAGE_TYPE_PIXELSTREAM_OPEN = 3,
    MESSAGE_TYPE_PIXELSTREAM_FINISH_FRAME = 4,
    MESSAGE_TYPE_PIXELSTREAM = 5,
    MESSAGE_TYPE_BIND_EVENTS = 6,
    MESSAGE_TYPE_BIND_EVENTS_EX = 7,
    MESSAGE_TYPE_BIND_EVENTS_REPLY = 8,
    MESSAGE_TYPE_EVENT = 9,
    MESSAGE_TYPE_QUIT = 12,
    MESSAGE_TYPE_SIZE_HINTS = 13,
    MESSAGE_TYPE_DATA = 14
};

#define MESSAGE_HEADER_URI_LENGTH 64

/** Fixed-size message header. */
struct MessageHeader
{
    /** Message type. */
    MessageType type;

    /** Size of the message payload. */
    uint32_t size;

    /**
     * Optional URI related to message.
     * @note Needs to be of fixed size so that sizeof(MessageHeader) is constant
     */
    char uri[MESSAGE_HEADER_URI_LENGTH];

    /** Construct a default message header */
    DEFLECT_API MessageHeader();

    /** Construct a message header with a uri */
    DEFLECT_API MessageHeader( const MessageType type, const uint32_t size,
                               const std::string& streamUri = "" );

    /** The size of the QDataStream serialized output. */
    static const size_t serializedSize;
};

}

/**
 * Serialization for network, where sizeof(MessageHeader) can differ between
 * compilers.
 */
DEFLECT_API QDataStream& operator<<( QDataStream& out,
                                     const deflect::MessageHeader& header );
DEFLECT_API QDataStream& operator>>( QDataStream& in,
                                     deflect::MessageHeader& header );

#endif
