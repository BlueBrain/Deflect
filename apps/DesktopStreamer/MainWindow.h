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

#include <QtGui>

#ifdef __APPLE__
#  include "AppNapSuspender.h"
#endif
#ifdef DEFLECT_USE_LUNCHBOX
#  include <lunchbox/servus.h>
#endif

namespace deflect
{
class Stream;
}

class DesktopSelectionWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

signals:
    void streaming(bool);

protected:
    virtual void closeEvent( QCloseEvent* event );

private slots:
    void shareDesktop(bool set);
    void showDesktopSelectionWindow(bool set);

    void update();
#ifdef DEFLECT_USE_LUNCHBOX
    void updateServus();
#endif

    void setCoordinates(const QRect coordinates);
    void updateCoordinates();

private:
    deflect::Stream* stream_;

    DesktopSelectionWindow* desktopSelectionWindow_;

    /** @name User Interface Elements */
    //@{
    QLineEdit hostnameLineEdit_;
    QLineEdit uriLineEdit_;
    QSpinBox xSpinBox_;
    QSpinBox ySpinBox_;
    QSpinBox widthSpinBox_;
    QSpinBox heightSpinBox_;
    QSpinBox frameRateSpinBox_;
    QLabel frameRateLabel_;
    QCheckBox eventsBox_;

    QAction* shareDesktopAction_;
    QAction* showDesktopSelectionWindowAction_;
    //@}

    /** @name Status */
    //@{
    int x_;
    int y_;
    int width_;
    int height_;
    //@}

    QImage cursor_;

    QTimer updateTimer_;

    // used for frame rate calculations
    std::vector<QTime> frameSentTimes_;

#ifdef __APPLE__
    AppNapSuspender napSuspender_;
#endif
#ifdef DEFLECT_USE_LUNCHBOX
    QTimer browseTimer_;
    lunchbox::Servus servus_;
#endif

    void setupUI();
    void generateCursorImage();

    void startStreaming();
    void stopStreaming();
    void handleStreamingError(const QString& errorMessage);
    void processStreamEvents();
    void shareDesktopUpdate();

    void sendMousePressEvent( const float x, const float y );
    void sendMouseMoveEvent( const float x, const float y );
    void sendMouseReleaseEvent( const float x, const float y );
    void sendMouseDoubleClickEvent( const float, const float );

    void regulateFrameRate(const int elapsedFrameTime);
};

#endif
