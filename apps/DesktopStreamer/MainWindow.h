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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <deflect/types.h>

#ifdef __APPLE__
#  include "AppNapSuspender.h"
#endif

#ifdef DEFLECT_USE_SERVUS
#  include <servus/servus.h>
#endif

#include <QtWidgets>

class DesktopSelectionWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

signals:
    void streaming( bool enabled );

protected:
    void closeEvent( QCloseEvent* event ) final;

private slots:
    void _shareDesktop( bool set );
    void _showDesktopSelectionWindow( bool set );

    void _update();
#ifdef DEFLECT_USE_SERVUS
    void _updateServus();
#endif

    void _setCoordinates( QRect coordinates );
    void _updateCoordinates();

    void _onStreamEventsBoxClicked( bool checked );
    void _openAboutWidget();

private:
    deflect::Stream* _stream;

    DesktopSelectionWindow* _desktopSelectionWindow;

    /** @name User Interface Elements */
    //@{
    QLineEdit _hostnameLineEdit;
    QLineEdit _uriLineEdit;
    QSpinBox _xSpinBox;
    QSpinBox _ySpinBox;
    QSpinBox _widthSpinBox;
    QSpinBox _heightSpinBox;
    QSpinBox _frameRateSpinBox;
    QLabel _frameRateLabel;
    QCheckBox _streamEventsBox;

    QAction* _shareDesktopAction;
    QAction* _showDesktopSelectionWindowAction;
    //@}

    /** @name Status */
    //@{
    int _x;
    int _y;
    int _width;
    int _height;
    //@}

    QImage _cursor;

    QTimer _updateTimer;

    // used for frame rate calculations
    std::vector<QTime> _frameSentTimes;

#ifdef __APPLE__
    AppNapSuspender _napSuspender;
#endif
#ifdef DEFLECT_USE_SERVUS
    QTimer _browseTimer;
    servus::Servus _servus;
#endif

    void _setupUI();
    void _generateCursorImage();

    void _startStreaming();
    void _stopStreaming();
    void _handleStreamingError( const QString& errorMessage );
    void _processStreamEvents();
    void _shareDesktopUpdate();

    void _sendMousePressEvent( float x, float y );
    void _sendMouseMoveEvent( float x, float y );
    void _sendMouseReleaseEvent( float x, float y );
    void _sendMouseDoubleClickEvent( float x, float y );

    void _regulateFrameRate( int elapsedFrameTime );
};

#endif
