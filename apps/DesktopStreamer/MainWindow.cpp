/*********************************************************************/
/* Copyright (c) 2011-2012, The University of Texas at Austin.       */
/* Copyright (c) 2013-2016, EPFL/Blue Brain Project                  */
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

#include "MainWindow.h"
#include "Stream.h"

#include <deflect/version.h>

#include <QHostInfo>
#include <QMessageBox>
#include <QPainter>
#include <QScreen>

#include <algorithm>
#include <iostream>

#ifdef _WIN32
typedef __int32 int32_t;
#  define NOMINMAX
#  include <windows.h>
#else
#  include <stdint.h>
#endif

#ifdef DEFLECT_USE_QT5MACEXTRAS
#  include "DesktopWindowsModel.h"
#endif

#define SHARE_DESKTOP_UPDATE_DELAY      0
#define FAILURE_UPDATE_DELAY          100
#define FRAME_RATE_DAMPING            0.1f // influence of new value between 0-1

namespace
{
const std::vector< std::pair< QString, QString > > defaultHosts = {
    { "DisplayWall Ground floor", "bbpav02.epfl.ch" },
    { "DisplayWall 3rd floor", "bbpav04.epfl.ch" },
    { "DisplayWall 5th floor", "bbpav05.epfl.ch" },
    { "DisplayWall 6th floor", "bbpav06.epfl.ch" }
};
const QString streamButtonDefaultText = "Stream";
const QString streamSelected = "Stream selected item(s)";
}

MainWindow::MainWindow()
    : _streamID( 0 )
    , _averageUpdate( 0 )
{
    setupUi( this );

    for( const auto& entry : defaultHosts )
        _hostComboBox->addItem( entry.first, entry.second );

    _hostComboBox->setCurrentIndex( -1 ); // no default host selected initially

    connect( _hostComboBox, &QComboBox::currentTextChanged,
             [&]( const QString& text )
             {
                _streamButton->setEnabled( !text.isEmpty( ));
                _listView->setEnabled( !text.isEmpty( ));
             });

    _streamIdLineEdit->setText( QHostInfo::localHostName( ));

    connect( _streamButton, &QPushButton::clicked,
             this, &MainWindow::_update );
    connect( _streamButton, &QPushButton::clicked,
             _actionMultiWindowMode, &QAction::setDisabled );

    connect( _remoteControlCheckBox, &QCheckBox::clicked,
             this, &MainWindow::_onStreamEventsBoxClicked );

    connect( _actionAdvancedSettings, &QAction::triggered,
             this, &MainWindow::_showAdvancedSettings );

    connect( _actionMultiWindowMode, &QAction::triggered,
             [this]( const bool checked )
             {
                if( checked )
                    _showMultiWindowMode();
                else
                    _showSingleWindowMode();
             });

    connect( _actionAbout, &QAction::triggered,
             this, &MainWindow::_openAboutWidget );

    connect( &_updateTimer, &QTimer::timeout, this, &MainWindow::_update );

#ifndef __APPLE__
    // Event injection support is currently limited to OSX
    _showRemoteControl( false );
#endif
#ifndef DEFLECT_USE_QT5MACEXTRAS
    _actionMultiWindowMode->setVisible( false );
#endif
    _showAdvancedSettings( false );
    _showSingleWindowMode();
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
    if( _streamButton->isChecked( ))
        _updateTimer.start( FAILURE_UPDATE_DELAY );
    else
    {
        _updateTimer.stop();
        _statusbar->clearMessage();
        _streams.clear();

#ifdef __APPLE__
        _napSuspender.resume();
#endif
        _streamID = 0;
    }
}

void MainWindow::closeEvent( QCloseEvent* closeEvt )
{
    _streamButton->setChecked( false );
    _stopStreaming();
    QMainWindow::closeEvent( closeEvt );
}

