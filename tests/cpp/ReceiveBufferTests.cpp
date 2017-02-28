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

#include <deflect/Frame.h>
#include <deflect/ReceiveBuffer.h>
#include <deflect/Segment.h>

inline std::ostream& operator<<(std::ostream& str, const QSize& s)
{
    str << s.width() << 'x' << s.height();
    return str;
}

BOOST_AUTO_TEST_CASE(TestAddAndRemoveSources)
{
    deflect::ReceiveBuffer buffer;

    BOOST_REQUIRE_EQUAL(buffer.getSourceCount(), 0);

    buffer.addSource(53);
    BOOST_CHECK_EQUAL(buffer.getSourceCount(), 1);

    buffer.addSource(11981);
    BOOST_CHECK_EQUAL(buffer.getSourceCount(), 2);

    buffer.addSource(888);
    buffer.removeSource(53);
    BOOST_CHECK_EQUAL(buffer.getSourceCount(), 2);

    buffer.removeSource(888);
    buffer.removeSource(11981);
    BOOST_CHECK_EQUAL(buffer.getSourceCount(), 0);

    buffer.removeSource(7777);
    BOOST_CHECK_EQUAL(buffer.getSourceCount(), 0);
}

BOOST_AUTO_TEST_CASE(TestAllowedToSend)
{
    deflect::ReceiveBuffer buffer;

    BOOST_CHECK(!buffer.isAllowedToSend(deflect::View::mono));
    BOOST_CHECK(!buffer.isAllowedToSend(deflect::View::left_eye));
    BOOST_CHECK(!buffer.isAllowedToSend(deflect::View::right_eye));

    buffer.setAllowedToSend(true, deflect::View::mono);
    BOOST_CHECK(buffer.isAllowedToSend(deflect::View::mono));
    buffer.setAllowedToSend(false, deflect::View::mono);
    BOOST_CHECK(!buffer.isAllowedToSend(deflect::View::mono));

    buffer.setAllowedToSend(true, deflect::View::left_eye);
    BOOST_CHECK(buffer.isAllowedToSend(deflect::View::left_eye));
    buffer.setAllowedToSend(false, deflect::View::left_eye);
    BOOST_CHECK(!buffer.isAllowedToSend(deflect::View::left_eye));

    buffer.setAllowedToSend(true, deflect::View::right_eye);
    BOOST_CHECK(buffer.isAllowedToSend(deflect::View::right_eye));
    buffer.setAllowedToSend(false, deflect::View::right_eye);
    BOOST_CHECK(!buffer.isAllowedToSend(deflect::View::right_eye));
}

BOOST_AUTO_TEST_CASE(TestCompleteAFrame)
{
    const size_t sourceIndex = 46;

    deflect::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex);

    deflect::Segment segment;
    segment.parameters.x = 0;
    segment.parameters.y = 0;
    segment.parameters.width = 128;
    segment.parameters.height = 256;

    buffer.insert(segment, sourceIndex);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());

    buffer.finishFrameForSource(sourceIndex);
    BOOST_CHECK(buffer.hasCompleteMonoFrame());
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());

    deflect::Segments segments = buffer.popFrame();

    BOOST_CHECK_EQUAL(segments.size(), 1);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());

    deflect::Frame frame;
    frame.segments = segments;
    const QSize frameSize = frame.computeDimensions();
    BOOST_CHECK_EQUAL(frameSize.width(), segment.parameters.width);
    BOOST_CHECK_EQUAL(frameSize.height(), segment.parameters.height);
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

    segments.push_back(segment1);
    segments.push_back(segment2);
    segments.push_back(segment3);
    segments.push_back(segment4);

    return segments;
}

BOOST_AUTO_TEST_CASE(TestCompleteACompositeFrameSingleSource)
{
    const size_t sourceIndex = 46;

    deflect::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex);

    deflect::Segments testSegments = generateTestSegments();

    buffer.insert(testSegments[0], sourceIndex);
    buffer.insert(testSegments[1], sourceIndex);
    buffer.insert(testSegments[2], sourceIndex);
    buffer.insert(testSegments[3], sourceIndex);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());

    buffer.finishFrameForSource(sourceIndex);
    BOOST_CHECK(buffer.hasCompleteMonoFrame());

    deflect::Segments segments = buffer.popFrame();
    BOOST_CHECK_EQUAL(segments.size(), 4);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());

    deflect::Frame frame;
    frame.segments = segments;
    BOOST_CHECK_EQUAL(frame.computeDimensions(), QSize(192, 768));
}

