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

#include <deflect/Stream.h>
#include <deflect/Server.h>

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

BOOST_GLOBAL_FIXTURE( MinimalGlobalQtApp );

BOOST_AUTO_TEST_CASE( testSizeHintsReceivedByServer )
{
    const QString testURI( "teststream" );
    deflect::SizeHints testHints;
    testHints.maxWidth = 500;
    testHints.preferredHeight= 200;

    QThread serverThread;
    QWaitCondition received;
    QMutex mutex;

    deflect::Server* server = new deflect::Server( 0 /* OS-chosen port */ );
    server->connect( server, &deflect::Server::receivedSizeHints,
                     [&]( QString uri, deflect::SizeHints hints )
                     {
                         BOOST_CHECK( uri == testURI );
                         BOOST_CHECK( hints == testHints );
                         mutex.lock();
                         received.wakeAll();
                         mutex.unlock();
                     });
    server->moveToThread( &serverThread );
    serverThread.start();

    {
        deflect::Stream stream( testURI.toStdString(), "localhost",
                                server->serverPort());
        BOOST_CHECK( stream.isConnected( ));
        stream.sendSizeHints( testHints );
    }

    mutex.lock();
    received.wait( &mutex, 2000 /*ms*/ );
    mutex.unlock();

    serverThread.quit();
    serverThread.wait();
    delete server;
}
