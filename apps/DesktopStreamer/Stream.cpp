/*********************************************************************/
/* Copyright (c) 2016-2016, EPFL/Blue Brain Project                  */
/*                          Stefan.Eilemann@epfl.ch                  */
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
/* or implied, of The Ecole Polytechnique Federal de Lausanne.       */
/*********************************************************************/

#include "Stream.h"
#include "MainWindow.h"

#ifdef __APPLE__
#  ifdef DEFLECT_USE_QT5MACEXTRAS
#    include "DesktopWindowsModel.h"
#  endif
#  if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_9
#    include <CoreGraphics/CoreGraphics.h>
#  else
#    include <ApplicationServices/ApplicationServices.h>
#  endif
#endif
#include <QPainter>
#include <QScreen>
#include <queue>

#define CURSOR_IMAGE_FILE     ":/cursor.png"
#define CURSOR_IMAGE_SIZE     20

class Stream::Impl
{
public:
    Impl( Stream& stream, const MainWindow& parent,
          const QPersistentModelIndex window, const int pid )
    : _stream( stream )
    , _parent( parent )
    , _window( window )
    , _cursor( QImage( CURSOR_IMAGE_FILE ).scaled(
                   CURSOR_IMAGE_SIZE * parent.devicePixelRatio(),
                   CURSOR_IMAGE_SIZE * parent.devicePixelRatio(),
                   Qt::KeepAspectRatio ))
    , _pid( pid )
    {}

bool processEvents( const bool interact )
{
    while( _stream.hasEvent( ))
    {
        const deflect::Event event = _stream.getEvent();

        if( event.type == deflect::Event::EVT_CLOSE )
            return false;

        if( !_stream.isRegisteredForEvents() || !interact )
            continue;

        switch( event.type )
        {
        case deflect::Event::EVT_PRESS:
            _sendMouseMoveEvent( event.mouseX, event.mouseY );
            _sendMousePressEvent( event.mouseX, event.mouseY );
            break;
        case deflect::Event::EVT_RELEASE:
            _sendMouseMoveEvent( event.mouseX, event.mouseY );
            _sendMouseReleaseEvent( event.mouseX, event.mouseY );
            break;
        case deflect::Event::EVT_DOUBLECLICK:
            _sendMouseDoubleClickEvent( event.mouseX, event.mouseY );
            break;

        case deflect::Event::EVT_MOVE:
            _sendMouseMoveEvent( event.mouseX, event.mouseY );
            break;
        case deflect::Event::EVT_CLICK:
        case deflect::Event::EVT_WHEEL:
        case deflect::Event::EVT_SWIPE_LEFT:
        case deflect::Event::EVT_SWIPE_RIGHT:
        case deflect::Event::EVT_SWIPE_UP:
        case deflect::Event::EVT_SWIPE_DOWN:
        case deflect::Event::EVT_KEY_PRESS:
        case deflect::Event::EVT_KEY_RELEASE:
        case deflect::Event::EVT_VIEW_SIZE_CHANGED:
        case deflect::Event::EVT_TAP_AND_HOLD:
        case deflect::Event::EVT_PAN:
        case deflect::Event::EVT_PINCH:
        default:
            break;
        }
    }
    return true;
}

std::string update()
{
    QPixmap pixmap;

    if( !_window.isValid( ) || _window.row() == 0 )
    {
        pixmap = QApplication::primaryScreen()->grabWindow( 0 );
        _windowRect = QRect( 0, 0, pixmap.width() / _parent.devicePixelRatio(),
                             pixmap.height() / _parent.devicePixelRatio( ));
    }
    else
    {
#ifdef DEFLECT_USE_QT5MACEXTRAS
        const QAbstractItemModel* model = _parent.getItemModel();
        pixmap = model->data( _window,
                          DesktopWindowsModel::ROLE_PIXMAP ).value< QPixmap >();
        _windowRect = model->data( _window,
                              DesktopWindowsModel::ROLE_RECT ).value< QRect >();
#endif
    }

    if( pixmap.isNull( ))
        return "Got no pixmap for desktop or window";

    QImage image = pixmap.toImage();
#ifdef DEFLECT_USE_QT5MACEXTRAS
    const QAbstractItemModel* model = _parent.getItemModel();
    // render mouse cursor only on active window and full desktop streams
    if( DesktopWindowsModel::isActive( _pid ) ||
        model->data( _window, Qt::DisplayRole ) == "Desktop" )
#endif
    {
        const QPoint mousePos = ( QCursor::pos() - _windowRect.topLeft( )) *
                                _parent.devicePixelRatio() -
                                QPoint( _cursor.width()/2, _cursor.height()/2 );
        QPainter painter( &image );
        painter.drawImage( mousePos, _cursor );
        painter.end(); // Make sure to release the QImage before using it
    }

    if( image == _image )
        return std::string(); // OPT: Image is unchanged
    _image = image;

    // QImage Format_RGB32 (0xffRRGGBB) corresponds to GL_BGRA == deflect::BGRA
    deflect::ImageWrapper deflectImage( (const void*)image.bits(),
                                        image.width(), image.height(),
                                        deflect::BGRA );
    deflectImage.compressionPolicy = deflect::COMPRESSION_ON;

    if( !_stream.send( deflectImage ) || !_stream.finishFrame( ))
        return "Streaming failure, connection closed";
    return std::string();
}

#ifdef __APPLE__
void _sendMouseEvent( const CGEventType type, const CGMouseButton button,
                      const CGPoint point )
{
    CGEventRef event = CGEventCreateMouseEvent( 0, type, point, button );
    CGEventSetType( event, type );

#ifdef DEFLECT_USE_QT5MACEXTRAS
    // If the destination app is not active, store the event in a queue and
    // consume it after it's been activated (next iteration of main run loop)
    if( !DesktopWindowsModel::isActive( _pid ))
    {
        DesktopWindowsModel::activate( _pid );
        EventQueue().swap( _events ); // empty the queue
        _events.push( event );
        return;
    }
#endif
    _events.push( event );

    while( !_events.empty( ))
    {
        event = _events.front();
        CGEventPost( kCGHIDEventTap, event );
        CFRelease( event );
        _events.pop();
    }
    return;
}

void _sendMousePressEvent( const float x, const float y )
{
    CGPoint point;
    point.x = _windowRect.topLeft().x() + x * _windowRect.width();
    point.y = _windowRect.topLeft().y() + y * _windowRect.height();
    _sendMouseEvent( kCGEventLeftMouseDown, kCGMouseButtonLeft, point );
}

void _sendMouseMoveEvent( const float x, const float y )
{
    CGPoint point;
    point.x = _windowRect.topLeft().x() + x * _windowRect.width();
    point.y = _windowRect.topLeft().y() + y * _windowRect.height();
    _sendMouseEvent( kCGEventMouseMoved, kCGMouseButtonLeft, point );
}

void _sendMouseReleaseEvent( const float x, const float y )
{
    CGPoint point;
    point.x = _windowRect.topLeft().x() + x * _windowRect.width();
    point.y = _windowRect.topLeft().y() + y * _windowRect.height();
    _sendMouseEvent( kCGEventLeftMouseUp, kCGMouseButtonLeft, point );
}

void _sendMouseDoubleClickEvent( const float x, const float y )
{
    CGPoint point;
    point.x = _windowRect.topLeft().x() + x * _windowRect.width();
    point.y = _windowRect.topLeft().y() + y * _windowRect.height();
    CGEventRef event = CGEventCreateMouseEvent( 0, kCGEventLeftMouseDown,
                                                point, kCGMouseButtonLeft );

    CGEventSetIntegerValueField( event, kCGMouseEventClickState, 2 );
    CGEventPost( kCGHIDEventTap, event );

    CGEventSetType( event, kCGEventLeftMouseUp );
    CGEventPost( kCGHIDEventTap, event );

    CGEventSetType( event, kCGEventLeftMouseDown );
    CGEventPost( kCGHIDEventTap, event );

    CGEventSetType( event, kCGEventLeftMouseUp );
    CGEventPost( kCGHIDEventTap, event );
    CFRelease( event );
}
#else
void _sendMousePressEvent( const float, const float ) {}
void _sendMouseMoveEvent( const float, const float ) {}
void _sendMouseReleaseEvent( const float, const float ) {}
void _sendMouseDoubleClickEvent( const float, const float ) {}
#endif

    Stream& _stream;
    const MainWindow& _parent;
    const QPersistentModelIndex _window;
    QRect _windowRect; // position on host in non-retina coordinates
    const QImage _cursor;
    QImage _image;
    const int _pid;
#ifdef __APPLE__
    typedef std::queue< CGEventRef > EventQueue;
    EventQueue _events;
#endif
};

Stream::Stream( const MainWindow& parent, const QPersistentModelIndex window,
                const std::string& name, const std::string& host, const int pid)
    : deflect::Stream( name, host )
    , _impl( new Impl( *this, parent, window, pid ))
{}

Stream::~Stream()
{}

std::string Stream::update()
{
    return _impl->update();
}

bool Stream::processEvents( const bool interact )
{
    return _impl->processEvents( interact );
}

const QPersistentModelIndex& Stream::getIndex() const
{
    return _impl->_window;
}
