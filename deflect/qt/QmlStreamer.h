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

#ifndef QMLSTREAMER_H
#define QMLSTREAMER_H

#include <deflect/qt/api.h>

#include <QString>
#include <QQuickItem>
#include <QQmlEngine>

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
 */
class QmlStreamer : public QObject
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
    DEFLECTQT_API QmlStreamer( const QString& qmlFile,
                               const std::string& streamHost,
                               const std::string& streamId = std::string( ));

    DEFLECTQT_API ~QmlStreamer();

    /** @return the QML root item, might be nullptr if not ready yet. */
    DEFLECTQT_API QQuickItem* getRootItem();

    /** @return the QML engine. */
    DEFLECTQT_API QQmlEngine* getQmlEngine();

signals:
    /** Emitted when the stream has been closed. */
    void streamClosed();

private:
    QmlStreamer( const QmlStreamer& ) = delete;
    QmlStreamer operator=( const QmlStreamer& ) = delete;
    class Impl;
    std::unique_ptr< Impl > _impl;
};

}
}

#endif
