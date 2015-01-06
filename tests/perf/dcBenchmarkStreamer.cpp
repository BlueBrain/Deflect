/*********************************************************************/
/* Copyright (c) 2014, EPFL/Blue Brain Project                       */
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

#include <string>
#include <iostream>
#include <QImage>

#include <deflect/Stream.h>
#include <deflect/StreamPrivate.h>
#include <deflect/ImageSegmenter.h>
#include <deflect/PixelStreamSegment.h>

#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <QMutexLocker>

#define MEGABYTE 1000000
#define MICROSEC 1000000

namespace
{
class Timer
{
public:
    void start()
    {
        lastTime_ = boost::posix_time::microsec_clock::universal_time();
    }

    void restart()
    {
        start();
    }

    float elapsed()
    {
        const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
        return (float)(now - lastTime_).total_milliseconds();
    }
private:
    boost::posix_time::ptime lastTime_;
};
}

struct BenchmarkOptions
{
    BenchmarkOptions(int &argc, char **argv)
        : desc_("Allowed options")
        , getHelp_(true)
        , width_(0)
        , height_(0)
        , nframes_(0)
        , framerate_(0)
        , compress_(false)
        , precompute_(false)
        , quality_(0)
    {
        initDesc();
        parseCommandLineArguments(argc, argv);
    }

    void showSyntax() const
    {
        std::cout << desc_;
    }

    void initDesc()
    {
        desc_.add_options()
            ("help", "produce help message")
            ("name", boost::program_options::value<std::string>()->default_value("BenchmarkStreamer"),
                     "identifier for the stream")
            ("width", boost::program_options::value<unsigned int>()->default_value(0),
                     "width of the stream in pixel")
            ("height", boost::program_options::value<unsigned int>()->default_value(0),
                     "height of the stream in pixel")
            ("nframes", boost::program_options::value<unsigned int>()->default_value(0),
                     "number of frames")
            ("framerate", boost::program_options::value<unsigned int>()->default_value(0),
                     "the framerate at which to send frame (defaults to unlimited)")
            ("hostname", boost::program_options::value<std::string>()->default_value("localhost"),
                     "DisplayCluster host name")
            ("compress", "compress segments using jpeg")
            ("precompute", "send precomputed segments (no encoding time)")
            ("quality", boost::program_options::value<unsigned int>()->default_value(80),
                     "quality of the jpeg compression. Only used if combined with --compress")
        ;
    }

    void parseCommandLineArguments(int &argc, char **argv)
    {
        if (argc <= 1)
            return;

        boost::program_options::variables_map vm;
        try
        {
            boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc_), vm);
            boost::program_options::notify(vm);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            return;
        }

        getHelp_ = vm.count("help");
        name_ = vm["name"].as<std::string>();
        width_ = vm["width"].as<unsigned int>();
        height_ = vm["height"].as<unsigned int>();
        nframes_ = vm["nframes"].as<unsigned int>();
        framerate_ = vm["framerate"].as<unsigned int>();
        hostname_ = vm["hostname"].as<std::string>();
        compress_ = vm.count("compress");
        precompute_ = vm.count("precompute");
        quality_ = vm["quality"].as<unsigned int>();
    }

    boost::program_options::options_description desc_;

    bool getHelp_;
    std::string name_;
    unsigned int width_, height_;
    unsigned int nframes_;
    unsigned int framerate_;
    std::string hostname_;
    bool compress_;
    bool precompute_;
    unsigned int quality_;
};

static bool append( deflect::PixelStreamSegments& segments,
                    const deflect::PixelStreamSegment& segment )
{
    static QMutex lock_;
    QMutexLocker locker( &lock_ );
    segments.push_back( segment );
    return true;
}

/**
 * Stream image segments for benchmarking purposes.
 */
class Application
{
public:
    Application(const BenchmarkOptions& options)
        : options_(options)
        , dcStream_(new deflect::Stream(options.name_, options.hostname_))
    {
        generateNoiseImage(options_.width_, options_.height_);
        generateJpegSegments();

        std::cout << "Image dimensions :        " << noiseImage_.width() << " x " << noiseImage_.height() << std::endl;
        std::cout << "Raw image size [Mbytes]:  " << (float)imageDataSize() / MEGABYTE << std::endl;
        std::cout << "Jpeg image size [Mbytes]: " << (float)jpegSegmentsSize() / MEGABYTE << std::endl;
        std::cout << "#segments per image :     " << jpegSegments_.size() << std::endl;
    }

