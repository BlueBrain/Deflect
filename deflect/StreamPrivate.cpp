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

#include "StreamPrivate.h"

#include <QHostInfo>

#include <sstream>
#include <stdexcept>

namespace
{
const char* STREAM_ID_ENV_VAR = "DEFLECT_ID";
const char* STREAM_HOST_ENV_VAR = "DEFLECT_HOST";

const unsigned int SEGMENT_SIZE = 512;
const unsigned int SMALL_IMAGE_SIZE = 64;

std::string _getStreamHost(const std::string& host)
{
    if (!host.empty())
        return host;

    const QString streamHost = qgetenv(STREAM_HOST_ENV_VAR).constData();
    if (!streamHost.isEmpty())
        return streamHost.toStdString();

    throw std::runtime_error("No host provided");
}

std::string _getStreamId(const std::string& id)
{
    if (!id.empty())
        return id;

    const QString streamId = qgetenv(STREAM_ID_ENV_VAR).constData();
    if (!streamId.isEmpty())
        return streamId.toStdString();

    return QString("%1_%2")
        .arg(QHostInfo::localHostName(), QString::number(rand(), 16))
        .toStdString();
}
}

namespace deflect
{
namespace
{
void _checkParameters(const ImageWrapper& image)
{
    if (image.compressionPolicy != COMPRESSION_ON && image.pixelFormat != RGBA)
    {
        throw std::invalid_argument(
            "Currently, RAW images can only be sent in RGBA format. Other "
            "formats support remain to be implemented.");
    }

    if (image.compressionPolicy == COMPRESSION_ON)
    {
        if (image.compressionQuality < 1 || image.compressionQuality > 100)
        {
            std::stringstream msg;
            msg << "JPEG compression quality must be between 1 and 100, got "
                << image.compressionQuality << std::endl;
            throw std::invalid_argument(msg.str());
        }
    }
}

bool _canSendAsSingleSegment(const ImageWrapper& image)
{
    return image.width <= SMALL_IMAGE_SIZE && image.height <= SMALL_IMAGE_SIZE;
}
}

StreamPrivate::StreamPrivate(const std::string& id_, const std::string& host,
                             const unsigned short port, const bool observer)
    : id{_getStreamId(id_)}
    , socket{_getStreamHost(host), port}
    , sendWorker{socket, id}
    , task{&sendWorker}
{
    _imageSegmenter.setNominalSegmentDimensions(SEGMENT_SIZE, SEGMENT_SIZE);

    socket.connect(&socket, &Socket::disconnected, [this]() {
        if (disconnectedCallback)
            disconnectedCallback();
    });

    socket.moveToThread(&sendWorker);
    sendWorker.start();

    if (observer)
        sendWorker.enqueueRequest(task.openObserver()).wait();
    else
        sendWorker.enqueueRequest(task.openStream()).wait();
}

StreamPrivate::~StreamPrivate()
{
    if (socket.isConnected())
        sendWorker.enqueueRequest(task.close()).wait();
}

Stream::Future StreamPrivate::bindEvents(const bool exclusive)
{
    return sendWorker.enqueueRequest(task.bindEvents(exclusive));
}

Stream::Future StreamPrivate::send(const SizeHints& hints)
{
    return sendWorker.enqueueRequest(task.send(hints));
}

Stream::Future StreamPrivate::send(QByteArray&& data)
{
    return sendWorker.enqueueRequest(task.send(std::move(data)));
}

Stream::Future StreamPrivate::sendImage(const ImageWrapper& image,
                                        const bool finish)
{
    try
    {
        if (!sendWorker.canAcceptNewImageSend())
            throw std::runtime_error("Pending finish, no send allowed");

        _checkParameters(image);

        if (_canSendAsSingleSegment(image))
        {
            // OPT for OSPRay-KNL with external thread pool - compress directly
            // in caller thread.
            auto segment = _imageSegmenter.createSingleSegment(image);
            // As we expect to encounter a lot of these small sends, be
            // optimistic and fulfill the promise already to reduce load in the
            // send thread (c.f. lock ops performance on KNL).
            sendWorker.enqueueFastRequest(task.send(std::move(segment)));
            return finish ? sendFinishFrame() : make_ready_future(true);
        }

        return sendWorker.enqueueRequest(
            task.sendUsingMTCompression(image, _imageSegmenter, finish));
    }
    catch (...)
    {
        return make_exception_future<bool>(std::current_exception());
    }
}

Stream::Future StreamPrivate::sendFinishFrame()
{
    return sendWorker.enqueueRequest(task.finishFrame(), true);
}
}
