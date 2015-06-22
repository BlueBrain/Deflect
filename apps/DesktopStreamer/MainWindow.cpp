/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
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

#include "MainWindow.h"

#include "DesktopSelectionWindow.h"
#include "DesktopSelectionRectangle.h"

#include <deflect/Server.h>
#include <deflect/Stream.h>

#include <iostream>

#ifdef _WIN32
typedef __int32 int32_t;
#  include <windows.h>
#else
#  include <stdint.h>
#  include <unistd.h>
#endif

#ifdef __APPLE__
#  include <CoreGraphics/CoreGraphics.h>
#endif

#define SHARE_DESKTOP_UPDATE_DELAY      1
#define SERVUS_BROWSE_DELAY           100
#define FRAME_RATE_AVERAGE_NUM_FRAMES  10

#define DEFAULT_HOST_ADDRESS  "bbpav02.epfl.ch"
#define CURSOR_IMAGE_FILE     ":/cursor.png"

MainWindow::MainWindow()
    : _stream( 0 )
    , _desktopSelectionWindow( new DesktopSelectionWindow( ))
    , _x( 0 )
    , _y( 0 )
    , _width( 0 )
    , _height( 0 )
#ifdef DEFLECT_USE_SERVUS
    , _servus( deflect::Server::serviceName )
#endif
{
    _generateCursorImage();
    _setupUI();

    // Receive changes from the selection rectangle
    connect( _desktopSelectionWindow->getSelectionRectangle(),
             SIGNAL( coordinatesChanged( QRect )),
             this, SLOT( _setCoordinates( QRect )));

    connect( _desktopSelectionWindow, SIGNAL( windowVisible( bool )),
             _showDesktopSelectionWindowAction, SLOT( setChecked( bool )));
}

void MainWindow::_generateCursorImage()
{
    _cursor = QImage( CURSOR_IMAGE_FILE ).scaled( 20, 20, Qt::KeepAspectRatio );
}

void MainWindow::_setupUI()
{
    QWidget* widget = new QWidget();
    QFormLayout* formLayout = new QFormLayout();
    widget->setLayout( formLayout );

    setCentralWidget( widget );

    connect( &_xSpinBox, SIGNAL( editingFinished( )),
             this, SLOT( _updateCoordinates( )));
    connect( &_ySpinBox, SIGNAL( editingFinished( )),
             this, SLOT( _updateCoordinates( )));
    connect( &_widthSpinBox, SIGNAL( editingFinished( )),
             this, SLOT( _updateCoordinates( )));
    connect( &_heightSpinBox, SIGNAL( editingFinished( )),
             this, SLOT( _updateCoordinates( )));

    _hostnameLineEdit.setText( DEFAULT_HOST_ADDRESS );

    char hostname[256] = { 0 };
    gethostname( hostname, 256 );
    _uriLineEdit.setText( hostname );

    const int screen = -1;
    QRect desktopRect = QApplication::desktop()->screenGeometry( screen );

    _xSpinBox.setRange( 0, desktopRect.width( ));
    _ySpinBox.setRange( 0, desktopRect.height( ));
    _widthSpinBox.setRange( 1, desktopRect.width( ));
    _heightSpinBox.setRange( 1, desktopRect.height( ));

    // default to full screen
    _xSpinBox.setValue( 0 );
    _ySpinBox.setValue( 0 );
    _widthSpinBox.setValue( desktopRect.width( ));
    _heightSpinBox.setValue( desktopRect.height( ));

    // call updateCoordinates() to commit coordinates from the UI
    _updateCoordinates();

    // frame rate limiting
    _frameRateSpinBox.setRange( 1, 60 );
    _frameRateSpinBox.setValue( 24 );

    // add widgets to UI
    formLayout->addRow( "Hostname", &_hostnameLineEdit );
    formLayout->addRow( "Stream name", &_uriLineEdit );
    formLayout->addRow( "X", &_xSpinBox );
    formLayout->addRow( "Y", &_ySpinBox );
    formLayout->addRow( "Width", &_widthSpinBox );
    formLayout->addRow( "Height", &_heightSpinBox );
    formLayout->addRow( "Allow desktop interaction", &_eventsBox );
    formLayout->addRow( "Max frame rate", &_frameRateSpinBox );
    formLayout->addRow( "Actual frame rate", &_frameRateLabel );
    _eventsBox.setChecked( true );

    // share desktop action
    _shareDesktopAction = new QAction( "Share Desktop", this );
    _shareDesktopAction->setStatusTip( "Share desktop" );
    _shareDesktopAction->setCheckable( true );
    _shareDesktopAction->setChecked( false );
    connect( _shareDesktopAction, SIGNAL( triggered( bool )), this,
             SLOT( _shareDesktop( bool )));
    connect( this, SIGNAL( streaming( bool )), _shareDesktopAction,
             SLOT( setChecked( bool )));

    // show desktop selection window action
    _showDesktopSelectionWindowAction = new QAction( "Show Rectangle", this );
    _showDesktopSelectionWindowAction->setStatusTip(
                "Show desktop selection rectangle" );
    _showDesktopSelectionWindowAction->setCheckable( true );
    _showDesktopSelectionWindowAction->setChecked( false );
    connect( _showDesktopSelectionWindowAction, SIGNAL( triggered( bool )),
             this, SLOT( _showDesktopSelectionWindow( bool )));

    QToolBar* toolbar = addToolBar( "toolbar" );
    toolbar->addAction( _shareDesktopAction );
    toolbar->addAction( _showDesktopSelectionWindowAction );

    // Update timer
    connect( &_updateTimer, SIGNAL( timeout( )), this, SLOT( _update( )));

#ifdef DEFLECT_USE_SERVUS
    _servus.beginBrowsing( servus::Servus::IF_ALL );
    connect( &_browseTimer, SIGNAL( timeout( )), this, SLOT( _updateServus( )));
    _browseTimer.start( SERVUS_BROWSE_DELAY );
#endif
}

