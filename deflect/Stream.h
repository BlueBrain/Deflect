/*********************************************************************/
/* Copyright (c) 2013-2018, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
/*                          Stefan.Eilemann@epfl.ch                  */
/*                          Daniel.Nachbaur@epfl.ch                  */
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

#ifndef DEFLECT_STREAM_H
#define DEFLECT_STREAM_H

#include <deflect/ImageWrapper.h>
#include <deflect/Observer.h>
#include <deflect/api.h>
#include <deflect/types.h>

namespace deflect
{
/**
 * Stream visual data to a deflect Server.
 *
 * A Stream can be subdivided into one or more images and eye passes. This
 * allows to have different applications each responsible for sending one part
 * of the global image.
 *
 * The methods in this class are reentrant (all instances are independant) but
 * are not thread-safe.
 */
class Stream : public Observer
{
public:
    /**
     * Open a new connection to the Server using environment variables.
     *
     * DEFLECT_HOST  The address[:port] of the target Server instance, required.
     *               If no port is provided, the default port 1701 is used.
     * DEFLECT_ID    The identifier for the stream. If not provided, a random
     *               unique identifier will be used.
     * @throw std::runtime_error if DEFLECT_HOST was not provided or no
     *                           connection to server could be established
     * @version 1.0
     */
    DEFLECT_API Stream();

    /**
     * Open a new connection to the Server using environment variables.
     *
     * DEFLECT_HOST  The address of the target Server instance (required).
     * DEFLECT_ID    The identifier for the stream. If not provided, a random
     *               unique identifier will be used.
     * @param port Port of the Server instance.
     * @throw std::runtime_error if DEFLECT_HOST was not provided or no
     *                           connection to server could be established
     * @version 1.0
     */
    DEFLECT_API explicit Stream(unsigned short port);

    /**
     * Open a new connection to the Server.
     *
     * The user can check if the connection was successfully established with
     * isConnected().
     *
     * Different Streams can contribute to a single window by using the same
     * identifier. All the Streams which contribute to the same window should be
     * created before any of them starts sending images.
     *
     * @param id The identifier for the stream. If left empty, the environment
     *           variable DEFLECT_ID will be used. If both values are empty,
     *           a random unique identifier will be used.
     * @param host The address of the target Server instance. It can be a
     *             hostname like "localhost" or an IP in string format like
     *             "192.168.1.83". If left empty, the environment variable
     *             DEFLECT_HOST will be used instead.
     * @param port Port of the Server instance, default 1701.
     * @throw std::runtime_error if no host was provided or no
     *                           connection to server could be established
     * @version 1.0
     */
    DEFLECT_API Stream(const std::string& id, const std::string& host,
                       unsigned short port = defaultPortNumber);

    /** Destruct the Stream, closing the connection. @version 1.0 */
    DEFLECT_API virtual ~Stream();

    /** @name Asynchronous send API */
    //@{
    /** Future signaling success of asyncSend(). @version 1.0 */
    using Future = std::future<bool>;

    /**
     * Send an image asynchronously.
     *
     * @param image The image to send. Note that the image is not copied, so the
     *              referenced must remain valid until the send is finished.
     * @return true if the image data could be sent, false otherwise
     * @throw std::invalid_argument if not RGBA and uncompressed
     * @throw std::invalid_argument if invalid JPEG compression arguments
     * @throw std::runtime_error if pending finishFrame() has not been completed
     * @throw std::runtime_error if JPEG compression failed
     * @version 1.0
     * @sa finishFrame()
     */
    DEFLECT_API Future send(const ImageWrapper& image);

    /**
     * Asynchronously notify that all the images for this frame have been sent.
     *
     * This method must be called everytime this Stream instance has finished
     * sending its image(s) for the current frame. The receiver will display
     * the images once all the senders which use the same identifier have
     * finished a frame. This is only to be called once per frame, even for
     * stereo rendering.
     *
     * @sa send()
     * @version 1.0
     */
    DEFLECT_API Future finishFrame();

    /**
     * Send an image and finish the frame asynchronously.
     *
     * The send (and the optional compression) and finishFrame() are executed in
     * a different thread. The result of this operation can be obtained by the
     * returned future object.
     *
     * @param image The image to send. Note that the image is not copied, so the
     *              referenced must remain valid until the send is finished
     * @return true if the image data could be sent, false otherwise.
     * @throw std::invalid_argument if RGBA and uncompressed
     * @throw std::invalid_argument if invalid JPEG compression arguments
     * @throw std::runtime_error if pending finishFrame() has not been completed
     * @throw std::runtime_error if JPEG compression failed
     * @see send()
     * @version 1.0
     */
    DEFLECT_API Future sendAndFinish(const ImageWrapper& image);
    //@}

private:
    Stream(const Stream&) = delete;
    const Stream& operator=(const Stream&) = delete;

    friend class deflect::test::Application;
};
}

#endif
