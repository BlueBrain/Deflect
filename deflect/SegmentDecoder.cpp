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

#include "SegmentDecoder.h"

#include "ImageJpegDecompressor.h"
#include "Segment.h"

#include <iostream>

#include <QFuture>
#include <QtConcurrentRun>

namespace deflect
{

class SegmentDecoder::Impl
{
public:
    Impl()
    {}

    /** The decompressor instance */
    ImageJpegDecompressor decompressor;

    /** Async image decoding future */
    QFuture<void> decodingFuture;
};

SegmentDecoder::SegmentDecoder()
    : _impl( new Impl )
{}

SegmentDecoder::~SegmentDecoder() {}

void decodeSegment( ImageJpegDecompressor* decompressor, Segment* segment )
{
    QByteArray decodedData = decompressor->decompress( segment->imageData );

    if( !decodedData.isEmpty( ))
    {
        segment->imageData = decodedData;
        segment->parameters.compressed = false;
    }
}

void SegmentDecoder::decode( Segment& segment )
{
    decodeSegment( &_impl->decompressor, &segment );
}

void SegmentDecoder::startDecoding( Segment& segment )
{
    // drop frames if we're currently processing
    if( isRunning( ))
    {
        std::cerr << "Decoding in process, Frame dropped. See if we need to "
                     "change this..." << std::endl;
        return;
    }

    _impl->decodingFuture = QtConcurrent::run( decodeSegment,
                                               &_impl->decompressor,
                                               &segment );
}

void SegmentDecoder::waitDecoding()
{
    _impl->decodingFuture.waitForFinished();
}

bool SegmentDecoder::isRunning() const
{
    return _impl->decodingFuture.isRunning();
}

}
