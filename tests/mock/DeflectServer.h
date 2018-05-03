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

#ifndef DEFLECT_DEFLECTSERVER_H
#define DEFLECT_DEFLECTSERVER_H

#ifdef _WIN32
typedef __int32 int32_t;
#endif

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include <deflect/server/EventReceiver.h>
#include <deflect/server/Server.h>

class DeflectServer
{
public:
    explicit DeflectServer();
    ~DeflectServer();

    quint16 serverPort() const { return _server->getPort(); }
    void requestFrame(QString uri) { _server->requestFrame(uri); }
    void waitForMessage();

    size_t getReceivedFrames() const { return _receivedFrames; }
    size_t getOpenedStreams() const { return _openedStreams; }
    using SizeHintsCallback =
        std::function<void(const QString id, const deflect::SizeHints hints)>;
    void setSizeHintsCallback(const SizeHintsCallback& callback)
    {
        _sizeHintsCallback = callback;
    }

    using RegisterToEventsCallback =
        std::function<void(const QString, const bool,
                           deflect::server::EventReceiver*)>;
    void setRegisterToEventsCallback(const RegisterToEventsCallback& callback)
    {
        _registerToEventsCallback = callback;
    }

    using DataReceivedCallback = std::function<void(const QString, QByteArray)>;
    void setDataReceivedCallback(const DataReceivedCallback& callback)
    {
        _dataReceivedCallback = callback;
    }

    using FrameReceivedCallback =
        std::function<void(deflect::server::FramePtr)>;
    void setFrameReceivedCallback(const FrameReceivedCallback& callback)
    {
        _frameReceivedCallback = callback;
    }

    void processEvent(const deflect::Event& event);

private:
    QThread _thread;
    // destroyed by Object::deleteLater
    deflect::server::Server* _server = nullptr;

    bool _receivedState{false};
    QWaitCondition _received;
    QMutex _mutex;

    size_t _openedStreams{0};
    size_t _receivedFrames{0};

    SizeHintsCallback _sizeHintsCallback;
    RegisterToEventsCallback _registerToEventsCallback;
    DataReceivedCallback _dataReceivedCallback;
    FrameReceivedCallback _frameReceivedCallback;

    deflect::server::EventReceiver* _eventReceiver{nullptr};
};

#endif
