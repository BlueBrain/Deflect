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

#ifndef DEFLECT_SERVER_SOURCEBUFFER_H
#define DEFLECT_SERVER_SOURCEBUFFER_H

#include <deflect/Segment.h>
#include <deflect/api.h>
#include <deflect/types.h>

#include <array>
#include <queue>

namespace deflect
{
namespace server
{
using FrameIndex = unsigned int;

/**
 * Buffer for a single source of segments.
 */
class SourceBuffer
{
public:
    /** Construct an empty buffer. */
    SourceBuffer();

    /** @return the segments at the front of the queue. */
    const Segments& getSegments() const;

    /** @return the frame index of the back of the buffer. */
    FrameIndex getBackFrameIndex() const;

    /** @return true if the back frame has no segments. */
    bool isBackFrameEmpty() const;

    /** Insert a segment into the back frame. */
    void insert(const Segment& segment);

    /** Push a new frame to the back. */
    void push();

    /** Pop the front frame. */
    void pop();

    /** @return the size of the queue. */
    size_t getQueueSize() const;

private:
    /** The collections of segments for each mono/left/right view. */
    std::queue<Segments> _segments;

    /** The current indices of the mono/left/right frame for this source. */
    FrameIndex _backFrameIndex = 0u;
};
}
}

#endif
