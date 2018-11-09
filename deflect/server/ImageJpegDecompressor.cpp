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

#include "ImageJpegDecompressor.h"

#include <iostream>
#include <stdexcept>

namespace
{
deflect::ChromaSubsampling _getSubsamp(const int tjJpegSubsamp)
{
    switch (tjJpegSubsamp)
    {
    case TJSAMP_444:
        return deflect::ChromaSubsampling::YUV444;
    case TJSAMP_422:
        return deflect::ChromaSubsampling::YUV422;
    case TJSAMP_420:
        return deflect::ChromaSubsampling::YUV420;
    default:
        throw std::runtime_error("unsupported subsampling format");
    }
}
}

namespace deflect
{
namespace server
{
ImageJpegDecompressor::ImageJpegDecompressor()
    : _tjHandle(tjInitDecompress())
{
}

ImageJpegDecompressor::~ImageJpegDecompressor()
{
    tjDestroy(_tjHandle);
}

JpegHeader ImageJpegDecompressor::decompressHeader(const QByteArray& jpegData)
{
    JpegHeader header;
    int jpegSubsamp = -1;

#ifdef TJ_NUMCS // introduced with tjDecompressHeader3()
    int jpegColorspace = -1;
    int err =
        tjDecompressHeader3(_tjHandle, (unsigned char*)jpegData.data(),
                            (unsigned long)jpegData.size(), &header.width,
                            &header.height, &jpegSubsamp, &jpegColorspace);
    if (err != 0 || jpegColorspace != TJCS_YCbCr)
#else
    int err = tjDecompressHeader2(_tjHandle, (unsigned char*)jpegData.data(),
                                  (unsigned long)jpegData.size(), &header.width,
                                  &header.height, &jpegSubsamp);
    if (err != 0)
#endif
        throw std::runtime_error("libjpeg-turbo header decompression failed");

    header.subsampling = _getSubsamp(jpegSubsamp);
    return header;
}

QByteArray ImageJpegDecompressor::decompress(const QByteArray& jpegData)
{
    const auto header = decompressHeader(jpegData);
    const int pixelFormat = TJPF_RGBX; // Format for OpenGL texture (GL_RGBA)
    const int pitch = header.width * tjPixelSize[pixelFormat];
    const int flags = TJ_FASTUPSAMPLE;

    QByteArray decodedData(header.height * pitch, Qt::Uninitialized);

    int err = tjDecompress2(_tjHandle, (unsigned char*)jpegData.data(),
                            (unsigned long)jpegData.size(),
                            (unsigned char*)decodedData.data(), header.width,
                            pitch, header.height, pixelFormat, flags);
    if (err != 0)
        throw std::runtime_error("libjpeg-turbo image decompression failed");

    return decodedData;
}

#ifndef DEFLECT_USE_LEGACY_LIBJPEGTURBO

ImageJpegDecompressor::YUVData ImageJpegDecompressor::decompressToYUV(
    const QByteArray& jpegData)
{
    const auto header = decompressHeader(jpegData);
    const int pad = 1; // no padding
    const int flags = 0;
    const int jpegSubsamp = int(header.subsampling);
    const auto decodedSize =
        tjBufSizeYUV2(header.width, pad, header.height, jpegSubsamp);

    auto decodedData = QByteArray(decodedSize, Qt::Uninitialized);

    int err = tjDecompressToYUV2(_tjHandle, (unsigned char*)jpegData.data(),
                                 (unsigned long)jpegData.size(),
                                 (unsigned char*)decodedData.data(),
                                 header.width, pad, header.height, flags);
    if (err != 0)
        throw std::runtime_error("libjpeg-turbo image decompression failed");

    return std::make_pair(std::move(decodedData), header.subsampling);
}

#endif
}
}
