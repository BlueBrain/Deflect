/*********************************************************************/
/* Copyright (c) 2017-2018, EPFL/Blue Brain Project                  */
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

#define BOOST_TEST_MODULE FrameTests

#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include "FrameUtils.h"

BOOST_AUTO_TEST_CASE(compute_frame_dimensions)
{
    auto frame = makeTestFrame(640, 480, 64);
    BOOST_CHECK_EQUAL(frame.computeDimensions(), QSize(640, 480));

    frame = makeTestFrame(1920, 1200, 64);
    BOOST_CHECK_EQUAL(frame.computeDimensions(), QSize(1920, 1200));

    frame = makeTestFrame(1920, 1080, 32);
    BOOST_CHECK_EQUAL(frame.computeDimensions(), QSize(1920, 1080));

    frame = makeTestFrame(2158, 1786, 56);
    BOOST_CHECK_EQUAL(frame.computeDimensions(), QSize(2158, 1786));
}

BOOST_AUTO_TEST_CASE(determine_frame_row_order)
{
    auto frame = makeTestFrame(640, 480, 64);
    BOOST_CHECK(frame.determineRowOrder() == deflect::RowOrder::top_down);

    frame.tiles[0].rowOrder = deflect::RowOrder::bottom_up;
    BOOST_CHECK_THROW(frame.determineRowOrder(), std::runtime_error);

    for (auto& tile : frame.tiles)
        tile.rowOrder = deflect::RowOrder::bottom_up;
    BOOST_CHECK(frame.determineRowOrder() == deflect::RowOrder::bottom_up);

    frame.tiles[0].rowOrder = deflect::RowOrder::top_down;
    BOOST_CHECK_THROW(frame.determineRowOrder(), std::runtime_error);
}
