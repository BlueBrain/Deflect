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

#ifndef DELFECT_QT_QMLSTREAMERIMPL_H
#define DELFECT_QT_QMLSTREAMERIMPL_H

#include <QImage>
#include <QObject>
#include <QThread>
#include <QTimer>

#include "../SizeHints.h"
#include "OffscreenQuickView.h"
#include "QmlStreamer.h"

#ifdef __APPLE__
#include <deflect/AppNapSuspender.h>
#endif
#include <deflect/Stream.h>

namespace deflect
{
namespace qt
{
class EventReceiver;
class QmlGestures;
class TouchInjector;

class QmlStreamer::Impl : public QObject
{
    Q_OBJECT

public:
    Impl(const QString& qmlFile, const std::string& streamHost,
         const std::string& streamId);
    ~Impl();

    void useAsyncSend(const bool async) { _asyncSend = async; }
    QQuickItem* getRootItem() { return _quickView->getRootItem(); }
    QQmlEngine* getQmlEngine() { return _quickView->getEngine(); }
    Stream* getStream() { return _stream.get(); }
signals:
    void streamClosed();

private slots:
    void _afterRender(QImage image);

    void _onPressed(QPointF position);
    void _onReleased(QPointF position);
    void _onMoved(QPointF position);

    void _onKeyPress(int key, int modifiers, QString text);
    void _onKeyRelease(int key, int modifiers, QString text);

    void _onStreamClosed();

private:
    void _setupSizeHintsConnections();
    void _send(QKeyEvent& keyEvent);
    bool _sendToWebengineviewItems(QKeyEvent& keyEvent);
    std::string _getDeflectStreamIdentifier() const;
    bool _setupDeflectStream();

    void _connectTouchInjector();
    void _setupMouseModeSwitcher();
    void _startMouseModeSwitchDetection(const QPointF& pos);
    bool _touchIsTapAndHold();
    void _switchFromTouchToMouseMode();
    void _switchBackToTouchMode();
    void _sendMouseEvent(QEvent::Type eventType, const QPointF& pos);

    QPointF _mapToScene(const QPointF& normalizedPos) const;

    std::unique_ptr<OffscreenQuickView> _quickView;

    std::unique_ptr<Stream> _stream;
    std::unique_ptr<EventReceiver> _eventReceiver;
    std::unique_ptr<QmlGestures> _qmlGestures;
    std::unique_ptr<TouchInjector> _touchInjector;

    const std::string _streamHost;
    const std::string _streamId;
    SizeHints _sizeHints;

    bool _asyncSend{false};
    Stream::Future _sendFuture;
    QImage _image;

    QTimer _mouseModeTimer;
    bool _mouseMode{false};
    QPointF _touchStartPos;
    QPointF _touchCurrentPos;

#ifdef __APPLE__
    AppNapSuspender _napSuspender;
#endif
};
}
}

#endif
