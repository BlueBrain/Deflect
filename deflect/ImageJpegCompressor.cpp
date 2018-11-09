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

#include "ImageJpegCompressor.h"

#include "ImageWrapper.h"

#include <iostream>
#include <sstream>

namespace deflect
{
ImageJpegCompressor::ImageJpegCompressor()
    : _tjHandle(tjInitCompress())
{
}

ImageJpegCompressor::~ImageJpegCompressor()
{
    tjDestroy(_tjHandle);
}

int _getTurboJpegFormat(const PixelFormat pixelFormat)
{
    switch (pixelFormat)
    {
    case RGB:
        return TJPF_RGB;
    case RGBA:
        return TJPF_RGBX;
    case ARGB:
        return TJPF_XRGB;
    case BGR:
        return TJPF_BGR;
    case BGRA:
        return TJPF_BGRX;
    case ABGR:
        return TJPF_XBGR;
    default:
        throw std::invalid_argument("unknown pixel format " +
                                    std::to_string((int)pixelFormat));
        return TJPF_RGB;
    }
}

int _getTurboJpegSubsamp(const ChromaSubsampling subsampling)
{
    switch (subsampling)
    {
    case ChromaSubsampling::YUV444:
        return TJSAMP_444;
    case ChromaSubsampling::YUV422:
        return TJSAMP_422;
    case ChromaSubsampling::YUV420:
        return TJSAMP_420;
    default:
        throw std::invalid_argument("unknown subsampling format " +
                                    std::to_string((int)subsampling));
        return TJSAMP_444;
    }
}

QByteArray ImageJpegCompressor::computeJpeg(const ImageWrapper& sourceImage,
                                            const QRect& imageRegion)
{
    // tjCompress API is incorrect and takes a non-const input buffer, even
    // though it does not modify it. It can "safely" be cast to non-const
    // pointer to comply with the incorrect API.
    unsigned char* tjSrcBuffer = (unsigned char*)sourceImage.data;
    if (!tjSrcBuffer)
        throw std::invalid_argument(
            "libjpeg-turbo image conversion failure: source image is NULL");

    tjSrcBuffer +=
        imageRegion.y() * sourceImage.width * sourceImage.getBytesPerPixel();
    tjSrcBuffer += imageRegion.x() * sourceImage.getBytesPerPixel();

    const int tjWidth = imageRegion.width();
    // assume imageBuffer isn't padded
    const int tjPitch = sourceImage.width * sourceImage.getBytesPerPixel();
    const int tjHeight = imageRegion.height();
    const int tjPixelFormat = _getTurboJpegFormat(sourceImage.pixelFormat);

    const int tjJpegSubsamp = _getTurboJpegSubsamp(sourceImage.subsampling);
    unsigned long tjJpegSize = tjBufSize(tjWidth, tjHeight, tjJpegSubsamp);

    _tjJpegBuf.resize(tjJpegSize);

    const int tjJpegQual = sourceImage.compressionQuality;
    const int tjFlags = TJFLAG_NOREALLOC; // or: TJFLAG_BOTTOMUP

    auto ptr = _tjJpegBuf.data();
    int err = tjCompress2(_tjHandle, tjSrcBuffer, tjWidth, tjPitch, tjHeight,
                          tjPixelFormat, &ptr, &tjJpegSize, tjJpegSubsamp,
                          tjJpegQual, tjFlags);
    if (err != 0)
    {
        std::stringstream msg;
        msg << "libjpeg-turbo image conversion failure: " << tjGetErrorStr();
        throw std::runtime_error(msg.str());
    }

    return QByteArray((const char*)ptr, tjJpegSize);
}
}
