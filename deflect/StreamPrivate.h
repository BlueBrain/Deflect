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

#ifndef DEFLECT_STREAMPRIVATE_H
#define DEFLECT_STREAMPRIVATE_H

#include "ImageSegmenter.h"   // member
#include "Socket.h"           // member
#include "StreamSendWorker.h" // member
#include "TaskBuilder.h"      // member

#include <functional>
#include <string>

namespace deflect
{
/** Private implementation for the Stream class. */
class StreamPrivate
{
public:
    /**
     * Create a new stream and open a new connection to the deflect Server.
     *
     * @param id the unique stream identifier
     * @param host Address of the target Server instance.
     * @param port Port of the target Server instance.
     * @param observer If the stream is used as a pure observer or not.
     * @throw std::runtime_error if the connection to server could not be
     *        established.
     */
    StreamPrivate(const std::string& id, const std::string& host,
                  unsigned short port, bool observer);

    /** Destructor, close the Stream. */
    ~StreamPrivate();

    /** The stream identifier. */
    const std::string id;

    /** The communication socket instance */
    Socket socket;

    /** Has a successful event registration reply been received */
    bool registeredForEvents = false;

    /** Optional callback when the socket is disconnected. */
    std::function<void()> disconnectedCallback;

    /** The worker doing all the socket send operations. */
    StreamSendWorker sendWorker;

    /** Prepare tasks for the sendWorker. */
    TaskBuilder task;

    /** The segmenter for doing multithreaded image segmentation + send. */
    ImageSegmenter _imageSegmenter;

    /** Remember a pending finishFrame where no sendImage() is allowed. */
    std::atomic_bool _pendingFinish{false};

    Stream::Future bindEvents(bool exclusive);
    Stream::Future send(const SizeHints& hints);
    Stream::Future send(QByteArray&& data);
    Stream::Future sendImage(const ImageWrapper& image, bool finish);
    Stream::Future sendFinishFrame();

    /** @internal Called by StreamSendWorker when finishFrame was processed. */
    bool _finishFrameDone();
};
}
#endif
