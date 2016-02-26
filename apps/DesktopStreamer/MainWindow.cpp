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
#include "Stream.h"

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

#ifdef DEFLECT_USE_QT5MACEXTRAS
#  include "DesktopWindowsModel.h"
#endif

#define SHARE_DESKTOP_UPDATE_DELAY      1
#define FAILURE_UPDATE_DELAY          100
#define FRAME_RATE_AVERAGE_NUM_FRAMES  10

const std::vector< std::pair< QString, QString > > defaultHosts = {
    { "DisplayWall Ground floor", "bbpav02.epfl.ch" },
    { "DisplayWall 5th floor", "bbpav05.epfl.ch" },
    { "DisplayWall 6th floor", "bbpav06.epfl.ch" }
};

MainWindow::MainWindow()
{
    setupUi( this );

    connect( _hostnameComboBox, &QComboBox::currentTextChanged,
             [&]( const QString& text )
                { _streamButton->setEnabled( !text.isEmpty( )); });

    for( const auto& entry : defaultHosts )
        _hostnameComboBox->addItem( entry.first, entry.second );

    char hostname[256] = { 0 };
    gethostname( hostname, 256 );
    _streamnameLineEdit->setText( QString( "%1" ).arg( hostname ));

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

MainWindow::~MainWindow() {}

void MainWindow::_startStreaming()
{
#ifdef __APPLE__
    _napSuspender.suspend();
#endif
    _updateTimer.start( SHARE_DESKTOP_UPDATE_DELAY );
}

void MainWindow::_stopStreaming()
{
    _stream.reset();
    if( _streamButton->isChecked( ))
        _updateTimer.start( FAILURE_UPDATE_DELAY );
    else
    {
        _updateTimer.stop();
        _statusbar->clearMessage();

#ifdef __APPLE__
        _napSuspender.resume();
#endif
        _streamButton->setChecked( false );
    }
}

void MainWindow::_handleStreamingError( const QString& errorMessage )
{
    _statusbar->showMessage( errorMessage );
    _stopStreaming();
}

void MainWindow::closeEvent( QCloseEvent* closeEvt )
{
    _streamButton->setChecked( false );
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
    if( _streamButton->isChecked( ))
    {
        _checkStream();
        _processStreamEvents();
        _shareDesktopUpdate();
    }
    else
        _stopStreaming();
}

void MainWindow::_checkStream()
{
    if( _stream )
        return;

    QString streamHost;
    if( _hostnameComboBox->findText( _hostnameComboBox->currentText( )) == -1 )
        streamHost = _hostnameComboBox->currentText();
    else
        streamHost = _hostnameComboBox->currentData().toString();

#ifdef DEFLECT_USE_QT5MACEXTRAS
    const QPersistentModelIndex windowIndex = _listView->currentIndex();
    if( !windowIndex.isValid( ))
    {
        _handleStreamingError( "No window to stream is selected" );
        return;
    }
#else
    const QPersistentModelIndex windowIndex;
#endif

    _stream.reset( new Stream( *this, windowIndex,
                               _streamnameLineEdit->text().toStdString(),
                               streamHost.toStdString( )));
    if( !_stream->isConnected( ))
    {
        _handleStreamingError( "Could not connect to host" );
        return;
    }

#ifdef STREAM_EVENTS_SUPPORTED
    if( _desktopInteractionCheckBox->isChecked( ))
        _stream->registerForEvents();
#endif
}

void MainWindow::_processStreamEvents()
{
    if( _stream )
    {
        if( _desktopInteractionCheckBox->checkState( ))
            _stream->processEvents();
        else
            _stream->drainEvents();
    }
}

void MainWindow::_shareDesktopUpdate()
{
    if( !_stream )
        return;

    QTime frameTime;
    frameTime.start();

    const std::string& error = _stream->update();
    if( error.empty( ))
        _regulateFrameRate( frameTime.elapsed( ));
    else
        _handleStreamingError( QString( error.c_str( )));
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
        _frameSentTimes.clear();
    else if( _frameSentTimes.size() == FRAME_RATE_AVERAGE_NUM_FRAMES )
    {
        const float fps = (float)_frameSentTimes.size() * 1000.f /
               (float)_frameSentTimes.front().msecsTo( _frameSentTimes.back( ));

        _statusbar->showMessage( QString( "Streaming to %1@%2 fps" )
                .arg( _hostnameComboBox->currentData().toString( )).arg( fps ));
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
