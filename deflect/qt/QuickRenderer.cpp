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

#include "QuickRenderer.h"

#include <QCoreApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQuickRenderControl>
#include <QQuickWindow>

#ifdef DEFLECT_USE_QT5GUI
#  include <QtGui/private/qopenglcontext_p.h>
#endif

namespace deflect
{
namespace qt
{

QuickRenderer::QuickRenderer( QQuickWindow& quickWindow,
                              QQuickRenderControl& renderControl,
                              const bool multithreaded,
                              const RenderTarget target )
    : _quickWindow( quickWindow )
    , _renderControl( renderControl )
    , _multithreaded( multithreaded )
    , _renderTarget( target )
    , _initialized( false )
{
    const auto connectionType = multithreaded ?
                Qt::BlockingQueuedConnection : Qt::DirectConnection;

    connect( this, &QuickRenderer::init, this,
             &QuickRenderer::_onInit, connectionType );
    connect( this, &QuickRenderer::stop, this,
             &QuickRenderer::_onStop, connectionType );
}

QuickRenderer::~QuickRenderer() {}

void QuickRenderer::render()
{
    if( _multithreaded )
    {
        QMutexLocker lock( &_mutex );
        QCoreApplication::postEvent( this, new QEvent( QEvent::User ));

        // the main thread has to be blocked for sync()
        _cond.wait( &_mutex );
    }
    else
        _onRender();
}

bool QuickRenderer::event( QEvent* e )
{
    if( e->type() == QEvent::User )
    {
        _onRender();
        return true;
    }
    return QObject::event( e );
}

void QuickRenderer::_onInit()
{
    if( _renderTarget == RenderTarget::NONE )
        return;

    // Qt Quick may need a depth and stencil buffer
    QSurfaceFormat format_;
    format_.setDepthBufferSize( 16 );
    format_.setStencilBufferSize( 8 );

    _context.reset( new QOpenGLContext );
    _context->setFormat( format_ );
    _context->create();

    // Test if user has setup shared GL contexts (QtWebEngine::initialize).
    // If so, setup global share context needed by the Qml WebEngineView.
    if( QCoreApplication::testAttribute( Qt::AA_ShareOpenGLContexts ))
#if DEFLECT_USE_QT5GUI
        qt_gl_set_global_share_context( _context.get( ));
#else
        qWarning() << "DeflectQt was not compiled with WebEngineView support";
#endif

    if( _renderTarget == RenderTarget::FBO )
    {
        _offscreenSurface.reset( new QOffscreenSurface );
        // Pass _context->format(), not format_. Format does not specify and color
        // buffer sizes, while the context, that has just been created, reports a
        // format that has these values filled in. Pass this to the offscreen
        // surface to make sure it will be compatible with the context's
        // configuration.
        _offscreenSurface->setFormat( _context->format( ));
        _offscreenSurface->create();
    }

    _context->makeCurrent( _getSurface( ));
    _renderControl.initialize( _context.get( ));
    _initialized = true;
}

void QuickRenderer::_onStop()
{
    if( _context )
        _context->makeCurrent( _getSurface( ));

    _renderControl.invalidate();

    _fbo.reset();

    if( _context )
        _context->doneCurrent();

    _offscreenSurface.reset();
#if DEFLECT_USE_QT5GUI
    qt_gl_set_global_share_context( nullptr );
#endif
    _context.reset();
}

void QuickRenderer::_ensureFBO()
{
    const auto winSize = _quickWindow.size() * _quickWindow.devicePixelRatio();

    if( !_fbo || _fbo->size() != winSize )
    {
        const auto attachment = QOpenGLFramebufferObject::CombinedDepthStencil;
        _fbo.reset( new QOpenGLFramebufferObject( winSize, attachment ));
        _quickWindow.setRenderTarget( _fbo.get( ));
    }
}

QSurface* QuickRenderer::_getSurface()
{
    if( _offscreenSurface )
        return _offscreenSurface.get();
    return &_quickWindow;
}

void QuickRenderer::_onRender()
{
    if( !_initialized || _renderTarget == RenderTarget::NONE )
        return;

    {
        QMutexLocker lock( &_mutex );

        _context->makeCurrent( _getSurface( ));

        if( _renderTarget == RenderTarget::FBO )
            _ensureFBO();

        _renderControl.sync();

        // unblock gui thread after sync in render thread is done
        if( _multithreaded )
            _cond.wakeOne();
    }

    _renderControl.render();
    _context->functions()->glFinish();

    emit afterRender();
}

}
}
