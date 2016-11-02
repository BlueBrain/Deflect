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
#include "QuickRenderer.h"
#include "QmlGestures.h"
#include "TouchInjector.h"

#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QThread>

namespace
{
const std::string DEFAULT_STREAM_ID( "QmlStreamer" );
const QString GESTURES_CONTEXT_PROPERTY( "deflectgestures" );
const QString WEBENGINEVIEW_OBJECT_NAME( "webengineview" );
const int TOUCH_TAPANDHOLD_DIST_PX = 20;
const int TOUCH_TAPANDHOLD_TIMEOUT_MS = 200;

deflect::Stream::Future make_ready_future( const bool value )
{
    std::promise< bool > promise;
    promise.set_value( value );
    return promise.get_future();
}
}

class RenderControl : public QQuickRenderControl
{
public:
    RenderControl( QWindow* w )
        : _window( w )
    {}

    QWindow* renderWindow( QPoint* offset ) final
    {
        if( offset )
            *offset = QPoint( 0, 0 );
        return _window;
    }

private:
    QWindow* _window;
};

namespace deflect
{
namespace qt
{

QmlStreamer::Impl::Impl( const QString& qmlFile, const std::string& streamHost,
                         const std::string& streamId )
    : QWindow()
    , _renderControl( new RenderControl( this ))
    // Create a QQuickWindow that is associated with out render control. Note
    // that this window never gets created or shown, meaning that it will never
    // get an underlying native (platform) window.
    , _quickWindow( new QQuickWindow( _renderControl ))
    , _qmlEngine( new QQmlEngine )
    , _qmlComponent( new QQmlComponent( _qmlEngine, QUrl( qmlFile )))
    , _qmlGestures( new QmlGestures )
    , _touchInjector( new TouchInjector( *_quickWindow,
                                         std::bind( &Impl::_mapToScene, this,
                                                    std::placeholders::_1 )))
    , _streamHost( streamHost )
    , _streamId( streamId )
    , _sendFuture( make_ready_future( true ))
{
    _mouseModeTimer.setSingleShot( true );
    _mouseModeTimer.setInterval( TOUCH_TAPANDHOLD_TIMEOUT_MS );

    // Expose stream gestures to qml objects
    _qmlEngine->rootContext()->setContextProperty( GESTURES_CONTEXT_PROPERTY,
                                                   _qmlGestures );

    setSurfaceType( QSurface::OpenGLSurface );

    connect( &_mouseModeTimer, &QTimer::timeout, [&] {
        if( _touchIsTapAndHold( ))
            _switchFromTouchToMouseMode();
    });

    if( !_qmlEngine->incubationController( ))
        _qmlEngine->setIncubationController( _quickWindow->incubationController( ));

    // Now hook up the signals. For simplicy we don't differentiate between
    // renderRequested (only render is needed, no sync) and sceneChanged (polish
    // and sync is needed too).
    connect( _renderControl, &QQuickRenderControl::renderRequested,
             this, &QmlStreamer::Impl::_requestRender );
    connect( _renderControl, &QQuickRenderControl::sceneChanged,
             this, &QmlStreamer::Impl::_requestRender );

    // remote URL to QML components are loaded asynchronously
    if( _qmlComponent->isLoading( ))
        connect( _qmlComponent, &QQmlComponent::statusChanged,
                 this, &QmlStreamer::Impl::_setupRootItem );
    else if( !_setupRootItem( ))
        throw std::runtime_error( "Failed to setup/load QML" );
}

QmlStreamer::Impl::~Impl()
{
    _quickRenderer->stop();
#ifdef DEFLECTQT_MULTITHREADED
    _quickRendererThread.quit();
    _quickRendererThread.wait();
#endif

    // delete first to free scenegraph resources for following destructions
    delete _renderControl;

    delete _rootItem;
    delete _qmlComponent;
    delete _quickWindow;
    delete _qmlEngine;

    delete _quickRenderer;
}

void QmlStreamer::Impl::_requestRender()
{
    killTimer( _stopRenderingDelayTimer );
    _stopRenderingDelayTimer = 0;

    if( _renderTimer == 0 )
        _renderTimer = startTimer( 5, Qt::PreciseTimer );
}

void QmlStreamer::Impl::_initRenderer()
{
    _updateSizes( QSize( _rootItem->width(), _rootItem->height( )));

#ifdef DEFLECTQT_MULTITHREADED
    _quickRenderer = new QuickRenderer( *_quickWindow, *_renderControl, true,
                                        true );

    // Call required to make QtGraphicalEffects work in the initial scene.
    _renderControl->prepareThread( &_quickRendererThread );

    _quickRenderer->moveToThread( &_quickRendererThread );

    _quickRendererThread.setObjectName( "Render" );
    _quickRendererThread.start();
#else
    _quickRenderer = new QuickRenderer( *_quickWindow, *_renderControl, false,
                                        true );
#endif

    _quickRenderer->init();

    connect( _quickRenderer, &QuickRenderer::afterRender,
             this, &QmlStreamer::Impl::_afterRender, Qt::DirectConnection );
    connect( _quickRenderer, &QuickRenderer::afterStop,
             this, &QmlStreamer::Impl::_afterStop, Qt::DirectConnection );
}

void QmlStreamer::Impl::_render()
{
    // Initialize the render control and our OpenGL resources. Do this as late
    // as possible to use the proper size reported by the rootItem.
    if( !_quickRenderer )
        _initRenderer();

    _renderControl->polishItems();
    _quickRenderer->render();

    if( _stopRenderingDelayTimer == 0 )
        _stopRenderingDelayTimer = startTimer( 5000 /*ms*/ );
}

void QmlStreamer::Impl::_afterRender()
{
    if( _stream )
        _streaming = _sendFuture.get();
    else
    {
        if( !_setupDeflectStream( ))
        {
            qWarning() << "Could not setup Deflect stream";
            _streaming = false;
            return;
        }
        _streaming = true;
    }

    if( !_streaming )
        return;

    _quickRenderer->context()->functions()->glFlush();
    _image = _quickRenderer->fbo()->toImage();
    if( _image.isNull( ))
    {
        qDebug() << "Empty image not streamed";
        return;
    }

    ImageWrapper imageWrapper( _image.constBits(), _image.width(),
                               _image.height(), BGRA );
    imageWrapper.compressionPolicy = COMPRESSION_ON;
    imageWrapper.compressionQuality = 80;

    if( _asyncSend )
        _sendFuture = _stream->asyncSend( imageWrapper );
    else
        _sendFuture = make_ready_future( _stream->send( imageWrapper ) &&
                                         _stream->finishFrame( ));
}

void QmlStreamer::Impl::_afterStop()
{
    if( _sendFuture.valid() && _asyncSend )
        _sendFuture.wait();
    delete _eventHandler;
    delete _stream;
}

void QmlStreamer::Impl::_onStreamClosed()
{
    killTimer( _renderTimer );
    _renderTimer = 0;
    killTimer( _stopRenderingDelayTimer );
    _stopRenderingDelayTimer = 0;
    emit streamClosed();
}

void QmlStreamer::Impl::_onPressed( const QPointF pos )
{
    _startMouseModeSwitchDetection( _mapToScene( pos ));
}

void QmlStreamer::Impl::_onMoved( const QPointF pos )
{
    if( _mouseMode )
    {
        _sendMouseEvent( QEvent::MouseMove, _mapToScene( pos ));
        return;
    }
    if( _mouseModeTimer.isActive( ))
        _touchCurrentPos = _mapToScene( pos );
}

void QmlStreamer::Impl::_onReleased( const QPointF pos )
{
    _mouseModeTimer.stop();
    if( _mouseMode )
    {
        _sendMouseEvent( QEvent::MouseButtonRelease, _mapToScene( pos ));
        _switchBackToTouchMode();
    }
}

void QmlStreamer::Impl::_onResized( const QSize newSize )
{
    QCoreApplication::postEvent( this, new QResizeEvent( newSize, size( )));
}

void QmlStreamer::Impl::_onKeyPress( int key, int modifiers, QString text )
{
    QKeyEvent keyEvent_( QEvent::KeyPress, key,
                         (Qt::KeyboardModifiers)modifiers, text );
    _send( keyEvent_ );
}

void QmlStreamer::Impl::_onKeyRelease( int key, int modifiers, QString text )
{
    QKeyEvent keyEvent_( QEvent::KeyRelease, key,
                         (Qt::KeyboardModifiers)modifiers, text );
    _send( keyEvent_ );
}

void QmlStreamer::Impl::_send( QKeyEvent& keyEvent_ )
{
    // Work around missing key event support in Qt for offscreen windows.

    if( _sendToWebengineviewItems( keyEvent_ ))
        return;

    const auto items = _rootItem->findChildren<QQuickItem*>();
    for( auto item : items )
    {
        if( item->hasFocus( ))
        {
            _quickWindow->sendEvent( item, &keyEvent_ );
            if( keyEvent_.isAccepted())
                break;
        }
    }
}

bool QmlStreamer::Impl::_sendToWebengineviewItems( QKeyEvent& keyEvent_ )
{
    // Special handling for WebEngineView in offscreen Qml windows.

    const auto items =
            _rootItem->findChildren<QQuickItem*>( WEBENGINEVIEW_OBJECT_NAME );
    for( auto webengineviewItem : items )
    {
        if( webengineviewItem->hasFocus( ))
        {
            for( auto child : webengineviewItem->childItems( ))
                QCoreApplication::instance()->sendEvent( child, &keyEvent_ );
            if( keyEvent_.isAccepted( ))
                return true;
        }
    }
    return false;
}

bool QmlStreamer::Impl::_setupRootItem()
{
    disconnect( _qmlComponent, &QQmlComponent::statusChanged,
                this, &QmlStreamer::Impl::_setupRootItem );

    if( _qmlComponent->isError( ))
    {
        QList< QQmlError > errorList = _qmlComponent->errors();
        foreach( const QQmlError &error, errorList )
            qWarning() << error.url() << error.line() << error;
        return false;
    }

    QObject* rootObject = _qmlComponent->create();
    if( _qmlComponent->isError( ))
    {
        QList< QQmlError > errorList = _qmlComponent->errors();
        foreach( const QQmlError &error, errorList )
            qWarning() << error.url() << error.line() << error;
        return false;
    }

    _rootItem = qobject_cast< QQuickItem* >( rootObject );
    if( !_rootItem )
    {
        delete rootObject;
        return false;
    }

    connect( _quickWindow, &QQuickWindow::minimumWidthChanged,
             [this]( int size_ ) { _sizeHints.minWidth = size_; } );
    connect( _quickWindow, &QQuickWindow::minimumHeightChanged,
             [this]( int size_ ) { _sizeHints.minHeight = size_; } );
    connect( _quickWindow, &QQuickWindow::maximumWidthChanged,
             [this]( int size_ ) { _sizeHints.maxWidth = size_; } );
    connect( _quickWindow, &QQuickWindow::maximumHeightChanged,
             [this]( int size_ ) { _sizeHints.maxHeight = size_; } );
    connect( _quickWindow, &QQuickWindow::widthChanged,
             [this]( int size_ ) { _sizeHints.preferredWidth = size_; } );
    connect( _quickWindow, &QQuickWindow::heightChanged,
             [this]( int size_ ) { _sizeHints.preferredHeight = size_; } );

    // The root item is ready. Associate it with the window and get sizeHints.
    _rootItem->setParentItem( _quickWindow->contentItem( ));

    return true;
}

std::string QmlStreamer::Impl::_getDeflectStreamIdentifier() const
{
    if( !_streamId.empty( ))
        return _streamId;

    const std::string streamId = _rootItem->objectName().toStdString();
    return streamId.empty() ? DEFAULT_STREAM_ID : streamId;
}

bool QmlStreamer::Impl::_setupDeflectStream()
{
    if( !_stream )
        _stream = new Stream( _getDeflectStreamIdentifier(), _streamHost );

    if( !_stream->isConnected( ))
        return false;

    if( !_stream->registerForEvents( ))
        return false;

    if( _sizeHints != SizeHints( ))
        _stream->sendSizeHints( _sizeHints );

    _streaming = true;
    _eventHandler = new EventReceiver( *_stream );

    // inject touch events
    _connectTouchInjector();

    // used for mouse mode switch for Qml WebengineView
    connect( _eventHandler, &EventReceiver::pressed,
             this, &QmlStreamer::Impl::_onPressed );
    connect( _eventHandler, &EventReceiver::released,
             this, &QmlStreamer::Impl::_onReleased );
    connect( _eventHandler, &EventReceiver::moved,
             this, &QmlStreamer::Impl::_onMoved );

    // handle view resize
    connect( _eventHandler, &EventReceiver::resized,
             this, &QmlStreamer::Impl::_onResized );

    // inject key events
    connect( _eventHandler, &EventReceiver::keyPress,
             this, &QmlStreamer::Impl::_onKeyPress );
    connect( _eventHandler, &EventReceiver::keyRelease,
             this, &QmlStreamer::Impl::_onKeyRelease );

    // Forward gestures to Qml context object
    connect( _eventHandler, &EventReceiver::swipeDown,
             _qmlGestures, &QmlGestures::swipeDown );
    connect( _eventHandler, &EventReceiver::swipeUp,
             _qmlGestures, &QmlGestures::swipeUp );
    connect( _eventHandler, &EventReceiver::swipeLeft,
             _qmlGestures, &QmlGestures::swipeLeft );
    connect( _eventHandler, &EventReceiver::swipeRight,
             _qmlGestures, &QmlGestures::swipeRight );

    connect( _eventHandler, &EventReceiver::closed,
             this, &QmlStreamer::Impl::_onStreamClosed );

    return true;
}

void QmlStreamer::Impl::_updateSizes( const QSize& size_ )
{
    resize( size_ );
    _quickWindow->blockSignals( true );
    _quickWindow->resize( size_ );
    _quickWindow->blockSignals( false );
    // emulate QQuickView::ResizeMode:SizeRootObjectToView
    if( _rootItem )
    {
        _rootItem->setWidth( size_.width( ));
        _rootItem->setHeight( size_.height( ));
    }
}

void QmlStreamer::Impl::_connectTouchInjector()
{
    connect( _eventHandler, &EventReceiver::touchPointAdded,
             _touchInjector, &TouchInjector::addTouchPoint );
    connect( _eventHandler, &EventReceiver::touchPointUpdated,
             _touchInjector, &TouchInjector::updateTouchPoint );
    connect( _eventHandler, &EventReceiver::touchPointRemoved,
             _touchInjector, &TouchInjector::removeTouchPoint );
}

void QmlStreamer::Impl::_startMouseModeSwitchDetection( const QPointF& pos )
{
    const auto item = _rootItem->childAt( pos.x(), pos.y( ));
    if( item->objectName() == WEBENGINEVIEW_OBJECT_NAME )
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
    _eventHandler->disconnect( _touchInjector );
    _touchInjector->removeAllTouchPoints();
    _mouseMode = true;
    _sendMouseEvent( QEvent::MouseButtonPress, _touchCurrentPos );
}

void QmlStreamer::Impl::_switchBackToTouchMode()
{
    _mouseMode = false;
    _connectTouchInjector();
}

QPointF QmlStreamer::Impl::_mapToScene( const QPointF& normalizedPos ) const
{
    return QPointF{ normalizedPos.x() * width(), normalizedPos.y() * height() };
}

void QmlStreamer::Impl::_sendMouseEvent( const QEvent::Type eventType,
                                         const QPointF& pos )
{
    auto e = new QMouseEvent( eventType, pos, Qt::LeftButton, Qt::LeftButton,
                              Qt::NoModifier );
    QCoreApplication::postEvent( _quickWindow, e );
}

void QmlStreamer::Impl::resizeEvent( QResizeEvent* e )
{
    _updateSizes( e->size( ));
}

void QmlStreamer::Impl::mousePressEvent( QMouseEvent* e )
{
    // Use the constructor taking localPos and screenPos. That puts localPos
    // into the event's localPos and windowPos, and screenPos into the event's
    // screenPos. This way the windowPos in e is ignored and is replaced by
    // localPos. This is necessary because QQuickWindow thinks of itself as a
    // top-level window always.
    QMouseEvent mappedEvent( e->type(), e->localPos(), e->screenPos(),
                             e->button(), e->buttons(), e->modifiers( ));
    QCoreApplication::sendEvent( _quickWindow, &mappedEvent );
}

void QmlStreamer::Impl::mouseReleaseEvent( QMouseEvent* e )
{
    QMouseEvent mappedEvent( e->type(), e->localPos(), e->screenPos(),
                             e->button(), e->buttons(), e->modifiers( ));
    QCoreApplication::sendEvent( _quickWindow, &mappedEvent );
}

void QmlStreamer::Impl::timerEvent( QTimerEvent* e )
{
    if( e->timerId() == _renderTimer )
        _render();
    else if( e->timerId() == _stopRenderingDelayTimer )
    {
        killTimer( _renderTimer );
        killTimer( _stopRenderingDelayTimer );
        _renderTimer = 0;
        _stopRenderingDelayTimer = 0;
    }
}

}
}
