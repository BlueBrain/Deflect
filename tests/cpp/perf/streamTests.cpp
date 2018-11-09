/*********************************************************************/
/* Copyright (c) 2014, EPFL/Blue Brain Project                       */
/*                     Stefan.Eilemann@epfl.ch                       */
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

#define BOOST_TEST_MODULE Stream
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include "MinimalGlobalQtApp.h"
#include "Timer.h"

#include <deflect/Stream.h>
#include <deflect/server/Server.h>

#include <iostream>

#include <QThread>

// Tests local throughput of the streaming library by sending raw as well as
// blank and random images through deflect::Stream. Baseline test for best-case
// performance when streaming pixels.

#ifdef _MSC_VER
#define WIDTH (800u)
#define HEIGHT (600u)
#define NIMAGES (10u)
#else
#define WIDTH (3840u)
#define HEIGHT (2160u)
#define NIMAGES (100u)
#endif
#define NPIXELS (WIDTH * HEIGHT)
#define NBYTES (NPIXELS * 4u)
// #define NTHREADS 20 // QT default if not defined

BOOST_GLOBAL_FIXTURE(MinimalGlobalQtApp);
using Futures = std::vector<deflect::Stream::Future>;

class DCThread : public QThread
{
    void run()
    {
        Timer timer;
        uint8_t* pixels = new uint8_t[NBYTES];
        ::memset(pixels, 0, NBYTES);
        deflect::ImageWrapper image(pixels, WIDTH, HEIGHT, deflect::RGBA);

        deflect::Stream stream("test", "localhost");
        BOOST_CHECK(stream.isConnected());

        image.compressionPolicy = deflect::COMPRESSION_OFF;
        Futures futures;
        futures.reserve(NIMAGES * 2);
        timer.start();
        for (size_t i = 0; i < NIMAGES; ++i)
        {
            futures.push_back(stream.send(image));
            futures.push_back(stream.finishFrame());
        }
        for (auto& future : futures)
            BOOST_CHECK(future.get());
        float time = timer.elapsed();
        std::cout << "raw " << NPIXELS / float(1024 * 1024) / time * NIMAGES
                  << " megapixel/s (" << NIMAGES / time << " FPS)" << std::endl;

        image.compressionPolicy = deflect::COMPRESSION_ON;
        futures.clear();
        futures.reserve(NIMAGES * 2);
        timer.restart();
        for (size_t i = 0; i < NIMAGES; ++i)
        {
            futures.push_back(stream.send(image));
            futures.push_back(stream.finishFrame());
        }
        for (auto& future : futures)
            BOOST_CHECK(future.get());
        time = timer.elapsed();
        std::cout << "blk " << NPIXELS / float(1024 * 1024) / time * NIMAGES
                  << " megapixel/s (" << NIMAGES / time << " FPS)" << std::endl;

        for (size_t i = 0; i < NBYTES; ++i)
            pixels[i] = uint8_t(qrand());
        futures.clear();
        futures.reserve(NIMAGES * 2);
        timer.restart();
        for (size_t i = 0; i < NIMAGES; ++i)
        {
            futures.push_back(stream.send(image));
            futures.push_back(stream.finishFrame());
        }
        for (auto& future : futures)
            BOOST_CHECK(future.get());
        time = timer.elapsed();
        std::cout << "rnd " << NPIXELS / float(1024 * 1024) / time * NIMAGES
                  << " megapixel/s (" << NIMAGES / time << " FPS)" << std::endl;

        std::cout << "raw: uncompressed, "
                  << "blk: Compressed blank images, "
                  << "rnd: Compressed random image content" << std::endl;

        delete[] pixels;
        QCoreApplication::instance()->exit();
    }
};

BOOST_AUTO_TEST_CASE(testSocketConnection)
{
    deflect::server::Server server;
#ifdef NTHREADS
    QThreadPool::globalInstance()->setMaxThreadCount(NTHREADS);
#endif

    DCThread thread;
    thread.start();
    QCoreApplication::instance()->exec();
    BOOST_CHECK(thread.wait());
}
