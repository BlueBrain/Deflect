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

#include "FrameDispatcher.h"

#include "Frame.h"
#include "ReceiveBuffer.h"

#include <cassert>

namespace deflect
{
namespace server
{
class FrameDispatcher::Impl
{
public:
    Impl() {}
    FramePtr consumeLatestFrame(const QString& uri)
    {
        auto frame = std::make_shared<Frame>();
        frame->uri = uri;

        auto& buffer = streams[uri].buffer;

        while (buffer.hasCompleteFrame())
            frame->tiles = buffer.popFrame();

        assert(!frame->tiles.empty());

        if (frame->determineRowOrder() == RowOrder::bottom_up)
            mirrorTilesPositionsVertically(*frame);

        // receiver will request a new frame once this frame was consumed
        buffer.setAllowedToSend(false);

        return frame;
    }

    void mirrorTilesPositionsVertically(Frame& frame) const
    {
        const auto height = frame.computeDimensions().height();
        for (auto& tile : frame.tiles)
            tile.y = height - tile.y - tile.height;
    }

    bool allConnectionsClosed(const QString& uri) const
    {
        const auto& stream = streams.at(uri);
        return stream.buffer.getSourceCount() == 0 && stream.observers == 0;
    }

    struct Stream
    {
        ReceiveBuffer buffer;
        size_t observers = 0;
    };
    std::map<QString, Stream> streams;
};

FrameDispatcher::FrameDispatcher(QObject* parent_)
    : QObject(parent_)
    , _impl(new Impl)
{
}

FrameDispatcher::~FrameDispatcher()
{
}

void FrameDispatcher::addSource(const QString uri, const size_t sourceIndex)
{
    try
    {
        auto& stream = _impl->streams[uri];

        stream.buffer.addSource(sourceIndex);

        if (stream.observers == 0 && stream.buffer.getSourceCount() == 1)
            emit pixelStreamOpened(uri);
    }
    catch (const std::runtime_error& e)
    {
        emit sourceRejected(uri, sourceIndex);
        emit pixelStreamWarning(uri, e.what());
    }
}

void FrameDispatcher::removeSource(const QString uri, const size_t sourceIndex)
{
    if (!_impl->streams.count(uri))
        return;

    _impl->streams[uri].buffer.removeSource(sourceIndex);

    if (_impl->allConnectionsClosed(uri))
        deleteStream(uri);
}

void FrameDispatcher::addObserver(const QString uri)
{
    auto& stream = _impl->streams[uri];

    ++stream.observers;
    if (stream.observers == 1 && stream.buffer.getSourceCount() == 0)
        emit pixelStreamOpened(uri);
}

void FrameDispatcher::removeObserver(QString uri)
{
    if (!_impl->streams.count(uri))
        return;

    auto& stream = _impl->streams[uri];
    if (stream.observers > 0)
        --stream.observers;

    if (_impl->allConnectionsClosed(uri))
        deleteStream(uri);
}

void FrameDispatcher::processTile(const QString uri, const size_t sourceIndex,
                                  deflect::server::Tile tile)
{
    if (_impl->streams.count(uri))
        _impl->streams[uri].buffer.insert(tile, sourceIndex);
}

void FrameDispatcher::processFrameFinished(const QString uri,
                                           const size_t sourceIndex)
{
    if (!_impl->streams.count(uri))
        return;

    auto& buffer = _impl->streams[uri].buffer;
    try
    {
        buffer.finishFrameForSource(sourceIndex);
        if (buffer.isAllowedToSend() && buffer.hasCompleteFrame())
            emit sendFrame(_impl->consumeLatestFrame(uri));
    }
    catch (const std::runtime_error& e)
    {
        emit pixelStreamError(uri, e.what());
    }
}

void FrameDispatcher::requestFrame(const QString uri)
{
    if (!_impl->streams.count(uri))
        return;

    auto& buffer = _impl->streams[uri].buffer;
    buffer.setAllowedToSend(true);
    try
    {
        if (buffer.hasCompleteFrame())
            emit sendFrame(_impl->consumeLatestFrame(uri));
    }
    catch (const std::runtime_error& e)
    {
        emit pixelStreamError(uri, e.what());
    }
}

void FrameDispatcher::deleteStream(const QString uri)
{
    _impl->streams.erase(uri);
    emit pixelStreamClosed(uri);
}
}
}
