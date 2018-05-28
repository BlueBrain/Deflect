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

#ifndef DEFLECT_SERVER_RECEIVEBUFFER_H
#define DEFLECT_SERVER_RECEIVEBUFFER_H

#include <deflect/api.h>
#include <deflect/server/SourceBuffer.h>

#include <map>
#include <queue>

namespace deflect
{
namespace server
{
/**
 * Buffer Tiles from (multiple) sources.
 *
 * The buffer aggregates tiles coming from different sources and delivers
 * complete frames.
 */
class ReceiveBuffer
{
public:
    /**
     * Add a source of tiles.
     * @param sourceIndex Unique source identifier
     * @throw std::runtime_error if finishFrameForSource() has already been
     *        called for all existing sources (reject late joiners).
     */
    DEFLECT_API void addSource(size_t sourceIndex);

    /**
     * Remove a source of tiles.
     * @param sourceIndex Unique source identifier
     */
    DEFLECT_API void removeSource(size_t sourceIndex);

    /** Get the number of sources for this Stream */
    DEFLECT_API size_t getSourceCount() const;

    /**
     * Insert a tile for the current frame and source.
     * @param tile The tile to insert
     * @param sourceIndex Unique source identifier
     */
    DEFLECT_API void insert(const Tile& tile, size_t sourceIndex);

    /**
     * Call when the source has finished sending tiles for the current frame.
     * @param sourceIndex Unique source identifier
     * @throw std::runtime_error if the buffer exceeds its maximum size
     */
    DEFLECT_API void finishFrameForSource(size_t sourceIndex);

    /** Does the Buffer have a new complete frame (from all sources) */
    DEFLECT_API bool hasCompleteFrame() const;

    /**
     * Get the finished frame.
     * @return A collection of tiles that form a frame
     */
    DEFLECT_API Tiles popFrame();

    /** Allow this buffer to be used by the next
     * FrameDispatcher::sendLatestFrame */
    DEFLECT_API void setAllowedToSend(bool enable);

    /** @return true if this buffer can be sent by FrameDispatcher */
    DEFLECT_API bool isAllowedToSend() const;

private:
    FrameIndex _lastFrameComplete = 0;
    std::map<size_t, SourceBuffer> _sourceBuffers;
    bool _allowedToSend = false;
};
}
}

#endif
