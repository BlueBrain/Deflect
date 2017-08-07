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

#include "MinimalGlobalQtApp.h"

#include <deflect/EventReceiver.h>
#include <deflect/Frame.h>
#include <deflect/Server.h>
#include <deflect/Stream.h>

#include <cmath>
#include <iostream>

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

namespace
{
const QString testStreamId("teststream");
}

BOOST_GLOBAL_FIXTURE(MinimalGlobalQtApp);

BOOST_AUTO_TEST_CASE(testSizeHintsReceivedByServer)
{
    QThread serverThread;
    deflect::Server* server = new deflect::Server(0 /* OS-chosen port */);
    server->moveToThread(&serverThread);
    serverThread.connect(&serverThread, &QThread::finished, server,
                         &deflect::Server::deleteLater);
    serverThread.start();

    QWaitCondition received;
    QMutex mutex;

    deflect::SizeHints testHints;
    testHints.maxWidth = 500;
    testHints.preferredHeight = 200;

    QString streamId;
    deflect::SizeHints sizeHints;

    bool receivedState = false;
    server->connect(server, &deflect::Server::receivedSizeHints,
                    [&](const QString id, const deflect::SizeHints hints) {
                        streamId = id;
                        sizeHints = hints;
                        mutex.lock();
                        receivedState = true;
                        received.wakeAll();
                        mutex.unlock();
                    });

    {
        deflect::Stream stream(testStreamId.toStdString(), "localhost",
                               server->serverPort());
        BOOST_CHECK(stream.isConnected());
        stream.sendSizeHints(testHints);

        for (size_t i = 0; i < 20; ++i)
        {
            mutex.lock();
            received.wait(&mutex, 100 /*ms*/);
            if (receivedState)
            {
                BOOST_CHECK_EQUAL(streamId.toStdString(),
                                  testStreamId.toStdString());
                BOOST_CHECK(sizeHints == testHints);

                serverThread.quit();
                serverThread.wait();
                mutex.unlock();
                return;
            }
            mutex.unlock();
        }
        BOOST_CHECK(!"reachable");
    }
}

BOOST_AUTO_TEST_CASE(testRegisterForEventReceivedByServer)
{
    QThread serverThread;
    deflect::Server* server = new deflect::Server(0 /* OS-chosen port */);
    server->moveToThread(&serverThread);
    serverThread.connect(&serverThread, &QThread::finished, server,
                         &deflect::Server::deleteLater);
    serverThread.start();

    QWaitCondition received;
    QMutex mutex;

    QString streamId;
    bool exclusiveBind = false;
    deflect::EventReceiver* eventReceiver = nullptr;

    bool receivedState = false;
    server->connect(server, &deflect::Server::registerToEvents,
                    [&](const QString id, const bool exclusive,
                        deflect::EventReceiver* receiver,
                        deflect::BoolPromisePtr success) {
                        streamId = id;
                        exclusiveBind = exclusive;
                        eventReceiver = receiver;
                        success->set_value(true);
                        mutex.lock();
                        receivedState = true;
                        received.wakeAll();
                        mutex.unlock();
                    });

    {
        deflect::Stream stream(testStreamId.toStdString(), "localhost",
                               server->serverPort());
        BOOST_REQUIRE(stream.isConnected());
        BOOST_CHECK(stream.registerForEvents(true));
    }

    for (size_t i = 0; i < 20; ++i)
    {
        mutex.lock();
        received.wait(&mutex, 100 /*ms*/);
        if (receivedState)
        {
            BOOST_CHECK_EQUAL(streamId.toStdString(),
                              testStreamId.toStdString());
            BOOST_CHECK(exclusiveBind);
            BOOST_CHECK(eventReceiver);

            serverThread.quit();
            serverThread.wait();
            mutex.unlock();
            return;
        }
        mutex.unlock();
    }
    BOOST_CHECK(!"reachable");
}

BOOST_AUTO_TEST_CASE(testDataReceivedByServer)
{
    if (getenv("TRAVIS"))
    {
        std::cout << "ignore testDataReceivedByServer on Jenkins" << std::endl;
        return;
    }

    QThread serverThread;
    deflect::Server* server = new deflect::Server(0 /* OS-chosen port */);
    server->moveToThread(&serverThread);
    serverThread.connect(&serverThread, &QThread::finished, server,
                         &deflect::Server::deleteLater);
    serverThread.start();

    QWaitCondition received;
    QMutex mutex;

    QString streamId;
    std::string receivedData;
    const auto sentData = std::string{"Hello World!"};

    bool receivedState = false;
    server->connect(server, &deflect::Server::receivedData,
                    [&](const QString id, QByteArray data) {
                        streamId = id;
                        receivedData = QString(data).toStdString();
                        mutex.lock();
                        receivedState = true;
                        received.wakeAll();
                        mutex.unlock();
                    });

    {
        deflect::Stream stream(testStreamId.toStdString(), "localhost",
                               server->serverPort());
        BOOST_REQUIRE(stream.isConnected());
        stream.sendData(sentData.data(), sentData.size());
    }

    for (size_t i = 0; i < 20; ++i)
    {
        mutex.lock();
        received.wait(&mutex, 100 /*ms*/);
        if (receivedState)
        {
            BOOST_CHECK_EQUAL(streamId.toStdString(),
                              testStreamId.toStdString());
            BOOST_CHECK_EQUAL(receivedData, sentData);

            serverThread.quit();
            serverThread.wait();
            mutex.unlock();
            return;
        }
        mutex.unlock();
    }
    BOOST_CHECK(!"reachable");
}