void MainWindow::_update()
{
#ifdef __APPLE__
    // On OS X >= 10.9 AppNap switches on/off automatically based on the app
    // visibility. We can avoid this by actively checking the state of the
    // AppNapSuspender (calling 'suspend' does the check, it will rarely need to
    // disable the AppNap feature if it was already disabled)
    _napSuspender.suspend();
#endif
    _frameTime.start();
    if( _streamButton->isChecked( ))
    {
        _updateStreams();
        _processStreamEvents();
        _shareDesktopUpdate();
        _regulateFrameRate();
    }
    else
        _stopStreaming();
}

void MainWindow::_showRemoteControl( const bool visible )
{
    _remoteControlLabel->setVisible( visible );
    _remoteControlCheckBox->setVisible( visible );
}

void MainWindow::_showMultiWindowMode()
{
#ifdef DEFLECT_USE_QT5MACEXTRAS
    if( !_listView->model( ))
    {
        auto model = new DesktopWindowsModel();
        model->setParent( _listView );
        _listView->setModel( model );
    }

    _listView->setVisible( true );

    // select 'Desktop' item as initial default stream item
    _listView->setCurrentIndex( _listView->model()->index( 0, 0 ));
    _streamButton->setText( streamSelected );

    const int itemsHorizontal = std::min( 3.f,
                std::ceil( std::sqrt( float(_listView->model()->rowCount( )))));
    const int itemsVertical = std::min( 3.f,
         std::ceil(float( _listView->model()->rowCount( )) / itemsHorizontal ));

    layout()->setSizeConstraint( QLayout::SetDefaultConstraint );
    setFixedSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
    // 230 (itemSize + spacing), frameWidth for decorations
    resize( QSize( 230 * itemsHorizontal + 2 * _listView->frameWidth(),
                   230 * itemsVertical   + 2 * _listView->frameWidth() + 50 ));
#endif
}

void MainWindow::_showSingleWindowMode()
{
    _listView->setHidden( true );
    _streamButton->setText( streamButtonDefaultText );

    layout()->setSizeConstraint( QLayout::SetFixedSize );
}

void MainWindow::_showAdvancedSettings( const bool visible )
{
    _maxFrameRateSpinBox->setVisible( visible );
    _maxFrameRateLabel->setVisible( visible );

    _streamIdLineEdit->setVisible( visible );
    _streamIdLabel->setVisible( visible );
}

void MainWindow::_updateStreams()
{
    if( _actionMultiWindowMode->isChecked( ))
        _updateMultipleStreams();
    else
        _updateSingleStream();
}

void MainWindow::_updateMultipleStreams()
{
#ifdef DEFLECT_USE_QT5MACEXTRAS
    const QModelIndexList windowIndices =
        _listView->selectionModel()->selectedIndexes();

    StreamMap streams;
    for( const QPersistentModelIndex& index : windowIndices )
    {
        if( _streams.count( index ))
        {
            streams[ index ] = _streams[ index ];
            continue;
        }

        const std::string appName = index.isValid() ?
            _listView->model()->data( index, Qt::DisplayRole ).
                toString().toStdString() : std::string();
        const std::string streamId = std::to_string( ++_streamID ) +
                                       " " + appName + " - " +
                                      _streamIdLineEdit->text().toStdString();
        const int pid = index.isValid() ?
            _listView->model()->data( index,
                                      DesktopWindowsModel::ROLE_PID).toInt(): 0;
        const std::string host = _getStreamHost();
        StreamPtr stream( new Stream( *this, index, streamId, host, pid ));

        if( !stream->isConnected( ))
        {
            _showConnectionErrorStatus();
            continue;
        }

        if( _remoteControlCheckBox->isChecked( ))
            stream->registerForEvents();

        streams[ index ] = stream;
    }
    _streams.swap( streams );

    if( !_streams.empty() && !_streamButton->isChecked( ))
    {
        _streamButton->setChecked( true );
        _startStreaming();
    };
#endif
}

