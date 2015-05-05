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
#include "DesktopSelectionView.h"
#include "DesktopSelectionRectangle.h"

#include <deflect/NetworkListener.h>
#include <deflect/Stream.h>

#ifdef _WIN32
    typedef __int32 int32_t;
    #include <windows.h>
#else
    #include <stdint.h>
    #include <unistd.h>
#endif
#ifdef __APPLE__
#  include <CoreGraphics/CoreGraphics.h>
#endif

#include <iostream>

#define SHARE_DESKTOP_UPDATE_DELAY      1
#define SERVUS_BROWSE_DELAY           100
#define FRAME_RATE_AVERAGE_NUM_FRAMES  10

#define DEFAULT_HOST_ADDRESS  "bbpav02.epfl.ch"
#define CURSOR_IMAGE_FILE     ":/cursor.png"

MainWindow::MainWindow()
    : stream_(0)
    , desktopSelectionWindow_(new DesktopSelectionWindow())
    , x_(0)
    , y_(0)
    , width_(0)
    , height_(0)
#ifdef DEFLECT_USE_LUNCHBOX
    , servus_( deflect::NetworkListener::serviceName )
#endif
{
    generateCursorImage();
    setupUI();

    // Receive changes from the selection rectangle
    connect(desktopSelectionWindow_->getDesktopSelectionView()->getDesktopSelectionRectangle(),
            SIGNAL(coordinatesChanged(QRect)), this, SLOT(setCoordinates(QRect)));

    connect(desktopSelectionWindow_, SIGNAL(windowVisible(bool)), showDesktopSelectionWindowAction_, SLOT(setChecked(bool)));
}

void MainWindow::generateCursorImage()
{
    cursor_ = QImage( CURSOR_IMAGE_FILE ).scaled( 20, 20, Qt::KeepAspectRatio );
}

void MainWindow::setupUI()
{
    QWidget * widget = new QWidget();
    QFormLayout * formLayout = new QFormLayout();
    widget->setLayout(formLayout);

    setCentralWidget(widget);

    // connect widget signals / slots
    connect(&xSpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));
    connect(&ySpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));
    connect(&widthSpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));
    connect(&heightSpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));

    hostnameLineEdit_.setText( DEFAULT_HOST_ADDRESS );

    char hostname[256] = {0};
    gethostname( hostname, 256 );
    uriLineEdit_.setText( hostname );

    const int screen = -1;
    QRect desktopRect = QApplication::desktop()->screenGeometry( screen );

    xSpinBox_.setRange(0, desktopRect.width());
    ySpinBox_.setRange(0, desktopRect.height());
    widthSpinBox_.setRange(1, desktopRect.width());
    heightSpinBox_.setRange(1, desktopRect.height());

    // default to full screen
    xSpinBox_.setValue(0);
    ySpinBox_.setValue(0);
    widthSpinBox_.setValue( desktopRect.width( ));
    heightSpinBox_.setValue( desktopRect.height( ));

    // call updateCoordinates() to commit coordinates from the UI
    updateCoordinates();

    // frame rate limiting
    frameRateSpinBox_.setRange(1, 60);
    frameRateSpinBox_.setValue(24);

    // add widgets to UI
    formLayout->addRow("Hostname", &hostnameLineEdit_);
    formLayout->addRow("Stream name", &uriLineEdit_);
    formLayout->addRow("X", &xSpinBox_);
    formLayout->addRow("Y", &ySpinBox_);
    formLayout->addRow("Width", &widthSpinBox_);
    formLayout->addRow("Height", &heightSpinBox_);
    formLayout->addRow("Allow desktop interaction", &eventsBox_);
    formLayout->addRow("Max frame rate", &frameRateSpinBox_);
    formLayout->addRow("Actual frame rate", &frameRateLabel_);
    eventsBox_.setChecked( true );

    // share desktop action
    shareDesktopAction_ = new QAction("Share Desktop", this);
    shareDesktopAction_->setStatusTip("Share desktop");
    shareDesktopAction_->setCheckable(true);
    shareDesktopAction_->setChecked(false);
    connect(shareDesktopAction_, SIGNAL(triggered(bool)), this, SLOT(shareDesktop(bool))); // Only user actions
    connect(this, SIGNAL(streaming(bool)), shareDesktopAction_, SLOT(setChecked(bool)));

    // show desktop selection window action
    showDesktopSelectionWindowAction_ = new QAction("Show Rectangle", this);
    showDesktopSelectionWindowAction_->setStatusTip("Show desktop selection rectangle");
    showDesktopSelectionWindowAction_->setCheckable(true);
    showDesktopSelectionWindowAction_->setChecked(false);
    connect(showDesktopSelectionWindowAction_, SIGNAL(triggered(bool)), this, SLOT(showDesktopSelectionWindow(bool))); // Only user actions

    // create toolbar
    QToolBar * toolbar = addToolBar("toolbar");

    // add buttons to toolbar
    toolbar->addAction(shareDesktopAction_);
    toolbar->addAction(showDesktopSelectionWindowAction_);

    // Update timer
    connect(&updateTimer_, SIGNAL(timeout()), this, SLOT(update()));

