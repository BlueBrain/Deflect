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

#ifndef DELFECT_QT_OFFSCREENQUICKVIEW_H
#define DELFECT_QT_OFFSCREENQUICKVIEW_H

#include <QQuickWindow>
#include <future>

QT_FORWARD_DECLARE_CLASS(QQmlComponent)
QT_FORWARD_DECLARE_CLASS(QQmlEngine)
QT_FORWARD_DECLARE_CLASS(QQuickItem)

namespace deflect
{
namespace qt
{
/**
 * The different modes of rendering.
 */
enum class RenderMode
{
    SINGLETHREADED, /**< Render and process events in the same thread */
    MULTITHREADED,  /**< Render in a thread separate from event processing */
    DISABLED        /**< Only process events without rendering */
};

class QuickRenderer;

/**
 * An offscreen Qt Quick window, similar to a QQuickView.
 */
class OffscreenQuickView : public QQuickWindow
{
    Q_OBJECT

public:
    /**
     * Create an offscreen qml view.
     * @param control the render control that will be used by the view.
     * @param mode the rendering mode
     */
    OffscreenQuickView(std::unique_ptr<QQuickRenderControl> control,
                       RenderMode mode = RenderMode::MULTITHREADED);

    /** Close the view, stopping the rendering. */
    ~OffscreenQuickView();

    /**
     * Load a qml file to render.
     * @param url qml file to load (will happen asynchronously if it is remote).
     * @return a future which will indicate the success of the operation.
     */
    std::future<bool> load(const QUrl& url);

    /** @return the root qml item after a successful load, else nullptr. */
    QQuickItem* getRootItem() const;

    /** @return the internal qml engine. */
    QQmlEngine* getEngine() const;

    /** @return the root qml context. */
    QQmlContext* getRootContext() const;

signals:
    /**
     * Notify that the scene has just finished rendering.
     *
     * @param image the newly rendered image.
     * @note this signal is emitted from the render thread. It is generally
     *       better to connect to it using an auto (queued) connection.
     */
    void afterRender(QImage image);

private:
    std::unique_ptr<QQuickRenderControl> _renderControl;
    const RenderMode _mode;

    std::unique_ptr<QQmlEngine> _qmlEngine;
    std::unique_ptr<QQmlComponent> _qmlComponent;
    std::unique_ptr<QQuickItem> _rootItem;

    std::unique_ptr<QThread> _quickRendererThread;
    std::unique_ptr<QuickRenderer> _quickRenderer;

    std::promise<bool> _loadPromise;

    int _renderTimer = 0;
    int _stopRenderingDelayTimer = 0;

    void timerEvent(QTimerEvent* e) final;

    void _setupRootItem();
    void _requestRender();
    void _initRenderer();
    void _render();
    void _afterRender();
};
}
}

#endif
