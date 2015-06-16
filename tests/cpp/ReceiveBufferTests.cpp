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

#define BOOST_TEST_MODULE ReceiveBufferTests
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include <deflect/ReceiveBuffer.h>
#include <deflect/Segment.h>

BOOST_AUTO_TEST_CASE( TestAddAndRemoveSources )
{
    deflect::ReceiveBuffer buffer;

    BOOST_REQUIRE_EQUAL( buffer.getSourceCount(), 0 );

    buffer.addSource( 53 );
    BOOST_CHECK_EQUAL( buffer.getSourceCount(), 1 );

    buffer.addSource( 11981 );
    BOOST_CHECK_EQUAL( buffer.getSourceCount(), 2 );

    buffer.addSource( 888 );
    buffer.removeSource( 53 );
    BOOST_CHECK_EQUAL( buffer.getSourceCount(), 2 );

    buffer.removeSource( 888 );
    buffer.removeSource( 11981 );
    BOOST_CHECK_EQUAL( buffer.getSourceCount(), 0 );
}


BOOST_AUTO_TEST_CASE( TestCompleteAFrame )
{
    const size_t sourceIndex = 46;

    deflect::ReceiveBuffer buffer;
    buffer.addSource( sourceIndex );

    deflect::Segment segment;
    segment.parameters.x = 0;
    segment.parameters.y = 0;
    segment.parameters.width = 128;
    segment.parameters.height = 256;

    buffer.insert( segment, sourceIndex );
    BOOST_CHECK( !buffer.hasCompleteFrame( ));
    BOOST_CHECK( !buffer.isFirstCompleteFrame( ));

    buffer.finishFrameForSource( sourceIndex );
    BOOST_CHECK( buffer.hasCompleteFrame( ));
    BOOST_CHECK( buffer.isFirstCompleteFrame( ));

    QSize frameSize = buffer.getFrameSize();
    BOOST_CHECK_EQUAL( frameSize.width(), segment.parameters.width );
    BOOST_CHECK_EQUAL( frameSize.height(), segment.parameters.height );

    deflect::Segments segments = buffer.popFrame();

    BOOST_CHECK_EQUAL( segments.size(), 1 );
    BOOST_CHECK( !buffer.hasCompleteFrame( ));
    BOOST_CHECK( !buffer.isFirstCompleteFrame( ));
}


deflect::Segments generateTestSegments()
{
    deflect::Segments segments;

    deflect::Segment segment1;
    segment1.parameters.x = 0;
    segment1.parameters.y = 0;
    segment1.parameters.width = 128;
    segment1.parameters.height = 256;

    deflect::Segment segment2;
    segment2.parameters.x = 128;
    segment2.parameters.y = 0;
    segment2.parameters.width = 64;
    segment2.parameters.height = 256;

    deflect::Segment segment3;
    segment3.parameters.x = 0;
    segment3.parameters.y = 256;
    segment3.parameters.width = 128;
    segment3.parameters.height = 512;

    deflect::Segment segment4;
    segment4.parameters.x = 128;
    segment4.parameters.y = 256;
    segment4.parameters.width = 64;
    segment4.parameters.height = 512;

    segments.push_back( segment1 );
    segments.push_back( segment2 );
    segments.push_back( segment3 );
    segments.push_back( segment4 );

    return segments;
}

BOOST_AUTO_TEST_CASE( TestCompleteACompositeFrameSingleSource )
{
    const size_t sourceIndex = 46;

    deflect::ReceiveBuffer buffer;
    buffer.addSource( sourceIndex );

    deflect::Segments testSegments = generateTestSegments();

    buffer.insert( testSegments[0], sourceIndex );
    buffer.insert( testSegments[1], sourceIndex );
    buffer.insert( testSegments[2], sourceIndex );
    buffer.insert( testSegments[3], sourceIndex );
    BOOST_CHECK( !buffer.hasCompleteFrame( ));

    buffer.finishFrameForSource( sourceIndex );
    BOOST_CHECK( buffer.hasCompleteFrame( ));

    QSize frameSize = buffer.getFrameSize();
    BOOST_CHECK( buffer.hasCompleteFrame( ));
    BOOST_CHECK( buffer.isFirstCompleteFrame( ));
    BOOST_CHECK_EQUAL( frameSize.width(), 192 );
    BOOST_CHECK_EQUAL( frameSize.height(), 768 );

    deflect::Segments segments = buffer.popFrame();
    frameSize = buffer.getFrameSize();
    BOOST_CHECK_EQUAL( frameSize.width(), 0 );
    BOOST_CHECK_EQUAL( frameSize.height(), 0 );

    BOOST_CHECK_EQUAL( segments.size(), 4 );
    BOOST_CHECK( !buffer.hasCompleteFrame( ));
    BOOST_CHECK( !buffer.isFirstCompleteFrame( ));
}