#ifdef DEFLECT_USE_LUNCHBOX
    servus_.beginBrowsing( lunchbox::Servus::IF_ALL );
    connect( &browseTimer_, SIGNAL( timeout( )), this, SLOT( updateServus( )));
    browseTimer_.start( SERVUS_BROWSE_DELAY );
#endif
}

void MainWindow::startStreaming()
{
    if( stream_ )
        return;

    stream_ = new deflect::Stream( uriLineEdit_.text().toStdString(),
                                   hostnameLineEdit_.text().toStdString( ));
    if (!stream_->isConnected())
    {
        handleStreamingError("Could not connect to host!");
        return;
    }

#ifdef __APPLE__
    napSuspender_.suspend();
    browseTimer_.stop();
#endif
    updateTimer_.start(SHARE_DESKTOP_UPDATE_DELAY);
}

void MainWindow::stopStreaming()
{
    updateTimer_.stop();
    frameRateLabel_.setText("");

    delete stream_;
    stream_ = 0;

#ifdef __APPLE__
    napSuspender_.resume();
#endif
    emit streaming(false);
}

void MainWindow::handleStreamingError(const QString& errorMessage)
{
    std::cerr << errorMessage.toStdString() << std::endl;
    QMessageBox::warning(this, "Error", errorMessage, QMessageBox::Ok, QMessageBox::Ok);

    stopStreaming();
}

void MainWindow::closeEvent( QCloseEvent* closeEvt )
{
    delete desktopSelectionWindow_;
    desktopSelectionWindow_ = 0;

    stopStreaming();

    QMainWindow::closeEvent( closeEvt );
}

void MainWindow::setCoordinates(const QRect coordinates)
{
    xSpinBox_.setValue(coordinates.x());
    ySpinBox_.setValue(coordinates.y());
    widthSpinBox_.setValue(coordinates.width());
    heightSpinBox_.setValue(coordinates.height());

    // the spinboxes only update the UI; we must update the actual values too
    x_ = xSpinBox_.value();
    y_ = ySpinBox_.value();
    width_ = widthSpinBox_.value();
    height_ = heightSpinBox_.value();
}

void MainWindow::shareDesktop( const bool set )
{
    if( set )
        startStreaming();
    else
        stopStreaming();
}

void MainWindow::showDesktopSelectionWindow( const bool set )
{
    if( set )
        desktopSelectionWindow_->showFullScreen();
    else
        desktopSelectionWindow_->hide();
}

void MainWindow::update()
{
    processStreamEvents();
    shareDesktopUpdate();
}

