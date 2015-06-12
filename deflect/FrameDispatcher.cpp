/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
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
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#include "FrameDispatcher.h"

#include "Frame.h"

namespace deflect
{

FrameDispatcher::FrameDispatcher()
{
}

void FrameDispatcher::addSource( const QString uri, const size_t sourceIndex )
{
    streamBuffers_[uri].addSource( sourceIndex );

    if( streamBuffers_[uri].getSourceCount() == 1 )
        emit openPixelStream( uri );
}

void FrameDispatcher::removeSource( const QString uri,
                                    const size_t sourceIndex )
{
    if( !streamBuffers_.count( uri ))
        return;

    streamBuffers_[uri].removeSource( sourceIndex );

    if( streamBuffers_[uri].getSourceCount() == 0 )
        deleteStream( uri );
}

void FrameDispatcher::processSegment( const QString uri,
                                      const size_t sourceIndex,
                                      Segment segment )
{
    if( streamBuffers_.count( uri ))
        streamBuffers_[uri].insertSegment( segment, sourceIndex );
}

void FrameDispatcher::processFrameFinished( const QString uri,
                                            const size_t sourceIndex )
{
    if( !streamBuffers_.count( uri ))
        return;

    ReceiveBuffer& buffer = streamBuffers_[uri];
    buffer.finishFrameForSource( sourceIndex );

    if( buffer.isAllowedToSend( ))
        sendLatestFrame( uri );
}

void FrameDispatcher::deleteStream( const QString uri )
{
    if( streamBuffers_.count( uri ))
    {
        streamBuffers_.erase( uri );
        emit deletePixelStream( uri );
    }
}

void FrameDispatcher::requestFrame( const QString uri )
{
    if( !streamBuffers_.count( uri ))
        return;

    ReceiveBuffer& buffer = streamBuffers_[uri];
    buffer.setAllowedToSend( true );
    sendLatestFrame( uri );
}

void FrameDispatcher::sendLatestFrame( const QString& uri )
{
    FramePtr frame( new Frame );
    frame->uri = uri;

    ReceiveBuffer& buffer = streamBuffers_[uri];

    // Only send the latest frame
    while( buffer.hasCompleteFrame( ))
        frame->segments = buffer.popFrame();

    if( frame->segments.empty( ))
        return;

    // receiver will request a new frame once this frame was consumed
    buffer.setAllowedToSend( false );

    emit sendFrame( frame );
}

}
