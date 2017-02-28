/*********************************************************************/
/* Copyright (c) 2016, EPFL/Blue Brain Project                       */
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

#include "TouchInjector.h"

#include "helpers.h"

#include <QCoreApplication>
#include <QWindow>

namespace deflect
{
namespace qt
{
TouchInjector::TouchInjector(QObject& target, MapToSceneFunc mapFunc)
    : _target(target)
    , _mapToSceneFunction(mapFunc)
{
    _device.setType(QTouchDevice::TouchScreen);
}

std::unique_ptr<TouchInjector> TouchInjector::create(QWindow& window)
{
    auto mapFunc = [&window](const QPointF& normPos) {
        return QPointF{normPos.x() * window.width(),
                       normPos.y() * window.height()};
    };
    return make_unique<TouchInjector>(window, mapFunc);
}

void TouchInjector::addTouchPoint(const int id, const QPointF position)
{
    if (_touchPointMap.contains(id))
        return;

    _handleEvent(id, position, QEvent::TouchBegin);
}

void TouchInjector::updateTouchPoint(const int id, const QPointF position)
{
    if (!_touchPointMap.contains(id))
        return;

    _handleEvent(id, position, QEvent::TouchUpdate);
}

void TouchInjector::removeTouchPoint(const int id, const QPointF position)
{
    if (!_touchPointMap.contains(id))
        return;

    _handleEvent(id, position, QEvent::TouchEnd);
}

void TouchInjector::removeAllTouchPoints()
{
    const auto ids = _touchPointMap.keys();
    for (auto id : ids)
        removeTouchPoint(id, _touchPointMap[id].normalizedPos());
}

void _fillTouchBegin(QTouchEvent::TouchPoint& touchPoint)
{
    touchPoint.setStartPos(touchPoint.pos());
    touchPoint.setStartScreenPos(touchPoint.screenPos());
    touchPoint.setStartNormalizedPos(touchPoint.normalizedPos());

    touchPoint.setLastPos(touchPoint.pos());
    touchPoint.setLastScreenPos(touchPoint.screenPos());
    touchPoint.setLastNormalizedPos(touchPoint.normalizedPos());
}

void _fill(QTouchEvent::TouchPoint& touchPoint,
           const QTouchEvent::TouchPoint& prevPoint)
{
    touchPoint.setStartPos(prevPoint.startPos());
    touchPoint.setStartScreenPos(prevPoint.startScreenPos());
    touchPoint.setStartNormalizedPos(prevPoint.startNormalizedPos());

    touchPoint.setLastPos(prevPoint.pos());
    touchPoint.setLastScreenPos(prevPoint.screenPos());
    touchPoint.setLastNormalizedPos(prevPoint.normalizedPos());
}

Qt::TouchPointStates _getTouchPointState(const QEvent::Type eventType)
{
    switch (eventType)
    {
    case QEvent::TouchBegin:
        return Qt::TouchPointPressed;
    case QEvent::TouchUpdate:
        return Qt::TouchPointMoved;
    case QEvent::TouchEnd:
        return Qt::TouchPointReleased;
    default:
        throw std::runtime_error(QString("unexpected QEvent::Type (%1)")
                                     .arg(int(eventType))
                                     .toStdString());
    }
}

void TouchInjector::_handleEvent(const int id, const QPointF& normalizedPos,
                                 const QEvent::Type eventType)
{
    const QPointF screenPos = _mapToSceneFunction(normalizedPos);

    QTouchEvent::TouchPoint touchPoint(id);
    touchPoint.setPressure(1.0);
    touchPoint.setPos(screenPos);
    touchPoint.setScreenPos(screenPos);
    touchPoint.setNormalizedPos(normalizedPos);

    switch (eventType)
    {
    case QEvent::TouchBegin:
        _fillTouchBegin(touchPoint);
        break;
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        _fill(touchPoint, _touchPointMap.value(id));
        break;
    default:
        return;
    }

    const auto touchPointState = _getTouchPointState(eventType);
    touchPoint.setState(touchPointState);
    _touchPointMap.insert(id, touchPoint);

    QEvent::Type touchEventType = eventType;
    if (touchEventType == QEvent::TouchEnd)
        touchEventType =
            _touchPointMap.isEmpty() ? QEvent::TouchEnd : QEvent::TouchUpdate;

    QEvent* touchEvent =
        new QTouchEvent(touchEventType, &_device, Qt::NoModifier,
                        touchPointState, _touchPointMap.values());
    QCoreApplication::postEvent(&_target, touchEvent);

    // Prepare state for next call to handle event
    if (eventType == QEvent::TouchEnd)
        _touchPointMap.remove(id);
    else
        _touchPointMap[id].setState(Qt::TouchPointStationary);
}
}
}
