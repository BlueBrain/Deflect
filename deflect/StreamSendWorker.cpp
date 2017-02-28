/*********************************************************************/
/* Copyright (c) 2013-2016, EPFL/Blue Brain Project                  */
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

#include "StreamSendWorker.h"

#include "StreamPrivate.h"

namespace deflect
{
StreamSendWorker::StreamSendWorker(StreamPrivate& stream)
    : _stream(stream)
    , _running(true)
    , _thread(std::bind(&StreamSendWorker::_run, this))
{
}

StreamSendWorker::~StreamSendWorker()
{
    _stop();
}

void StreamSendWorker::_run()
{
    std::unique_lock<std::mutex> lock(_mutex);
    while (true)
    {
        while (_requests.empty() && _running)
            _condition.wait(lock);
        if (!_running)
            break;

        const Request& request = _requests.back();
        request.first->set_value(_stream.send(request.second) &&
                                 _stream.finishFrame());
        _requests.pop_back();
    }
}

void StreamSendWorker::_stop()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _running = false;
        _condition.notify_all();
    }

    _thread.join();
    while (!_requests.empty())
    {
        _requests.back().first->set_value(false);
        _requests.pop_back();
    }
}

Stream::Future StreamSendWorker::enqueueImage(const ImageWrapper& image)
{
    std::lock_guard<std::mutex> lock(_mutex);
    PromisePtr promise(new Promise);
    _requests.push_back(Request(promise, image));
    _condition.notify_all();
    return promise->get_future();
}
}
