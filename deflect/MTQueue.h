/*********************************************************************/
/* Copyright (c) 2015, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
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

#ifndef DEFLECT_MTQUEUE_H
#define DEFLECT_MTQUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>

namespace deflect
{
/** Thread-safe multiple producer, multiple consumer queue. */
template <class T>
class MTQueue
{
public:
    /** @param maxSize maximum size of the queue after which enqueue() blocks */
    MTQueue(const size_t maxSize = std::numeric_limits<size_t>::max())
        : _maxSize(maxSize)
    {
    }

    /**
     * Push a new value to the end of the queue. Blocks if maxSize is reached.
     */
    void enqueue(const T& value)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while (_queue.size() >= _maxSize)
            _full.wait(lock);
        _queue.push(value);
        _empty.notify_one();
    }

    /** Pop a value from the front of the queue. Blocks if queue is empty. */
    T dequeue()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while (_queue.empty())
            _empty.wait(lock);
        T value = _queue.front();
        _queue.pop();
        _full.notify_one();
        return value;
    }

    /** Clears the queue. */
    void clear()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue = {};
        _full.notify_one();
    }

    /** @return the current size of the queue. */
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

    /** @return true if the queue is empty. */
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.empty();
    }

private:
    size_t _maxSize;
    std::queue<T> _queue;
    mutable std::mutex _mutex;
    std::condition_variable _empty;
    std::condition_variable _full;
};
}

#endif
