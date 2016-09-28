/*********************************************************************/
/* Copyright (c) 2013-2015, EPFL/Blue Brain Project                  */
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

#include <deflect/api.h>
#include <deflect/types.h>
#include <deflect/Segment.h>

#include <QObject>
#include <map>

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
    DEFLECT_API FrameDispatcher();

    /** Destructor. */
    DEFLECT_API ~FrameDispatcher();

public slots:
    /**
     * Add a source of Segments for a Stream.
     *
     * @param uri Identifier for the Stream
     * @param sourceIndex Identifier for the source in this stream
     */
    DEFLECT_API void addSource( QString uri, size_t sourceIndex );

    /**
     * Add a source of Segments for a Stream.
     *
     * @param uri Identifier for the Stream
     * @param sourceIndex Identifier for the source in this stream
     */
    DEFLECT_API void removeSource( QString uri, size_t sourceIndex );

    /**
     * Process a new Segement.
     *
     * @param uri Identifier for the Stream
     * @param sourceIndex Identifier for the source in this stream
     * @param segment The segment to process
     */
    DEFLECT_API void processSegment( QString uri, size_t sourceIndex,
                                     deflect::Segment segment );

    /**
     * The given source has finished sending segments for the current frame.
     *
     * @param uri Identifier for the Stream
     * @param sourceIndex Identifier for the source in this stream
     */
    DEFLECT_API void processFrameFinished( QString uri, size_t sourceIndex );

    /**
     * Delete an entire stream.
     *
     * @param uri Identifier for the Stream
     */
    DEFLECT_API void deleteStream( QString uri );

    /**
     * Called by the user to request the dispatching of a new frame.
     *
     * A sendFrame() signal will be emitted as soon as a frame is available.
     * @param uri Identifier for the stream
     */
    DEFLECT_API void requestFrame( QString uri );

signals:
    /**
     * Notify that a PixelStream has been opened.
     *
     * @param uri Identifier for the Stream
     */
    DEFLECT_API void openPixelStream( QString uri );

    /**
     * Notify that a pixel stream has been deleted.
     *
     * @param uri Identifier for the Stream
     */
    DEFLECT_API void deletePixelStream( QString uri );

    /**
     * Dispatch a full frame.
     *
     * @param frame The frame to dispatch
     */
    DEFLECT_API void sendFrame( deflect::FramePtr frame );

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};

}

#endif
