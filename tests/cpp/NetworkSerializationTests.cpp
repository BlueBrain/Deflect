/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
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

#define BOOST_TEST_MODULE NetworkSerializationTests
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include <deflect/Event.h>
#include <deflect/MessageHeader.h>

#include <QByteArray>
#include <QDataStream>

BOOST_AUTO_TEST_CASE(testMessageHeaderSerialization)
{
    QByteArray storage;

    deflect::MessageHeader header(deflect::MESSAGE_TYPE_PIXELSTREAM, 512,
                                  std::string("MyUri"));
    QDataStream dataStreamOut(&storage, QIODevice::Append);
    dataStreamOut << header;

    deflect::MessageHeader messageHeaderDeserialized;
    QDataStream dataStreamIn(storage);
    dataStreamIn >> messageHeaderDeserialized;

    BOOST_CHECK_EQUAL(messageHeaderDeserialized.type, header.type);
    BOOST_CHECK_EQUAL(messageHeaderDeserialized.size, header.size);
    BOOST_CHECK_EQUAL(std::string(messageHeaderDeserialized.uri),
                      std::string(header.uri));
}

BOOST_AUTO_TEST_CASE(testEventSerialization)
{
    QByteArray storage;

    deflect::Event event;
    event.type = deflect::Event::EVT_PRESS;
    event.mouseX = 0.78;
    event.mouseY = 0.456;

    event.dx = 0.6;
    event.dy = 0.2;

    event.mouseLeft = true;
    event.mouseRight = true;
    event.mouseMiddle = true;

    event.key = 'Y';
    event.modifiers = Qt::ControlModifier;

    QDataStream dataStreamOut(&storage, QIODevice::Append);
    dataStreamOut << event;

    deflect::Event eventDeserialized;
    QDataStream dataStreamIn(storage);
    dataStreamIn >> eventDeserialized;

    BOOST_CHECK_EQUAL(eventDeserialized.type, event.type);
    BOOST_CHECK_EQUAL(eventDeserialized.mouseX, event.mouseX);
    BOOST_CHECK_EQUAL(eventDeserialized.mouseY, event.mouseY);
    BOOST_CHECK_EQUAL(eventDeserialized.dx, event.dx);
    BOOST_CHECK_EQUAL(eventDeserialized.dy, event.dy);
    BOOST_CHECK_EQUAL(eventDeserialized.mouseLeft, event.mouseLeft);
    BOOST_CHECK_EQUAL(eventDeserialized.mouseRight, event.mouseRight);
    BOOST_CHECK_EQUAL(eventDeserialized.mouseMiddle, event.mouseMiddle);
    BOOST_CHECK_EQUAL(eventDeserialized.key, event.key);
    BOOST_CHECK_EQUAL(eventDeserialized.modifiers, event.modifiers);
}
