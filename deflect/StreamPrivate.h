/*********************************************************************/
/* Copyright (c) 2013-2014, EPFL/Blue Brain Project                  */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
/*                     Stefan.Eilemann@epfl.ch                       */
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
#include "MessageHeader.h"
#include "ImageSegmenter.h"
#include "Socket.h" // member
#include "Stream.h" // Stream::Future

#include <QMutex>
#include <string>

class QString;

namespace deflect
{

class StreamSendWorker;

/**
 * Private implementation for the Stream class.
 */
class StreamPrivate : public QObject
{
    Q_OBJECT

public:
    /**
     * Create a new stream and open a new connection to the DisplayCluster.
     *
     * It can be a hostname like "localhost" or an IP in string format,
     * e.g. "192.168.1.83" This method must be called by all Streams sharing a
     * common identifier before any of them starts sending images.
     *
     * @param stream the parent object owning this object
     * @param name the unique stream name
     * @param address Address of the target DisplayCluster instance.
     */
    StreamPrivate( Stream* stream, const std::string& name,
                   const std::string& address );

    /** Destructor, close the Stream. */
    ~StreamPrivate();

    /** The stream identifier. */
    const std::string name_;

    /** The communication socket instance */
    Socket socket_;

    /** The image segmenter */
    ImageSegmenter imageSegmenter_;

    /** Has a successful event registration reply been received */
    bool registeredForEvents_;

    /**
     * Close the stream.
     * @return true if the connection could be terminated or the Stream was not connected, false otherwise
     */
    bool close();

    /** @sa Stream::send */
    bool send( const ImageWrapper& image );

    /** @sa Stream::asyncSend */
    Stream::Future asyncSend(const ImageWrapper& image);

    /** @sa Stream::finishFrame */
    bool finishFrame();

    /**
     * Send an existing PixelStreamSegment via the Socket.
     * @param socket The Socket instance
     * @param segment A pixel stream segement with valid parameters and imageData
     * @param senderName Used to identifiy the sender on the receiver side
     * @return true if the message could be sent
     */
    DEFLECT_API bool sendPixelStreamSegment(const Segment& segment);

    /**
     * Send a command to the wall
     * @param command A command string formatted by the Command class.
     * @return true if the request could be sent, false otherwise.
     */
    bool sendCommand(const QString& command);

    QMutex sendLock_;

private slots:
    void onDisconnected();

private:
    Stream* parent_;
    StreamSendWorker* sendWorker_;
};

}
#endif
