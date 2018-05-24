/*********************************************************************/
/* Copyright (c) 2017, EPFL/Blue Brain Project                       */
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

#include "DeflectServer.h"

#include <boost/test/unit_test.hpp>

DeflectServer::DeflectServer()
{
    _server = new deflect::server::Server(0 /* OS-chosen port */);
    _server->moveToThread(&_thread);
    _thread.connect(&_thread, &QThread::finished, _server,
                    &deflect::server::Server::deleteLater);
    _thread.start();

    _server->connect(_server, &deflect::server::Server::pixelStreamOpened,
                     [&](const QString) {
                         _mutex.lock();
                         ++_openedStreams;
                         _receivedState = true;
                         _received.wakeAll();
                         _mutex.unlock();
                     });

    _server->connect(_server, &deflect::server::Server::pixelStreamClosed,
                     [&](const QString) {
                         _mutex.lock();
                         --_openedStreams;
                         _receivedState = true;
                         _received.wakeAll();
                         _mutex.unlock();
                     });

    _server->connect(_server, &deflect::server::Server::receivedSizeHints,
                     [&](const QString id, const deflect::SizeHints hints) {
                         _mutex.lock();
                         if (_sizeHintsCallback)
                             _sizeHintsCallback(id, hints);
                         _receivedState = true;
                         _received.wakeAll();
                         _mutex.unlock();
                     });

    _server->connect(_server, &deflect::server::Server::receivedData,
                     [&](const QString id, QByteArray data) {
                         _mutex.lock();
                         if (_dataReceivedCallback)
                             _dataReceivedCallback(id, data);
                         _receivedState = true;
                         _received.wakeAll();
                         _mutex.unlock();
                     });

    _server->connect(_server, &deflect::server::Server::receivedFrame,
                     [&](deflect::server::FramePtr frame) {
                         _mutex.lock();
                         if (_frameReceivedCallback)
                             _frameReceivedCallback(frame);
                         ++_receivedFrames;
                         _receivedState = true;
                         _received.wakeAll();
                         _mutex.unlock();
                     });

    _server->connect(_server, &deflect::server::Server::registerToEvents,
                     [&](const QString id, const bool exclusive,
                         deflect::server::EventReceiver* evtReceiver,
                         deflect::server::BoolPromisePtr success) {

                         if (_registerToEventsCallback)
                             _registerToEventsCallback(id, exclusive,
                                                       evtReceiver);

                         _eventReceiver = evtReceiver;
                         success->set_value(true);
                     });
}

DeflectServer::~DeflectServer()
{
    _thread.quit();
    _thread.wait();
}

void DeflectServer::waitForMessage()
{
    for (;;)
    {
        _mutex.lock();
        _received.wait(&_mutex, 100 /*ms*/);
        if (_receivedState)
        {
            _mutex.unlock();
            break;
        }
        _mutex.unlock();
    }
    BOOST_CHECK(_receivedState);
    _receivedState = false;
}

void DeflectServer::processEvent(const deflect::Event& event)
{
    BOOST_REQUIRE(_eventReceiver);
    _eventReceiver->processEvent(event);
}
