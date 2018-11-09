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

#define BOOST_TEST_MODULE TileDecoderTests

#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include <deflect/ImageJpegCompressor.h>
#include <deflect/ImageSegmenter.h>
#include <deflect/ImageWrapper.h>
#include <deflect/server/ImageJpegDecompressor.h>
#include <deflect/server/Tile.h>
#include <deflect/server/TileDecoder.h>

#include <QMutex>
#include <cmath> // std::round

namespace
{
int _toY(const int r, const int g, const int b)
{
    return std::round(0.299 * r + 0.587 * g + 0.114 * b);
}
int _toU(const int r, const int g, const int b)
{
    const auto u = 0.492 * (b - _toY(r, g, b));
    const auto uMinMax = 0.436;
    return std::round((u + 255.0 * uMinMax) / (2.0 * uMinMax));
}
int _toV(const int r, const int g, const int b)
{
    const auto v = 0.877 * (r - _toY(r, g, b));
    const auto vMinMax = 0.615;
    return std::round((v + 255.0 * vMinMax) / (2.0 * vMinMax));
}

const std::vector<char> expectedYData(8 * 8, _toY(92, 28, 0));
const std::vector<char> expectedUData(8 * 8, _toU(92, 28, 0));
const std::vector<char> expectedVData(8 * 8, _toV(92, 28, 0));
}

namespace deflect
{
inline std::ostream& operator<<(std::ostream& str, const Format t)
{
    str << "Format(" << as_underlying_type(t) << ")";
    return str;
}
inline std::ostream& operator<<(std::ostream& str, const ChromaSubsampling s)
{
    str << "ChromaSubsampling(" << as_underlying_type(s) << ")";
    return str;
}
}

std::vector<char> makeTestImage()
{
    std::vector<char> data;
    data.reserve(8 * 8 * 4);
    for (size_t i = 0; i < 8 * 8; ++i)
    {
        data.push_back(92); // R
        data.push_back(28); // G
        data.push_back(0);  // B
        data.push_back(-1); // A
    }
    return data;
}

BOOST_AUTO_TEST_CASE(testImageCompressionAndDecompression)
{
    // Vector of RGBA data
    const auto data = makeTestImage();
    deflect::ImageWrapper imageWrapper(data.data(), 8, 8, deflect::RGBA);
    imageWrapper.compressionQuality = 100;

    // Compress image
    deflect::ImageJpegCompressor compressor;
    QByteArray jpegData =
        compressor.computeJpeg(imageWrapper, QRect(0, 0, 8, 8));

    BOOST_REQUIRE(jpegData.size() > 0);
    BOOST_REQUIRE(jpegData.size() != (int)data.size());

    // Decompress image
    deflect::server::ImageJpegDecompressor decompressor;
    QByteArray decodedData = decompressor.decompress(jpegData);

    // Check decoded image in format RGBA
    BOOST_REQUIRE(!decodedData.isEmpty());
    BOOST_REQUIRE_EQUAL(decodedData.size(), data.size());

    const char* dataOut = decodedData.constData();
    BOOST_CHECK_EQUAL_COLLECTIONS(data.data(), data.data() + data.size(),
                                  dataOut, dataOut + data.size());
}

#ifndef DEFLECT_USE_LEGACY_LIBJPEGTURBO

QByteArray decodeToYUVWithDecompressor(
    const QByteArray& jpegData, const deflect::ChromaSubsampling expected)
{
    deflect::server::ImageJpegDecompressor decompressor;
    const auto yuvData = decompressor.decompressToYUV(jpegData);
    BOOST_CHECK_EQUAL(yuvData.second, expected);
    return yuvData.first;
}

QByteArray decodeToYUVWithTileDecoder(const QByteArray& jpegData,
                                      const deflect::ChromaSubsampling expected)
{
    deflect::server::Tile tile;
    tile.width = 8;
    tile.height = 8;
    tile.format = deflect::Format::jpeg;
    tile.imageData = QByteArray::fromRawData(jpegData.data(), jpegData.size());

    deflect::server::TileDecoder decoder;
    BOOST_CHECK_EQUAL(decoder.decodeType(tile), expected);

    decoder.decodeToYUV(tile);
    BOOST_CHECK_NE(tile.format, deflect::Format::jpeg);
    BOOST_CHECK_NE(tile.format, deflect::Format::rgba);
    return tile.imageData;
}

using DecodeFunc =
    std::function<QByteArray(const QByteArray&, deflect::ChromaSubsampling)>;

