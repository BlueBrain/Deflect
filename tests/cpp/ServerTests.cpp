/*********************************************************************/
/* Copyright (c) 2015, EPFL/Blue Brain Project                       */
/*                     Daniel.Nachbaur@epfl.ch                       */
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
#include <deflect/Server.h>
#include <deflect/Stream.h>

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

namespace
{
const QString testStreamId( "teststream" );
}

BOOST_GLOBAL_FIXTURE( MinimalGlobalQtApp );

BOOST_AUTO_TEST_CASE( testSizeHintsReceivedByServer )
{
    QThread serverThread;
    deflect::Server* server = new deflect::Server( 0 /* OS-chosen port */ );
    server->moveToThread( &serverThread );
    serverThread.connect( &serverThread, &QThread::finished,
                          server, &deflect::Server::deleteLater );
    serverThread.start();

    QWaitCondition received;
    QMutex mutex;

    deflect::SizeHints testHints;
    testHints.maxWidth = 500;
    testHints.preferredHeight= 200;

    QString streamId;
    deflect::SizeHints sizeHints;

    bool receivedState = false;
    server->connect( server, &deflect::Server::receivedSizeHints,
                     [&]( const QString id, const deflect::SizeHints hints )
    {
        streamId = id;
        sizeHints = hints;
        mutex.lock();
        receivedState = true;
        received.wakeAll();
        mutex.unlock();
    });

    {
        deflect::Stream stream( testStreamId.toStdString(), "localhost",
                                server->serverPort( ));
        BOOST_CHECK( stream.isConnected( ));
        stream.sendSizeHints( testHints );

        for( size_t i = 0; i < 20; ++i )
        {
            mutex.lock();
            received.wait( &mutex, 100 /*ms*/ );
            if( receivedState )
            {
                BOOST_CHECK_EQUAL( streamId.toStdString(),
                                   testStreamId.toStdString( ));
                BOOST_CHECK( sizeHints == testHints );

                serverThread.quit();
                serverThread.wait();
                mutex.unlock();
                return;
            }
            mutex.unlock();
        }
        BOOST_CHECK( !"reachable" );
    }
}

BOOST_AUTO_TEST_CASE( testRegisterForEventReceivedByServer )
{
    QThread serverThread;
    deflect::Server* server = new deflect::Server( 0 /* OS-chosen port */ );
    server->moveToThread( &serverThread );
    serverThread.connect( &serverThread, &QThread::finished,
                          server, &deflect::Server::deleteLater );
    serverThread.start();

    QWaitCondition received;
    QMutex mutex;

    QString streamId;
    bool exclusiveBind = false;
    deflect::EventReceiver* eventReceiver = nullptr;

    bool receivedState = false;
    server->connect( server, &deflect::Server::registerToEvents,
             [&]( const QString id, const bool exclusive,
                     deflect::EventReceiver* receiver )
    {
        streamId = id;
        exclusiveBind = exclusive;
        eventReceiver = receiver;
        mutex.lock();
        receivedState = true;
        received.wakeAll();
        mutex.unlock();
    });

    {
        deflect::Stream stream( testStreamId.toStdString(), "localhost",
                                server->serverPort( ));
        BOOST_REQUIRE( stream.isConnected( ));
        // Just send an event to check the streamId received by the server
        stream.registerForEvents( true );
    }

    for( size_t i = 0; i < 20; ++i )
    {
        mutex.lock();
        received.wait( &mutex, 100 /*ms*/ );
        if( receivedState )
        {
            BOOST_CHECK_EQUAL( streamId.toStdString(),
                               testStreamId.toStdString( ));
            BOOST_CHECK( exclusiveBind );
            BOOST_CHECK( eventReceiver );

            serverThread.quit();
            serverThread.wait();
            mutex.unlock();
            return;
        }
        mutex.unlock();
    }
    BOOST_CHECK( !"reachable" );
}

BOOST_AUTO_TEST_CASE( testDataReceivedByServer )
{
    QThread serverThread;
    deflect::Server* server = new deflect::Server( 0 /* OS-chosen port */ );
    server->moveToThread( &serverThread );
    serverThread.connect( &serverThread, &QThread::finished,
                          server, &deflect::Server::deleteLater );
    serverThread.start();

    QWaitCondition received;
    QMutex mutex;

    QString streamId;
    std::string receivedData;
    const auto sentData = std::string{ "Hello World!" };

    bool receivedState = false;
    server->connect( server, &deflect::Server::receivedData,
             [&]( const QString id, QByteArray data )
    {
        streamId = id;
        receivedData = QString( data ).toStdString();
        mutex.lock();
        receivedState = true;
        received.wakeAll();
        mutex.unlock();
    });

    {
        deflect::Stream stream( testStreamId.toStdString(), "localhost",
                                server->serverPort( ));
        BOOST_REQUIRE( stream.isConnected( ));
        stream.sendData( sentData.data(), sentData.size( ));
    }

    for( size_t i = 0; i < 20; ++i )
    {
        mutex.lock();
        received.wait( &mutex, 100 /*ms*/ );
        if( receivedState )
        {
            BOOST_CHECK_EQUAL( streamId.toStdString(),
                               testStreamId.toStdString( ));
            BOOST_CHECK_EQUAL( receivedData, sentData );

            serverThread.quit();
            serverThread.wait();
            mutex.unlock();
            return;
        }
        mutex.unlock();
    }
    BOOST_CHECK( !"reachable" );
}
