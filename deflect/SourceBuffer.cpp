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

#include "SourceBuffer.h"

#include <exception>

namespace deflect
{
SourceBuffer::SourceBuffer()
{
    _getQueue(View::mono).push(Segments());
    _getQueue(View::left_eye).push(Segments());
    _getQueue(View::right_eye).push(Segments());
}

const Segments& SourceBuffer::getSegments(const View view) const
{
    return _getQueue(view).front();
}

FrameIndex SourceBuffer::getBackFrameIndex(const View view) const
{
    return _backFrameIndex[as_underlying_type(view)];
}

bool SourceBuffer::isBackFrameEmpty(const View view) const
{
    return _getQueue(view).back().empty();
}

void SourceBuffer::pop(const View view)
{
    _getQueue(view).pop();
}

void SourceBuffer::push(const View view)
{
    _getQueue(view).push(Segments());
    ++_backFrameIndex[as_underlying_type(view)];
}

void SourceBuffer::insert(const Segment& segment, const View view)
{
    _getQueue(view).back().push_back(segment);
}

size_t SourceBuffer::getQueueSize(const View view) const
{
    return _getQueue(view).size();
}

std::queue<Segments>& SourceBuffer::_getQueue(const View view)
{
    return _segments[as_underlying_type(view)];
}

const std::queue<Segments>& SourceBuffer::_getQueue(const View view) const
{
    return _segments[as_underlying_type(view)];
}
}
