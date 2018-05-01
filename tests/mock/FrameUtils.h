/*********************************************************************/
/* Copyright (c) 2017, EPFL/Blue Brain Project                       */
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

#include <deflect/server/Frame.h>

#include <cmath> // std::ceil

inline deflect::server::Frame makeTestFrame(int width, int height,
                                            int segmentSize)
{
    const int numSegmentsX = std::ceil((float)width / (float)segmentSize);
    const int numSegmentsY = std::ceil((float)height / (float)segmentSize);

    const int lastSegmentWidth =
        (width % segmentSize) > 0 ? (width % segmentSize) : segmentSize;
    const int lastSegmentHeight =
        (height % segmentSize) > 0 ? (height % segmentSize) : segmentSize;

    deflect::server::Frame frame;
    for (int y = 0; y < numSegmentsY; ++y)
    {
        for (int x = 0; x < numSegmentsX; ++x)
        {
            deflect::Segment segment;
            segment.parameters.x = x * segmentSize;
            segment.parameters.y = y * segmentSize;
            segment.parameters.width = segmentSize;
            segment.parameters.height = segmentSize;
            if (x == numSegmentsX - 1)
                segment.parameters.width = lastSegmentWidth;
            if (y == numSegmentsY - 1)
                segment.parameters.height = lastSegmentHeight;
            frame.segments.push_back(segment);
        }
    }
    return frame;
}

inline void compare(const deflect::server::Frame& frame1,
                    const deflect::server::Frame& frame2)
{
    BOOST_REQUIRE_EQUAL(frame1.segments.size(), frame2.segments.size());

    for (size_t i = 0; i < frame1.segments.size(); ++i)
    {
        const auto& s1 = frame1.segments[i];
        const auto& s2 = frame2.segments[i];
        BOOST_CHECK(s1.view == s2.view);
        BOOST_CHECK(s1.rowOrder == s2.rowOrder);

        const auto& p1 = s1.parameters;
        const auto& p2 = s2.parameters;
        BOOST_CHECK_EQUAL(p1.x, p2.x);
        BOOST_CHECK_EQUAL(p1.y, p2.y);
        BOOST_CHECK_EQUAL(p1.width, p2.width);
        BOOST_CHECK_EQUAL(p1.height, p2.height);
    }
}

inline std::ostream& operator<<(std::ostream& str, const QSize& s)
{
    str << s.width() << 'x' << s.height();
    return str;
}
