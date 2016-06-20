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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <apps/DesktopStreamer/ui_MainWindow.h>

#ifdef __APPLE__
#  include "AppNapSuspender.h"
#endif

#ifdef DEFLECT_USE_SERVUS
#  include <servus/servus.h>
#endif

#include <QMainWindow>
#include <QTimer>
#include <QTime>
#include <map>
#include <memory>

class Stream;

class MainWindow : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

    const QAbstractItemModel* getItemModel() const { return _listView->model();}

protected:
    void closeEvent( QCloseEvent* event ) final;

private slots:
    void _update();
    void _onStreamEventsBoxClicked( bool checked );
    void _openAboutWidget();

private:
    typedef std::shared_ptr< Stream > StreamPtr;
    typedef std::shared_ptr< const Stream > ConstStreamPtr;
    typedef std::map< QPersistentModelIndex, StreamPtr > StreamMap;
    StreamMap _streams;
    uint32_t _streamID;

    QTimer _updateTimer;
    QTime _frameTime;
    float _averageUpdate;

#ifdef __APPLE__
    AppNapSuspender _napSuspender;
#endif

    void _showMultiWindowMode();
    void _showSingleWindowMode();

    void _showRemoteControl( bool visible );
    void _showAdvancedSettings( bool visible );

    void _startStreaming();
    void _stopStreaming();
    void _updateStreams();
    void _updateMultipleStreams();
    void _updateSingleStream();
    void _showConnectionErrorStatus();

    void _deselect( ConstStreamPtr stream );
    void _processStreamEvents();
    void _shareDesktopUpdate();
    void _regulateFrameRate();
    std::string _getStreamHost() const;
};

#endif
