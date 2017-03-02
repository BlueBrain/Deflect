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
    Impl() {}
    /** The decompressor instance */
    ImageJpegDecompressor decompressor;

    /** Async image decoding future */
    QFuture<void> decodingFuture;
};

SegmentDecoder::SegmentDecoder()
    : _impl(new Impl)
{
}

SegmentDecoder::~SegmentDecoder()
{
}

ChromaSubsampling SegmentDecoder::decodeType(const Segment& segment)
{
    if (segment.parameters.dataType != DataType::jpeg)
        throw std::runtime_error("Segment is not in JPEG format");

    return _impl->decompressor.decompressHeader(segment.imageData).subsampling;
}

size_t _getExpectedSize(const DataType dataType,
                        const SegmentParameters& params)
{
    const size_t imageSize = params.height * params.width;
    switch (dataType)
    {
    case DataType::rgba:
        return imageSize * 4;
    case DataType::yuv444:
        return imageSize * 3;
    case DataType::yuv422:
        return imageSize * 2;
    case DataType::yuv420:
        return imageSize + (imageSize >> 1);
    default:
        return 0;
    };
}

void _decodeSegment(ImageJpegDecompressor* decompressor, Segment* segment,
                    const bool skipRgbConversion)
{
    if (segment->parameters.dataType != DataType::jpeg)
        return;

    QByteArray decodedData;
    DataType dataType;
    try
    {
#ifndef DEFLECT_USE_LEGACY_LIBJPEGTURBO
        if (skipRgbConversion)
        {
            const auto yuv = decompressor->decompressToYUV(segment->imageData);
            decodedData = yuv.first;
            switch (yuv.second)
            {
            case ChromaSubsampling::YUV444:
                dataType = DataType::yuv444;
                break;
            case ChromaSubsampling::YUV422:
                dataType = DataType::yuv422;
                break;
            case ChromaSubsampling::YUV420:
                dataType = DataType::yuv420;
                break;
            default:
                throw std::runtime_error("unexpected ChromaSubsampling mode");
            };
        }
        else
#else
        Q_UNUSED(skipRgbConversion);
#endif
        {
            decodedData = decompressor->decompress(segment->imageData);
            dataType = DataType::rgba;
        }
    }
    catch (const std::runtime_error&)
    {
        throw;
    }

    const auto expectedSize = _getExpectedSize(dataType, segment->parameters);
    if (size_t(decodedData.size()) != expectedSize)
        throw std::runtime_error("unexpected segment size");

    segment->imageData = decodedData;
    segment->parameters.dataType = dataType;
}

void SegmentDecoder::decode(Segment& segment)
{
    _decodeSegment(&_impl->decompressor, &segment, false);
}

#ifndef DEFLECT_USE_LEGACY_LIBJPEGTURBO

void SegmentDecoder::decodeToYUV(Segment& segment)
{
    _decodeSegment(&_impl->decompressor, &segment, true);
}

#endif

void SegmentDecoder::startDecoding(Segment& segment)
{
    // drop frames if we're currently processing
    if (isRunning())
    {
        std::cerr << "Decoding in process, Frame dropped. See if we need to "
                     "change this..."
                  << std::endl;
        return;
    }

    _impl->decodingFuture =
        QtConcurrent::run(_decodeSegment, &_impl->decompressor, &segment,
                          false);
}

void SegmentDecoder::waitDecoding()
{
    try
    {
        _impl->decodingFuture.waitForFinished();
    }
    catch (const QUnhandledException&)
    {
        // Let Qt throws a QUnhandledException and rewrite the error message.
        // QtConcurrent::run can only forward QException subclasses, which does
        // not even work on 5.7.1: https://bugreports.qt.io/browse/QTBUG-58021
        throw std::runtime_error("Segment decoding failed");
    }
}

bool SegmentDecoder::isRunning() const
{
    return _impl->decodingFuture.isRunning();
}
}
