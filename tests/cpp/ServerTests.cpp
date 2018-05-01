/*********************************************************************/
/* Copyright (c) 2015-2017, EPFL/Blue Brain Project                  */
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

#define BOOST_TEST_MODULE Server
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include "DeflectServer.h"
#include "MinimalGlobalQtApp.h"
#include "boost_test_thread_safe.h"

#include <deflect/Stream.h>
#include <deflect/server/Frame.h>

#include <boost/mpl/vector.hpp>
#include <cmath>

namespace
{
const QString testStreamId("teststream");
}

BOOST_GLOBAL_FIXTURE(MinimalGlobalQtApp);

BOOST_FIXTURE_TEST_SUITE(server, DeflectServer)

BOOST_AUTO_TEST_CASE(sizeHintsReceivedByServer)
{
    deflect::SizeHints testHints;
    testHints.maxWidth = 500;
    testHints.preferredHeight = 200;

    bool received = false;
    setSizeHintsCallback([&](const QString id, const deflect::SizeHints hints) {
        SAFE_BOOST_CHECK_EQUAL(id.toStdString(), testStreamId.toStdString());
        SAFE_BOOST_CHECK(hints == testHints);
        received = true;
    });

    {
        deflect::Stream stream(testStreamId.toStdString(), "localhost",
                               serverPort());
        BOOST_CHECK(stream.isConnected());
        stream.sendSizeHints(testHints);
        waitForMessage();
    }

    waitForMessage();
    SAFE_BOOST_CHECK_EQUAL(getOpenedStreams(), 0);
    SAFE_BOOST_CHECK(received);
}

BOOST_AUTO_TEST_CASE(registerForEventReceivedByServer)
{
    bool received = false;
    setRegisterToEventsCallback(
        [&](const QString id, const bool exclusive,
            deflect::server::EventReceiver* eventReceiver) {
            SAFE_BOOST_CHECK_EQUAL(id.toStdString(),
                                   testStreamId.toStdString());
            SAFE_BOOST_CHECK(exclusive);
            SAFE_BOOST_CHECK(eventReceiver);
            received = true;
        });

    {
        deflect::Stream stream(testStreamId.toStdString(), "localhost",
                               serverPort());
        BOOST_REQUIRE(stream.isConnected());
        BOOST_CHECK(stream.registerForEvents(true));
        waitForMessage();
    }

    waitForMessage();
    SAFE_BOOST_CHECK_EQUAL(getOpenedStreams(), 0);
    SAFE_BOOST_CHECK(received);
}

BOOST_AUTO_TEST_CASE(dataReceivedByServer)
{
    const auto sentData = std::string{"Hello World!"};

    bool received = false;
    setDataReceivedCallback([&](const QString id, QByteArray data) {
        SAFE_BOOST_CHECK_EQUAL(id.toStdString(), testStreamId.toStdString());
        SAFE_BOOST_CHECK_EQUAL(std::string(data.constData()), sentData);
        received = true;
    });

    {
        deflect::Stream stream(testStreamId.toStdString(), "localhost",
                               serverPort());
        SAFE_BOOST_REQUIRE(stream.isConnected());
        stream.sendData(sentData.data(), sentData.size());
        waitForMessage();
    }

    waitForMessage();
    SAFE_BOOST_CHECK_EQUAL(getOpenedStreams(), 0);
    SAFE_BOOST_CHECK(received);
}

