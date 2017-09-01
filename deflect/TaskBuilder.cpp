/*********************************************************************/
/* Copyright (c) 2017, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
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
/* or implied, of Ecole polytechnique federale de Lausanne.          */
/*********************************************************************/

#include "TaskBuilder.h"

#include "ImageSegmenter.h"
#include "SizeHints.h"

namespace deflect
{
TaskBuilder::TaskBuilder(StreamSendWorker* worker)
    : _worker{worker}
{
}

Task TaskBuilder::openStream()
{
    return std::bind(&StreamSendWorker::_sendOpenStream, _worker);
}

Task TaskBuilder::close()
{
    return std::bind(&StreamSendWorker::_sendClose, _worker);
}

Task TaskBuilder::openObserver()
{
    return std::bind(&StreamSendWorker::_sendOpenObserver, _worker);
}

Task TaskBuilder::bindEvents(const bool exclusive)
{
    return std::bind(&StreamSendWorker::_sendBindEvents, _worker, exclusive);
}

Task TaskBuilder::send(const SizeHints& hints)
{
    return std::bind(&StreamSendWorker::_sendSizeHints, _worker, hints);
}

Task TaskBuilder::send(const QByteArray& data)
{
    return std::bind(&StreamSendWorker::_sendData, _worker, data);
}

std::vector<Task> TaskBuilder::sendUsingMTCompression(
    const ImageWrapper& image, ImageSegmenter& imageSegmenter,
    const bool finish)
{
    std::vector<Task> tasks;
    tasks.emplace_back(send(image, imageSegmenter));
    if (finish)
        tasks.emplace_back(finishFrame());
    return tasks;
}

Task TaskBuilder::finishFrame()
{
    return std::bind(&StreamSendWorker::_sendFinish, _worker);
}

Task TaskBuilder::send(Segment&& segment)
{
    return std::bind(&StreamSendWorker::_sendSegment, _worker, segment);
}

Task TaskBuilder::send(const ImageWrapper& image,
                       ImageSegmenter& imageSegmenter)
{
    auto sendFunc = std::bind(&StreamSendWorker::_sendSegment, _worker,
                              std::placeholders::_1);
    return [&imageSegmenter, image, sendFunc]() {
        return imageSegmenter.generate(image, sendFunc);
    };
}
}
