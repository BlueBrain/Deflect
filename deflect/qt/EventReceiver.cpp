/*********************************************************************/
/* Copyright (c) 2015-2016, EPFL/Blue Brain Project                  */
/*                     Daniel.Nachbaur <daniel.nachbaur@epfl.ch>     */
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

#include "EventReceiver.h"

#include <QCoreApplication>

namespace deflect
{
namespace qt
{
EventReceiver::EventReceiver(Stream& stream)
    : QObject()
    , _stream(stream)
    , _notifier(
          new QSocketNotifier(_stream.getDescriptor(), QSocketNotifier::Read))
    , _timer(new QTimer)
{
    connect(_notifier.get(), &QSocketNotifier::activated, this,
            &EventReceiver::_onEvent);

    // QSocketNotifier sometimes does not fire, help with a timer
    connect(_timer.get(), &QTimer::timeout,
            [this] { _onEvent(_stream.getDescriptor()); });
    _timer->start(1);
}

EventReceiver::~EventReceiver()
{
}

inline QPointF _pos(const Event& deflectEvent)
{
    return QPointF{deflectEvent.mouseX, deflectEvent.mouseY};
}

void EventReceiver::_onEvent(const int socket)
{
    if (socket != _stream.getDescriptor())
        return;

    while (_stream.hasEvent())
    {
        const Event& deflectEvent = _stream.getEvent();

        switch (deflectEvent.type)
        {
        case Event::EVT_CLOSE:
            _stop();
            return;
        case Event::EVT_PRESS:
            emit pressed(_pos(deflectEvent));
            break;
        case Event::EVT_RELEASE:
            emit released(_pos(deflectEvent));
            break;
        case Event::EVT_MOVE:
            emit moved(_pos(deflectEvent));
            break;
        case Event::EVT_VIEW_SIZE_CHANGED:
            emit resized(QSize{int(deflectEvent.dx), int(deflectEvent.dy)});
            break;
        case Event::EVT_SWIPE_LEFT:
            emit swipeLeft();
            break;
        case Event::EVT_SWIPE_RIGHT:
            emit swipeRight();
            break;
        case Event::EVT_SWIPE_UP:
            emit swipeUp();
            break;
        case Event::EVT_SWIPE_DOWN:
            emit swipeDown();
            break;
        case Event::EVT_KEY_PRESS:
            emit keyPress(deflectEvent.key, deflectEvent.modifiers,
                          QString::fromStdString(deflectEvent.text));
            break;
        case Event::EVT_KEY_RELEASE:
            emit keyRelease(deflectEvent.key, deflectEvent.modifiers,
                            QString::fromStdString(deflectEvent.text));
            break;
        case Event::EVT_TOUCH_ADD:
            emit touchPointAdded(deflectEvent.key, _pos(deflectEvent));
            break;
        case Event::EVT_TOUCH_UPDATE:
            emit touchPointUpdated(deflectEvent.key, _pos(deflectEvent));
            break;
        case Event::EVT_TOUCH_REMOVE:
            emit touchPointRemoved(deflectEvent.key, _pos(deflectEvent));
            break;
        case Event::EVT_CLICK:
        case Event::EVT_DOUBLECLICK:
        case Event::EVT_PINCH:
        case Event::EVT_WHEEL:
        default:
            break;
        }
    }

    if (!_stream.isConnected())
        _stop();
}

void EventReceiver::_stop()
{
    _notifier->setEnabled(false);
    _timer->stop();
    emit closed();
}
}
}
