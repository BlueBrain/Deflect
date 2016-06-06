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

#include <deflect/ImageSegmenter.h>
#include <deflect/Segment.h>
#include <deflect/Stream.h>
#include <deflect/StreamPrivate.h>

#include <string>
#include <iostream>
#include <QImage>
#include <QMutexLocker>

#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#define MEGABYTE 1000000
#define MICROSEC 1000000

namespace
{
class Timer
{
public:
    void start()
    {
        _lastTime = boost::posix_time::microsec_clock::universal_time();
    }

    void restart()
    {
        start();
    }

    float elapsed()
    {
        const boost::posix_time::ptime now =
                boost::posix_time::microsec_clock::universal_time();
        return (float)(now - _lastTime).total_milliseconds();
    }

private:
    boost::posix_time::ptime _lastTime;
};
}

struct BenchmarkOptions
{
    BenchmarkOptions( int& argc, char** argv )
        : desc( "Allowed options" )
        , getHelp( true )
        , width( 0 )
        , height( 0 )
        , nframes( 0 )
        , framerate( 0 )
        , compress( false )
        , precompute( false )
        , quality( 0 )
    {
        initDesc();
        parseCommandLineArguments( argc, argv );
    }

    void showSyntax() const
    {
        std::cout << desc;
    }

    void initDesc()
    {
        using namespace boost::program_options;
        desc.add_options()
            ("help", "produce help message")
            ("id", value<std::string>()->default_value( "BenchmarkStreamer" ),
                   "identifier for the stream")
            ("width", value<unsigned int>()->default_value( 0 ),
                     "width of the stream in pixel")
            ("height", value<unsigned int>()->default_value( 0 ),
                     "height of the stream in pixel")
            ("nframes", value<unsigned int>()->default_value( 0 ),
                     "number of frames")
            ("framerate", value<unsigned int>()->default_value( 0 ),
                     "framerate at which to send frames (default: unlimited)")
            ("host", value<std::string>()->default_value( "localhost" ),
                     "Target Deflect server host")
            ("compress", "compress segments using jpeg")
            ("precompute", "send precomputed segments (no encoding time)")
            ("quality", value<unsigned int>()->default_value( 80 ),
                     "quality of the jpeg compression. Only used if combined with --compress")
        ;
    }

    void parseCommandLineArguments( int& argc, char** argv )
    {
        if( argc <= 1 )
            return;

        boost::program_options::variables_map vm;
        try
        {
            using namespace boost::program_options;
            store( parse_command_line( argc, argv, desc ), vm );
            notify( vm );
        }
        catch( const std::exception& e )
        {
            std::cerr << e.what() << std::endl;
            return;
        }

        getHelp = vm.count("help");
        id = vm["id"].as<std::string>();
        width = vm["width"].as<unsigned int>();
        height = vm["height"].as<unsigned int>();
        nframes = vm["nframes"].as<unsigned int>();
        framerate = vm["framerate"].as<unsigned int>();
        host = vm["host"].as<std::string>();
        compress = vm.count("compress");
        precompute = vm.count("precompute");
        quality = vm["quality"].as<unsigned int>();
    }

    boost::program_options::options_description desc;

    bool getHelp;
    std::string id;
    unsigned int width, height;
    unsigned int nframes;
    unsigned int framerate;
    std::string host;
    bool compress;
    bool precompute;
    unsigned int quality;
};

static bool append( deflect::Segments& segments,
                    const deflect::Segment& segment )
{
    static QMutex lock;
    QMutexLocker locker( &lock );
    segments.push_back( segment );
    return true;
}

/**
 * Stream image segments for benchmarking purposes.
 */
class Application
{
public:
    explicit Application( const BenchmarkOptions& options )
        : _options( options )
        , _stream( new deflect::Stream( options.id, options.host ))
    {
        generateNoiseImage( _options.width, _options.height );
        generateJpegSegments();

        std::cout << "Image dimensions :        " << _noiseImage.width() <<
                     " x " << _noiseImage.height() << std::endl;
        std::cout << "Raw image size [Mbytes]:  " <<
                     (float)imageDataSize() / MEGABYTE << std::endl;
        std::cout << "Jpeg image size [Mbytes]: " <<
                     (float)jpegSegmentsSize() / MEGABYTE << std::endl;
        std::cout << "#segments per image :     " <<
                     _jpegSegments.size() << std::endl;
    }