void MainWindow::_startStreaming()
{
    if( _stream )
        return;

    _stream = new deflect::Stream( _uriLineEdit.text().toStdString(),
                                   _hostnameLineEdit.text().toStdString( ));
    if( !_stream->isConnected( ))
    {
        _handleStreamingError( "Could not connect to host!" );
        return;
    }
    _stream->registerForEvents();

#ifdef __APPLE__
    _napSuspender.suspend();
    _browseTimer.stop();
#endif
    _updateTimer.start( SHARE_DESKTOP_UPDATE_DELAY );
}

void MainWindow::_stopStreaming()
{
    _updateTimer.stop();
    _frameRateLabel.setText( "" );

    delete _stream;
    _stream = 0;

#ifdef __APPLE__
    _napSuspender.resume();
#endif
    emit streaming( false );
}

void MainWindow::_handleStreamingError( const QString& errorMessage )
{
    std::cerr << errorMessage.toStdString() << std::endl;
    QMessageBox::warning( this, "Error", errorMessage, QMessageBox::Ok,
                          QMessageBox::Ok );

    _stopStreaming();
}

void MainWindow::closeEvent( QCloseEvent* closeEvt )
{
    delete _desktopSelectionWindow;
    _desktopSelectionWindow = 0;

    _stopStreaming();

    QMainWindow::closeEvent( closeEvt );
}

void MainWindow::_setCoordinates( const QRect coordinates )
{
    _xSpinBox.setValue( coordinates.x( ));
    _ySpinBox.setValue( coordinates.y( ));
    _widthSpinBox.setValue( coordinates.width( ));
    _heightSpinBox.setValue( coordinates.height( ));

    // the spinboxes only update the UI; we must update the actual values too
    _x = _xSpinBox.value();
    _y = _ySpinBox.value();
    _width = _widthSpinBox.value();
    _height = _heightSpinBox.value();
}

void MainWindow::_shareDesktop( const bool set )
{
    if( set )
        _startStreaming();
    else
        _stopStreaming();
}

void MainWindow::_showDesktopSelectionWindow( const bool set )
{
    if( set )
        _desktopSelectionWindow->showFullScreen();
    else
        _desktopSelectionWindow->hide();
}