BOOST_AUTO_TEST_CASE(testOneObserverAndOneStream)
{
    QThread serverThread;
    deflect::Server* server = new deflect::Server(0 /* OS-chosen port */);
    server->moveToThread(&serverThread);
    serverThread.connect(&serverThread, &QThread::finished, server,
                         &deflect::Server::deleteLater);
    serverThread.start();

    // to wait in this thread until server thread is done with certain
    // operations
    QWaitCondition received;
    QMutex mutex;
    bool receivedState = false;

    auto processServerMessages = [&] {
        for (size_t j = 0; j < 20; ++j)
        {
            mutex.lock();
            received.wait(&mutex, 100 /*ms*/);
            if (receivedState)
            {
                mutex.unlock();
                break;
            }
            mutex.unlock();
        }
        BOOST_REQUIRE(receivedState);
        receivedState = false;
    };

    size_t openedStreams = 0;

    // only continue once we have the stream, whichever comes first
    server->connect(server, &deflect::Server::pixelStreamOpened,
                    [&](const QString) {
                        ++openedStreams;
                        if (openedStreams == 1)
                        {
                            mutex.lock();
                            receivedState = true;
                            received.wakeAll();
                            mutex.unlock();
                        }
                    });

    // make sure that we get the close of the stream and the observer
    server->connect(server, &deflect::Server::pixelStreamClosed,
                    [&](const QString) {
                        --openedStreams;
                        if (openedStreams == 0)
                        {
                            mutex.lock();
                            receivedState = true;
                            received.wakeAll();
                            mutex.unlock();
                        }
                    });

    // register to events to test the observer's purpose
    deflect::EventReceiver* eventReceiver = nullptr;
    server->connect(server, &deflect::Server::registerToEvents,
                    [&](const QString, const bool,
                        deflect::EventReceiver* evtReceiver,
                        deflect::BoolPromisePtr success) {
                        eventReceiver = evtReceiver;
                        success->set_value(true);
                    });

    // handle received frames to test the stream's purpose
    const size_t expectedFrames = 2;
    size_t receivedFrames = 0;
    server->connect(server, &deflect::Server::receivedFrame,
                    [&](deflect::FramePtr frame) {
                        BOOST_CHECK_EQUAL(frame->segments.size(), 1);
                        BOOST_CHECK_EQUAL(frame->uri.toStdString(),
                                          testStreamId.toStdString());
                        ++receivedFrames;
                        mutex.lock();
                        receivedState = true;
                        received.wakeAll();
                        mutex.unlock();
                    });

    {
        deflect::Stream stream(testStreamId.toStdString(), "localhost",
                               server->serverPort());
        BOOST_REQUIRE(stream.isConnected());

        deflect::Observer observer(testStreamId.toStdString(), "localhost",
                                   server->serverPort());
        BOOST_REQUIRE(observer.isConnected());

        BOOST_CHECK(observer.registerForEvents(true));

        // handle connects first before sending and receiving frames
        processServerMessages();

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
            receivedState = false;
            stream.sendAndFinish(image).wait();
            server->requestFrame(testStreamId);
            event.key = i;
            eventReceiver->processEvent(event);

            // process event sending first before receiving in observer
            processServerMessages();

            while (!observer.hasEvent())
                ;

            const auto receivedEvent = observer.getEvent();
            BOOST_CHECK_EQUAL(receivedEvent.key, event.key);
            BOOST_CHECK_EQUAL(receivedEvent.type, event.type);
        }
    }

    // handle close of streamer and observer
    processServerMessages();

    serverThread.quit();
    serverThread.wait();

    BOOST_CHECK_EQUAL(openedStreams, 0);
    BOOST_CHECK_EQUAL(receivedFrames, expectedFrames);
}

