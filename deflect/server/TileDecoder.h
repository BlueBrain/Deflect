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

#ifndef DEFLECT_SERVER_TILEDECODER_H
#define DEFLECT_SERVER_TILEDECODER_H

#include <deflect/api.h>
#include <deflect/defines.h>
#include <deflect/server/types.h>

namespace deflect
{
namespace server
{
/**
 * Decode a Tile's image asynchronously.
 */
class TileDecoder
{
public:
    /** Construct a Decoder */
    DEFLECT_API TileDecoder();

    /** Destruct a Decoder */
    DEFLECT_API ~TileDecoder();

    /**
     * Decode the data type of a JPEG tile.
     *
     * @param tile The tile to decode.
     * @throw std::runtime_error if a decompression error occured
     */
    DEFLECT_API ChromaSubsampling decodeType(const Tile& tile);

    /**
     * Decode a JPEG tile to RGB.
     *
     * @param tile The tile to decode. Upon success, its imageData member
     *        will hold the decompressed RGB image and its "format" flag will
     *        be set to Format::rgba.
     * @throw std::runtime_error if a decompression error occured
     */
    DEFLECT_API void decode(Tile& tile);

#ifndef DEFLECT_USE_LEGACY_LIBJPEGTURBO

    /**
     * Decode a JPEG tile to YUV, skipping the YUV -> RGB step.
     *
     * @param tile The tile to decode. Upon success, its imageData member
     *        will hold the decompressed YUV image and its "format" flag will
     *        be set to the matching Format::yuv4**.
     * @throw std::runtime_error if a decompression error occured
     */
    DEFLECT_API void decodeToYUV(Tile& tile);

#endif

    /**
     * Start decoding a tile.
     *
     * This function will silently ignore the request if a decoding is already
     * in progress.
     * @param tile The tile to decode. The tile will be modified by
     *        this function. It must remain valid and should not be accessed
     *        until the decoding procedure has completed.
     * @see isRunning()
     */
    DEFLECT_API void startDecoding(Tile& tile);

    /**
     * Waits for the decoding of a tile to finish, initiated by
     * startDecoding().
     * @throw std::runtime_error if a decompression error occured
     */
    DEFLECT_API void waitDecoding();

    /** Check if the decoding thread is running. */
    DEFLECT_API bool isRunning() const;

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};
}
}

#endif
