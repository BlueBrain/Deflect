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

#ifndef DEFLECT_SOURCEBUFFER_H
#define DEFLECT_SOURCEBUFFER_H

#include <deflect/Segment.h>
#include <deflect/api.h>
#include <deflect/types.h>

#include <array>
#include <queue>

namespace deflect
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

    /** @return the segments at the front of the queue for a given view. */
    const Segments& getSegments(View view) const;

    /** @return the frame index of the back of the buffer for a given view. */
    FrameIndex getBackFrameIndex(View view) const;

    /** @return true if the back frame of the given view has no segments. */
    bool isBackFrameEmpty(View view) const;

    /** Insert a segment into the back frame of the appropriate queue. */
    void insert(const Segment& segment, View view);

    /** Push a new frame to the back of given view. */
    void push(View view);

    /** Pop the front frame of the buffer for the given view. */
    void pop(View view);

    /** @return the size of the queue for the given view. */
    size_t getQueueSize(View view) const;

private:
    /** The collections of segments for each mono/left/right view. */
    std::queue<Segments> _segments[3];

    /** The current indices of the mono/left/right frame for this source. */
    std::array<FrameIndex, 3> _backFrameIndex = {{0u, 0u, 0u}};

    std::queue<Segments>& _getQueue(View view);
    const std::queue<Segments>& _getQueue(View view) const;
};
}

#endif
