/*********************************************************************/
/* Copyright (c) 2013-2017, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
/*                          Stefan.Eilemann@epfl.ch                  */
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

#include "ImageSegmenter.h"

#include "ImageWrapper.h"
#ifdef DEFLECT_USE_LIBJPEGTURBO
#include "ImageJpegCompressor.h"
#endif

#include <QThreadStorage>
#include <QtConcurrentMap>

#include <iostream>
#include <stdexcept>

namespace deflect
{
namespace
{
bool _isOnRightSideOfSideBySideImage(const Segment& segment)
{
    return segment.sourceImage->view == View::side_by_side &&
           segment.view == View::right_eye;
}
}

bool ImageSegmenter::generate(const ImageWrapper& image, const Handler& handler)
{
    if (image.compressionPolicy == COMPRESSION_ON)
        return _generateJpeg(image, handler);
    return _generateRaw(image, handler);
}

Segment ImageSegmenter::compressSingleSegment(const ImageWrapper& image)
{
#ifdef DEFLECT_USE_LIBJPEGTURBO
    auto segments = _generateSegments(image);
    if (segments.size() > 1)
        throw std::runtime_error(
            "compressSingleSegment only works for small images");
    _computeJpeg(segments[0], false);
    return segments[0];
#else
    throw std::runtime_error(
        "LibJpegTurbo not available, needed for compressSingleSegment");
#endif
}

void ImageSegmenter::setNominalSegmentDimensions(const uint width,
                                                 const uint height)
{
    _nominalSegmentWidth = width;
    _nominalSegmentHeight = height;
}

bool ImageSegmenter::_generateJpeg(const ImageWrapper& image,
                                   const Handler& handler)
{
#ifdef DEFLECT_USE_LIBJPEGTURBO
    // The resulting Jpeg segments
    auto segments = _generateSegments(image);

    // start creating JPEGs for each segment, in parallel
    QtConcurrent::map(segments, std::bind(&ImageSegmenter::_computeJpeg, this,
                                          std::placeholders::_1, true));

    // Sending compressed jpeg segments while they arrive in the queue.
    // Note: Qt insists that sending (by calling handler()) should happen
    // exclusively from the QThread where the socket lives. Sending from the
    // worker threads triggers a qWarning.
    bool result = true;
    for (size_t i = 0; i < segments.size(); ++i)
        if (!handler(_sendQueue.dequeue()))
            result = false;
    return result;
#else
    static bool first = true;
    if (first)
    {
        first = false;
        std::cerr << "LibJpegTurbo not available, not using compression"
                  << std::endl;
    }
    return generateRaw(image, handler);
#endif
}

void ImageSegmenter::_computeJpeg(Segment& segment, const bool sendSegment)
{
#ifdef DEFLECT_USE_LIBJPEGTURBO
    QRect imageRegion(segment.parameters.x - segment.sourceImage->x,
                      segment.parameters.y - segment.sourceImage->y,
                      segment.parameters.width, segment.parameters.height);

    if (_isOnRightSideOfSideBySideImage(segment))
        imageRegion.translate(segment.sourceImage->width / 2, 0);

    // turbojpeg handles need to be per thread, and this function is called from
    // multiple threads by QtConcurrent::map
    static QThreadStorage<ImageJpegCompressor> compressor;
    segment.imageData =
        compressor.localData().computeJpeg(*segment.sourceImage, imageRegion);
    segment.parameters.dataType = DataType::jpeg;
    if (sendSegment)
        _sendQueue.enqueue(segment);
#endif
}

bool ImageSegmenter::_generateRaw(const ImageWrapper& image,
                                  const Handler& handler) const
{
    auto segments = _generateSegments(image);
    for (auto& segment : segments)
    {
        segment.imageData.reserve(segment.parameters.width *
                                  segment.parameters.height *
                                  image.getBytesPerPixel());
        segment.parameters.dataType = DataType::rgba;

        if (segments.size() == 1)
        {
            // If we are not segmenting the image, just append the image data
            segment.imageData.append((const char*)image.data,
                                     int(image.getBufferSize()));
        }
        else // Copy the image subregion
        {
            // assume imageBuffer isn't padded
            const auto bytesPerPixel = image.getBytesPerPixel();
            const size_t imagePitch = image.width * bytesPerPixel;
            size_t offset = segment.parameters.y * imagePitch +
                            segment.parameters.x * bytesPerPixel;

            if (_isOnRightSideOfSideBySideImage(segment))
                offset += segment.sourceImage->width / 2 * bytesPerPixel;

            const char* lineData = (const char*)image.data + offset;

            for (uint i = 0; i < segment.parameters.height; ++i)
            {
                segment.imageData.append(lineData, segment.parameters.width *
                                                       bytesPerPixel);
                lineData += imagePitch;
            }
        }

        if (!handler(segment))
            return false;
    }

    return true;
}

Segments ImageSegmenter::_generateSegments(const ImageWrapper& image) const
{
    Segments segments;
    for (const auto& params : _makeSegmentParameters(image))
    {
        Segment segment;
        segment.parameters = params;
        segment.view =
            image.view == View::side_by_side ? View::left_eye : image.view;
        segment.sourceImage = &image;
        segments.push_back(segment);
    }

    if (image.view == View::side_by_side)
    {
        if (image.width % 2 != 0)
            throw std::runtime_error("side_by_side image width must be even!");

        // create copy of segments for right view
        auto segmentsRight = segments;
        for (auto& segment : segmentsRight)
            segment.view = View::right_eye;

        segments.insert(segments.end(), segmentsRight.begin(),
                        segmentsRight.end());
    }

    return segments;
}

SegmentParametersList ImageSegmenter::_makeSegmentParameters(
    const ImageWrapper& image) const
{
    const auto info = _makeSegmentationInfo(image);

    SegmentParametersList parameters;
    for (uint j = 0; j < info.countY; ++j)
    {
        for (uint i = 0; i < info.countX; ++i)
        {
            SegmentParameters p;
            p.x = image.x + i * info.width;
            p.y = image.y + j * info.height;
            p.width = (i < info.countX - 1) ? info.width : info.lastWidth;
            p.height = (j < info.countY - 1) ? info.height : info.lastHeight;
            parameters.emplace_back(p);
        }
    }
    return parameters;
}

ImageSegmenter::SegmentationInfo ImageSegmenter::_makeSegmentationInfo(
    const ImageWrapper& image) const
{
    const auto imageWidth =
        image.view == View::side_by_side ? image.width / 2 : image.width;

    SegmentationInfo info;
    info.width = _nominalSegmentWidth;
    info.height = _nominalSegmentHeight;

    if (_nominalSegmentWidth == 0 || _nominalSegmentHeight == 0)
    {
        info.countX = 1;
        info.countY = 1;
        info.lastWidth = imageWidth;
        info.lastHeight = image.height;
        return info;
    }

    info.countX = imageWidth / _nominalSegmentWidth + 1;
    info.countY = image.height / _nominalSegmentHeight + 1;

    info.lastWidth = imageWidth % _nominalSegmentWidth;
    info.lastHeight = image.height % _nominalSegmentHeight;

    if (info.lastWidth == 0)
    {
        info.lastWidth = _nominalSegmentWidth;
        --info.countX;
    }
    if (info.lastHeight == 0)
    {
        info.lastHeight = _nominalSegmentHeight;
        --info.countY;
    }
    return info;
}
}
