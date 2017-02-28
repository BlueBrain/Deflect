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

#include "ImageJpegCompressor.h"

#include "ImageWrapper.h"

#include <iostream>

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

int getTurboJpegFormat(const PixelFormat pixelFormat)
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
        std::cerr << "unknown pixel format" << std::endl;
        return TJPF_RGB;
    }
}

QByteArray ImageJpegCompressor::computeJpeg(const ImageWrapper& sourceImage,
                                            const QRect& imageRegion)
{
    // tjCompress API is incorrect and takes a non-const input buffer, even
    // though it does not modify it. It can "safely" be cast to non-const
    // pointer to comply to the incorrect API.
    unsigned char* tjSrcBuffer = (unsigned char*)sourceImage.data;
    tjSrcBuffer +=
        imageRegion.y() * sourceImage.width * sourceImage.getBytesPerPixel();
    tjSrcBuffer += imageRegion.x() * sourceImage.getBytesPerPixel();

    const int tjWidth = imageRegion.width();
    // assume imageBuffer isn't padded
    const int tjPitch = sourceImage.width * sourceImage.getBytesPerPixel();
    const int tjHeight = imageRegion.height();
    const int tjPixelFormat = getTurboJpegFormat(sourceImage.pixelFormat);
    unsigned char* tjJpegBuf = 0;
    unsigned long tjJpegSize = 0;
    const int tjJpegSubsamp = TJSAMP_444;
    const int tjJpegQual = sourceImage.compressionQuality;
    const int tjFlags = 0; // or: TJFLAG_BOTTOMUP

    int err = tjCompress2(_tjHandle, tjSrcBuffer, tjWidth, tjPitch, tjHeight,
                          tjPixelFormat, &tjJpegBuf, &tjJpegSize, tjJpegSubsamp,
                          tjJpegQual, tjFlags);
    if (err != 0)
    {
        std::cerr << "libjpeg-turbo image conversion failure" << std::endl;
        return QByteArray();
    }

    // move the JPEG buffer to a byte array
    const QByteArray jpegData((char*)tjJpegBuf, tjJpegSize);

    // free the libjpeg-turbo allocated memory
    tjFree(tjJpegBuf);

    return jpegData;
}
}
