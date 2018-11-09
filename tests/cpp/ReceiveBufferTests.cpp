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

#define BOOST_TEST_MODULE ReceiveBufferTests
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include <deflect/server/Frame.h>
#include <deflect/server/ReceiveBuffer.h>

inline std::ostream& operator<<(std::ostream& str, const QSize& s)
{
    str << s.width() << 'x' << s.height();
    return str;
}

BOOST_AUTO_TEST_CASE(TestAddAndRemoveSources)
{
    deflect::server::ReceiveBuffer buffer;

    BOOST_REQUIRE_EQUAL(buffer.getSourceCount(), 0);

    buffer.addSource(53);
    BOOST_CHECK_EQUAL(buffer.getSourceCount(), 1);

    buffer.addSource(11981);
    BOOST_CHECK_EQUAL(buffer.getSourceCount(), 2);

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

BOOST_AUTO_TEST_CASE(TestLateJoinAfterFirstFrameIsForbidden)
{
    deflect::server::ReceiveBuffer buffer;
    buffer.addSource(53);
    buffer.addSource(11981);

    buffer.insert(deflect::server::Tile(), 53);
    buffer.insert(deflect::server::Tile(), 11981);
    buffer.finishFrameForSource(53);
    buffer.finishFrameForSource(11981);
    buffer.popFrame();

    BOOST_CHECK_THROW(buffer.addSource(888), std::runtime_error);

    buffer.removeSource(53);
    buffer.removeSource(11981);

    BOOST_REQUIRE_EQUAL(buffer.getSourceCount(), 0);
    BOOST_CHECK_NO_THROW(buffer.addSource(888));
}

BOOST_AUTO_TEST_CASE(TestAllowedToSend)
{
    deflect::server::ReceiveBuffer buffer;

    BOOST_CHECK(!buffer.isAllowedToSend());

    buffer.setAllowedToSend(true);
    BOOST_CHECK(buffer.isAllowedToSend());
    buffer.setAllowedToSend(false);
    BOOST_CHECK(!buffer.isAllowedToSend());
}

BOOST_AUTO_TEST_CASE(TestCompleteAFrame)
{
    const size_t sourceIndex = 46;

    deflect::server::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex);

    deflect::server::Tile tile;
    tile.x = 0;
    tile.y = 0;
    tile.width = 128;
    tile.height = 256;

    buffer.insert(tile, sourceIndex);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    buffer.finishFrameForSource(sourceIndex);
    BOOST_CHECK(buffer.hasCompleteFrame());

    const auto tiles = buffer.popFrame();

    BOOST_CHECK_EQUAL(tiles.size(), 1);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    deflect::server::Frame frame;
    frame.tiles = tiles;
    const QSize frameSize = frame.computeDimensions();
    BOOST_CHECK_EQUAL(frameSize.width(), tile.width);
    BOOST_CHECK_EQUAL(frameSize.height(), tile.height);
}

deflect::server::Tiles generateTestTiles(
    const deflect::View view = deflect::View::mono)
{
    deflect::server::Tiles tiles;

    deflect::server::Tile tile1;
    tile1.x = 0;
    tile1.y = 0;
    tile1.width = 128;
    tile1.height = 256;
    tile1.view = view;

    deflect::server::Tile tile2;
    tile2.x = 128;
    tile2.y = 0;
    tile2.width = 64;
    tile2.height = 256;
    tile2.view = view;

    deflect::server::Tile tile3;
    tile3.x = 0;
    tile3.y = 256;
    tile3.width = 128;
    tile3.height = 512;
    tile3.view = view;

    deflect::server::Tile tile4;
    tile4.x = 128;
    tile4.y = 256;
    tile4.width = 64;
    tile4.height = 512;
    tile4.view = view;

    tiles.push_back(tile1);
    tiles.push_back(tile2);
    tiles.push_back(tile3);
    tiles.push_back(tile4);

    return tiles;
}

BOOST_AUTO_TEST_CASE(TestCompleteACompositeFrameSingleSource)
{
    const size_t sourceIndex = 46;

    deflect::server::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex);

    const auto testTiles = generateTestTiles();

    buffer.insert(testTiles[0], sourceIndex);
    buffer.insert(testTiles[1], sourceIndex);
    buffer.insert(testTiles[2], sourceIndex);
    buffer.insert(testTiles[3], sourceIndex);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    buffer.finishFrameForSource(sourceIndex);
    BOOST_CHECK(buffer.hasCompleteFrame());

    const auto tiles = buffer.popFrame();
    BOOST_CHECK_EQUAL(tiles.size(), 4);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    deflect::server::Frame frame;
    frame.tiles = tiles;
    BOOST_CHECK_EQUAL(frame.computeDimensions(), QSize(192, 768));
}

