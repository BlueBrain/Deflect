/*********************************************************************/
/* Copyright (c) 2013-2017, EPFL/Blue Brain Project                  */
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

#ifndef DEFLECT_FRAMEDISPATCHER_H
#define DEFLECT_FRAMEDISPATCHER_H

#include <deflect/Segment.h>
#include <deflect/api.h>
#include <deflect/types.h>

#include <QObject>

namespace deflect
{
/**
 * Gather segments from multiple sources and dispatch full frames.
 */
class FrameDispatcher : public QObject
{
    Q_OBJECT

public:
    /** Construct a dispatcher */
    FrameDispatcher();

    /** Destructor. */
    ~FrameDispatcher();

public slots:
    /**
     * Add a source of Segments for a Stream.
     *
     * @param uri Identifier for the stream
     * @param sourceIndex Identifier for the source in this stream
     */
    void addSource(QString uri, size_t sourceIndex);

    /**
     * Remove a source of Segments for a Stream.
     *
     * @param uri Identifier for the stream
     * @param sourceIndex Identifier for the source in this stream
     */
    void removeSource(QString uri, size_t sourceIndex);

    /**
     * Process a new Segment.
     *
     * @param uri Identifier for the stream
     * @param sourceIndex Identifier for the source in the stream
     * @param segment to process
     * @param view to which the segment belongs
     */
    void processSegment(QString uri, size_t sourceIndex,
                        deflect::Segment segment, deflect::View view);

    /**
     * The given source has finished sending segments for the current frame.
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
     * Notify that a PixelStream has been opened.
     *
     * @param uri Identifier for the stream
     */
    void pixelStreamOpened(QString uri);

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
    void sendFrame(deflect::FramePtr frame);

    /**
     * Notify that a pixel stream has exceeded its maximum allowed size.
     *
     * @param uri Identifier for the stream
     */
    void bufferSizeExceeded(QString uri);

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};
}

#endif
