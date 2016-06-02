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

#ifndef STREAM_H
#define STREAM_H

#include <deflect/Stream.h> // base class
#include <QImage> // member
#include <QPersistentModelIndex> // member
#include <QRect> // member

class MainWindow;

class Stream : public deflect::Stream
{
public:
    /** Construct a new stream for the given desktop window. */
    Stream( const MainWindow& parent, const QPersistentModelIndex window,
            const std::string& id, const std::string& host );
    ~Stream();

    /**
     * Send an update to the server.
     * @return an empty string on success, the error message otherwise.
     */
    std::string update();

    /**
     * Process all pending events.
     *
     * @param interact enable interaction from the server
     * @return true on success, false if the stream should be closed.
     */
    bool processEvents( bool interact );

    const QPersistentModelIndex& getIndex() const { return _window; }

private:
    const MainWindow& _parent;
    const QPersistentModelIndex _window;
    QRect _windowRect; // position on host in non-retina coordinates
    const QImage _cursor;
    QImage _image;

    Stream( const Stream& ) = delete;
    Stream( Stream&& ) = delete;

    void _sendMousePressEvent( float x, float y );
    void _sendMouseMoveEvent( float x, float y );
    void _sendMouseReleaseEvent( float x, float y );
    void _sendMouseDoubleClickEvent( float x, float y );
};

#endif
