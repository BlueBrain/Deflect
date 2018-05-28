/*********************************************************************/
/* Copyright (c) 2013-2018, EPFL/Blue Brain Project                  */
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

namespace
{
const size_t MAX_QUEUE_SIZE = 150; // stream blocked for ~5 seconds at 30Hz
}

namespace deflect
{
namespace server
{
void ReceiveBuffer::addSource(const size_t sourceIndex)
{
    if (_lastFrameComplete > 0)
        throw std::runtime_error("Stream already started; late join forbidden");

    _sourceBuffers.emplace(sourceIndex, SourceBuffer());
}

void ReceiveBuffer::removeSource(const size_t sourceIndex)
{
    _sourceBuffers.erase(sourceIndex);

    // reset for new sources starting with getBackFrameIndex() == 0
    if (_sourceBuffers.empty())
        _lastFrameComplete = 0;
}

size_t ReceiveBuffer::getSourceCount() const
{
    return _sourceBuffers.size();
}

void ReceiveBuffer::insert(const Tile& tile, const size_t sourceIndex)
{
    assert(_sourceBuffers.count(sourceIndex));

    _sourceBuffers[sourceIndex].insert(tile);
}

void ReceiveBuffer::finishFrameForSource(const size_t sourceIndex)
{
    assert(_sourceBuffers.count(sourceIndex));

    auto& buffer = _sourceBuffers[sourceIndex];
    if (buffer.getQueueSize() > MAX_QUEUE_SIZE)
        throw std::runtime_error("maximum queue size exceeded");

    buffer.push();
}

bool ReceiveBuffer::hasCompleteFrame() const
{
    // Check if all sources for Stream have reached the same index
    for (const auto& kv : _sourceBuffers)
    {
        const auto& buffer = kv.second;
        if (buffer.getBackFrameIndex() <= _lastFrameComplete)
            return false;
    }
    return !_sourceBuffers.empty();
}

Tiles ReceiveBuffer::popFrame()
{
    Tiles frame;
    for (auto& kv : _sourceBuffers)
    {
        auto& buffer = kv.second;
        if (buffer.getBackFrameIndex() > _lastFrameComplete)
        {
            const auto& tiles = buffer.getTiles();
            frame.insert(frame.end(), tiles.begin(), tiles.end());
            buffer.pop();
        }
    }
    ++_lastFrameComplete;
    return frame;
}

void ReceiveBuffer::setAllowedToSend(const bool enable)
{
    _allowedToSend = enable;
}

bool ReceiveBuffer::isAllowedToSend() const
{
    return _allowedToSend;
}
}
}