void MainWindow::_updateSingleStream()
{
    if( !_streamButton->isChecked( ))
    {
        _stopStreaming();
        return;
    }

    if( _streams.empty( ))
    {
        const QPersistentModelIndex index; // default == use desktop
        StreamPtr stream( new Stream( *this, index,
                                      _streamIdLineEdit->text().toStdString(),
                                      _getStreamHost( )));
        if( stream->isConnected( ))
        {
            if( _remoteControlCheckBox->isChecked( ))
                stream->registerForEvents();
            _streams[ index ] = stream;
            _startStreaming();
        }
        else
            _showConnectionErrorStatus();
    }
}

void MainWindow::_showConnectionErrorStatus()
{
    _statusbar->showMessage( QString( "Cannot connect to host: '%1'" ).
                             arg( _getStreamHost().c_str( )));
}

void MainWindow::_processStreamEvents()
{
    const bool interact = _remoteControlCheckBox->checkState();
    std::vector< ConstStreamPtr > closedStreams;

    for( auto i = _streams.begin(); i != _streams.end(); )
    {
        if( i->second->processEvents( interact ))
            ++i;
        else
        {
            closedStreams.push_back( i->second );
            i = _streams.erase( i );
        }
    }

    for( ConstStreamPtr stream : closedStreams )
        _deselect( stream );
}

void MainWindow::_shareDesktopUpdate()
{
    if( _streams.empty( ))
        return;

    for( auto i = _streams.begin(); i != _streams.end(); )
    {
        const std::string& error = i->second->update();
        if( error.empty( ))
            ++i;
        else
        {
            _statusbar->showMessage( QString( error.c_str( )));
            i = _streams.erase( i );
        }
    }
}

void MainWindow::_deselect( ConstStreamPtr stream )
{
    if( _actionMultiWindowMode->isChecked( ))
    {
        const QPersistentModelIndex& index = stream->getIndex();
        if( index.isValid( ))
        {
            QItemSelectionModel* model = _listView->selectionModel();
            model->select( index, QItemSelectionModel::Deselect );
        }
    }
    else
        _streamButton->setChecked( false );
}

void MainWindow::_regulateFrameRate()
{
    // update smoothed average
    const int elapsed = _frameTime.elapsed();
    _averageUpdate = FRAME_RATE_DAMPING * elapsed +
                     (1.0f - FRAME_RATE_DAMPING) * _averageUpdate;

    // frame rate limiting
    const int desiredFrameTime = int( 0.5f + 1000.f /
                                      float( _maxFrameRateSpinBox->value( )));
    const int sleepTime = std::max( desiredFrameTime - int( _averageUpdate ),
                                    SHARE_DESKTOP_UPDATE_DELAY );
    _updateTimer.start( sleepTime );

    if( !_streams.empty( ))
    {
        const int fps = int( 0.5f + 1000.f / ( _averageUpdate + sleepTime ));
        const std::string message = std::to_string( _streams.size( )) +
            ( _streams.size() == 1 ? " stream to " : " streams to " ) +
            _getStreamHost() + " @ " + std::to_string( fps ) + " fps";
        _statusbar->showMessage( message.c_str( ));
    }
}

std::string MainWindow::_getStreamHost() const
{
    if( _hostComboBox->findText( _hostComboBox->currentText( )) != -1 &&
            _hostComboBox->currentData().isValid( ))
    {
        // hardcoded preset (with associated data different than displayed text)
        return _hostComboBox->currentData().toString().toStdString();
    }

    // user input text, either stored or not entered (yet), no associated data
    return _hostComboBox->currentText().toStdString();
}

void MainWindow::_onStreamEventsBoxClicked( const bool checked )
{
    if( !checked )
        return;
    for( auto i : _streams )
        if( i.second->isConnected() && !i.second->isRegisteredForEvents( ))
            i.second->registerForEvents();
}

void MainWindow::_openAboutWidget()
{
    const int revision = deflect::Version::getRevision();

    std::ostringstream aboutMsg;
    aboutMsg << "Current version: " << deflect::Version::getString()
             << std::endl
             << "   git revision: " << std::hex << revision << std::dec;

    QMessageBox::about( this, "About DesktopStreamer", aboutMsg.str().c_str( ));
}