BOOST_AUTO_TEST_CASE(TestCompleteACompositeFrameMultipleSources)
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;
    const size_t sourceIndex3 = 11;

    deflect::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex1);
    buffer.addSource(sourceIndex2);
    buffer.addSource(sourceIndex3);

    deflect::Segments testSegments = generateTestSegments();

    buffer.insert(testSegments[0], sourceIndex1);
    buffer.insert(testSegments[1], sourceIndex2);
    buffer.insert(testSegments[2], sourceIndex3);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());

    buffer.finishFrameForSource(sourceIndex1);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());

    buffer.finishFrameForSource(sourceIndex2);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());

    buffer.insert(testSegments[3], sourceIndex3);
    buffer.finishFrameForSource(sourceIndex3);
    BOOST_CHECK(buffer.hasCompleteMonoFrame());

    deflect::Segments segments = buffer.popFrame();
    BOOST_CHECK_EQUAL(segments.size(), 4);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());

    deflect::Frame frame;
    frame.segments = segments;
    BOOST_CHECK_EQUAL(frame.computeDimensions(), QSize(192, 768));
}

BOOST_AUTO_TEST_CASE(TestRemoveSourceWhileStreaming)
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;

    deflect::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex1);
    buffer.addSource(sourceIndex2);

    deflect::Segments testSegments = generateTestSegments();

    // First Frame - 2 sources
    buffer.insert(testSegments[0], sourceIndex1);
    buffer.insert(testSegments[1], sourceIndex1);
    buffer.insert(testSegments[2], sourceIndex2);
    buffer.insert(testSegments[3], sourceIndex2);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());
    buffer.finishFrameForSource(sourceIndex1);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());
    buffer.finishFrameForSource(sourceIndex2);
    BOOST_CHECK(buffer.hasCompleteMonoFrame());

    deflect::Segments segments = buffer.popFrame();

    BOOST_CHECK_EQUAL(segments.size(), 4);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());

    // Second frame - 1 source
    buffer.removeSource(sourceIndex2);

    buffer.insert(testSegments[0], sourceIndex1);
    buffer.insert(testSegments[1], sourceIndex1);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());
    buffer.finishFrameForSource(sourceIndex1);
    BOOST_CHECK(buffer.hasCompleteMonoFrame());

    segments = buffer.popFrame();
    BOOST_CHECK_EQUAL(segments.size(), 2);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());

    deflect::Frame frame;
    frame.segments = segments;
    BOOST_CHECK_EQUAL(frame.computeDimensions(), QSize(192, 256));
}

BOOST_AUTO_TEST_CASE(TestOneOfTwoSourceStopsSendingSegments)
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;

    deflect::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex1);
    buffer.addSource(sourceIndex2);

    deflect::Segments testSegments = generateTestSegments();

    // First Frame - 2 sources
    buffer.insert(testSegments[0], sourceIndex1);
    buffer.insert(testSegments[1], sourceIndex1);
    buffer.insert(testSegments[2], sourceIndex2);
    buffer.insert(testSegments[3], sourceIndex2);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());
    buffer.finishFrameForSource(sourceIndex1);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());
    buffer.finishFrameForSource(sourceIndex2);
    BOOST_CHECK(buffer.hasCompleteMonoFrame());

    deflect::Segments segments = buffer.popFrame();

    BOOST_CHECK_EQUAL(segments.size(), 4);
    BOOST_CHECK(!buffer.hasCompleteMonoFrame());

    // Next frames - one source stops sending segments
    for (int i = 0; i < 150; ++i)
    {
        buffer.insert(testSegments[0], sourceIndex1);
        buffer.insert(testSegments[1], sourceIndex1);
        BOOST_REQUIRE_NO_THROW(buffer.finishFrameForSource(sourceIndex1));
        BOOST_REQUIRE(!buffer.hasCompleteMonoFrame());
    }
    // Test buffer exceeds maximum allowed size
    buffer.insert(testSegments[0], sourceIndex1);
    buffer.insert(testSegments[1], sourceIndex1);
    BOOST_CHECK_THROW(buffer.finishFrameForSource(sourceIndex1),
                      std::runtime_error);
}

void _insert(deflect::ReceiveBuffer& buffer, const size_t sourceIndex,
             const deflect::Segments& frame, const deflect::View view)
{
    for (const auto& segment : frame)
        buffer.insert(segment, sourceIndex, view);
    buffer.finishFrameForSource(sourceIndex);
}

void _testStereoBuffer(deflect::ReceiveBuffer& buffer)
{
    const auto leftSegments = buffer.popFrame(deflect::View::left_eye);
    BOOST_CHECK_EQUAL(leftSegments.size(), 4);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());

    const auto rightSegments = buffer.popFrame(deflect::View::right_eye);
    BOOST_CHECK_EQUAL(rightSegments.size(), 4);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());

    deflect::Frame frame;
    frame.segments = leftSegments;
    BOOST_CHECK_EQUAL(frame.computeDimensions(), QSize(192, 768));
    frame.segments = rightSegments;
    BOOST_CHECK_EQUAL(frame.computeDimensions(), QSize(192, 768));
}

