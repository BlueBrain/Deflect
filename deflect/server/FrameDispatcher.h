/*********************************************************************/
/* Copyright (c) 2013-2018, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
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

#ifndef DEFLECT_SERVER_FRAMEDISPATCHER_H
#define DEFLECT_SERVER_FRAMEDISPATCHER_H

#include <deflect/api.h>
#include <deflect/server/Tile.h>

#include <QObject>
#include <map>

namespace deflect
{
namespace server
{
/**
 * Gather Tiles from multiple sources and dispatch full frames.
 */
class FrameDispatcher : public QObject
{
    Q_OBJECT

public:
    /** Construct a dispatcher */
    FrameDispatcher(QObject* parent = nullptr);

    /** Destructor. */
    ~FrameDispatcher();

public slots:
    /**
     * Add a source of Tiles for a Stream.
     *
     * @param uri Identifier for the stream
     * @param sourceIndex Identifier for the source in this stream
     */
    void addSource(QString uri, size_t sourceIndex);

    /**
     * Remove a source of Tiles for a Stream.
     *
     * @param uri Identifier for the stream
     * @param sourceIndex Identifier for the source in this stream
     */
    void removeSource(QString uri, size_t sourceIndex);

    /**
     * Add a stream source as an observer which does not contribute tiles.
     * Emits pixelStreamOpened() if no other observer or source is present.
     *
     * @param uri Identifier for the stream
     */
    void addObserver(QString uri);

    /**
     * Remove a stream observer, emits pixelStreamClosed() if all observers and
     * sources are gone.
     *
     * @param uri Identifier for the stream
     */
    void removeObserver(QString uri);

    /**
     * Process a new Tile.
     *
     * @param uri Identifier for the stream
     * @param sourceIndex Identifier for the source in the stream
     * @param tile to process
     */
    void processTile(QString uri, size_t sourceIndex,
                     deflect::server::Tile tile);

    /**
     * The given source has finished sending Tiles for the current frame.
     *
     * @param uri Identifier for the stream
     * @param sourceIndex Identifier for the source in the stream
     */
    void processFrameFinished(QString uri, size_t sourceIndex);

    /**
     * Request the dispatching of a new frame for any stream (mono/stereo).
     *
     * A sendFrame() signal will be emitted for each of the view for which a
     * frame becomes available.
     *
     * Stereo left/right frames will only be be dispatched together when both
     * are available to ensure that the two eye channels remain synchronized.
     *
     * @param uri Identifier for the stream
     */
    void requestFrame(QString uri);

    /**
     * Delete all the buffers for a Stream.
     *
     * @param uri Identifier for the stream
     */
    void deleteStream(QString uri);

signals:
    /**
     * Notify that a pixel stream source has been rejected.
     *
     * @param uri Identifier for the stream
     * @param sourceIndex Identifier for the source in the stream
     */
    void sourceRejected(QString uri, size_t sourceIndex);

    /**
     * Notify that a PixelStream has been opened.
     *
     * @param uri Identifier for the stream
     */
    void pixelStreamOpened(QString uri);

    /**
     * Notify that an unexpected but non-fatal event occured.
     *
     * @param uri Identifier for the stream
     * @param what The description of the exception that occured
     */
    void pixelStreamWarning(QString uri, QString what);

    /**
     * Notify that an error occured and the stream should be closed.
     *
     * @param uri Identifier for the stream
     * @param what The description of the exception that occured
     */
    void pixelStreamError(QString uri, QString what);

    /**
     * Notify that a pixel stream has been closed.
     *
     * @param uri Identifier for the stream
     */
    void pixelStreamClosed(QString uri);

    /**
     * Dispatch a full frame.
     *
     * @param frame The latest frame available for a stream
     */
    void sendFrame(deflect::server::FramePtr frame);

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};
}
}

#endif
