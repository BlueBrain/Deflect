/*********************************************************************/
/* Copyright (c) 2013-2016, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
/*                          Stefan.Eilemann@epfl.ch                  */
/*                          Daniel.Nachbaur@epfl.ch                  */
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

#include "StreamPrivate.h"

#include "Segment.h"
#include "SegmentParameters.h"
#include "SizeHints.h"
#include "Stream.h"
#include "StreamSendWorker.h"

#include <iostream>

#include <boost/thread/thread.hpp>
#define SEGMENT_SIZE 512

namespace deflect
{

StreamPrivate::StreamPrivate( const std::string &name_,
                              const std::string& address,
                              const unsigned short port )
    : name( name_ )
    , socket( address, port )
    , registeredForEvents( false )
{
    imageSegmenter.setNominalSegmentDimensions( SEGMENT_SIZE, SEGMENT_SIZE );

    if( name.empty( ))
        throw std::runtime_error( "Invalid Stream name: " + name );
}

StreamPrivate::~StreamPrivate()
{
    _sendWorker.reset();

    if( !socket.isConnected( ))
        return;

    sendClose();
}

void StreamPrivate::sendOpen()
{
    const MessageHeader mh( MESSAGE_TYPE_PIXELSTREAM_OPEN, 0, name );
    socket.send( mh, QByteArray( ));
}

void StreamPrivate::sendClose()
{
    const MessageHeader mh( MESSAGE_TYPE_QUIT, 0, name );
    socket.send( mh, QByteArray( ));
}

bool StreamPrivate::send( const ImageWrapper& image )
{
    if( image.compressionPolicy != COMPRESSION_ON &&
        image.pixelFormat != RGBA )
    {
        std::cerr << "Currently, RAW images can only be sent in RGBA format. "
                 "Other formats support remain to be implemented." << std::endl;
        return false;
    }

    const ImageSegmenter::Handler sendFunc =
        boost::bind( &StreamPrivate::sendPixelStreamSegment, this, _1 );
    return imageSegmenter.generate( image, sendFunc );
}

Stream::Future StreamPrivate::asyncSend( const ImageWrapper& image )
{
    if( !_sendWorker )
        _sendWorker.reset( new StreamSendWorker( *this ));

    return _sendWorker->enqueueImage( image );
}

bool StreamPrivate::finishFrame()
{
    // Open a window for the PixelStream
    const MessageHeader mh( MESSAGE_TYPE_PIXELSTREAM_FINISH_FRAME, 0, name );
    return socket.send( mh, QByteArray( ));
}

bool StreamPrivate::sendPixelStreamSegment( const Segment& segment )
{
    // Create message header
    const uint32_t segmentSize( sizeof( SegmentParameters ) +
                                segment.imageData.size( ));
    const MessageHeader mh( MESSAGE_TYPE_PIXELSTREAM, segmentSize, name );

    // This byte array will hold the message to be sent over the socket
    QByteArray message;

    // Message payload part 1: segment parameters
    message.append( (const char*)( &segment.parameters ),
                    sizeof( SegmentParameters ) );

    // Message payload part 2: image data
    message.append( segment.imageData );

    return socket.send( mh, message );
}

bool StreamPrivate::sendSizeHints( const SizeHints& hints )
{
    const MessageHeader mh( MESSAGE_TYPE_SIZE_HINTS, sizeof( hints ), name );

    QByteArray message;
    message.append( (const char*)( &hints ), sizeof( hints ) );
    return socket.send( mh, message );
}

}