    size_t imageDataSize() const
    {
        return 4 * noiseImage_.width() * noiseImage_.height();
    }

    size_t jpegSegmentsSize() const
    {
        size_t size = 0;

        for(deflect::PixelStreamSegments::const_iterator it = jpegSegments_.begin();
            it != jpegSegments_.end(); ++it)
        {
            size += it->imageData.size();
        }

        return size;
    }

    void generateNoiseImage(const int width, const int height)
    {
        noiseImage_ = QImage(width, height, QImage::Format_RGB32);

        uchar* data = noiseImage_.bits();
        const size_t dataSize = imageDataSize();

        for (size_t i = 0; i < dataSize; ++i)
            data[i] = rand();
    }

    bool generateJpegSegments()
    {
        deflect::ImageWrapper deflectImage((const void*)noiseImage_.bits(),
                                            noiseImage_.width(),
                                            noiseImage_.height(),
                                            deflect::RGBA);

        deflectImage.compressionPolicy = deflect::COMPRESSION_ON;
        deflectImage.compressionQuality = options_.quality_;

        const deflect::ImageSegmenter::Handler appendHandler =
            boost::bind( &append, boost::ref( jpegSegments_ ), _1 );

        return dcStream_->impl_->imageSegmenter_.generate(deflectImage, appendHandler);
    }

    bool send()
    {
        if (options_.compress_)
        {
            if (options_.precompute_)
                return sendPrecompressedJpeg();
            else
                return sendJpeg();
        }

        return sendRaw();
    }

    bool sendRaw()
    {
        deflect::ImageWrapper deflectImage((const void*)noiseImage_.bits(),
                                            noiseImage_.width(),
                                            noiseImage_.height(),
                                            deflect::RGBA);
        deflectImage.compressionPolicy = deflect::COMPRESSION_OFF;

        return dcStream_->send(deflectImage) && dcStream_->finishFrame();
    }

    bool sendJpeg()
    {
        deflect::ImageWrapper deflectImage((const void*)noiseImage_.bits(),
                                            noiseImage_.width(),
                                            noiseImage_.height(),
                                            deflect::RGBA);
        deflectImage.compressionPolicy = deflect::COMPRESSION_ON;
        deflectImage.compressionQuality = options_.quality_;

        return dcStream_->send(deflectImage) && dcStream_->finishFrame();
    }

    bool sendPrecompressedJpeg()
    {
        for(deflect::PixelStreamSegments::const_iterator it = jpegSegments_.begin();
            it != jpegSegments_.end(); ++it)
        {
            if (!dcStream_->impl_->sendPixelStreamSegment(*it))
                return false;
        }

        return dcStream_->finishFrame();
    }

private:
    const BenchmarkOptions& options_;
    QImage noiseImage_;
    boost::scoped_ptr<deflect::Stream> dcStream_;
    deflect::PixelStreamSegments jpegSegments_;
};

int main(int argc, char **argv)
{
    const BenchmarkOptions options(argc, argv);

    if (options.getHelp_)
    {
        options.showSyntax();
        return 0;
    }

    Application benchmarkStreamer(options);

    Timer timer;
    timer.start();

    size_t counter = 0;

    bool streamOpen = true;
    while(streamOpen && (options.nframes_ == 0 || counter < options.nframes_))
    {
        if (options.framerate_)
            usleep(MICROSEC / options.framerate_);
        streamOpen = benchmarkStreamer.send();
        ++counter;
    }

    float time = timer.elapsed() / 1000.f;

    const size_t frameSize = options.compress_ ?
                             benchmarkStreamer.jpegSegmentsSize() :
                             benchmarkStreamer.imageDataSize();

    std::cout << "Target framerate: " << options.framerate_ << std::endl;
    std::cout << "Time to send " << counter << " frames: " << time << std::endl;
    std::cout << "Time per frame: " << time / counter << std::endl;
    std::cout << "Throughput [Mbytes/sec]: " << counter * frameSize / time / MEGABYTE << std::endl;

    return 0;
}
