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

#include "QmlStreamerImpl.h"

#include "EventReceiver.h"
#include "QmlGestures.h"
#include "TouchInjector.h"
#include "helpers.h"

#include <QCoreApplication>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickRenderControl>

namespace
{
const std::string DEFAULT_STREAM_ID("QmlStreamer");
const QString GESTURES_CONTEXT_PROPERTY("deflectgestures");
const QString WEBENGINEVIEW_OBJECT_NAME("webengineview");
const int TOUCH_TAPANDHOLD_DIST_PX = 20;
const int TOUCH_TAPANDHOLD_TIMEOUT_MS = 200;
#ifdef DEFLECTQT_MULTITHREADED
const auto renderMode = deflect::qt::RenderMode::MULTITHREADED;
#else
const auto renderMode = deflect::qt::RenderMode::SINGLETHREADED;
#endif
}

namespace deflect
{
namespace qt
{
QmlStreamer::Impl::Impl(const QString& qmlFile, const std::string& streamHost,
                        const std::string& streamId)
    : _quickView{new OffscreenQuickView{make_unique<QQuickRenderControl>(),
                                        renderMode}}
    , _qmlGestures{new QmlGestures}
    , _touchInjector{TouchInjector::create(*_quickView)}
    , _streamHost{streamHost}
    , _streamId{streamId}
    , _sendFuture{make_ready_future(true)}
{
    _setupMouseModeSwitcher();
    _setupSizeHintsConnections();

    connect(_quickView.get(), &OffscreenQuickView::afterRender, this,
            &QmlStreamer::Impl::_afterRender);

    // Expose stream gestures to qml objects
    auto context = _quickView->getRootContext();
    context->setContextProperty(GESTURES_CONTEXT_PROPERTY, _qmlGestures.get());

    if (!_quickView->load(qmlFile).get())
        throw std::runtime_error("Failed to setup/load QML");

#ifdef __APPLE__
    _napSuspender.suspend();
#endif
}

QmlStreamer::Impl::~Impl()
{
}

void QmlStreamer::Impl::_afterRender(const QImage image)
{
    if (!_sendFuture.valid() || !_sendFuture.get())
        return;

    if (!_stream && !_setupDeflectStream())
    {
        qWarning() << "Could not setup Deflect stream";
        return;
    }

    if (image.isNull())
    {
        qDebug() << "Empty image not streamed";
        return;
    }

    _image = image;
    ImageWrapper imageWrapper(_image.constBits(), _image.width(),
                              _image.height(), BGRA);
    imageWrapper.compressionPolicy = COMPRESSION_ON;
    imageWrapper.compressionQuality = 80;

    if (_asyncSend)
        _sendFuture = _stream->asyncSend(imageWrapper);
    else
        _sendFuture = make_ready_future(_stream->send(imageWrapper) &&
                                        _stream->finishFrame());
}

void QmlStreamer::Impl::_onStreamClosed()
{
    // Stop rendering
    disconnect(_quickView.get(), &OffscreenQuickView::afterRender, this,
               &QmlStreamer::Impl::_afterRender);
    _quickView.reset();

    // Terminate the stream
    if (_sendFuture.valid())
        _sendFuture.wait();
    _eventReceiver.reset();
    _stream.reset();

    emit streamClosed();
}

void QmlStreamer::Impl::_onPressed(const QPointF pos)
{
    _startMouseModeSwitchDetection(_mapToScene(pos));
}

void QmlStreamer::Impl::_onMoved(const QPointF pos)
{
    if (_mouseMode)
    {
        _sendMouseEvent(QEvent::MouseMove, _mapToScene(pos));
        return;
    }
    if (_mouseModeTimer.isActive())
        _touchCurrentPos = _mapToScene(pos);
}

void QmlStreamer::Impl::_onReleased(const QPointF pos)
{
    _mouseModeTimer.stop();
    if (_mouseMode)
    {
        _sendMouseEvent(QEvent::MouseButtonRelease, _mapToScene(pos));
        _switchBackToTouchMode();
    }
}

void QmlStreamer::Impl::_onKeyPress(int key, int modifiers, QString text)
{
    QKeyEvent keyEvent_(QEvent::KeyPress, key, (Qt::KeyboardModifiers)modifiers,
                        text);
    _send(keyEvent_);
}

void QmlStreamer::Impl::_onKeyRelease(int key, int modifiers, QString text)
{
    QKeyEvent keyEvent_(QEvent::KeyRelease, key,
                        (Qt::KeyboardModifiers)modifiers, text);
    _send(keyEvent_);
}

void QmlStreamer::Impl::_send(QKeyEvent& keyEvent_)
{
    // Work around missing key event support in Qt for offscreen windows.

    if (_sendToWebengineviewItems(keyEvent_))
        return;

    const auto items = _quickView->getRootItem()->findChildren<QQuickItem*>();
    for (auto item : items)
    {
        if (item->hasFocus())
        {
            _quickView->sendEvent(item, &keyEvent_);
            if (keyEvent_.isAccepted())
                break;
        }
    }
}

bool QmlStreamer::Impl::_sendToWebengineviewItems(QKeyEvent& keyEvent_)
{
    // Special handling for WebEngineView in offscreen Qml windows.

    const auto root = _quickView->getRootItem();
    const auto items =
        root->findChildren<QQuickItem*>(WEBENGINEVIEW_OBJECT_NAME);
    for (auto webengineviewItem : items)
    {
        if (webengineviewItem->hasFocus())
        {
            for (auto child : webengineviewItem->childItems())
                QCoreApplication::instance()->sendEvent(child, &keyEvent_);
            if (keyEvent_.isAccepted())
                return true;
        }
    }
    return false;
}

void QmlStreamer::Impl::_setupSizeHintsConnections()
{
    connect(_quickView.get(), &QQuickWindow::minimumWidthChanged,
            [this](int size_) { _sizeHints.minWidth = size_; });
    connect(_quickView.get(), &QQuickWindow::minimumHeightChanged,
            [this](int size_) { _sizeHints.minHeight = size_; });
    connect(_quickView.get(), &QQuickWindow::maximumWidthChanged,
            [this](int size_) { _sizeHints.maxWidth = size_; });
    connect(_quickView.get(), &QQuickWindow::maximumHeightChanged,
            [this](int size_) { _sizeHints.maxHeight = size_; });
    connect(_quickView.get(), &QQuickWindow::widthChanged,
            [this](int size_) { _sizeHints.preferredWidth = size_; });
    connect(_quickView.get(), &QQuickWindow::heightChanged,
            [this](int size_) { _sizeHints.preferredHeight = size_; });
}

std::string QmlStreamer::Impl::_getDeflectStreamIdentifier() const
{
    if (!_streamId.empty())
        return _streamId;

    const auto streamId = _quickView->getRootItem()->objectName().toStdString();
    return streamId.empty() ? DEFAULT_STREAM_ID : streamId;
}

bool QmlStreamer::Impl::_setupDeflectStream()
{
    if (!_stream)
        _stream.reset(new Stream(_getDeflectStreamIdentifier(), _streamHost));

    if (!_stream->isConnected())
        return false;

    if (!_stream->registerForEvents())
        return false;

    if (_sizeHints != SizeHints())
        _stream->sendSizeHints(_sizeHints);

    _eventReceiver.reset(new EventReceiver(*_stream));

    // inject touch events
    _connectTouchInjector();

    // used for mouse mode switch for Qml WebengineView
    connect(_eventReceiver.get(), &EventReceiver::pressed, this,
            &QmlStreamer::Impl::_onPressed);
    connect(_eventReceiver.get(), &EventReceiver::released, this,
            &QmlStreamer::Impl::_onReleased);
    connect(_eventReceiver.get(), &EventReceiver::moved, this,
            &QmlStreamer::Impl::_onMoved);

    // handle view resize
    connect(_eventReceiver.get(), &EventReceiver::resized,
            [this](const QSize size) { _quickView->resize(size); });

    // inject key events
    connect(_eventReceiver.get(), &EventReceiver::keyPress, this,
            &QmlStreamer::Impl::_onKeyPress);
    connect(_eventReceiver.get(), &EventReceiver::keyRelease, this,
            &QmlStreamer::Impl::_onKeyRelease);

    // Forward gestures to Qml context object
    connect(_eventReceiver.get(), &EventReceiver::swipeDown, _qmlGestures.get(),
            &QmlGestures::swipeDown);
    connect(_eventReceiver.get(), &EventReceiver::swipeUp, _qmlGestures.get(),
            &QmlGestures::swipeUp);
    connect(_eventReceiver.get(), &EventReceiver::swipeLeft, _qmlGestures.get(),
            &QmlGestures::swipeLeft);
    connect(_eventReceiver.get(), &EventReceiver::swipeRight,
            _qmlGestures.get(), &QmlGestures::swipeRight);

    connect(_eventReceiver.get(), &EventReceiver::closed, this,
            &QmlStreamer::Impl::_onStreamClosed);

    return true;
}

void QmlStreamer::Impl::_connectTouchInjector()
{
    connect(_eventReceiver.get(), &EventReceiver::touchPointAdded,
            _touchInjector.get(), &TouchInjector::addTouchPoint);
    connect(_eventReceiver.get(), &EventReceiver::touchPointUpdated,
            _touchInjector.get(), &TouchInjector::updateTouchPoint);
    connect(_eventReceiver.get(), &EventReceiver::touchPointRemoved,
            _touchInjector.get(), &TouchInjector::removeTouchPoint);
}

void QmlStreamer::Impl::_setupMouseModeSwitcher()
{
    _mouseModeTimer.setSingleShot(true);
    _mouseModeTimer.setInterval(TOUCH_TAPANDHOLD_TIMEOUT_MS);
    connect(&_mouseModeTimer, &QTimer::timeout, [this] {
        if (_touchIsTapAndHold())
            _switchFromTouchToMouseMode();
    });
}

void QmlStreamer::Impl::_startMouseModeSwitchDetection(const QPointF& pos)
{
    const auto item = _quickView->getRootItem()->childAt(pos.x(), pos.y());
    if (item->objectName() == WEBENGINEVIEW_OBJECT_NAME)
    {
        _mouseModeTimer.start();
        _touchStartPos = pos;
        _touchCurrentPos = pos;
    }
}

bool QmlStreamer::Impl::_touchIsTapAndHold()
{
    const auto distance = (_touchCurrentPos - _touchStartPos).manhattanLength();
    return distance < TOUCH_TAPANDHOLD_DIST_PX;
}

void QmlStreamer::Impl::_switchFromTouchToMouseMode()
{
    _eventReceiver->disconnect(_touchInjector.get());
    _touchInjector->removeAllTouchPoints();
    _mouseMode = true;
    _sendMouseEvent(QEvent::MouseButtonPress, _touchCurrentPos);
}

void QmlStreamer::Impl::_switchBackToTouchMode()
{
    _mouseMode = false;
    _connectTouchInjector();
}

QPointF QmlStreamer::Impl::_mapToScene(const QPointF& normalizedPos) const
{
    return {normalizedPos.x() * _quickView->width(),
            normalizedPos.y() * _quickView->height()};
}

void QmlStreamer::Impl::_sendMouseEvent(const QEvent::Type eventType,
                                        const QPointF& pos)
{
    auto e = new QMouseEvent(eventType, pos, Qt::LeftButton, Qt::LeftButton,
                             Qt::NoModifier);
    QCoreApplication::postEvent(_quickView.get(), e);
}
}
}