void MainWindow::processStreamEvents()
{
    if( !eventsBox_.checkState( ))
        return;
    if( !stream_->isRegisteredForEvents() && !stream_->registerForEvents( ))
        return;

    while( stream_->hasEvent( ))
    {
        const deflect::Event& wallEvent = stream_->getEvent();
#ifndef NDEBUG
        std::cout << "----------" << std::endl;
#endif
        switch( wallEvent.type )
        {
        case deflect::Event::EVT_CLOSE:
            stopStreaming();
            break;
        case deflect::Event::EVT_PRESS:
            sendMouseMoveEvent( wallEvent.mouseX, wallEvent.mouseY );
            sendMousePressEvent( wallEvent.mouseX, wallEvent.mouseY );
            break;
        case deflect::Event::EVT_RELEASE:
            sendMouseMoveEvent( wallEvent.mouseX, wallEvent.mouseY );
            sendMouseReleaseEvent( wallEvent.mouseX, wallEvent.mouseY );
            break;
        case deflect::Event::EVT_CLICK:
            sendMouseMoveEvent( wallEvent.mouseX, wallEvent.mouseY );
            sendMousePressEvent( wallEvent.mouseX, wallEvent.mouseY );
            sendMouseReleaseEvent( wallEvent.mouseX, wallEvent.mouseY );
            break;
        case deflect::Event::EVT_DOUBLECLICK:
            sendMouseDoubleClickEvent( wallEvent.mouseX, wallEvent.mouseY );
            break;

        case deflect::Event::EVT_MOVE:
            sendMouseMoveEvent( wallEvent.mouseX, wallEvent.mouseY );
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

#ifdef DEFLECT_USE_LUNCHBOX
void MainWindow::updateServus()
{
    if( hostnameLineEdit_.text() != DEFAULT_HOST_ADDRESS )
    {
        browseTimer_.stop();
        return;
    }

    servus_.browse( 0 );
    const lunchbox::Strings& hosts = servus_.getInstances();
    if( hosts.empty( ))
        return;

    browseTimer_.stop();
    hostnameLineEdit_.setText( hosts.front().c_str( ));
}
#endif

void MainWindow::shareDesktopUpdate()
{
    // time the frame
    QTime frameTime;
    frameTime.start();

    // take screenshot
    const QPixmap desktopPixmap =
        QPixmap::grabWindow( QApplication::desktop()->winId(), x_, y_,
                             width_, height_ );

    if( desktopPixmap.isNull( ))
    {
        handleStreamingError("Got NULL desktop pixmap");
        return;
    }

    QImage image = desktopPixmap.toImage();

    // render mouse cursor
    QPoint mousePos = ( QCursor::pos() - QPoint( x_, y_ )) -
                        QPoint( cursor_.width()/2, cursor_.height()/2);

    QPainter painter( &image );
    painter.drawImage( mousePos, cursor_ );
    painter.end(); // Make sure to release the QImage before using it to update the segements

    // QImage Format_RGB32 (0xffRRGGBB) corresponds in fact to GL_BGRA == deflect::BGRA
    deflect::ImageWrapper deflectImage((const void*)image.bits(), image.width(), image.height(), deflect::BGRA);
    deflectImage.compressionPolicy = deflect::COMPRESSION_ON;

    bool success = stream_->send(deflectImage) && stream_->finishFrame();

    if( !success )
    {
        handleStreamingError("Streaming failure, connection closed.");
        return;
    }

    regulateFrameRate(frameTime.elapsed());
}

void MainWindow::regulateFrameRate(const int elapsedFrameTime)
{
    // frame rate limiting
    const int maxFrameRate = frameRateSpinBox_.value();
    const int desiredFrameTime = (int)(1000. * 1. / (float)maxFrameRate);
    const int sleepTime = desiredFrameTime - elapsedFrameTime;

    if(sleepTime > 0)
    {
#ifdef _WIN32
        Sleep(sleepTime);
#else
        usleep(1000 * sleepTime);
#endif
    }

    // frame rate is calculated for every FRAME_RATE_AVERAGE_NUM_FRAMES sequential frames
    frameSentTimes_.push_back(QTime::currentTime());

    if(frameSentTimes_.size() > FRAME_RATE_AVERAGE_NUM_FRAMES)
    {
        frameSentTimes_.clear();
    }
    else if(frameSentTimes_.size() == FRAME_RATE_AVERAGE_NUM_FRAMES)
    {
        const float fps = (float)frameSentTimes_.size() / (float)frameSentTimes_.front().msecsTo(frameSentTimes_.back()) * 1000.;

        frameRateLabel_.setText(QString::number(fps) + QString(" fps"));
    }
}

void MainWindow::updateCoordinates()
{
    x_ = xSpinBox_.value();
    y_ = ySpinBox_.value();
    width_ = widthSpinBox_.value();
    height_ = heightSpinBox_.value();

    generateCursorImage();

    const QRect coordinates(x_, y_, width_, height_);
    desktopSelectionWindow_->getDesktopSelectionView()->getDesktopSelectionRectangle()->setCoordinates(coordinates);
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

void MainWindow::sendMousePressEvent( const float x, const float y )
{
    CGPoint point;
    point.x = x_ + x * width_;
    point.y = y_ + y * height_;
#ifndef NDEBUG
    std::cout << "Press " << point.x << ", " << point.y << " ("
              << x << ", " << y << ")"<< std::endl;
#endif
    sendMouseEvent( kCGEventLeftMouseDown, kCGMouseButtonLeft, point );
}
void MainWindow::sendMouseMoveEvent( const float x, const float y )
{
    CGPoint point;
    point.x = x_ + x * width_;
    point.y = y_ + y * height_;
#ifndef NDEBUG
    std::cout << "Move " << point.x << ", " << point.y << " ("
              << x << ", " << y << ")"<< std::endl;
#endif
    sendMouseEvent( kCGEventMouseMoved, kCGMouseButtonLeft, point );
}
void MainWindow::sendMouseReleaseEvent( const float x, const float y )
{
    CGPoint point;
    point.x = x_ + x * width_;
    point.y = y_ + y * height_;
#ifndef NDEBUG
    std::cout << "Release " << point.x << ", " << point.y << " ("
              << x << ", " << y << ")"<< std::endl;
#endif
    sendMouseEvent( kCGEventLeftMouseUp, kCGMouseButtonLeft, point );
}
void MainWindow::sendMouseDoubleClickEvent( const float x, const float y )
{
    CGPoint point;
    point.x = x_ + x * width_;
    point.y = y_ + y * height_;
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
void MainWindow::sendMousePressEvent( const float, const float ) {}
void MainWindow::sendMouseMoveEvent( const float, const float ) {}
void MainWindow::sendMouseReleaseEvent( const float, const float ) {}
void MainWindow::sendMouseDoubleClickEvent( const float, const float ) {}
#endif
