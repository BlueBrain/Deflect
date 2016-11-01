/*********************************************************************/
/* Copyright (c) 2016, EPFL/Blue Brain Project                       */
/*                     Daniel.Nachbaur@epfl.ch                       */
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
/* or implied, of Ecole polytechnique federale de Lausanne.          */
/*********************************************************************/

#ifndef QUICKRENDERER_H
#define QUICKRENDERER_H

#include <QMutex>
#include <QObject>
#include <QOpenGLFramebufferObject>
#include <QWaitCondition>

QT_FORWARD_DECLARE_CLASS(QOffscreenSurface)
QT_FORWARD_DECLARE_CLASS(QOpenGLContext)
QT_FORWARD_DECLARE_CLASS(QQuickRenderControl)
QT_FORWARD_DECLARE_CLASS(QQuickWindow)
QT_FORWARD_DECLARE_CLASS(QSurface)

namespace deflect
{
namespace qt
{

/**
 * Renders the QML scene from the given window using QQuickRenderControl onto
 * the surface of the window or to an offscreen surface. Note that this object
 * needs to be moved to a seperate (render)thread to function properly.
 *
 * Inspired by http://doc.qt.io/qt-5/qtquick-rendercontrol-window-multithreaded-cpp.html
 */
class QuickRenderer : public QObject
{
    Q_OBJECT

public:
    /**
     * After construction, move the object to a dedicated render thread before
     * calling any other function.
     *
     * @param quickWindow the window to render into if not @sa offscreen,
     *                    otherwise an FBO with the windows' size is attached as
     *                    the render target.
     * @param renderControl associated with the QML scene of quickWindow to
     *                      trigger the actual rendering.
     * @param multithreaded whether the QuickRenderer is used in a multithreaded
     *                      fashion and should setup accordingly
     * @param offscreen render into an offscreen surface rather than the
     *                  quickWindow. It creates an FBO internally to hold the
     *                  rendered pixels.
     */
    QuickRenderer( QQuickWindow& quickWindow,
                   QQuickRenderControl& renderControl,
                   bool multithreaded = true, bool offscreen = false );

    /** @return OpenGL context used for rendering; lives in render thread. */
    QOpenGLContext* context() { return _context; }

    /**
     * @return nullptr if !offscreen, otherwise accessible in afterRender();
     *         lives in render thread.
     */
    QOpenGLFramebufferObject* fbo() { return _fbo; }

    /**
     * To be called from GUI/main thread to trigger rendering.
     *
     * This call is blocking until sync() is done in render thread.
     */
    void render();

signals:
    /**
     * Emitted at the end of render(). Does not do swapBuffers() in onscreen
     * case. Originates from render thread.
     */
    void afterRender();

    /** Emitted at the end of stop(). Originates from render thread. */
    void afterStop();

    /**
     * To be called from GUI/main thread to initialize this object on render
     * thread. Blocks until operation on render thread is done.
     */
    void init();

    /**
     * To be called from GUI/main thread to stop using this object on the render
     * thread. Blocks until operation on render thread is done.
     */
    void stop();

private:
    bool event( QEvent* qtEvent ) final;
    void _onInit();
    void _onRender();
    void _onStop();

    void _ensureFBO();
    QSurface* _getSurface();

    QQuickWindow& _quickWindow;
    QQuickRenderControl& _renderControl;

    QOpenGLContext* _context{ nullptr };
    QOffscreenSurface* _offscreenSurface{ nullptr };
    QOpenGLFramebufferObject* _fbo{ nullptr };

    bool _multithreaded;
    bool _offscreen;
    bool _initialized{ false };

    QMutex _mutex;
    QWaitCondition _cond;
};

}
}

#endif