    size_t imageDataSize() const
    {
        return 4 * _noiseImage.width() * _noiseImage.height();
    }

    size_t jpegSegmentsSize() const
    {
        size_t size = 0;

        for( deflect::Segments::const_iterator it = _jpegSegments.begin();
             it != _jpegSegments.end(); ++it )
        {
            size += it->imageData.size();
        }

        return size;
    }

    void generateNoiseImage( const int width, const int height )
    {
        _noiseImage = QImage( width, height, QImage::Format_RGB32 );

        uchar* data = _noiseImage.bits();
        const size_t dataSize = imageDataSize();

        for( size_t i = 0; i < dataSize; ++i )
            data[i] = rand();
    }

    bool generateJpegSegments()
    {
        deflect::ImageWrapper deflectImage( (const void*)_noiseImage.bits(),
                                            _noiseImage.width(),
                                            _noiseImage.height(),
                                            deflect::RGBA );

        deflectImage.compressionPolicy = deflect::COMPRESSION_ON;
        deflectImage.compressionQuality = _options.quality;

        const deflect::ImageSegmenter::Handler appendHandler =
            boost::bind( &append, boost::ref( _jpegSegments ), _1 );

        return _stream->_impl->imageSegmenter.generate( deflectImage,
                                                        appendHandler );
    }

    bool send()
    {
        if( _options.compress )
        {
            if( _options.precompute )
                return sendPrecompressedJpeg();
            else
                return sendJpeg();
        }

        return sendRaw();
    }

    bool sendRaw()
    {
        deflect::ImageWrapper deflectImage( (const void*)_noiseImage.bits(),
                                            _noiseImage.width(),
                                            _noiseImage.height(),
                                            deflect::RGBA );
        deflectImage.compressionPolicy = deflect::COMPRESSION_OFF;

        return _stream->send( deflectImage ) && _stream->finishFrame();
    }

    bool sendJpeg()
    {
        deflect::ImageWrapper deflectImage( (const void*)_noiseImage.bits(),
                                            _noiseImage.width(),
                                            _noiseImage.height(),
                                            deflect::RGBA );
        deflectImage.compressionPolicy = deflect::COMPRESSION_ON;
        deflectImage.compressionQuality = _options.quality;

        return _stream->send( deflectImage ) && _stream->finishFrame();
    }

    bool sendPrecompressedJpeg()
    {
        for( deflect::Segments::const_iterator it = _jpegSegments.begin();
             it != _jpegSegments.end(); ++it )
        {
            if( !_stream->_impl->sendPixelStreamSegment( *it ))
                return false;
        }

        return _stream->finishFrame();
    }

private:
    const BenchmarkOptions& _options;
    QImage _noiseImage;
    boost::scoped_ptr<deflect::Stream> _stream;
    deflect::Segments _jpegSegments;
};

int main( int argc, char** argv )
{
    const BenchmarkOptions options( argc, argv );

    if( options.getHelp )
    {
        options.showSyntax();
        return 0;
    }

    Application benchmarkStreamer( options );

    Timer timer;
    timer.start();

    size_t counter = 0;

    bool streamOpen = true;
    while( streamOpen && ( options.nframes == 0 || counter < options.nframes ))
    {
        if( options.framerate )
            boost::this_thread::sleep(
               boost::posix_time::microseconds( MICROSEC / options.framerate ));
        streamOpen = benchmarkStreamer.send();
        ++counter;
    }

    float time = timer.elapsed() / 1000.f;

    const size_t frameSize = options.compress ?
                             benchmarkStreamer.jpegSegmentsSize() :
                             benchmarkStreamer.imageDataSize();

    std::cout << "Target framerate: " << options.framerate << std::endl;
    std::cout << "Time to send " << counter << " frames: " << time << std::endl;
    std::cout << "Time per frame: " << time / counter << std::endl;
    std::cout << "Throughput [Mbytes/sec]: " <<
                 counter * frameSize / time / MEGABYTE << std::endl;

    return 0;
}