BOOST_AUTO_TEST_CASE(TestStereoOneSource)
{
    const size_t sourceIndex = 46;

    deflect::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex);

    deflect::Segments testSegments = generateTestSegments();

    _insert(buffer, sourceIndex, testSegments, deflect::View::left_eye);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());

    _insert(buffer, sourceIndex, testSegments, deflect::View::right_eye);
    BOOST_CHECK(buffer.hasCompleteStereoFrame());

    _testStereoBuffer(buffer);
}

BOOST_AUTO_TEST_CASE(TestStereoSingleFinishFrame)
{
    const size_t sourceIndex = 46;

    deflect::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex);

    const deflect::Segments testSegments = generateTestSegments();

    for (const auto& segment : testSegments)
        buffer.insert(segment, sourceIndex, deflect::View::left_eye);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());

    for (const auto& segment : testSegments)
        buffer.insert(segment, sourceIndex, deflect::View::right_eye);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());

    buffer.finishFrameForSource(sourceIndex);
    BOOST_CHECK(buffer.hasCompleteStereoFrame());

    _testStereoBuffer(buffer);
}

BOOST_AUTO_TEST_CASE(TestStereoTwoSourcesScreenSpaceSplit)
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;

    deflect::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex1);
    buffer.addSource(sourceIndex2);

    const auto testSegments = generateTestSegments();
    const auto segmentsScreen1 =
        deflect::Segments{testSegments[0], testSegments[1]};
    const auto segmentsScreen2 =
        deflect::Segments{testSegments[2], testSegments[3]};

    _insert(buffer, sourceIndex1, segmentsScreen1, deflect::View::left_eye);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());
    _insert(buffer, sourceIndex1, segmentsScreen1, deflect::View::right_eye);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());

    _insert(buffer, sourceIndex2, segmentsScreen2, deflect::View::left_eye);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());
    _insert(buffer, sourceIndex2, segmentsScreen2, deflect::View::right_eye);
    BOOST_CHECK(buffer.hasCompleteStereoFrame());

    _testStereoBuffer(buffer);
}

BOOST_AUTO_TEST_CASE(TestStereoTwoSourcesStereoSplit)
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;

    deflect::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex1);
    buffer.addSource(sourceIndex2);

    const auto testSegments = generateTestSegments();

    _insert(buffer, sourceIndex1, testSegments, deflect::View::left_eye);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());
    _insert(buffer, sourceIndex2, testSegments, deflect::View::right_eye);
    BOOST_CHECK(buffer.hasCompleteStereoFrame());

    _testStereoBuffer(buffer);
}

BOOST_AUTO_TEST_CASE(TestStereoFourSourcesScreenSpaceAndStereoSplit)
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;
    const size_t sourceIndex3 = 489;
    const size_t sourceIndex4 = 113;

    deflect::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex1);
    buffer.addSource(sourceIndex2);
    buffer.addSource(sourceIndex3);
    buffer.addSource(sourceIndex4);

    const auto testSegments = generateTestSegments();
    const auto segmentsScreen1 =
        deflect::Segments{testSegments[0], testSegments[1]};
    const auto segmentsScreen2 =
        deflect::Segments{testSegments[2], testSegments[3]};

    _insert(buffer, sourceIndex1, segmentsScreen1, deflect::View::left_eye);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());
    _insert(buffer, sourceIndex2, segmentsScreen1, deflect::View::right_eye);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());
    _insert(buffer, sourceIndex3, segmentsScreen2, deflect::View::left_eye);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());
    _insert(buffer, sourceIndex4, segmentsScreen2, deflect::View::right_eye);
    BOOST_CHECK(buffer.hasCompleteStereoFrame());

    _testStereoBuffer(buffer);

    // Random insertion order
    _insert(buffer, sourceIndex1, segmentsScreen1, deflect::View::left_eye);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());
    _insert(buffer, sourceIndex3, segmentsScreen2, deflect::View::left_eye);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());
    _insert(buffer, sourceIndex2, segmentsScreen1, deflect::View::right_eye);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());
    _insert(buffer, sourceIndex1, segmentsScreen1, deflect::View::left_eye);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());
    _insert(buffer, sourceIndex2, segmentsScreen1, deflect::View::right_eye);
    BOOST_CHECK(!buffer.hasCompleteStereoFrame());
    _insert(buffer, sourceIndex4, segmentsScreen2, deflect::View::right_eye);
    BOOST_CHECK(buffer.hasCompleteStereoFrame());

    _testStereoBuffer(buffer);
}
