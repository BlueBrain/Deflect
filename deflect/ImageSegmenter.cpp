/*********************************************************************/
/* Copyright (c) 2013-2014, EPFL/Blue Brain Project                  */
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

#include "ImageJpegCompressor.h"
#include "ImageWrapper.h"
#include "PixelStreamSegment.h"

#include <QtConcurrentMap>

namespace deflect
{

ImageSegmenter::ImageSegmenter()
    : nominalSegmentWidth_(0)
    , nominalSegmentHeight_(0)
{
}

bool ImageSegmenter::generate( const ImageWrapper& image,
                               const Handler& handler ) const
{
    if (image.compressionPolicy == COMPRESSION_ON)
        return generateJpeg( image, handler );
    return generateRaw( image, handler );
}

/**
 * The SegmentCompressionWrapper struct is used to pass additional parameters
 * to the JpegCompression function. It is required because
 * QtConcurrent::blockingMapped only allows one parameter to be passed to the
 * function being called.
 */
struct SegmentCompressionWrapper
{
    PixelStreamSegment segment;
    ImageSegmenter::Handler handler;
    const ImageWrapper* image;
    bool* result;

    SegmentCompressionWrapper( const ImageWrapper& image_,
                               const ImageSegmenter::Handler& handler_,
                               bool& res )
        : handler( handler_ )
        , image( &image_ )
        , result( &res )
    {}
};

// use libjpeg-turbo for JPEG conversion
void computeJpeg( SegmentCompressionWrapper& task )
{
    QRect imageRegion( task.segment.parameters.x - task.image->x,
                       task.segment.parameters.y - task.image->y,
                       task.segment.parameters.width,
                       task.segment.parameters.height);
    ImageJpegCompressor compressor;

    task.segment.imageData = compressor.computeJpeg( *task.image, imageRegion );
    if( !task.handler( task.segment ))
        *task.result = false;
}

bool ImageSegmenter::generateJpeg( const ImageWrapper& image,
                                   const Handler& handler ) const
{
    const SegmentParameters& segmentParams = generateSegmentParameters( image );

    // The resulting Jpeg segments
    bool result = true;
    std::vector<SegmentCompressionWrapper> tasks;
    for( SegmentParameters::const_iterator it = segmentParams.begin();
         it != segmentParams.end(); ++it )
    {
        SegmentCompressionWrapper task( image, handler, result );
        task.segment.parameters = *it;
        tasks.push_back( task );
    }

    // create JPEGs for each segment, in parallel
    QtConcurrent::blockingMap( tasks, &computeJpeg );
    return result;
}

bool ImageSegmenter::generateRaw( const ImageWrapper &image,
                                  const Handler& handler ) const
{
    const SegmentParameters& segmentParams = generateSegmentParameters( image );

    // resulting Raw segments
    for( SegmentParameters::const_iterator it = segmentParams.begin();
         it != segmentParams.end(); ++it )
    {
        PixelStreamSegment segment;
        segment.parameters = *it;
        segment.imageData.reserve( segment.parameters.width *
                                   segment.parameters.height *
                                   image.getBytesPerPixel( ));

        if (segmentParams.size() == 1)
        {
            // If we are not segmenting the image, just append the image data
            segment.imageData.append((const char*)image.data,
                                     int(image.getBufferSize()));
        }
        else // Copy the image subregion
        {
            // assume imageBuffer isn't padded
            const size_t imagePitch = image.width * image.getBytesPerPixel();
            const char* lineData = (const char*)image.data +
                segment.parameters.y*imagePitch +
                segment.parameters.x*image.getBytesPerPixel();

            for (unsigned int i=0; i < segment.parameters.height; ++i)
            {
                segment.imageData.append( lineData, segment.parameters.width *
                                                    image.getBytesPerPixel( ));
                lineData += imagePitch;
            }
        }

        if( !handler( segment ))
            return false;
    }

    return true;
}

void ImageSegmenter::setNominalSegmentDimensions(const unsigned int nominalSegmentWidth, const unsigned int nominalSegmentHeight)
{
    nominalSegmentWidth_ = nominalSegmentWidth;
    nominalSegmentHeight_ = nominalSegmentHeight;
}

#ifdef UNIORM_SEGMENT_WIDTH
SegmentParameters ImageSegmenter::generateSegmentParameters(const ImageWrapper &image) const
{
    unsigned int numSubdivisionsX = 1;
    unsigned int numSubdivisionsY = 1;

    unsigned int uniformSegmentWidth = image.width;
    unsigned int uniformSegmentHeight = image.height;

    bool segmentImage = (nominalSegmentWidth_ > 0 && nominalSegmentHeight_ > 0);
    if (segmentImage)
    {
        numSubdivisionsX = (unsigned int)floor((float)image.width / (float)nominalSegmentWidth_ + 0.5);
        numSubdivisionsY = (unsigned int)floor((float)image.height / (float)nominalSegmentHeight_ + 0.5);

        uniformSegmentWidth = (unsigned int)((float)image.width / (float)numSubdivisionsX);
        uniformSegmentHeight = (unsigned int)((float)image.height / (float)numSubdivisionsY);
    }

    // now, create parameters for each segment
    SegmentParameters parameters;

    for(unsigned int i=0; i<numSubdivisionsX; i++)
    {
        for(unsigned int j=0; j<numSubdivisionsY; j++)
        {
            PixelStreamSegmentParameters p;

            p.x = image.x + i * uniformSegmentWidth;
            p.y = image.y + j * uniformSegmentHeight;
            p.width = uniformSegmentWidth;
            p.height = uniformSegmentHeight;

            p.compressed = (image.compressionPolicy == COMPRESSION_ON);

            parameters.push_back(p);
        }
    }

    return parameters;
}
#else
SegmentParameters ImageSegmenter::generateSegmentParameters(const ImageWrapper &image) const
{
    unsigned int numSubdivisionsX = 1;
    unsigned int numSubdivisionsY = 1;

    unsigned int lastSegmentWidth = image.width;
    unsigned int lastSegmentHeight = image.height;

    bool segmentImage = (nominalSegmentWidth_ > 0 && nominalSegmentHeight_ > 0);
    if (segmentImage)
    {
        numSubdivisionsX = image.width / nominalSegmentWidth_ + 1;
        numSubdivisionsY = image.height / nominalSegmentHeight_ + 1;

        lastSegmentWidth = image.width % nominalSegmentWidth_;
        lastSegmentHeight = image.height % nominalSegmentHeight_;

        if (lastSegmentWidth == 0)
        {
            lastSegmentWidth = nominalSegmentWidth_;
            --numSubdivisionsX;
        }
        if (lastSegmentHeight == 0)
        {
            lastSegmentHeight = nominalSegmentHeight_;
            --numSubdivisionsY;
        }
    }

    // now, create parameters for each segment
    SegmentParameters parameters;

    for(unsigned int j=0; j<numSubdivisionsY; j++)
    {
        for(unsigned int i=0; i<numSubdivisionsX; i++)
        {
            PixelStreamSegmentParameters p;

            p.x = image.x + i * nominalSegmentWidth_;
            p.y = image.y + j * nominalSegmentHeight_;
            p.width = (i < numSubdivisionsX-1) ? nominalSegmentWidth_ : lastSegmentWidth;
            p.height = (j < numSubdivisionsY-1) ? nominalSegmentHeight_ : lastSegmentHeight;

            p.compressed = (image.compressionPolicy == COMPRESSION_ON);

            parameters.push_back(p);
        }
    }

    return parameters;
}
#endif

}
