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
/* or implied, of Ecole polytechnique federale de Lausanne.          */
/*********************************************************************/

#include "SourceBuffer.h"

#include <exception>

namespace deflect
{

SourceBuffer::SourceBuffer()
{
    _segmentsMono.push( Segments( ));
    _segmentsLeft.push( Segments( ));
    _segmentsRight.push( Segments( ));
}

const Segments& SourceBuffer::getSegments( const View view ) const
{
    return _getQueue( view ).front();
}

FrameIndex SourceBuffer::getBackFrameIndex( const View view ) const
{
    switch( view )
    {
    case View::MONO:
        return _backFrameIndexMono;
    case View::LEFT_EYE:
        return _backFrameIndexLeft;
    case View::RIGHT_EYE:
        return _backFrameIndexRight;
    default:
        throw std::invalid_argument( "no such view" ); // keep compiler happy
    };
}

void SourceBuffer::pop( const View view )
{
    _getQueue( view ).pop();
}

void SourceBuffer::push( const View view )
{
    _getQueue( view ).push( Segments( ));

    switch( view )
    {
    case View::MONO:
        ++_backFrameIndexMono;
        break;
    case View::LEFT_EYE:
        ++_backFrameIndexLeft;
        break;
    case View::RIGHT_EYE:
        ++_backFrameIndexRight;
        break;
    };
}

void SourceBuffer::insert( const Segment& segment, const deflect::View view )
{
    _getQueue( view ).back().push_back( segment );
}

std::queue<Segments>& SourceBuffer::_getQueue( const deflect::View view )
{
    switch( view )
    {
    case View::MONO:
        return _segmentsMono;
    case View::LEFT_EYE:
        return _segmentsLeft;
    case View::RIGHT_EYE:
        return _segmentsRight;
    default:
        throw std::invalid_argument( "no such view" ); // keep compiler happy
    };
}

const std::queue<Segments>&
SourceBuffer::_getQueue( const deflect::View view ) const
{
    switch( view )
    {
    case View::MONO:
        return _segmentsMono;
    case View::LEFT_EYE:
        return _segmentsLeft;
    case View::RIGHT_EYE:
        return _segmentsRight;
    default:
        throw std::invalid_argument( "no such view" ); // keep compiler happy
    };
}

}
