/*********************************************************************/
/* Copyright (c) 2016-2016, EPFL/Blue Brain Project                  */
/*                          Stefan.Eilemann@epfl.ch                  */
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
/* or implied, of The Ecole Polytechnique Federal de Lausanne.       */
/*********************************************************************/

#ifndef DESKTOPSTREAMER_STREAM_H
#define DESKTOPSTREAMER_STREAM_H

#include <QImage>                // member
#include <QPersistentModelIndex> // member
#include <QRect>                 // member
#include <deflect/Stream.h>      // base class

class MainWindow;

class Stream : public deflect::Stream
{
public:
    /** Construct a new stream for the given desktop window. */
    Stream(const MainWindow& parent, const QPersistentModelIndex window,
           const std::string& id, const std::string& host, int pid = 0);
    ~Stream();

    /**
     * Send an update to the server.
     * @param quality the quality setting for compression [1; 100]
     * @return an empty string on success, the error message otherwise.
     */
    std::string update(int quality);

    /**
     * Process all pending events.
     *
     * @param interact enable interaction from the server
     * @return true on success, false if the stream should be closed.
     */
    bool processEvents(bool interact);

    const QPersistentModelIndex& getIndex() const;

private:
    Stream(const Stream&) = delete;
    Stream(Stream&&) = delete;

    class Impl;
    std::unique_ptr<Impl> _impl;
};

#endif