BOOST_AUTO_TEST_CASE(oneObserverAndOneStream)
{
    setFrameReceivedCallback([&](deflect::server::FramePtr frame) {
        SAFE_BOOST_CHECK_EQUAL(frame->segments.size(), 1);
        SAFE_BOOST_CHECK_EQUAL(frame->uri.toStdString(),
                               testStreamId.toStdString());
    });

    // handle received frames to test the stream's purpose
    const size_t expectedFrames = 2;

    {
        deflect::Stream stream(testStreamId.toStdString(), "localhost",
                               serverPort());
        SAFE_BOOST_REQUIRE(stream.isConnected());

        deflect::Observer observer(testStreamId.toStdString(), "localhost",
                                   serverPort());
        SAFE_BOOST_REQUIRE(observer.isConnected());

        SAFE_BOOST_CHECK(observer.registerForEvents(true));

        // handle connects first before sending and receiving frames
        waitForMessage();

        const unsigned int width = 4;
        const unsigned int height = 4;
        const unsigned int byte = width * height * 4;
        std::unique_ptr<uint8_t[]> pixels(new uint8_t[byte]);
        ::memset(pixels.get(), 0, byte);
        deflect::ImageWrapper image(pixels.get(), width, height, deflect::RGBA);

        deflect::Event event;
        event.type = deflect::Event::EVT_CLICK;

        // send images and events on the respective sides and test that both
        // are received accordingly
        for (size_t i = 0; i < expectedFrames; ++i)
        {
            stream.sendAndFinish(image).wait();
            requestFrame(testStreamId);
            event.key = i;
            processEvent(event);

            // process event sending first before receiving in observer
            waitForMessage();

            while (!observer.hasEvent())
                ;

            const auto receivedEvent = observer.getEvent();
            SAFE_BOOST_CHECK_EQUAL(receivedEvent.key, event.key);
            SAFE_BOOST_CHECK_EQUAL(receivedEvent.type, event.type);
        }
    }

    // handle close of streamer and observer
    waitForMessage();

    SAFE_BOOST_CHECK_EQUAL(getOpenedStreams(), 0);
    SAFE_BOOST_CHECK_EQUAL(getReceivedFrames(), expectedFrames);
}

BOOST_AUTO_TEST_CASE(closeObserverBeforeStream)
{
    {
        deflect::Stream stream(testStreamId.toStdString(), "localhost",
                               serverPort());
        BOOST_REQUIRE(stream.isConnected());

        waitForMessage();
        BOOST_CHECK_EQUAL(getOpenedStreams(), 1);

        {
            deflect::Observer observer(testStreamId.toStdString(), "localhost",
                                       serverPort());
            BOOST_REQUIRE(observer.isConnected());
        }
    }

    waitForMessage();
    BOOST_CHECK_EQUAL(getOpenedStreams(), 0);
}

BOOST_AUTO_TEST_CASE(closeStreamBeforeObserver)
{
    {
        deflect::Observer observer(testStreamId.toStdString(), "localhost",
                                   serverPort());
        SAFE_BOOST_REQUIRE(observer.isConnected());

        waitForMessage();
        BOOST_CHECK_EQUAL(getOpenedStreams(), 1);

        {
            deflect::Stream stream(testStreamId.toStdString(), "localhost",
                                   serverPort());
            BOOST_REQUIRE(stream.isConnected());
        }
    }

    waitForMessage();
    BOOST_CHECK_EQUAL(getOpenedStreams(), 0);
}

struct Fixture1 : public DeflectServer
{
    bool requestBeforeSource{true};
};
struct Fixture2 : public DeflectServer
{
    bool requestBeforeSource{false};
};
using Fixtures = boost::mpl::vector<Fixture1, Fixture2>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(
    closeAndReopenStreamWithObserverStayingAliveAndRendering, F, Fixtures, F)
{
    const unsigned int width = 4;
    const unsigned int height = 4;
    const unsigned int byte = width * height * 4;
    std::unique_ptr<uint8_t[]> pixels(new uint8_t[byte]);
    ::memset(pixels.get(), 0, byte);
    deflect::ImageWrapper image(pixels.get(), width, height, deflect::RGBA);

    const size_t expectedFrames = 5;

    {
        deflect::Observer observer(testStreamId.toStdString(), "localhost",
                                   F::serverPort());
        BOOST_REQUIRE(observer.isConnected());
        F::waitForMessage();

        if (F::requestBeforeSource)
            F::requestFrame(testStreamId);

        {
            deflect::Stream stream(testStreamId.toStdString(), "localhost",
                                   F::serverPort());
            BOOST_REQUIRE(stream.isConnected());

            for (size_t i = 0; i < expectedFrames; ++i)
            {
                stream.sendAndFinish(image).wait();
                if (!F::requestBeforeSource)
                    F::requestFrame(testStreamId);

                F::waitForMessage();

                if (F::requestBeforeSource)
                    F::requestFrame(testStreamId);
                BOOST_CHECK_EQUAL(F::getReceivedFrames(), i + 1);
            }
        }

        {
            deflect::Stream stream(testStreamId.toStdString(), "localhost",
                                   F::serverPort());
            BOOST_REQUIRE(stream.isConnected());

            for (size_t i = 0; i < expectedFrames; ++i)
            {
                stream.sendAndFinish(image).wait();
                F::requestFrame(testStreamId);
                F::waitForMessage();
                BOOST_CHECK_EQUAL(F::getReceivedFrames(),
                                  expectedFrames + i + 1);
            }
        }
    }

    F::waitForMessage();
    BOOST_CHECK_EQUAL(F::getOpenedStreams(), 0);
    BOOST_CHECK_EQUAL(F::getReceivedFrames(), expectedFrames * 2);
}

