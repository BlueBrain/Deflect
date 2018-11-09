/*********************************************************************/
/* Copyright (c) 2013-2018, EPFL/Blue Brain Project                  */
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

#include "StreamSendWorker.h"

#include "NetworkProtocol.h"
#include "Segment.h"
#include "SizeHints.h"

#include <iostream>

namespace deflect
{
StreamSendWorker::StreamSendWorker(Socket& socket, const std::string& id)
    : _socket(socket)
    , _id(id)
    , _dequeuedRequests(std::thread::hardware_concurrency() / 2)
{
}

StreamSendWorker::~StreamSendWorker()
{
    stop();
}

void StreamSendWorker::stop()
{
    {
        _running = false;
        enqueueRequest(std::vector<Task>());
    }

    quit();
    wait();

    Request request;
    while (_requests.try_dequeue(request))
    {
        if (request.promise)
            request.promise->set_value(false);
    }
}

void StreamSendWorker::run()
{
    _running = true;
    while (true)
    {
        if (!_running)
            break;

        size_t count = 0;
        if (!_pendingFinish)
            count = _requests.wait_dequeue_bulk(_dequeuedRequests.begin(),
                                                _dequeuedRequests.size());
        else
        {
            // in case we encountered a finish request, get all remaining send
            // requests w/o waiting
            count = _requests.try_dequeue_bulk(_dequeuedRequests.begin(),
                                               _dequeuedRequests.size());

            // no more pending sends, now process the finish request and reset
            // for next finish
            if (count == 0)
            {
                count = 1;
                _finishRequest.isFinish = false; // reset this to process this
                                                 // request now
                _dequeuedRequests[0] = _finishRequest;
                _pendingFinish = false;
            }
        }

        for (size_t i = 0; i < count; ++i)
        {
            auto& request = _dequeuedRequests[i];

            // postpone a finish request to maintain order (as the lockfree
            // does not guarantee order)
            if (request.isFinish)
            {
                if (_pendingFinish)
                {
                    if (request.promise)
                        request.promise->set_exception(std::make_exception_ptr(
                            std::runtime_error("Already have pending finish")));
                    continue;
                }

                _finishRequest = request;
                _pendingFinish = true;
                continue;
            }

            try
            {
                bool success = true;
                for (auto& task : request.tasks)
                {
                    if (!task())
                    {
                        success = false;
                        break;
                    }
                }

                if (request.promise)
                    request.promise->set_value(success);
            }
            catch (...)
            {
                if (request.promise)
                    request.promise->set_exception(std::current_exception());
            }
        }
    }
}

Stream::Future StreamSendWorker::enqueueRequest(Task&& action, bool isFinish)
{
    return enqueueRequest(std::vector<Task>{std::move(action)}, isFinish);
}

Stream::Future StreamSendWorker::enqueueRequest(std::vector<Task>&& tasks,
                                                const bool isFinish)
{
    auto promise = std::make_shared<Promise>();
    auto future = promise->get_future();
    _requests.enqueue({std::move(promise), std::move(tasks), isFinish});
    return future;
}

void StreamSendWorker::enqueueFastRequest(Task&& task)
{
    _requests.enqueue({nullptr, std::vector<Task>{std::move(task)}, false});
}

bool StreamSendWorker::_sendOpenObserver()
{
    return _send(MESSAGE_TYPE_OBSERVER_OPEN,
                 QByteArray::number(NETWORK_PROTOCOL_VERSION));
}

bool StreamSendWorker::_sendOpenStream()
{
    return _send(MESSAGE_TYPE_PIXELSTREAM_OPEN,
                 QByteArray::number(NETWORK_PROTOCOL_VERSION));
}

bool StreamSendWorker::_sendClose()
{
    return _send(MESSAGE_TYPE_QUIT, {});
}

bool StreamSendWorker::_sendSegment(const Segment& segment)
{
    if (segment.view != _currentView)
    {
        if (!_sendImageView(segment.view))
            return false;
        _currentView = segment.view;
    }
    _sendRowOrderIfChanged(segment.rowOrder);
    _sendImageChannelIfChanged(segment.channel);

    auto message = QByteArray{(const char*)(&segment.parameters),
                              sizeof(SegmentParameters)};
    message.append(segment.imageData);
    return _send(MESSAGE_TYPE_PIXELSTREAM, message, false);
}

bool StreamSendWorker::_sendImageView(const View view)
{
    return _send(MESSAGE_TYPE_IMAGE_VIEW,
                 QByteArray{(const char*)(&view), sizeof(View)});
}

bool StreamSendWorker::_sendRowOrderIfChanged(const RowOrder rowOrder)
{
    if (rowOrder != _currentRowOrder)
    {
        if (!_sendImageRowOrder(rowOrder))
            return false;
        _currentRowOrder = rowOrder;
    }
    return true;
}

bool StreamSendWorker::_sendImageRowOrder(const RowOrder rowOrder)
{
    return _send(MESSAGE_TYPE_IMAGE_ROW_ORDER,
                 QByteArray{(const char*)(&rowOrder), sizeof(RowOrder)});
}

bool StreamSendWorker::_sendImageChannelIfChanged(const uint8_t channel)
{
    if (channel != _currentChannel)
    {
        if (!_sendImageChannel(channel))
            return false;
        _currentChannel = channel;
    }
    return true;
}

bool StreamSendWorker::_sendImageChannel(const uint8_t channel)
{
    return _send(MESSAGE_TYPE_IMAGE_CHANNEL,
                 QByteArray{(const char*)(&channel), sizeof(uint8_t)});
}

bool StreamSendWorker::_sendFinish()
{
    return _send(MESSAGE_TYPE_PIXELSTREAM_FINISH_FRAME, {});
}

bool StreamSendWorker::_sendData(const QByteArray data)
{
    return _send(MESSAGE_TYPE_DATA, data);
}

bool StreamSendWorker::_sendSizeHints(const SizeHints& hints)
{
    return _send(MESSAGE_TYPE_SIZE_HINTS,
                 QByteArray{(const char*)(&hints), sizeof(SizeHints)});
}

bool StreamSendWorker::_sendBindEvents(const bool exclusive)
{
    return _send(exclusive ? MESSAGE_TYPE_BIND_EVENTS_EX
                           : MESSAGE_TYPE_BIND_EVENTS,
                 {});
}

bool StreamSendWorker::_send(const MessageType type, const QByteArray& message,
                             const bool waitForBytesWritten)
{
    return _socket.send(MessageHeader(type, message.size(), _id), message,
                        waitForBytesWritten);
}
}