BOOST_AUTO_TEST_CASE(testThreadedSmallSegmentStream)
{
    QThread serverThread;
    deflect::Server* server = new deflect::Server(0 /* OS-chosen port */);
    server->moveToThread(&serverThread);
    serverThread.connect(&serverThread, &QThread::finished, server,
                         &deflect::Server::deleteLater);
    serverThread.start();

    // to wait in this thread until server thread is done with certain
    // operations
    QWaitCondition received;
    QMutex mutex;
    bool receivedState = false;

    auto processServerMessages = [&] {
        for (size_t j = 0; j < 20; ++j)
        {
            mutex.lock();
            received.wait(&mutex, 100 /*ms*/);
            if (receivedState)
            {
                mutex.unlock();
                break;
            }
            mutex.unlock();
        }
        BOOST_REQUIRE(receivedState);
        receivedState = false;
    };

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
    BOOST_REQUIRE_EQUAL(numSegments,
                        std::ceil((float)width / segmentSize) *
                            std::ceil((float)height / segmentSize));

    const std::vector<uint8_t> pixels(segmentSize * segmentSize * 4, 42);

    size_t openedStreams = 0;
    // only continue once we have the stream
    server->connect(server, &deflect::Server::pixelStreamOpened,
                    [&](const QString) {
                        ++openedStreams;
                        if (openedStreams == 1)
                        {
                            mutex.lock();
                            receivedState = true;
                            received.wakeAll();
                            mutex.unlock();
                        }
                    });

    // make sure that we get the close of the stream
    server->connect(server, &deflect::Server::pixelStreamClosed,
                    [&](const QString) {
                        --openedStreams;
                        if (openedStreams == 0)
                        {
                            mutex.lock();
                            receivedState = true;
                            received.wakeAll();
                            mutex.unlock();
                        }
                    });

    // handle received frames to test the stream's purpose
    const size_t expectedFrames = 10;
    size_t receivedFrames = 0;
    server->connect(server, &deflect::Server::receivedFrame,
                    [&](deflect::FramePtr frame) {
                        BOOST_CHECK_EQUAL(frame->segments.size(), numSegments);
                        BOOST_CHECK_EQUAL(frame->uri.toStdString(),
                                          testStreamId.toStdString());
                        const auto dim = frame->computeDimensions();
                        BOOST_CHECK_EQUAL(dim.width(), width);
                        BOOST_CHECK_EQUAL(dim.height(), height);
                        ++receivedFrames;
                        mutex.lock();
                        receivedState = true;
                        received.wakeAll();
                        mutex.unlock();
                    });

    {
        deflect::Stream stream(testStreamId.toStdString(), "localhost",
                               server->serverPort());
        BOOST_REQUIRE(stream.isConnected());

        // handle connects first before sending and receiving frames
        processServerMessages();

        std::mutex testMutex; // BOOST_CHECK is not thread-safe
        for (size_t i = 0; i < expectedFrames; ++i)
        {
            receivedState = false;

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
                BOOST_CHECK(success);
            }

            BOOST_CHECK(stream.finishFrame().get());

            server->requestFrame(testStreamId);

            // process frame receive
            processServerMessages();
        }
    }

    // handle close of streamer
    processServerMessages();

    serverThread.quit();
    serverThread.wait();

    BOOST_CHECK_EQUAL(openedStreams, 0);
    BOOST_CHECK_EQUAL(receivedFrames, expectedFrames);
}

BOOST_AUTO_TEST_CASE(testCompressionErrorForBigNullImage)
{
    QThread serverThread;
    deflect::Server* server = new deflect::Server(0 /* OS-chosen port */);
    server->moveToThread(&serverThread);
    serverThread.connect(&serverThread, &QThread::finished, server,
                         &deflect::Server::deleteLater);
    serverThread.start();

    // to wait in this thread until server thread is done with certain
    // operations
    QWaitCondition received;
    QMutex mutex;
    bool receivedState = false;

    auto processServerMessages = [&] {
        for (size_t j = 0; j < 20; ++j)
        {
            mutex.lock();
            received.wait(&mutex, 100 /*ms*/);
            if (receivedState)
            {
                mutex.unlock();
                break;
            }
            mutex.unlock();
        }
        BOOST_REQUIRE(receivedState);
        receivedState = false;
    };

    // only continue once we have the stream
    server->connect(server, &deflect::Server::pixelStreamOpened,
                    [&](const QString) {
                        mutex.lock();
                        receivedState = true;
                        received.wakeAll();
                        mutex.unlock();
                    });

    // make sure that we get the close of the stream
    server->connect(server, &deflect::Server::pixelStreamClosed,
                    [&](const QString) {
                        mutex.lock();
                        receivedState = true;
                        received.wakeAll();
                        mutex.unlock();
                    });

    {
        deflect::Stream stream(testStreamId.toStdString(), "localhost",
                               server->serverPort());
        BOOST_REQUIRE(stream.isConnected());

        processServerMessages();

        deflect::ImageWrapper bigImage(nullptr, 1000, 1000, deflect::ARGB);
        bigImage.compressionPolicy = deflect::COMPRESSION_ON;
        BOOST_CHECK(!stream.send(bigImage).get());
    }

    // handle close of streamer
    processServerMessages();

    serverThread.quit();
    serverThread.wait();
}