void MainWindow::_update()
{
    _processStreamEvents();
    _shareDesktopUpdate();
}

void MainWindow::_processStreamEvents()
{
    if( !_eventsBox.checkState( ))
        return;
    if( !_stream->isRegisteredForEvents( ))
        return;

    while( _stream->hasEvent( ))
    {
        const deflect::Event& wallEvent = _stream->getEvent();
#ifndef NDEBUG
        std::cout << "----------" << std::endl;
#endif
        switch( wallEvent.type )
        {
        case deflect::Event::EVT_CLOSE:
            _stopStreaming();
            break;
        case deflect::Event::EVT_PRESS:
            _sendMouseMoveEvent( wallEvent.mouseX, wallEvent.mouseY );
            _sendMousePressEvent( wallEvent.mouseX, wallEvent.mouseY );
            break;
        case deflect::Event::EVT_RELEASE:
            _sendMouseMoveEvent( wallEvent.mouseX, wallEvent.mouseY );
            _sendMouseReleaseEvent( wallEvent.mouseX, wallEvent.mouseY );
            break;
        case deflect::Event::EVT_CLICK:
            _sendMouseMoveEvent( wallEvent.mouseX, wallEvent.mouseY );
            _sendMousePressEvent( wallEvent.mouseX, wallEvent.mouseY );
            _sendMouseReleaseEvent( wallEvent.mouseX, wallEvent.mouseY );
            break;
        case deflect::Event::EVT_DOUBLECLICK:
            _sendMouseDoubleClickEvent( wallEvent.mouseX, wallEvent.mouseY );
            break;

        case deflect::Event::EVT_MOVE:
            _sendMouseMoveEvent( wallEvent.mouseX, wallEvent.mouseY );
            break;
        case deflect::Event::EVT_WHEEL:
        case deflect::Event::EVT_SWIPE_LEFT:
        case deflect::Event::EVT_SWIPE_RIGHT:
        case deflect::Event::EVT_SWIPE_UP:
        case deflect::Event::EVT_SWIPE_DOWN:
        case deflect::Event::EVT_KEY_PRESS:
        case deflect::Event::EVT_KEY_RELEASE:
        case deflect::Event::EVT_VIEW_SIZE_CHANGED:
        default:
            break;
        }
    }
}

#ifdef DEFLECT_USE_SERVUS
void MainWindow::_updateServus()
{
    if( _hostnameLineEdit.text() != DEFAULT_HOST_ADDRESS )
    {
        _browseTimer.stop();
        return;
    }

    _servus.browse( 0 );
    const servus::Strings& hosts = _servus.getInstances();
    if( hosts.empty( ))
        return;

    _browseTimer.stop();
    _hostnameLineEdit.setText( hosts.front().c_str( ));
}
#endif

void MainWindow::_shareDesktopUpdate()
{
    QTime frameTime;
    frameTime.start();

    const QPixmap desktopPixmap =
        QApplication::primaryScreen()->grabWindow( 0, _x, _y, _width, _height );

    if( desktopPixmap.isNull( ))
    {
        _handleStreamingError( "Got NULL desktop pixmap" );
        return;
    }
    QImage image = desktopPixmap.toImage();

    // render mouse cursor
    QPoint mousePos = ( QCursor::pos() - QPoint( _x, _y )) -
                        QPoint( _cursor.width() / 2, _cursor.height() / 2 );
    QPainter painter( &image );
    painter.drawImage( mousePos, _cursor );
    painter.end(); // Make sure to release the QImage before using it

    // QImage Format_RGB32 (0xffRRGGBB) corresponds to GL_BGRA == deflect::BGRA
    deflect::ImageWrapper deflectImage( (const void*)image.bits(),
                                        image.width(), image.height(),
                                        deflect::BGRA );
    deflectImage.compressionPolicy = deflect::COMPRESSION_ON;

    bool success = _stream->send( deflectImage ) && _stream->finishFrame();
    if( !success )
    {
        _handleStreamingError( "Streaming failure, connection closed." );
        return;
    }

    _regulateFrameRate( frameTime.elapsed( ));
}

