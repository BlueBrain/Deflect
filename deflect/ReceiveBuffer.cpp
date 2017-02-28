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

#include "ReceiveBuffer.h"

#include <cassert>
#include <set>

namespace
{
const size_t MAX_QUEUE_SIZE = 150; // stream blocked for ~5 seconds at 30Hz
const auto views = std::array<deflect::View, 3>{
    {deflect::View::mono, deflect::View::left_eye, deflect::View::right_eye}};
}

namespace deflect
{
bool ReceiveBuffer::addSource(const size_t sourceIndex)
{
    assert(!_sourceBuffers.count(sourceIndex));

    // TODO: This function must return false if the stream was already started!
    // This requires a full adaptation of the Stream library (DISCL-241)
    if (_sourceBuffers.count(sourceIndex))
        return false;

    _sourceBuffers[sourceIndex] = SourceBuffer();
    return true;
}

void ReceiveBuffer::removeSource(const size_t sourceIndex)
{
    _sourceBuffers.erase(sourceIndex);
}

size_t ReceiveBuffer::getSourceCount() const
{
    return _sourceBuffers.size();
}

void ReceiveBuffer::insert(const Segment& segment, const size_t sourceIndex,
                           const View view)
{
    assert(_sourceBuffers.count(sourceIndex));

    _sourceBuffers[sourceIndex].insert(segment, view);
}

void ReceiveBuffer::finishFrameForSource(const size_t sourceIndex)
{
    assert(_sourceBuffers.count(sourceIndex));

    auto& buffer = _sourceBuffers[sourceIndex];

    for (const auto view : views)
    {
        if (buffer.isBackFrameEmpty(view))
            continue;

        if (buffer.getQueueSize(view) > MAX_QUEUE_SIZE)
            throw std::runtime_error("maximum queue size exceeded");

        buffer.push(view);
    }
}

bool ReceiveBuffer::hasCompleteMonoFrame() const
{
    assert(!_sourceBuffers.empty());

    // Check if all sources for Stream have reached the same index
    const auto lastCompleteFrame = _getLastCompleteFrameIndex(View::mono);
    for (const auto& kv : _sourceBuffers)
    {
        const auto& buffer = kv.second;
        if (buffer.getBackFrameIndex(View::mono) <= lastCompleteFrame)
            return false;
    }
    return true;
}

bool ReceiveBuffer::hasCompleteStereoFrame() const
{
    std::set<size_t> leftSources;
    std::set<size_t> rightSources;

    const auto lastFrameLeft = _getLastCompleteFrameIndex(View::left_eye);
    const auto lastFrameRight = _getLastCompleteFrameIndex(View::right_eye);

    for (const auto& kv : _sourceBuffers)
    {
        const auto& buffer = kv.second;
        if (buffer.getBackFrameIndex(View::left_eye) > lastFrameLeft)
            leftSources.insert(kv.first);
        if (buffer.getBackFrameIndex(View::right_eye) > lastFrameRight)
            rightSources.insert(kv.first);
    }

    if (leftSources.empty() || rightSources.empty())
        return false;

    std::set<size_t> leftAndRight;
    std::set_intersection(leftSources.begin(), leftSources.end(),
                          rightSources.begin(), rightSources.end(),
                          std::inserter(leftAndRight, leftAndRight.end()));

    // if at least one source sends both left AND right, assume all sources do.
    if (!leftAndRight.empty())
        return leftAndRight.size() == _sourceBuffers.size();

    // otherwise, assume all streams send either left OR right.
    return rightSources.size() + leftSources.size() == _sourceBuffers.size();
}

Segments ReceiveBuffer::popFrame(const View view)
{
    const auto lastCompleteFrameIndex = _getLastCompleteFrameIndex(view);

    Segments frame;
    for (auto& kv : _sourceBuffers)
    {
        auto& buffer = kv.second;
        if (buffer.getBackFrameIndex(view) > lastCompleteFrameIndex)
        {
            const auto& segments = buffer.getSegments(view);
            frame.insert(frame.end(), segments.begin(), segments.end());
            buffer.pop(view);
        }
    }
    _incrementLastFrameComplete(view);
    return frame;
}

void ReceiveBuffer::setAllowedToSend(const bool enable, const View view)
{
    _allowedToSend[as_underlying_type(view)] = enable;
}

bool ReceiveBuffer::isAllowedToSend(const View view) const
{
    return _allowedToSend[as_underlying_type(view)];
}

FrameIndex ReceiveBuffer::_getLastCompleteFrameIndex(const View view) const
{
    return _lastFrameComplete[as_underlying_type(view)];
}

void ReceiveBuffer::_incrementLastFrameComplete(const View view)
{
    ++_lastFrameComplete[as_underlying_type(view)];
}
}