void testImageDecompressionToYUV(const deflect::ChromaSubsampling subsamp,
                                 DecodeFunc decode)
{
    // Vector of RGBA data
    const auto data = makeTestImage();
    deflect::ImageWrapper imageWrapper(data.data(), 8, 8, deflect::RGBA);
    imageWrapper.compressionQuality = 100;
    imageWrapper.subsampling = subsamp;

    // Compress image
    deflect::ImageJpegCompressor compressor;
    const auto jpegData =
        compressor.computeJpeg(imageWrapper, QRect(0, 0, 8, 8));

    BOOST_REQUIRE(jpegData.size() > 0);
    BOOST_REQUIRE(jpegData.size() != (int)data.size());

    const auto yuvImageData = decode(jpegData, subsamp);

    const auto imageSize = imageWrapper.width * imageWrapper.height;
    auto uvSize = imageSize;
    if (subsamp == deflect::ChromaSubsampling::YUV422)
        uvSize >>= 1;
    else if (subsamp == deflect::ChromaSubsampling::YUV420)
        uvSize >>= 2;

    // Check decoded image in format YUV
    BOOST_REQUIRE(!yuvImageData.isEmpty());
    BOOST_REQUIRE_EQUAL(yuvImageData.size(), imageSize + 2 * uvSize);

    // Y component
    const char* yData = expectedYData.data();
    const char* yDataOut = yuvImageData.constData();
    BOOST_CHECK_EQUAL_COLLECTIONS(yData, yData + expectedYData.size(), yDataOut,
                                  yDataOut + expectedYData.size());

    // U component
    const char* uData = expectedUData.data();
    const char* uDataOut = yuvImageData.constData() + imageSize;
    BOOST_CHECK_EQUAL_COLLECTIONS(uData, uData + uvSize, uDataOut,
                                  uDataOut + uvSize);

    // V component
    const char* vData = expectedVData.data();
    const char* vDataOut = yuvImageData.constData() + imageSize + uvSize;
    BOOST_CHECK_EQUAL_COLLECTIONS(vData, vData + uvSize, vDataOut,
                                  vDataOut + uvSize);
}

BOOST_AUTO_TEST_CASE(testImageCompressionAndDecompressionYUV)
{
    testImageDecompressionToYUV(deflect::ChromaSubsampling::YUV444,
                                &decodeToYUVWithDecompressor);
    testImageDecompressionToYUV(deflect::ChromaSubsampling::YUV422,
                                &decodeToYUVWithDecompressor);
    testImageDecompressionToYUV(deflect::ChromaSubsampling::YUV420,
                                &decodeToYUVWithDecompressor);

    testImageDecompressionToYUV(deflect::ChromaSubsampling::YUV444,
                                &decodeToYUVWithTileDecoder);
    testImageDecompressionToYUV(deflect::ChromaSubsampling::YUV422,
                                &decodeToYUVWithTileDecoder);
    testImageDecompressionToYUV(deflect::ChromaSubsampling::YUV420,
                                &decodeToYUVWithTileDecoder);
}

#endif

static bool append(deflect::Segments& segments, const deflect::Segment& segment)
{
    static QMutex lock;
    QMutexLocker locker(&lock);
    segments.push_back(segment);
    return true;
}

BOOST_AUTO_TEST_CASE(testImageSegmentationWithCompressionAndDecompression)
{
    // Vector of rgba data
    const auto data = makeTestImage();

    // Compress image
    deflect::ImageWrapper imageWrapper(data.data(), 8, 8, deflect::RGBA);
    imageWrapper.compressionPolicy = deflect::COMPRESSION_ON;

    deflect::Segments segments;
    deflect::ImageSegmenter segmenter;
    const auto appendFunc =
        std::bind(&append, std::ref(segments), std::placeholders::_1);

    segmenter.generate(imageWrapper, appendFunc);
    BOOST_REQUIRE_EQUAL(segments.size(), 1);

    deflect::Segment& segment = segments.front();
    BOOST_REQUIRE_EQUAL(segment.parameters.format, deflect::Format::jpeg);
    BOOST_REQUIRE(segment.imageData.size() != (int)data.size());

    // Decompress image
    deflect::server::Tile tile;
    tile.width = segment.parameters.width;
    tile.height = segment.parameters.height;
    tile.format = segment.parameters.format;
    tile.imageData = segment.imageData;

    deflect::server::TileDecoder decoder;
    decoder.startDecoding(tile);
    decoder.waitDecoding();

    // Check decoded image in format RGBA
    BOOST_REQUIRE_EQUAL(tile.format, deflect::Format::rgba);
    BOOST_REQUIRE_EQUAL(tile.imageData.size(), data.size());

    const char* dataOut = tile.imageData.constData();
    BOOST_CHECK_EQUAL_COLLECTIONS(data.data(),
                                  data.data() + tile.imageData.size(), dataOut,
                                  dataOut + tile.imageData.size());
}

BOOST_AUTO_TEST_CASE(testDecompressionOfInvalidData)
{
    const QByteArray invalidJpegData{"notjpeg923%^#8"};
    deflect::server::ImageJpegDecompressor decompressor;
    BOOST_CHECK_THROW(decompressor.decompress(invalidJpegData),
                      std::runtime_error);

    deflect::server::Tile tile;
    tile.width = 32;
    tile.height = 32;
    tile.imageData = invalidJpegData;

    deflect::server::TileDecoder decoder;
    BOOST_CHECK_THROW(decoder.decode(tile), std::runtime_error);

    BOOST_CHECK_NO_THROW(decoder.startDecoding(tile));
    BOOST_CHECK_THROW(decoder.waitDecoding(), std::runtime_error);
}
