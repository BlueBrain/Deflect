/*********************************************************************/
/* Copyright (c) 2015, EPFL/Blue Brain Project                       */
/*                     Daniel.Nachbaur <daniel.nachbaur@epfl.ch>     */
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

#include "EventReceiver.h"

#include <QCoreApplication>

namespace deflect
{
namespace qt
{

EventReceiver::EventReceiver( Stream& stream )
    : QObject()
    , _stream( stream )
    , _notifier( new QSocketNotifier( _stream.getDescriptor(),
                                      QSocketNotifier::Read ))
    , _timer( new QTimer )
{
    connect( _notifier.get(), &QSocketNotifier::activated,
             this, &EventReceiver::_onEvent );

    // QSocketNotifier sometimes does not fire, help with a timer
    connect( _timer.get(), &QTimer::timeout,
             [this] { _onEvent( _stream.getDescriptor( )); });
    _timer->start( 1 );
}

EventReceiver::~EventReceiver()
{
}

void EventReceiver::_onEvent( int socket )
{
    if( socket != _stream.getDescriptor( ))
        return;

    while( _stream.hasEvent( ))
    {
        const Event& deflectEvent = _stream.getEvent();

        switch( deflectEvent.type )
        {
        case Event::EVT_CLOSE:
            _notifier->setEnabled( false );
            QCoreApplication::quit();
            break;
        case Event::EVT_PRESS:
            emit pressed( deflectEvent.mouseX, deflectEvent.mouseY );
            break;
        case Event::EVT_RELEASE:
            emit released( deflectEvent.mouseX, deflectEvent.mouseY );
            break;
        case Event::EVT_MOVE:
            emit moved( deflectEvent.mouseX, deflectEvent.mouseY );
            break;
        case Event::EVT_VIEW_SIZE_CHANGED:
            emit resized( deflectEvent.dx, deflectEvent.dy );
            break;
        case Event::EVT_WHEEL:
            emit wheeled( deflectEvent.mouseX, deflectEvent.mouseY,
                          deflectEvent.dy );
            break;

        case Event::EVT_CLICK:
        case Event::EVT_DOUBLECLICK:
        case Event::EVT_SWIPE_LEFT:
        case Event::EVT_SWIPE_RIGHT:
        case Event::EVT_SWIPE_UP:
        case Event::EVT_SWIPE_DOWN:
        case Event::EVT_KEY_PRESS:
            emit keyPress( deflectEvent.key, deflectEvent.modifiers,
                           QString::fromStdString( deflectEvent.text ));
            break;
        case Event::EVT_KEY_RELEASE:
            emit keyRelease( deflectEvent.key, deflectEvent.modifiers,
                             QString::fromStdString( deflectEvent.text ));
            break;
        default:
            break;
        }
    }
}
}

}
