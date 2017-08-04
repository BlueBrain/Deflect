/*********************************************************************/
/* Copyright (c) 2014-2017, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
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

#ifndef DEFLECT_TYPES_H
#define DEFLECT_TYPES_H

#include <deflect/config.h>

#include <future>
#include <memory>
#include <vector>

namespace deflect
{
/** The different types of view. */
enum class View : std::uint8_t
{
    mono,
    left_eye,
    right_eye,
    side_by_side
};

/** Sub-sampling of the image chrominance components in YCbCr color space. */
enum class ChromaSubsampling
{
    YUV444, /**< No sub-sampling */
    YUV422, /**< 50% vertical sub-sampling */
    YUV420  /**< 50% vertical + horizontal sub-sampling */
};

/** Cast an enum class value to its underlying type. */
template <typename E>
constexpr typename std::underlying_type<E>::type as_underlying_type(E e)
{
    return static_cast<typename std::underlying_type<E>::type>(e);
}

template <typename T>
std::future<T> make_ready_future(T&& value)
{
    std::promise<T> promise;
    promise.set_value(std::forward<T>(value));
    return promise.get_future();
}

class EventReceiver;
class Frame;
class FrameDispatcher;
class SegmentDecoder;
class Server;
class Stream;

struct Event;
struct ImageWrapper;
struct MessageHeader;
struct Segment;
struct SegmentParameters;
struct SizeHints;

using BoolPromisePtr = std::shared_ptr<std::promise<bool>>;
using FramePtr = std::shared_ptr<Frame>;
using Segments = std::vector<Segment>;
using SegmentParametersList = std::vector<SegmentParameters>;

namespace qt
{
class QuickRenderer;
class QmlStreamer;
class TouchInjector;
}

/** @internal */
namespace test
{
class Application;
}
}

#endif