BOOST_AUTO_TEST_CASE(TestCompleteACompositeFrameMultipleSources)
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;
    const size_t sourceIndex3 = 11;

    deflect::server::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex1);
    buffer.addSource(sourceIndex2);
    buffer.addSource(sourceIndex3);

    const auto testTiles = generateTestTiles();

    buffer.insert(testTiles[0], sourceIndex1);
    buffer.insert(testTiles[1], sourceIndex2);
    buffer.insert(testTiles[2], sourceIndex3);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    buffer.finishFrameForSource(sourceIndex1);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    buffer.finishFrameForSource(sourceIndex2);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    buffer.insert(testTiles[3], sourceIndex3);
    buffer.finishFrameForSource(sourceIndex3);
    BOOST_CHECK(buffer.hasCompleteFrame());

    const auto tiles = buffer.popFrame();
    BOOST_CHECK_EQUAL(tiles.size(), 4);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    deflect::server::Frame frame;
    frame.tiles = tiles;
    BOOST_CHECK_EQUAL(frame.computeDimensions(), QSize(192, 768));
}

BOOST_AUTO_TEST_CASE(TestRemoveSourceWhileStreaming)
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;

    deflect::server::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex1);
    buffer.addSource(sourceIndex2);

    const auto testTiles = generateTestTiles();

    // First Frame - 2 sources
    buffer.insert(testTiles[0], sourceIndex1);
    buffer.insert(testTiles[1], sourceIndex1);
    buffer.insert(testTiles[2], sourceIndex2);
    buffer.insert(testTiles[3], sourceIndex2);
    BOOST_CHECK(!buffer.hasCompleteFrame());
    buffer.finishFrameForSource(sourceIndex1);
    BOOST_CHECK(!buffer.hasCompleteFrame());
    buffer.finishFrameForSource(sourceIndex2);
    BOOST_CHECK(buffer.hasCompleteFrame());

    auto tiles = buffer.popFrame();

    BOOST_CHECK_EQUAL(tiles.size(), 4);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    // Second frame - 1 source
    buffer.removeSource(sourceIndex2);

    buffer.insert(testTiles[0], sourceIndex1);
    buffer.insert(testTiles[1], sourceIndex1);
    BOOST_CHECK(!buffer.hasCompleteFrame());
    buffer.finishFrameForSource(sourceIndex1);
    BOOST_CHECK(buffer.hasCompleteFrame());

    tiles = buffer.popFrame();
    BOOST_CHECK_EQUAL(tiles.size(), 2);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    deflect::server::Frame frame;
    frame.tiles = tiles;
    BOOST_CHECK_EQUAL(frame.computeDimensions(), QSize(192, 256));
}

BOOST_AUTO_TEST_CASE(TestOneOfTwoSourceStopsSendingTiles)
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;

    deflect::server::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex1);
    buffer.addSource(sourceIndex2);

    const auto testTiles = generateTestTiles();

    // First Frame - 2 sources
    buffer.insert(testTiles[0], sourceIndex1);
    buffer.insert(testTiles[1], sourceIndex1);
    buffer.insert(testTiles[2], sourceIndex2);
    buffer.insert(testTiles[3], sourceIndex2);
    BOOST_CHECK(!buffer.hasCompleteFrame());
    buffer.finishFrameForSource(sourceIndex1);
    BOOST_CHECK(!buffer.hasCompleteFrame());
    buffer.finishFrameForSource(sourceIndex2);
    BOOST_CHECK(buffer.hasCompleteFrame());

    const auto tiles = buffer.popFrame();

    BOOST_CHECK_EQUAL(tiles.size(), 4);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    // Next frames - one source stops sending tiles
    for (int i = 0; i < 150; ++i)
    {
        buffer.insert(testTiles[0], sourceIndex1);
        buffer.insert(testTiles[1], sourceIndex1);
        BOOST_REQUIRE_NO_THROW(buffer.finishFrameForSource(sourceIndex1));
        BOOST_REQUIRE(!buffer.hasCompleteFrame());
    }
    // Test buffer exceeds maximum allowed size
    buffer.insert(testTiles[0], sourceIndex1);
    buffer.insert(testTiles[1], sourceIndex1);
    BOOST_CHECK_THROW(buffer.finishFrameForSource(sourceIndex1),
                      std::runtime_error);
}

void _insert(deflect::server::ReceiveBuffer& buffer, const size_t sourceIndex,
             const deflect::server::Tiles& frame)
{
    for (const auto& tile : frame)
        buffer.insert(tile, sourceIndex);
}