void MainWindow::_regulateFrameRate( const int elapsedFrameTime )
{
    // frame rate limiting
    const int maxFrameRate = _frameRateSpinBox.value();
    const int desiredFrameTime = (int)( 1000.f * 1.f / (float)maxFrameRate );
    const int sleepTime = desiredFrameTime - elapsedFrameTime;

    if( sleepTime > 0 )
    {
#ifdef _WIN32
        Sleep( sleepTime );
#else
        usleep( 1000 * sleepTime );
#endif
    }

    // frame rate is calculated for every FRAME_RATE_AVERAGE_NUM_FRAMES
    // sequential frames
    _frameSentTimes.push_back( QTime::currentTime( ));

    if( _frameSentTimes.size() > FRAME_RATE_AVERAGE_NUM_FRAMES )
    {
        _frameSentTimes.clear();
    }
    else if( _frameSentTimes.size() == FRAME_RATE_AVERAGE_NUM_FRAMES )
    {
        const float fps = (float)_frameSentTimes.size() * 1000.f /
               (float)_frameSentTimes.front().msecsTo( _frameSentTimes.back( ));

        _frameRateLabel.setText( QString::number( fps ) + QString( " fps" ));
    }
}

void MainWindow::_updateCoordinates()
{
    _x = _xSpinBox.value();
    _y = _ySpinBox.value();
    _width = _widthSpinBox.value();
    _height = _heightSpinBox.value();

    _generateCursorImage();

    const QRect coord( _x, _y, _width, _height );
    _desktopSelectionWindow->getSelectionRectangle()->setCoordinates( coord );
}

#ifdef __APPLE__
void sendMouseEvent( const CGEventType type, const CGMouseButton button,
                     const CGPoint point )
{
    CGEventRef event = CGEventCreateMouseEvent( 0, type, point, button );
    CGEventSetType( event, type );
    CGEventPost( kCGHIDEventTap, event );
    CFRelease( event );
}

void MainWindow::_sendMousePressEvent( const float x, const float y )
{
    CGPoint point;
    point.x = _x + x * _width;
    point.y = _y + y * _height;
#ifndef NDEBUG
    std::cout << "Press " << point.x << ", " << point.y << " ("
              << x << ", " << y << ")"<< std::endl;
#endif
    sendMouseEvent( kCGEventLeftMouseDown, kCGMouseButtonLeft, point );
}

void MainWindow::_sendMouseMoveEvent( const float x, const float y )
{
    CGPoint point;
    point.x = _x + x * _width;
    point.y = _y + y * _height;
#ifndef NDEBUG
    std::cout << "Move " << point.x << ", " << point.y << " ("
              << x << ", " << y << ")"<< std::endl;
#endif
    sendMouseEvent( kCGEventMouseMoved, kCGMouseButtonLeft, point );
}

void MainWindow::_sendMouseReleaseEvent( const float x, const float y )
{
    CGPoint point;
    point.x = _x + x * _width;
    point.y = _y + y * _height;
#ifndef NDEBUG
    std::cout << "Release " << point.x << ", " << point.y << " ("
              << x << ", " << y << ")"<< std::endl;
#endif
    sendMouseEvent( kCGEventLeftMouseUp, kCGMouseButtonLeft, point );
}

void MainWindow::_sendMouseDoubleClickEvent( const float x, const float y )
{
    CGPoint point;
    point.x = _x + x * _width;
    point.y = _y + y * _height;
    CGEventRef event = CGEventCreateMouseEvent( 0, kCGEventLeftMouseDown,
                                                point, kCGMouseButtonLeft );
#ifndef NDEBUG
    std::cout << "Double click " << point.x << ", " << point.y << " ("
              << x << ", " << y << ")"<< std::endl;
#endif

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
void MainWindow::_sendMousePressEvent( const float, const float ) {}
void MainWindow::_sendMouseMoveEvent( const float, const float ) {}
void MainWindow::_sendMouseReleaseEvent( const float, const float ) {}
void MainWindow::_sendMouseDoubleClickEvent( const float, const float ) {}
#endif
