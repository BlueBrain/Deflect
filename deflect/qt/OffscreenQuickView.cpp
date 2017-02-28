/*********************************************************************/
/* Copyright (c) 2016, EPFL/Blue Brain Project                       */
/*                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>     */
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
/* or implied, of Ecole polytechnique federale de Lausanne.          */
/*********************************************************************/

#include "OffscreenQuickView.h"

#include "QuickRenderer.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QThread>

namespace deflect
{
namespace qt
{
OffscreenQuickView::OffscreenQuickView(
    std::unique_ptr<QQuickRenderControl> renderControl, const RenderMode mode)
    : QQuickWindow{renderControl.get()}
    , _renderControl{std::move(renderControl)}
    , _mode{mode}
    , _qmlEngine{new QQmlEngine}
    , _qmlComponent{new QQmlComponent{_qmlEngine.get()}}
{
    if (!_qmlEngine->incubationController())
        _qmlEngine->setIncubationController(incubationController());

    // remote URL to QML components are loaded asynchronously
    connect(_qmlComponent.get(), &QQmlComponent::statusChanged, this,
            &OffscreenQuickView::_setupRootItem);

    // Now hook up the signals. For simplicity we don't differentiate between
    // renderRequested (only render is needed, no sync) and sceneChanged (polish
    // and sync is needed too).
    connect(_renderControl.get(), &QQuickRenderControl::renderRequested, this,
            &OffscreenQuickView::_requestRender);
    connect(_renderControl.get(), &QQuickRenderControl::sceneChanged, this,
            &OffscreenQuickView::_requestRender);
}

OffscreenQuickView::~OffscreenQuickView()
{
    killTimer(_renderTimer);
    _renderTimer = 0;
    killTimer(_stopRenderingDelayTimer);
    _stopRenderingDelayTimer = 0;

    _quickRenderer->stop();
    if (_quickRendererThread)
    {
        _quickRendererThread->quit();
        _quickRendererThread->wait();
    }

    // delete first to free scenegraph resources for following destructions
    _renderControl.reset();
}

std::future<bool> OffscreenQuickView::load(const QUrl& url)
{
    _qmlComponent->loadUrl(url);
    return _loadPromise.get_future();
}

QQuickItem* OffscreenQuickView::getRootItem() const
{
    return _rootItem.get();
}

QQmlEngine* OffscreenQuickView::getEngine() const
{
    return _qmlEngine.get();
}

QQmlContext* OffscreenQuickView::getRootContext() const
{
    return _qmlEngine->rootContext();
}

void OffscreenQuickView::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == _renderTimer)
        _render();
    else if (e->timerId() == _stopRenderingDelayTimer)
    {
        killTimer(_renderTimer);
        killTimer(_stopRenderingDelayTimer);
        _renderTimer = 0;
        _stopRenderingDelayTimer = 0;
    }
}

void OffscreenQuickView::_setupRootItem()
{
    disconnect(_qmlComponent.get(), &QQmlComponent::statusChanged, this,
               &OffscreenQuickView::_setupRootItem);

    if (_qmlComponent->isError())
    {
        for (const auto& error : _qmlComponent->errors())
            qWarning() << error.url() << error.line() << error;
        _loadPromise.set_value(false);
    }

    QObject* rootObject = _qmlComponent->create();
    if (_qmlComponent->isError())
    {
        for (const auto& error : _qmlComponent->errors())
            qWarning() << error.url() << error.line() << error;
        _loadPromise.set_value(false);
        return;
    }

    _rootItem.reset(qobject_cast<QQuickItem*>(rootObject));
    if (!_rootItem)
    {
        delete rootObject;
        _loadPromise.set_value(false);
        return;
    }
    _rootItem->setParentItem(contentItem());
    resize(_rootItem->width(), _rootItem->height());

    // emulate QQuickView::ResizeMode::SizeRootObjectToView
    connect(this, &QQuickWindow::widthChanged, _rootItem.get(),
            &QQuickItem::setWidth);
    connect(this, &QQuickWindow::heightChanged, _rootItem.get(),
            &QQuickItem::setHeight);

    // also bind view to root object, to allow programmatic resize of rootItem
    connect(_rootItem.get(), &QQuickItem::widthChanged,
            [this]() { setWidth(_rootItem->width()); });
    connect(_rootItem.get(), &QQuickItem::heightChanged,
            [this]() { setHeight(_rootItem->height()); });

    _loadPromise.set_value(true);
}

void OffscreenQuickView::_requestRender()
{
    killTimer(_stopRenderingDelayTimer);
    _stopRenderingDelayTimer = 0;

    if (_renderTimer == 0)
        _renderTimer = startTimer(5, Qt::PreciseTimer);
}

void OffscreenQuickView::_initRenderer()
{
    switch (_mode)
    {
    case RenderMode::MULTITHREADED:
#ifdef DEFLECTQT_MULTITHREADED
        _quickRenderer.reset(
            new QuickRenderer{*this, *_renderControl, true, RenderTarget::FBO});

        _quickRendererThread.reset(new QThread);
        _quickRendererThread->setObjectName("Render");

        // Call required to make QtGraphicalEffects work in the initial scene.
        _renderControl->prepareThread(_quickRendererThread.get());
        _quickRenderer->moveToThread(_quickRendererThread.get());
        _quickRendererThread->start();
#else
        throw std::runtime_error(
            "This version of deflect does not support multithreaded rendering");
#endif
        break;
    case RenderMode::SINGLETHREADED:
        _quickRenderer.reset(new QuickRenderer{*this, *_renderControl, false,
                                               RenderTarget::FBO});
        break;
    case RenderMode::DISABLED:
        _quickRenderer.reset(new QuickRenderer{*this, *_renderControl, false,
                                               RenderTarget::NONE});
        break;
    }

    connect(_quickRenderer.get(), &QuickRenderer::afterRender, this,
            &OffscreenQuickView::_afterRender, Qt::DirectConnection);

    _quickRenderer->init();
}

void OffscreenQuickView::_render()
{
    // Initialize the render control and our OpenGL resources. Do this as late
    // as possible to use the proper size reported by the rootItem.
    if (!_quickRenderer)
        _initRenderer();

    _renderControl->polishItems();
    _quickRenderer->render();

    if (_stopRenderingDelayTimer == 0)
        _stopRenderingDelayTimer = startTimer(5000 /*ms*/);
}

void OffscreenQuickView::_afterRender()
{
    // Called directly by the render thread just after the rendering is done.
    // The fbo can be safely assumed to be valid when this function executes.
    emit afterRender(_quickRenderer->fbo()->toImage());
}
}
}
