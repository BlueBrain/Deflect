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

#include <deflect/Server.h>
#include <deflect/Stream.h>
#include <deflect/version.h>

#include <iostream>

#include <QMessageBox>
#include <QPainter>
#include <QScreen>

#ifdef _WIN32
typedef __int32 int32_t;
#  include <windows.h>
#else
#  include <stdint.h>
#  include <unistd.h>
#endif

#ifdef __APPLE__
#  ifdef DEFLECT_USE_QT5MACEXTRAS
#    include "DesktopWindowsModel.h"
#  endif
#  define STREAM_EVENTS_SUPPORTED TRUE
#  if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_9
#    include <CoreGraphics/CoreGraphics.h>
#  else
#    include <ApplicationServices/ApplicationServices.h>
#  endif
#endif

#define SHARE_DESKTOP_UPDATE_DELAY      1
#define SERVUS_BROWSE_DELAY           100
#define FRAME_RATE_AVERAGE_NUM_FRAMES  10
#define CURSOR_IMAGE_FILE     ":/cursor.png"
#define CURSOR_IMAGE_SIZE     20

const std::vector< std::pair< QString, QString > > defaultHosts = {
    { "DisplayWall Ground floor", "bbpav02.epfl.ch" },
    { "DisplayWall 5th floor", "bbpav05.epfl.ch" },
    { "DisplayWall 6th floor", "bbpav06.epfl.ch" }
};

MainWindow::MainWindow()
    : _stream( 0 )
    , _cursor( QImage( CURSOR_IMAGE_FILE ).scaled(
                  CURSOR_IMAGE_SIZE * devicePixelRatio(),
                  CURSOR_IMAGE_SIZE * devicePixelRatio(), Qt::KeepAspectRatio ))
#ifdef DEFLECT_USE_SERVUS
    , _servus( deflect::Server::serviceName )
#endif
{
    setupUi( this );

    connect( _hostnameComboBox, &QComboBox::currentTextChanged,
             [&]( const QString& text )
                { _streamButton->setEnabled( !text.isEmpty( )); });

    for( const auto& entry : defaultHosts )
        _hostnameComboBox->addItem( entry.first, entry.second );

    char hostname[256] = { 0 };
    gethostname( hostname, 256 );
    _streamnameLineEdit->setText( QString( "Desktop - %1" ).arg( hostname ));

#ifdef DEFLECT_USE_QT5MACEXTRAS
    _listView->setModel( new DesktopWindowsModel );

    connect( _listView->selectionModel(), &QItemSelectionModel::currentChanged,
             [=]( const QModelIndex& current, const QModelIndex& )
    {
        const QString& windowName =
                _listView->model()->data( current, Qt::DisplayRole ).toString();
        _streamnameLineEdit->setText( QString( "%1 - %2" ).arg( windowName )
                                                          .arg( hostname ));
    });

    connect( _listView->model(), &DesktopWindowsModel::rowsRemoved,
             [&]( const QModelIndex&, int, int )
    {
        if( !_windowIndex.isValid() && _stream )
        {
            _stopStreaming();
            _listView->setCurrentIndex( _listView->model()->index( 0, 0 ));
        }
    });
#else
    _listView->setHidden( true );
    adjustSize();
#endif

    connect( _desktopInteractionCheckBox, &QCheckBox::clicked,
             this, &MainWindow::_onStreamEventsBoxClicked );

    connect( _streamButton, &QPushButton::clicked,
             this, &MainWindow::_shareDesktop );

    connect( _actionAbout, &QAction::triggered,
             this, &MainWindow::_openAboutWidget );

    // Update timer
    connect( &_updateTimer, &QTimer::timeout, this, &MainWindow::_update );
}

void MainWindow::_startStreaming()
{
    if( _stream )
        return;

    QString streamHost =
            _hostnameComboBox->currentData().toString();
    if( streamHost.isEmpty( ))
        streamHost = _hostnameComboBox->currentData( Qt::DisplayRole ).toString();
    _stream = new deflect::Stream( _streamnameLineEdit->text().toStdString(),
                                   streamHost.toStdString( ));
    if( !_stream->isConnected( ))
    {
        _handleStreamingError( "Could not connect to host" );
        return;
    }

#ifdef DEFLECT_USE_QT5MACEXTRAS
    _windowIndex = _listView->currentIndex();
    if( !_windowIndex.isValid( ))
    {
        _handleStreamingError( "No window to stream is selected" );
        return;
    }
#endif

#ifdef STREAM_EVENTS_SUPPORTED
    if( _desktopInteractionCheckBox->isChecked( ))
        _stream->registerForEvents();
#endif

#ifdef __APPLE__
    _napSuspender.suspend();
#endif
#ifdef DEFLECT_USE_SERVUS
    _browseTimer.stop();
#endif
    _updateTimer.start( SHARE_DESKTOP_UPDATE_DELAY );
}

void MainWindow::_stopStreaming()
{
    _updateTimer.stop();
    _statusbar->clearMessage();

    delete _stream;
    _stream = 0;

#ifdef __APPLE__
    _napSuspender.resume();
#endif
    _streamButton->setChecked( false );
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
    _stopStreaming();
    QMainWindow::closeEvent( closeEvt );
}

void MainWindow::_shareDesktop( const bool set )
{
    if( set )
        _startStreaming();
    else
        _stopStreaming();
}

void MainWindow::_update()
{
    if( _stream->isRegisteredForEvents( ))
        _processStreamEvents();
    _shareDesktopUpdate();
}

