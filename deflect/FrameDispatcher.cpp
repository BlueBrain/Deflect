/*********************************************************************/
/* Copyright (c) 2013-2017, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
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

#include "FrameDispatcher.h"

#include "Frame.h"
#include "ReceiveBuffer.h"

#include <cassert>
#include <map>

namespace deflect
{
class FrameDispatcher::Impl
{
public:
    Impl() {}
    FramePtr consumeLatestMonoFrame(const QString& uri)
    {
        FramePtr frame(new Frame);
        frame->uri = uri;
        frame->view = View::mono;

        ReceiveBuffer& buffer = streamBuffers[uri];
        while (buffer.hasCompleteMonoFrame())
            frame->segments = buffer.popFrame(View::mono);
        assert(!frame->segments.empty());

        // receiver will request a new frame once this frame was consumed
        buffer.setAllowedToSend(false, View::mono);
        return frame;
    }

    std::pair<FramePtr, FramePtr> consumeLatestStereoFrame(const QString& uri)
    {
        FramePtr frameLeft(new Frame);
        frameLeft->uri = uri;
        frameLeft->view = View::left_eye;

        FramePtr frameRight(new Frame);
        frameRight->uri = uri;
        frameRight->view = View::right_eye;

        ReceiveBuffer& buffer = streamBuffers[uri];

        while (buffer.hasCompleteStereoFrame())
        {
            frameLeft->segments = buffer.popFrame(View::left_eye);
            frameRight->segments = buffer.popFrame(View::right_eye);
        }
        assert(!frameLeft->segments.empty());
        assert(!frameRight->segments.empty());

        // receiver will request a new frame once this frame was consumed
        buffer.setAllowedToSend(false, View::left_eye);
        buffer.setAllowedToSend(false, View::right_eye);
        return std::make_pair(std::move(frameLeft), std::move(frameRight));
    }

    std::map<QString, ReceiveBuffer> streamBuffers;
};

FrameDispatcher::FrameDispatcher()
    : _impl(new Impl)
{
}

FrameDispatcher::~FrameDispatcher()
{
}

void FrameDispatcher::addSource(const QString uri, const size_t sourceIndex)
{
    _impl->streamBuffers[uri].addSource(sourceIndex);

    if (_impl->streamBuffers[uri].getSourceCount() == 1)
        emit pixelStreamOpened(uri);
}

void FrameDispatcher::removeSource(const QString uri, const size_t sourceIndex)
{
    if (!_impl->streamBuffers.count(uri))
        return;

    _impl->streamBuffers[uri].removeSource(sourceIndex);

    if (_impl->streamBuffers[uri].getSourceCount() == 0)
        deleteStream(uri);
}

void FrameDispatcher::processSegment(const QString uri,
                                     const size_t sourceIndex,
                                     deflect::Segment segment,
                                     const deflect::View view)
{
    if (_impl->streamBuffers.count(uri))
        _impl->streamBuffers[uri].insert(segment, sourceIndex, view);
}

void FrameDispatcher::processFrameFinished(const QString uri,
                                           const size_t sourceIndex)
{
    if (!_impl->streamBuffers.count(uri))
        return;

    ReceiveBuffer& buffer = _impl->streamBuffers[uri];
    try
    {
        buffer.finishFrameForSource(sourceIndex);
    }
    catch (const std::runtime_error&)
    {
        emit bufferSizeExceeded(uri);
        return;
    }

    if (buffer.isAllowedToSend(View::mono) && buffer.hasCompleteMonoFrame())
        emit sendFrame(_impl->consumeLatestMonoFrame(uri));

    if (buffer.isAllowedToSend(View::left_eye) &&
        buffer.isAllowedToSend(View::right_eye) &&
        buffer.hasCompleteStereoFrame())
    {
        const auto frames = _impl->consumeLatestStereoFrame(uri);
        emit sendFrame(frames.first);
        emit sendFrame(frames.second);
    }
}

void FrameDispatcher::requestFrame(const QString uri)
{
    if (!_impl->streamBuffers.count(uri))
        return;

    ReceiveBuffer& buffer = _impl->streamBuffers[uri];
    buffer.setAllowedToSend(true, View::mono);
    buffer.setAllowedToSend(true, View::left_eye);
    buffer.setAllowedToSend(true, View::right_eye);

    if (buffer.hasCompleteMonoFrame())
        emit sendFrame(_impl->consumeLatestMonoFrame(uri));

    if (buffer.hasCompleteStereoFrame())
    {
        const auto frames = _impl->consumeLatestStereoFrame(uri);
        emit sendFrame(frames.first);
        emit sendFrame(frames.second);
    }
}

void FrameDispatcher::deleteStream(const QString uri)
{
    if (_impl->streamBuffers.count(uri))
    {
        _impl->streamBuffers.erase(uri);
        emit pixelStreamClosed(uri);
    }
}
}
