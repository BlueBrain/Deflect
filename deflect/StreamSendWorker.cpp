/*********************************************************************/
/* Copyright (c) 2013-2017, EPFL/Blue Brain Project                  */
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

#include "StreamSendWorker.h"

#include "StreamPrivate.h"

#include <iostream>

namespace
{
const unsigned int SEGMENT_SIZE = 512;
}

namespace deflect
{
StreamSendWorker::StreamSendWorker(StreamPrivate& stream)
    : _stream(stream)
    , _running(true)
    , _thread(std::bind(&StreamSendWorker::_run, this))
{
    _imageSegmenter.setNominalSegmentDimensions(SEGMENT_SIZE, SEGMENT_SIZE);
}

StreamSendWorker::~StreamSendWorker()
{
    stop();
}

void StreamSendWorker::_run()
{
    const auto sendFunc = std::bind(&StreamPrivate::sendPixelStreamSegment,
                                    &_stream, std::placeholders::_1);

    std::unique_lock<std::mutex> lock(_mutex);
    while (true)
    {
        while (_requests.empty() && _running)
            _condition.wait(lock);

        if (!_running)
            break;

        const Request& request = _requests.back();

        if (request.tasks & Request::TASK_IMAGE)
        {
            if (!_stream.sendImageView(request.image.view) ||
                !_imageSegmenter.generate(request.image, sendFunc))
            {
                request.promise->set_value(false);
                _requests.pop_back();
                continue;
            }
        }

        if (request.tasks & Request::TASK_FINISH)
        {
            if (!_stream.sendFinish())
            {
                request.promise->set_value(false);
                _requests.pop_back();
                continue;
            }
        }

        request.promise->set_value(true);
        _requests.pop_back();
    }
}

void StreamSendWorker::stop()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_running)
            return;
        _running = false;
        _condition.notify_all();
    }

    _thread.join();
    while (!_requests.empty())
    {
        _requests.front().promise->set_value(false);
        _requests.pop_front();
    }
}

Stream::Future StreamSendWorker::enqueueImage(const ImageWrapper& image,
                                              const bool finish)
{
    if (image.compressionPolicy != COMPRESSION_ON && image.pixelFormat != RGBA)
    {
        std::cerr << "Currently, RAW images can only be sent in RGBA format. "
                     "Other formats support remain to be implemented."
                  << std::endl;
        std::promise<bool> promise;
        promise.set_value(false);
        return promise.get_future();
    }

    const uint32_t tasks = finish ? Request::TASK_IMAGE | Request::TASK_FINISH
                                  : Request::TASK_IMAGE;
    PromisePtr promise(new Promise);

    std::lock_guard<std::mutex> lock(_mutex);
    _requests.push_back({promise, image, tasks});
    _condition.notify_all();
    return promise->get_future();
}

Stream::Future StreamSendWorker::enqueueFinish()
{
    PromisePtr promise(new Promise);

    std::lock_guard<std::mutex> lock(_mutex);
    _requests.push_back(
        {promise, ImageWrapper(nullptr, 0, 0, RGBA), Request::TASK_FINISH});
    _condition.notify_all();
    return promise->get_future();
}
}
