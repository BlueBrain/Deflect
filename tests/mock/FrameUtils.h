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

#include <deflect/server/Frame.h>

#include <cmath> // std::ceil

inline deflect::server::Frame makeTestFrame(int width, int height, int tileSize)
{
    const int numTilesX = std::ceil((float)width / (float)tileSize);
    const int numTilesY = std::ceil((float)height / (float)tileSize);

    const int lastTileWidth =
        (width % tileSize) > 0 ? (width % tileSize) : tileSize;
    const int lastTileHeight =
        (height % tileSize) > 0 ? (height % tileSize) : tileSize;

    deflect::server::Frame frame;
    for (int y = 0; y < numTilesY; ++y)
    {
        for (int x = 0; x < numTilesX; ++x)
        {
            deflect::server::Tile tile;
            tile.x = x * tileSize;
            tile.y = y * tileSize;
            tile.width = tileSize;
            tile.height = tileSize;
            if (x == numTilesX - 1)
                tile.width = lastTileWidth;
            if (y == numTilesY - 1)
                tile.height = lastTileHeight;
            frame.tiles.push_back(tile);
        }
    }
    return frame;
}

inline void compare(const deflect::server::Frame& frame1,
                    const deflect::server::Frame& frame2)
{
    BOOST_REQUIRE_EQUAL(frame1.tiles.size(), frame2.tiles.size());

    for (size_t i = 0; i < frame1.tiles.size(); ++i)
    {
        const auto& t1 = frame1.tiles[i];
        const auto& t2 = frame2.tiles[i];
        BOOST_CHECK(t1.view == t2.view);
        BOOST_CHECK(t1.rowOrder == t2.rowOrder);
        BOOST_CHECK_EQUAL(t1.x, t2.x);
        BOOST_CHECK_EQUAL(t1.y, t2.y);
        BOOST_CHECK_EQUAL(t1.width, t2.width);
        BOOST_CHECK_EQUAL(t1.height, t2.height);
    }
}

inline std::ostream& operator<<(std::ostream& str, const QSize& s)
{
    str << s.width() << 'x' << s.height();
    return str;
}
