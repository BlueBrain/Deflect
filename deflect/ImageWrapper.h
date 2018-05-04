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

#ifndef DEFLECT_IMAGEWRAPPER_H
#define DEFLECT_IMAGEWRAPPER_H

#include <cstddef>
#include <deflect/api.h>
#include <deflect/types.h>

namespace deflect
{
/**
 *  The PixelFormat describes the organisation of the bytes in the image buffer.
 *  Formats are 8 bits per channel unless specified otherwise.
 *  @version 1.0
 */
enum PixelFormat
{
    RGB,
    RGBA,
    ARGB,
    BGR,
    BGRA,
    ABGR
};

/** Image compression policy */
enum CompressionPolicy
{
    COMPRESSION_AUTO, /**< Implementation specific */
    COMPRESSION_ON,   /**< Force enable */
    COMPRESSION_OFF   /**< Force disable */
};

/**
 * A simple wrapper around an image data buffer.
 *
 * Used by the Stream class to send images to a remote Server instance. It also
 * contains fields to indicate if the image should be compressed for sending
 * (disabled by default).
 * @version 1.0
 */
struct ImageWrapper
{
    /**
     * ImageWrapper constructor
     *
     * The first pixel is the top-left corner of the image, going to the
     * bottom-right corner. Data arrays which follow the GL convention (as
     * obtained by glReadPixels()) should be reordered using swapYAxis() prior
     * to constructing the image wrapper.
     *
     * @param data The source image buffer, containing getBufferSize() bytes
     * @param width The width of the image
     * @param height The height of the image
     * @param format The format of the imageBuffer
     * @param x The global position of the image in the stream
     * @param y The global position of the image in the stream
     * @version 1.0
     */
    DEFLECT_API
    ImageWrapper(const void* data, unsigned int width, unsigned int height,
                 PixelFormat format, unsigned int x = 0, unsigned int y = 0);

    /** Pointer to the image data of size getBufferSize(). @version 1.0 */
    const void* const data;

    /** @name Dimensions */
    //@{
    const unsigned int width;  /**< The image width in pixels. @version 1.0 */
    const unsigned int height; /**< The image height in pixels. @version 1.0 */
                               //@}

    /**
     * The pixel format describing the arrangement of the data buffer.
     * @version 1.0
     */
    const PixelFormat pixelFormat;

    /** @name Position of the image in the stream */
    //@{
    const unsigned int x; /**< The X coordinate. @version 1.0 */
    const unsigned int y; /**< The Y coordinate. @version 1.0 */
    //@}

    /** @name Compression parameters */
    //@{
    CompressionPolicy compressionPolicy; /**< Is the image to be compressed
                                              (default: auto). @version 1.0 */
    unsigned int compressionQuality;     /**< Compression quality (1 worst,
                                              100 best, default: 75).
                                              @version 1.0 */
    ChromaSubsampling subsampling;       /**< Chrominance sub-sampling.
                                              (default: YUV444). @version 1.0 */
    //@}

    /**
     * The view that this image represents.
     * @version 1.0
     */
    View view = View::mono;

    /**
     * The order of the image's data rows in memory.
     *
     * Set this value to bottom_up if the image data is stored following the
     * OpenGL convention.
     *
     * All images that form a frame (possibly from multiple Streams) must have
     * the same row order, otherwise the frame is invalid. This is because the
     * frame's tiles need to be reordered in addtion to flipping them
     * individually.
     *
     * @version 1.0
     */
    RowOrder rowOrder = RowOrder::top_down;

    /**
     * The index of the channel which the image is a part of.
     * @version 1.0
     */
    uint8_t channel = 0;

    /**
     * Get the number of bytes per pixel based on the pixelFormat.
     * @version 1.0
     */
    DEFLECT_API unsigned int getBytesPerPixel() const;

    /**
     * Get the size of the data buffer in bytes: width*height*format.bpp.
     * @version 1.0
     */
    DEFLECT_API size_t getBufferSize() const;
};
}

#endif
