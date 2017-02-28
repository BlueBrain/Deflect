/*********************************************************************/
/* Copyright (c) 2013-2015, EPFL/Blue Brain Project                  */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
/*                     Stefan.Eilemann@epfl.ch                       */
/*                     Daniel.Nachbaur@epfl.ch                       */
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

#ifndef DEFLECT_STREAMPRIVATE_H
#define DEFLECT_STREAMPRIVATE_H

#include <deflect/api.h>

#include "Event.h"
#include "ImageSegmenter.h"
#include "MessageHeader.h"
#include "Socket.h" // member
#include "Stream.h" // Stream::Future

#include <functional>
#include <memory>
#include <string>

namespace deflect
{
class StreamSendWorker;

/**
 * Private implementation for the Stream class.
 */
class StreamPrivate
{
public:
    /**
     * Create a new stream and open a new connection to the deflect::Server.
     *
     * @param id the unique stream identifier
     * @param host Address of the target Server instance.
     * @param port Port of the target Server instance.
     */
    StreamPrivate(const std::string& id, const std::string& host,
                  unsigned short port);

    /** Destructor, close the Stream. */
    ~StreamPrivate();

    /** Send the open message to the server. */
    void sendOpen();

    /** Send the quit message to the server. */
    void sendClose();

    /**
     * Close the stream.
     * @return true on success or if the Stream was not connected
     */
    bool close();

    /** @sa Stream::send */
    bool send(const ImageWrapper& image);

    /** @sa Stream::asyncSend */
    Stream::Future asyncSend(const ImageWrapper& image);

    /** @sa Stream::finishFrame */
    bool finishFrame();

    /** Send the view for the image to be sent with sendPixelStreamSegment. */
    bool sendImageView(View view);

    /**
     * Send a Segment through the Stream.
     * @param segment An image segment with valid parameters and data
     * @return true if the message could be sent
     */
    DEFLECT_API bool sendPixelStreamSegment(const Segment& segment);

    /** @sa Stream::sendSizeHints */
    bool sendSizeHints(const SizeHints& hints);

    /** Send a user-defined block of data to the server. */
    bool send(QByteArray data);

    /** The stream identifier. */
    const std::string id;

    /** The communication socket instance */
    Socket socket;

    /** The image segmenter */
    ImageSegmenter imageSegmenter;

    /** Has a successful event registration reply been received */
    bool registeredForEvents;

    std::function<void()> disconnectedCallback;

private:
    std::unique_ptr<StreamSendWorker> _sendWorker;
};
}
#endif