void MainWindow::_processStreamEvents()
{
    while( _stream->hasEvent( ))
    {
        const deflect::Event& wallEvent = _stream->getEvent();
        // Once registered for events they must be consumed, otherwise they
        // queue up. Until unregister is implemented, just ignore them.
        if( !_desktopInteractionCheckBox->checkState( ))
            break;
#ifndef NDEBUG
        std::cout << "----------" << std::endl;
#endif
        switch( wallEvent.type )
        {
        case deflect::Event::EVT_CLOSE:
            return;
        case deflect::Event::EVT_PRESS:
            _sendMouseMoveEvent( wallEvent.mouseX, wallEvent.mouseY );
            _sendMousePressEvent( wallEvent.mouseX, wallEvent.mouseY );
            break;
        case deflect::Event::EVT_RELEASE:
            _sendMouseMoveEvent( wallEvent.mouseX, wallEvent.mouseY );
            _sendMouseReleaseEvent( wallEvent.mouseX, wallEvent.mouseY );
            break;
        case deflect::Event::EVT_DOUBLECLICK:
            _sendMouseDoubleClickEvent( wallEvent.mouseX, wallEvent.mouseY );
            break;

        case deflect::Event::EVT_MOVE:
            _sendMouseMoveEvent( wallEvent.mouseX, wallEvent.mouseY );
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
        default:
            break;
        }
    }
}

void MainWindow::_shareDesktopUpdate()
{
    if( !_stream )
        return;

    QTime frameTime;
    frameTime.start();

    QPixmap pixmap;
#ifdef DEFLECT_USE_QT5MACEXTRAS
    if( !_windowIndex.isValid( ))
        return;
    if( _windowIndex.row() > 0 )
    {
        QAbstractItemModel* model = _listView->model();
        pixmap = model->data( _windowIndex,
                          DesktopWindowsModel::ROLE_PIXMAP ).value< QPixmap >();
        _windowRect = model->data( _windowIndex,
                              DesktopWindowsModel::ROLE_RECT ).value< QRect >();
    }
    else
#endif
    {
        pixmap = QApplication::primaryScreen()->grabWindow( 0 );
        _windowRect = QRect( 0, 0, pixmap.width(), pixmap.height( ));
    }

    if( pixmap.isNull( ))
    {
        _handleStreamingError( "Got NULL desktop pixmap" );
        return;
    }
    QImage image = pixmap.toImage();

    // render mouse cursor
    const QPoint mousePos =
            ( devicePixelRatio() * QCursor::pos() - _windowRect.topLeft()) -
              QPoint( _cursor.width() / 2, _cursor.height() / 2 );
    QPainter painter( &image );
    painter.drawImage( mousePos, _cursor );
    painter.end(); // Make sure to release the QImage before using it

    // QImage Format_RGB32 (0xffRRGGBB) corresponds to GL_BGRA == deflect::BGRA
    deflect::ImageWrapper deflectImage( (const void*)image.bits(),
                                        image.width(), image.height(),
                                        deflect::BGRA );
    deflectImage.compressionPolicy = deflect::COMPRESSION_ON;

    const bool success = _stream->send( deflectImage ) &&
                         _stream->finishFrame();
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
    const int maxFrameRate = _maxFrameRateSpinBox->value();
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

        _statusbar->showMessage( QString( "Streaming to %1@%2 fps" )
                           .arg( _hostnameComboBox->currentText( )).arg( fps ));
    }
}

void MainWindow::_onStreamEventsBoxClicked( const bool checked )
{
    if( !checked )
        return;
#ifdef STREAM_EVENTS_SUPPORTED
    if( _stream && _stream->isConnected() && !_stream->isRegisteredForEvents( ))
        _stream->registerForEvents();
#endif
}

void MainWindow::_openAboutWidget()
{
    const int revision = deflect::Version::getRevision();

    std::ostringstream aboutMsg;
    aboutMsg << "Current version: " << deflect::Version::getString();
    aboutMsg << std::endl;
    aboutMsg << "SCM revision: " << std::hex << revision << std::dec;

    QMessageBox::about( this, "About DesktopStreamer", aboutMsg.str().c_str( ));
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
    point.x = _windowRect.topLeft().x() + x * _windowRect.width();
    point.y = _windowRect.topLeft().y() + y * _windowRect.height();
#ifndef NDEBUG
    std::cout << "Press " << point.x << ", " << point.y << " ("
              << x << ", " << y << ")"<< std::endl;
#endif
    sendMouseEvent( kCGEventLeftMouseDown, kCGMouseButtonLeft, point );
}

void MainWindow::_sendMouseMoveEvent( const float x, const float y )
{
    CGPoint point;
    point.x = _windowRect.topLeft().x() + x * _windowRect.width();
    point.y = _windowRect.topLeft().y() + y * _windowRect.height();
#ifndef NDEBUG
    std::cout << "Move " << point.x << ", " << point.y << " ("
              << x << ", " << y << ")"<< std::endl;
#endif
    sendMouseEvent( kCGEventMouseMoved, kCGMouseButtonLeft, point );
}

void MainWindow::_sendMouseReleaseEvent( const float x, const float y )
{
    CGPoint point;
    point.x = _windowRect.topLeft().x() + x * _windowRect.width();
    point.y = _windowRect.topLeft().y() + y * _windowRect.height();
#ifndef NDEBUG
    std::cout << "Release " << point.x << ", " << point.y << " ("
              << x << ", " << y << ")"<< std::endl;
#endif
    sendMouseEvent( kCGEventLeftMouseUp, kCGMouseButtonLeft, point );
}

void MainWindow::_sendMouseDoubleClickEvent( const float x, const float y )
{
    CGPoint point;
    point.x = _windowRect.topLeft().x() + x * _windowRect.width();
    point.y = _windowRect.topLeft().y() + y * _windowRect.height();
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
