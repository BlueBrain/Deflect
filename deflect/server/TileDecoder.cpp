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
/*    THIS  SOFTWARE  IS  PROVIDED  BY  THE  ECOLE  POLYTECHNIQUE    */
/*    FEDERALE DE LAUSANNE  ''AS IS''  AND ANY EXPRESS OR IMPLIED    */
/*    WARRANTIES, INCLUDING, BUT  NOT  LIMITED  TO,  THE  IMPLIED    */
/*    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  A PARTICULAR    */
/*    PURPOSE  ARE  DISCLAIMED.  IN  NO  EVENT  SHALL  THE  ECOLE    */
/*    POLYTECHNIQUE  FEDERALE  DE  LAUSANNE  OR  CONTRIBUTORS  BE    */
/*    LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,    */
/*    EXEMPLARY,  OR  CONSEQUENTIAL  DAMAGES  (INCLUDING, BUT NOT    */
/*    LIMITED TO,  PROCUREMENT  OF  SUBSTITUTE GOODS OR SERVICES;    */
/*    LOSS OF USE, DATA, OR  PROFITS;  OR  BUSINESS INTERRUPTION)    */
/*    HOWEVER CAUSED AND  ON ANY THEORY OF LIABILITY,  WHETHER IN    */
/*    CONTRACT, STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE    */
/*    OR OTHERWISE) ARISING  IN ANY WAY  OUT OF  THE USE OF  THIS    */
/*    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.   */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of Ecole polytechnique federale de Lausanne.          */
/*********************************************************************/

#include "TileDecoder.h"

#include "ImageJpegDecompressor.h"
#include "Tile.h"

#include <QFuture>
#include <QtConcurrentRun>

#include <iostream>

namespace deflect
{
namespace server
{
class TileDecoder::Impl
{
public:
    Impl() {}
    /** The decompressor instance */
    ImageJpegDecompressor decompressor;

    /** Async image decoding future */
    QFuture<void> decodingFuture;
};

TileDecoder::TileDecoder()
    : _impl(new Impl)
{
}

TileDecoder::~TileDecoder()
{
}

ChromaSubsampling TileDecoder::decodeType(const Tile& tile)
{
    if (tile.format != Format::jpeg)
        throw std::runtime_error("Tile is not in JPEG format");

    return _impl->decompressor.decompressHeader(tile.imageData).subsampling;
}

size_t _getExpectedSize(const Format format, const Tile& tile)
{
    const size_t imageSize = tile.height * tile.width;
    switch (format)
    {
    case Format::rgba:
        return imageSize * 4;
    case Format::yuv444:
        return imageSize * 3;
    case Format::yuv422:
        return imageSize * 2;
    case Format::yuv420:
        return imageSize + (imageSize >> 1);
    default:
        return 0;
    };
}

void _decodeTile(ImageJpegDecompressor* decompressor, Tile* tile,
                 const bool skipRgbConversion)
{
    if (tile->format != Format::jpeg)
        return;

    QByteArray decodedData;
    Format format;
    try
    {
#ifndef DEFLECT_USE_LEGACY_LIBJPEGTURBO
        if (skipRgbConversion)
        {
            const auto yuv = decompressor->decompressToYUV(tile->imageData);
            decodedData = yuv.first;
            switch (yuv.second)
            {
            case ChromaSubsampling::YUV444:
                format = Format::yuv444;
                break;
            case ChromaSubsampling::YUV422:
                format = Format::yuv422;
                break;
            case ChromaSubsampling::YUV420:
                format = Format::yuv420;
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
            decodedData = decompressor->decompress(tile->imageData);
            format = Format::rgba;
        }
    }
    catch (const std::runtime_error&)
    {
        throw;
    }

    const auto expectedSize = _getExpectedSize(format, *tile);
    if (size_t(decodedData.size()) != expectedSize)
        throw std::runtime_error("unexpected tile size");

    tile->imageData = decodedData;
    tile->format = format;
}

void TileDecoder::decode(Tile& tile)
{
    _decodeTile(&_impl->decompressor, &tile, false);
}

#ifndef DEFLECT_USE_LEGACY_LIBJPEGTURBO

void TileDecoder::decodeToYUV(Tile& tile)
{
    _decodeTile(&_impl->decompressor, &tile, true);
}

#endif

void TileDecoder::startDecoding(Tile& tile)
{
    // drop frames if we're currently processing
    if (isRunning())
        return;

    _impl->decodingFuture =
        QtConcurrent::run(_decodeTile, &_impl->decompressor, &tile, false);
}

void TileDecoder::waitDecoding()
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
        throw std::runtime_error("Tile decoding failed");
    }
}

bool TileDecoder::isRunning() const
{
    return _impl->decodingFuture.isRunning();
}
}
}