BOOST_AUTO_TEST_CASE( TestCompleteACompositeFrameMultipleSources )
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;
    const size_t sourceIndex3 = 11;

    deflect::ReceiveBuffer buffer;
    buffer.addSource( sourceIndex1 );
    buffer.addSource( sourceIndex2 );
    buffer.addSource( sourceIndex3 );

    deflect::Segments testSegments = generateTestSegments();

    buffer.insert( testSegments[0], sourceIndex1 );
    buffer.insert( testSegments[1], sourceIndex2 );
    buffer.insert( testSegments[2], sourceIndex3 );
    BOOST_CHECK( !buffer.hasCompleteFrame( ));

    buffer.finishFrameForSource( sourceIndex1 );
    BOOST_CHECK( !buffer.hasCompleteFrame( ));

    buffer.finishFrameForSource( sourceIndex2 );
    BOOST_CHECK( !buffer.hasCompleteFrame( ));

    buffer.insert( testSegments[3], sourceIndex3 );
    buffer.finishFrameForSource( sourceIndex3 );
    BOOST_CHECK( buffer.hasCompleteFrame( ));

    BOOST_CHECK( buffer.hasCompleteFrame( ));
    BOOST_CHECK( buffer.isFirstCompleteFrame( ));
    QSize frameSize = buffer.getFrameSize();
    BOOST_CHECK_EQUAL( frameSize.width(), 192 );
    BOOST_CHECK_EQUAL( frameSize.height(), 768 );

    deflect::Segments segments = buffer.popFrame();

    BOOST_CHECK_EQUAL( segments.size(), 4 );
    BOOST_CHECK( !buffer.hasCompleteFrame( ));
    BOOST_CHECK( !buffer.isFirstCompleteFrame( ));
}


BOOST_AUTO_TEST_CASE( TestRemoveSourceWhileStreaming )
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;

    deflect::ReceiveBuffer buffer;
    buffer.addSource( sourceIndex1 );
    buffer.addSource( sourceIndex2 );

    deflect::Segments testSegments = generateTestSegments();

    // First Frame - 2 sources
    buffer.insert( testSegments[0], sourceIndex1 );
    buffer.insert( testSegments[1], sourceIndex1 );
    buffer.insert( testSegments[2], sourceIndex2 );
    buffer.insert( testSegments[3], sourceIndex2 );
    BOOST_CHECK( !buffer.hasCompleteFrame( ));
    BOOST_CHECK( !buffer.isFirstCompleteFrame( ));
    buffer.finishFrameForSource(sourceIndex1);
    BOOST_CHECK( !buffer.hasCompleteFrame( ));
    BOOST_CHECK( !buffer.isFirstCompleteFrame( ));
    buffer.finishFrameForSource( sourceIndex2 );
    BOOST_CHECK( buffer.hasCompleteFrame( ));
    BOOST_CHECK( buffer.isFirstCompleteFrame( ));

    deflect::Segments segments = buffer.popFrame();

    BOOST_CHECK_EQUAL( segments.size(), 4 );
    BOOST_CHECK( !buffer.hasCompleteFrame( ));

    // Second frame - 1 source
    buffer.removeSource( sourceIndex2 );

    buffer.insert( testSegments[0], sourceIndex1 );
    buffer.insert( testSegments[1], sourceIndex1 );
    BOOST_CHECK( !buffer.hasCompleteFrame( ));
    buffer.finishFrameForSource( sourceIndex1 );
    BOOST_CHECK( buffer.hasCompleteFrame( ));

    QSize frameSize = buffer.getFrameSize();
    BOOST_CHECK_EQUAL( frameSize.width(), 192 );
    BOOST_CHECK_EQUAL( frameSize.height(), 256 );

    segments = buffer.popFrame();
    BOOST_CHECK_EQUAL( segments.size(), 2 );
    BOOST_CHECK( !buffer.hasCompleteFrame( ));
    BOOST_CHECK( !buffer.isFirstCompleteFrame( ));
}
