/*********************************************************************/
/* Copyright (c) 2013-2017, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
/*                          Stefan.Eilemann@epfl.ch                  */
/*                          Daniel.Nachbaur@epfl.ch                  */
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

#include "StreamPrivate.h"

#include <QHostInfo>

#include <stdexcept>

namespace
{
const char* STREAM_ID_ENV_VAR = "DEFLECT_ID";
const char* STREAM_HOST_ENV_VAR = "DEFLECT_HOST";

std::string _getStreamHost(const std::string& host)
{
    if (!host.empty())
        return host;

    const QString streamHost = qgetenv(STREAM_HOST_ENV_VAR).constData();
    if (!streamHost.isEmpty())
        return streamHost.toStdString();

    throw std::runtime_error("No host provided");
}

std::string _getStreamId(const std::string& id)
{
    if (!id.empty())
        return id;

    const QString streamId = qgetenv(STREAM_ID_ENV_VAR).constData();
    if (!streamId.isEmpty())
        return streamId.toStdString();

    return QString("%1_%2")
        .arg(QHostInfo::localHostName(), QString::number(rand(), 16))
        .toStdString();
}
}

namespace deflect
{
StreamPrivate::StreamPrivate(const std::string& id_, const std::string& host,
                             const unsigned short port, const bool observer)
    : id{_getStreamId(id_)}
    , socket{_getStreamHost(host), port}
    , sendWorker{socket, id}
{
    if (!socket.isConnected())
        throw std::runtime_error(
            "Connection to deflect server could not be established");

    socket.connect(&socket, &Socket::disconnected, [this]() {
        if (disconnectedCallback)
            disconnectedCallback();
    });

    socket.moveToThread(&sendWorker);
    sendWorker.start();

    if (observer)
        sendWorker.enqueueObserverOpen().wait();
    else
        sendWorker.enqueueOpen().wait();
}

StreamPrivate::~StreamPrivate()
{
    if (socket.isConnected())
        sendWorker.enqueueClose().wait();
}
}
