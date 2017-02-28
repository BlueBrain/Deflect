/*********************************************************************/
/* Copyright (c) 2015, EPFL/Blue Brain Project                       */
/*                     Daniel.Nachbaur <daniel.nachbaur@epfl.ch>     */
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

#ifndef DELFECT_QT_QMLSTREAMER_H
#define DELFECT_QT_QMLSTREAMER_H

#include <deflect/qt/api.h>

#include <QQmlEngine>
#include <QQuickItem>
#include <QString>

#include <memory>
#include <string>

namespace deflect
{
namespace qt
{
/** Based on http://doc.qt.io/qt-5/qtquick-rendercontrol-example.html
 *
 * This class renders the given QML file in an offscreen fashion and streams
 * on each update on the given Deflect stream. It automatically register also
 * for Deflect events, which can be directly handled in the QML.

 * Users can make connections to the "deflectgestures" context property to react
 * to certain gestures received as Event, currently swipe[Left|Right|Up|Down].
 *
 * When using a WebEngineView, users must call QtWebEngine::initialize() in the
 * QApplication before creating the streamer. Also, due to a limitiation in Qt,
 * the objectName property of any WebEngineView must be set to "webengineview".
 * This is necessary for it to receive keyboard events and to correct the
 * default behaviour of the tapAndHold gesture. Deflect will prevent the opening
 * of an on-screen context menu (which may crash the application) and instead
 * switch to a "mouse" interaction mode. This allows users to interact within
 * a WebGL canevas or select text instead of scrolling the page.
 */
class DEFLECTQT_API QmlStreamer : public QObject
{
    Q_OBJECT

public:
    /**
     * Construct a new qml streamer by loading the QML, accessible by
     * getRootItem() and sets up the Deflect stream.
     *
     * @param qmlFile URL to QML file to load.
     * @param streamHost host where the Deflect server is running.
     * @param streamId identifier for the Deflect stream (optional). Setting
     *        this value overrides the 'objectName' property of the root QML
     *        item. If neither is provided, "QmlStreamer" is used instead.
     */
    QmlStreamer(const QString& qmlFile, const std::string& streamHost,
                const std::string& streamId = std::string());

    ~QmlStreamer();

    /** Use asynchronous send of images via Deflect stream. Default off. */
    void useAsyncSend(bool async);

    /** @return the QML root item, might be nullptr if not ready yet. */
    QQuickItem* getRootItem();

    /** @return the QML engine. */
    QQmlEngine* getQmlEngine();

    /**
     * Send data to the Server.
     *
     * @param data the data buffer
     * @return true if the data could be sent, false otherwise
     */
    bool sendData(QByteArray data);

signals:
    /** Emitted when the stream has been closed. */
    void streamClosed();

private:
    QmlStreamer(const QmlStreamer&) = delete;
    QmlStreamer operator=(const QmlStreamer&) = delete;
    class Impl;
    std::unique_ptr<Impl> _impl;
};
}
}

#endif