BOOST_AUTO_TEST_CASE(threadedSmallSegmentStream)
{
    const unsigned int segmentSize = 64;
    const unsigned int width = 1920;
    const unsigned int height = 1088;

    struct Segment
    {
        unsigned int x;
        unsigned int y;
    };
    std::vector<Segment> segments;
    for (unsigned int i = 0; i < width; i += segmentSize)
    {
        for (unsigned int j = 0; j < height; j += segmentSize)
            segments.emplace_back(Segment{i, j});
    }

    const unsigned int numSegments = segments.size();
    SAFE_BOOST_REQUIRE_EQUAL(numSegments,
                             std::ceil((float)width / segmentSize) *
                                 std::ceil((float)height / segmentSize));

    setFrameReceivedCallback([&](deflect::server::FramePtr frame) {
        SAFE_BOOST_CHECK_EQUAL(frame->segments.size(), numSegments);
        SAFE_BOOST_CHECK_EQUAL(frame->uri.toStdString(),
                               testStreamId.toStdString());
        const auto dim = frame->computeDimensions();
        SAFE_BOOST_CHECK_EQUAL(dim.width(), width);
        SAFE_BOOST_CHECK_EQUAL(dim.height(), height);
    });

    const std::vector<uint8_t> pixels(segmentSize * segmentSize * 4, 42);

    // handle received frames to test the stream's purpose
    const size_t expectedFrames = 10;

    {
        deflect::Stream stream(testStreamId.toStdString(), "localhost",
                               serverPort());
        SAFE_BOOST_REQUIRE(stream.isConnected());

        // handle connects first before sending and receiving frames
        waitForMessage();

        std::mutex testMutex; // BOOST_CHECK is not thread-safe
        for (size_t i = 0; i < expectedFrames; ++i)
        {
// to make coverage report work; otherwise fails for unknown reasons
#ifdef NDEBUG
#pragma omp parallel for
#endif
            for (int j = 0; j < int(segments.size()); ++j)
            {
                deflect::ImageWrapper deflectImage((const void*)pixels.data(),
                                                   segmentSize, segmentSize,
                                                   deflect::RGBA, segments[j].x,
                                                   segments[j].y);
                deflectImage.compressionPolicy = deflect::COMPRESSION_ON;

                const bool success = stream.send(deflectImage).get();

                std::lock_guard<std::mutex> lock(testMutex);
                SAFE_BOOST_CHECK(success);
            }

            SAFE_BOOST_CHECK(stream.finishFrame().get());

            requestFrame(testStreamId);

            // process frame receive
            waitForMessage();
        }
    }

    // handle close of streamer
    waitForMessage();

    SAFE_BOOST_CHECK_EQUAL(getOpenedStreams(), 0);
    SAFE_BOOST_CHECK_EQUAL(getReceivedFrames(), expectedFrames);
}

BOOST_AUTO_TEST_CASE(compressionErrorForBigNullImage)
{
    deflect::Stream stream(testStreamId.toStdString(), "localhost",
                           serverPort());
    SAFE_BOOST_REQUIRE(stream.isConnected());

    // handle connect of stream
    waitForMessage();

    deflect::ImageWrapper bigImage(nullptr, 1000, 1000, deflect::ARGB);
    bigImage.compressionPolicy = deflect::COMPRESSION_ON;
    SAFE_BOOST_CHECK_THROW(stream.send(bigImage).get(), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
