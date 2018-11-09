/*********************************************************************/
/* Copyright (c) 2017-2018, EPFL/Blue Brain Project                  */
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
/*    THIS  SOFTWARE  IS  PROVIDED  BY  THE  ECOLE  POLYTECHNIQUE    */
/*    FEDERALE DE LAUSANNE  ''AS IS''  AND ANY EXPRESS OR IMPLIED    */
/*    WARRANTIES, INCLUDING, BUT  NOT  LIMITED  TO,  THE  IMPLIED    */
/*    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  A PARTICULAR    */
/*    PURPOSE  ARE  DISCLAIMED.  IN  NO  EVENT  SHALL  THE  ECOLE    */
/*    POLYTECHNIQUE  FEDERALE  DE  LAUSANNE  OR  CONTRIBUTORS  BE    */
/*    LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,    */
/*    EXEMPLARY,  OR  CONSEQUENTIAL  DAMAGES  (INCLUDING, BUT NOT    */
/*    LIMITED TO,  PROCUREMENT  OF  SUBSTITUTE GOODS OR SERVICES;    */
/*    LOSS OF USE, DATA, OR  PROFITS;  OR  BUSINESS INTERRUPTION)    */
/*    HOWEVER CAUSED AND  ON ANY THEORY OF LIABILITY,  WHETHER IN    */
/*    CONTRACT, STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE    */
/*    OR OTHERWISE) ARISING  IN ANY WAY  OUT OF  THE USE OF  THIS    */
/*    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.   */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of Ecole polytechnique federale de Lausanne.          */
/*********************************************************************/

#define BOOST_TEST_MODULE FrameDispatcherTests

#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include "FrameUtils.h"

#include <deflect/server/FrameDispatcher.h>

namespace
{
const char* streamId = "test";
const size_t sourceIndex = 54;
}

struct Fixture
{
    void dispatch(const deflect::server::Frame& frame)
    {
        for (auto& tile : frame.tiles)
            dispatcher.processTile(streamId, sourceIndex, tile);
        dispatcher.processFrameFinished(streamId, sourceIndex);
        dispatcher.requestFrame(streamId);
    }

    deflect::server::FrameDispatcher dispatcher;
};

struct FixtureFrame : Fixture
{
    FixtureFrame()
    {
        QObject::connect(&dispatcher,
                         &deflect::server::FrameDispatcher::sendFrame,
                         [this](deflect::server::FramePtr frame) {
                             receivedFrame = frame;
                         });

        dispatcher.addSource(streamId, sourceIndex);
    }
    deflect::server::FramePtr receivedFrame;
};

BOOST_FIXTURE_TEST_CASE(dispatch_multiple_frames, FixtureFrame)
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

BOOST_FIXTURE_TEST_CASE(dispatch_frame_bottom_up, FixtureFrame)
{
    auto frame = makeTestFrame(640, 480, 64);
    for (auto& tile : frame.tiles)
        tile.rowOrder = deflect::RowOrder::bottom_up;

    dispatch(frame);
    BOOST_REQUIRE(receivedFrame);

    // mirror tiles positions vertically
    for (auto& tile : frame.tiles)
        tile.y = 480 - tile.y - tile.height;

    compare(frame, *receivedFrame);
}

BOOST_FIXTURE_TEST_CASE(dispatch_frame_with_inconsistent_row_order,
                        FixtureFrame)
{
    auto frame = makeTestFrame(640, 480, 64);
    frame.tiles[2].rowOrder = deflect::RowOrder::bottom_up;
    dispatch(frame);
    BOOST_CHECK(!receivedFrame);
}

struct FixtureSignals : Fixture
{
    FixtureSignals()
    {
        QObject::connect(&dispatcher,
                         &deflect::server::FrameDispatcher::sourceRejected,
                         [this](QString uri, size_t index) {
                             BOOST_CHECK_EQUAL(uri.toStdString(), streamId);
                             rejectedSourceIndex = index;
                         });
        QObject::connect(&dispatcher,
                         &deflect::server::FrameDispatcher::pixelStreamOpened,
                         [this](QString uri) { openedSourceUri = uri; });
        QObject::connect(&dispatcher,
                         &deflect::server::FrameDispatcher::pixelStreamClosed,
                         [this](QString uri) { closedSourceUri = uri; });
        QObject::connect(&dispatcher,
                         &deflect::server::FrameDispatcher::pixelStreamWarning,
                         [this](QString uri, QString what) {
                             BOOST_CHECK_EQUAL(uri.toStdString(), streamId);
                             warning = what;
                         });
        QObject::connect(&dispatcher,
                         &deflect::server::FrameDispatcher::pixelStreamError,
                         [this](QString uri, QString what) {
                             BOOST_CHECK_EQUAL(uri.toStdString(), streamId);
                             error = what;
                         });
    }

    QString openedSourceUri;
    size_t rejectedSourceIndex = 0;
    QString closedSourceUri;
    QString warning;
    QString error;
};

BOOST_FIXTURE_TEST_CASE(add_remove_sources, FixtureSignals)
{
    dispatcher.addSource(streamId, sourceIndex);
    BOOST_CHECK_EQUAL(openedSourceUri.toStdString(), streamId);

    dispatcher.addSource("other", sourceIndex);
    BOOST_CHECK_EQUAL(openedSourceUri.toStdString(), "other");

    dispatcher.removeSource(streamId, sourceIndex);
    BOOST_CHECK_EQUAL(closedSourceUri.toStdString(), streamId);

    dispatcher.deleteStream("other");
    BOOST_CHECK_EQUAL(closedSourceUri.toStdString(), "other");
}

BOOST_FIXTURE_TEST_CASE(late_join_rejected, FixtureSignals)
{
    dispatcher.addSource(streamId, sourceIndex);
    dispatch(makeTestFrame(640, 480, 64));

    dispatcher.addSource(streamId, 8697);
    BOOST_CHECK_EQUAL(rejectedSourceIndex, 8697);
    BOOST_CHECK(!warning.isEmpty());
}

BOOST_FIXTURE_TEST_CASE(max_queue_size_exceeded_when_one_stream_idle,
                        FixtureSignals)
{
    dispatcher.addSource(streamId, sourceIndex);
    dispatcher.addSource(streamId, 8697);

    const auto frame = makeTestFrame(128, 128, 64);
    for (auto i = 0; i < 150; ++i)
        dispatch(frame);

    BOOST_CHECK(error.isEmpty());

    dispatch(frame);

    BOOST_CHECK(!error.isEmpty());
}
