/*********************************************************************/
/* Copyright (c) 2013-2017, EPFL/Blue Brain Project                  */
/*                          Daniel.Nachbaur@epfl.ch                  */
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

#ifndef DEFLECT_STREAMSENDWORKER_H
#define DEFLECT_STREAMSENDWORKER_H

#include "MessageHeader.h" // MessageType
#include "Socket.h"        // member
#include "Stream.h"        // Stream::Future

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#include "moodycamel/blockingconcurrentqueue.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <QThread>

namespace deflect
{
using Task = std::function<bool()>;

/**
 * Worker thread class that sends images and messages through a Socket.
 *
 * When a QApplication is present, Qt insists that all write operations on
 * QTcpSocket are done from a single QThread, otherwise a qWarning is issued:
 * "QSocketNotifier: Socket notifiers cannot be enabled or disabled from another
 * thread".
 * To avoid it, the Socket must be moved to the worker thread (moveToThread()).
 */
class StreamSendWorker : public QThread
{
public:
    /** Create a new stream worker associated to an existing socket. */
    StreamSendWorker(Socket& socket, const std::string& id);

    /** Stop and destroy the worker. */
    ~StreamSendWorker();

    /** Enqueue a request to be send during the execution of run(). */
    Stream::Future enqueueRequest(Task&& action, bool isFinish = false);

    /** Enqueue a request to be send during the execution of run(). */
    Stream::Future enqueueRequest(std::vector<Task>&& actions,
                                  bool isFinish = false);

    /** Enqueue a request with no future to check for its completion. */
    void enqueueFastRequest(Task&& task);

private:
    using Promise = std::promise<bool>;
    using PromisePtr = std::shared_ptr<Promise>;

    struct Request
    {
        PromisePtr promise;
        std::vector<Task> tasks;
        bool isFinish;
    };

    Socket& _socket;
    const std::string& _id;

    moodycamel::BlockingConcurrentQueue<Request> _requests;
    bool _running = false;
    View _currentView = View::mono;
    RowOrder _currentRowOrder = RowOrder::top_down;
    uint8_t _currentChannel = 0;

    std::vector<Request> _dequeuedRequests;
    bool _pendingFinish = false;
    Request _finishRequest;

    /** Stop the worker and clear any pending send tasks. */
    void stop();

    /** Main QThread loop doing asynchronous processing of queued tasks. */
    void run() final;

    friend class deflect::test::Application; // to send pre-compressed segments
    friend class TaskBuilder;

    bool _sendOpenObserver();
    bool _sendOpenStream();
    bool _sendClose();
    bool _sendSegment(const Segment& segment);
    bool _sendImageView(View view);
    bool _sendRowOrderIfChanged(RowOrder rowOrder);
    bool _sendImageRowOrder(RowOrder rowOrder);
    bool _sendImageChannelIfChanged(uint8_t channel);
    bool _sendImageChannel(uint8_t channel);
    bool _sendFinish();
    bool _sendData(const QByteArray data);
    bool _sendSizeHints(const SizeHints& hints);
    bool _sendBindEvents(const bool exclusive);

    bool _send(MessageType type, const QByteArray& message,
               bool waitForBytesWritten = true);
};
}
#endif
