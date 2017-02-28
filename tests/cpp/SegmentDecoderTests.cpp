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

#define BOOST_TEST_MODULE SegmentDecoderTests
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include <deflect/ImageJpegCompressor.h>
#include <deflect/ImageJpegDecompressor.h>
#include <deflect/ImageSegmenter.h>
#include <deflect/ImageWrapper.h>
#include <deflect/Segment.h>
#include <deflect/SegmentDecoder.h>

#include <QMutex>

void fillTestImage(std::vector<char>& data)
{
    data.reserve(8 * 8 * 4);
    for (size_t i = 0; i < 8 * 8; ++i)
    {
        data.push_back(92); // R
        data.push_back(28); // G
        data.push_back(0);  // B
        data.push_back(-1); // A
    }
}

BOOST_AUTO_TEST_CASE(testImageCompressionAndDecompression)
{
    // Vector of RGBA data
    std::vector<char> data;
    fillTestImage(data);
    deflect::ImageWrapper imageWrapper(data.data(), 8, 8, deflect::RGBA);
    imageWrapper.compressionQuality = 100;

    // Compress image
    deflect::ImageJpegCompressor compressor;
    QByteArray jpegData =
        compressor.computeJpeg(imageWrapper, QRect(0, 0, 8, 8));

    BOOST_REQUIRE(jpegData.size() > 0);
    BOOST_REQUIRE(jpegData.size() != (int)data.size());

    // Decompress image
    deflect::ImageJpegDecompressor decompressor;
    QByteArray decodedData = decompressor.decompress(jpegData);

    // Check decoded image in format RGBA
    BOOST_REQUIRE(!decodedData.isEmpty());
    BOOST_REQUIRE_EQUAL(decodedData.size(), data.size());

    const char* dataOut = decodedData.constData();
    BOOST_CHECK_EQUAL_COLLECTIONS(data.data(), data.data() + data.size(),
                                  dataOut, dataOut + data.size());
}

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
    std::vector<char> data;
    fillTestImage(data);

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
    BOOST_REQUIRE(segment.parameters.compressed);
    BOOST_REQUIRE(segment.imageData.size() != (int)data.size());

    // Decompress image
    deflect::SegmentDecoder decoder;
    decoder.startDecoding(segment);
    decoder.waitDecoding();

    // Check decoded image in format RGBA
    BOOST_REQUIRE(!segment.parameters.compressed);
    BOOST_REQUIRE_EQUAL(segment.imageData.size(), data.size());

    const char* dataOut = segment.imageData.constData();
    BOOST_CHECK_EQUAL_COLLECTIONS(data.data(),
                                  data.data() + segment.imageData.size(),
                                  dataOut, dataOut + segment.imageData.size());
}

BOOST_AUTO_TEST_CASE(testDecompressionOfInvalidData)
{
    const QByteArray invalidJpegData{"notjpeg923%^#8"};
    deflect::ImageJpegDecompressor decompressor;
    BOOST_CHECK_THROW(decompressor.decompress(invalidJpegData),
                      std::runtime_error);

    deflect::Segment segment;
    segment.parameters.width = 32;
    segment.parameters.height = 32;
    segment.imageData = invalidJpegData;

    deflect::SegmentDecoder decoder;
    BOOST_CHECK_THROW(decoder.decode(segment), std::runtime_error);

    BOOST_CHECK_NO_THROW(decoder.startDecoding(segment));
    BOOST_CHECK_THROW(decoder.waitDecoding(), std::runtime_error);
}