void _testStereoBuffer(deflect::server::ReceiveBuffer& buffer)
{
    const auto tiles = buffer.popFrame();
    BOOST_CHECK_EQUAL(tiles.size(), 8);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    size_t left = 0;
    size_t right = 0;
    for (const auto tile : tiles)
    {
        switch (tile.view)
        {
        case deflect::View::left_eye:
            ++left;
            break;
        case deflect::View::right_eye:
            ++right;
            break;
        default:
            BOOST_CHECK_EQUAL("", "Unexpected eye pass");
            break;
        }
    }
    BOOST_CHECK_EQUAL(left, 4);
    BOOST_CHECK_EQUAL(right, 4);

    deflect::server::Frame frame;
    frame.tiles = tiles;
    BOOST_CHECK_EQUAL(frame.computeDimensions(), QSize(192, 768));
}

BOOST_AUTO_TEST_CASE(TestStereoOneSource)
{
    const size_t sourceIndex = 46;

    deflect::server::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex);

    auto leftTiles = generateTestTiles(deflect::View::left_eye);
    _insert(buffer, sourceIndex, leftTiles);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    auto rightTiles = generateTestTiles(deflect::View::right_eye);
    _insert(buffer, sourceIndex, rightTiles);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    buffer.finishFrameForSource(sourceIndex);
    BOOST_CHECK(buffer.hasCompleteFrame());

    _testStereoBuffer(buffer);
}

BOOST_AUTO_TEST_CASE(TestStereoTwoSourcesScreenSpaceSplit)
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;

    deflect::server::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex1);
    buffer.addSource(sourceIndex2);

    const auto leftTiles = generateTestTiles(deflect::View::left_eye);
    const auto rightTiles = generateTestTiles(deflect::View::right_eye);

    _insert(buffer, sourceIndex1, {leftTiles[0], leftTiles[1]});
    BOOST_CHECK(!buffer.hasCompleteFrame());

    _insert(buffer, sourceIndex1, {rightTiles[0], rightTiles[1]});
    BOOST_CHECK(!buffer.hasCompleteFrame());

    _insert(buffer, sourceIndex2, {leftTiles[2], leftTiles[3]});
    BOOST_CHECK(!buffer.hasCompleteFrame());
    _insert(buffer, sourceIndex2, {rightTiles[2], rightTiles[3]});
    BOOST_CHECK(!buffer.hasCompleteFrame());

    buffer.finishFrameForSource(sourceIndex1);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    buffer.finishFrameForSource(sourceIndex2);
    BOOST_CHECK(buffer.hasCompleteFrame());

    _testStereoBuffer(buffer);
}

BOOST_AUTO_TEST_CASE(TestStereoTwoSourcesStereoSplit)
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;

    deflect::server::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex1);
    buffer.addSource(sourceIndex2);

    const auto leftTiles = generateTestTiles(deflect::View::left_eye);
    const auto rightTiles = generateTestTiles(deflect::View::right_eye);

    _insert(buffer, sourceIndex1, leftTiles);
    BOOST_CHECK(!buffer.hasCompleteFrame());
    _insert(buffer, sourceIndex2, rightTiles);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    buffer.finishFrameForSource(sourceIndex1);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    buffer.finishFrameForSource(sourceIndex2);
    BOOST_CHECK(buffer.hasCompleteFrame());

    _testStereoBuffer(buffer);
}

BOOST_AUTO_TEST_CASE(TestStereoFourSourcesScreenSpaceAndStereoSplit)
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;
    const size_t sourceIndex3 = 489;
    const size_t sourceIndex4 = 113;

    deflect::server::ReceiveBuffer buffer;
    buffer.addSource(sourceIndex1);
    buffer.addSource(sourceIndex2);
    buffer.addSource(sourceIndex3);
    buffer.addSource(sourceIndex4);

    const auto leftTiles = generateTestTiles(deflect::View::left_eye);
    const auto rightTiles = generateTestTiles(deflect::View::right_eye);

    _insert(buffer, sourceIndex1, {leftTiles[0], leftTiles[1]});
    BOOST_CHECK(!buffer.hasCompleteFrame());

    _insert(buffer, sourceIndex2, {rightTiles[0], rightTiles[1]});
    BOOST_CHECK(!buffer.hasCompleteFrame());

    _insert(buffer, sourceIndex3, {leftTiles[2], leftTiles[3]});
    BOOST_CHECK(!buffer.hasCompleteFrame());

    _insert(buffer, sourceIndex4, {rightTiles[2], rightTiles[3]});
    BOOST_CHECK(!buffer.hasCompleteFrame());

    buffer.finishFrameForSource(sourceIndex1);
    buffer.finishFrameForSource(sourceIndex3);
    buffer.finishFrameForSource(sourceIndex4);
    BOOST_CHECK(!buffer.hasCompleteFrame());

    buffer.finishFrameForSource(sourceIndex2);
    BOOST_CHECK(buffer.hasCompleteFrame());

    _testStereoBuffer(buffer);
}
