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

#ifndef QMLSTREAMERIMPL_H
#define QMLSTREAMERIMPL_H

#include <QTimer>
#include <QWindow>

#include "QmlStreamer.h"
#include "../SizeHints.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLContext)
QT_FORWARD_DECLARE_CLASS(QOpenGLFramebufferObject)
QT_FORWARD_DECLARE_CLASS(QOffscreenSurface)
QT_FORWARD_DECLARE_CLASS(QQuickRenderControl)
QT_FORWARD_DECLARE_CLASS(QQuickWindow)
QT_FORWARD_DECLARE_CLASS(QQmlEngine)
QT_FORWARD_DECLARE_CLASS(QQmlComponent)
QT_FORWARD_DECLARE_CLASS(QQuickItem)

namespace deflect
{

class Stream;

namespace qt
{

class EventReceiver;
class QmlGestures;
class TouchInjector;

class QmlStreamer::Impl : public QWindow
{
    Q_OBJECT

public:
    Impl( const QString& qmlFile, const std::string& streamHost,
          const std::string& streamId );

    ~Impl();

    QQuickItem* getRootItem() { return _rootItem; }
    QQmlEngine* getQmlEngine() { return _qmlEngine; }
    Stream* getStream() { return _stream; }

protected:
    void resizeEvent( QResizeEvent* e ) final;
    void mousePressEvent( QMouseEvent* e ) final;
    void mouseReleaseEvent( QMouseEvent* e ) final;
    void timerEvent( QTimerEvent* e ) final;

private slots:
    bool _setupRootItem();

    void _createFbo();
    void _destroyFbo();
    void _render();
    void _requestRender();

    void _onPressed( QPointF position );
    void _onReleased( QPointF position );
    void _onMoved( QPointF position );

    void _onResized( QSize newSize );

    void _onKeyPress( int key, int modifiers, QString text );
    void _onKeyRelease( int key, int modifiers, QString text );

signals:
    void streamClosed();

private:
    void _send( QKeyEvent& keyEvent );
    bool _sendToWebengineviewItems( QKeyEvent& keyEvent );
    std::string _getDeflectStreamIdentifier() const;
    bool _setupDeflectStream();
    void _updateSizes( const QSize& size );

    void _connectTouchInjector();
    void _startMouseModeSwitchDetection( const QPointF& pos );
    bool _touchIsTapAndHold();
    void _switchFromTouchToMouseMode();
    void _switchBackToTouchMode();
    void _sendMouseEvent( QEvent::Type eventType, const QPointF& pos );

    QPointF _mapToScene( const QPointF& normalizedPos ) const;

    QOpenGLContext* _context;
    QOffscreenSurface* _offscreenSurface;
    QQuickRenderControl* _renderControl;
    QQuickWindow* _quickWindow;
    QQmlEngine* _qmlEngine;
    QQmlComponent* _qmlComponent;
    QQuickItem* _rootItem;
    QOpenGLFramebufferObject* _fbo;

    int _renderTimer;
    int _stopRenderingDelayTimer;

    Stream* _stream;
    EventReceiver* _eventHandler;
    QmlGestures* _qmlGestures;
    TouchInjector* _touchInjector;
    bool _streaming;
    const std::string _streamHost;
    const std::string _streamId;
    SizeHints _sizeHints;

    QTimer _mouseModeTimer;
    bool _mouseMode;
    QPointF _touchStartPos;
    QPointF _touchCurrentPos;
};

}
}

#endif
