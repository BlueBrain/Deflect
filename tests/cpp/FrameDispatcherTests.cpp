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
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#define BOOST_TEST_MODULE FrameDispatcherTests

#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include "FrameUtils.h"

#include <deflect/server/FrameDispatcher.h>

namespace
{
const char* streamId = "test";
}

struct Fixture
{
    deflect::server::FrameDispatcher dispatcher;
    deflect::server::FramePtr receivedFrame;
    Fixture()
    {
        QObject::connect(&dispatcher,
                         &deflect::server::FrameDispatcher::sendFrame,
                         [&](deflect::server::FramePtr f) {
                             receivedFrame = f;
                         });

        dispatcher.addSource(streamId, 0);
    }

    void dispatch(const deflect::server::Frame& frame)
    {
        for (auto& segment : frame.segments)
            dispatcher.processSegment(streamId, 0, segment);
        dispatcher.processFrameFinished(streamId, 0);
        dispatcher.requestFrame(streamId);
    }
};

BOOST_FIXTURE_TEST_CASE(dispatch_multiple_frames, Fixture)
{
    auto frame = makeTestFrame(640, 480, 64);

    dispatch(frame);
    BOOST_REQUIRE(receivedFrame);
    compare(frame, *receivedFrame);

    receivedFrame = nullptr;
    dispatch(frame);
    BOOST_REQUIRE(receivedFrame);
    compare(frame, *receivedFrame);

    receivedFrame = nullptr;
    dispatch(frame);
    BOOST_REQUIRE(receivedFrame);
    compare(frame, *receivedFrame);
}

BOOST_FIXTURE_TEST_CASE(dispatch_frame_bottom_up, Fixture)
{
    auto frame = makeTestFrame(640, 480, 64);
    for (auto& segment : frame.segments)
        segment.rowOrder = deflect::RowOrder::bottom_up;

    dispatch(frame);
    BOOST_REQUIRE(receivedFrame);

    // mirror segments positions vertically
    for (auto& s : frame.segments)
        s.parameters.y = 480 - s.parameters.y - s.parameters.height;

    compare(frame, *receivedFrame);
}

BOOST_FIXTURE_TEST_CASE(dispatch_frame_with_inconsistent_row_order, Fixture)
{
    auto frame = makeTestFrame(640, 480, 64);
    frame.segments[2].rowOrder = deflect::RowOrder::bottom_up;
    dispatch(frame);
    BOOST_CHECK(!receivedFrame);
}
