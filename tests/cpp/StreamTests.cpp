/*********************************************************************/
/* Copyright (c) 2016, EPFL/Blue Brain Project                       */
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

#define BOOST_TEST_MODULE Stream
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include <deflect/Stream.h>

#include <QString>
#include <QtGlobal>
#include <memory>

namespace
{
const char* STREAM_ID_ENV_VAR = "DEFLECT_ID";
const char* STREAM_HOST_ENV_VAR = "DEFLECT_HOST";
}

BOOST_AUTO_TEST_CASE( testParameterizedConstructorWithValues )
{
    const deflect::Stream stream( "mystream", "somehost" );
    BOOST_CHECK_EQUAL( stream.getId(), "mystream" );
    BOOST_CHECK_EQUAL( stream.getHost(), "somehost" );
}

BOOST_AUTO_TEST_CASE( testDefaultConstructorReadsEnvironmentVariables )
{
    qputenv( STREAM_ID_ENV_VAR, "mystream" );
    qputenv( STREAM_HOST_ENV_VAR, "somehost" );
    deflect::Stream stream;
    BOOST_CHECK_EQUAL( stream.getId(), "mystream" );
    BOOST_CHECK_EQUAL( stream.getHost(), "somehost" );
    qunsetenv( STREAM_ID_ENV_VAR );
    qunsetenv( STREAM_HOST_ENV_VAR );
}

BOOST_AUTO_TEST_CASE( testParameterizedConstructorReadsEnvironmentVariables )
{
    qputenv( STREAM_ID_ENV_VAR, "mystream" );
    qputenv( STREAM_HOST_ENV_VAR, "somehost" );
    const deflect::Stream stream( "", "" );
    BOOST_CHECK_EQUAL( stream.getId(), "mystream" );
    BOOST_CHECK_EQUAL( stream.getHost(), "somehost" );
    qunsetenv( STREAM_ID_ENV_VAR );
    qunsetenv( STREAM_HOST_ENV_VAR );
}

BOOST_AUTO_TEST_CASE( testWhenSteamHostNotProvidedThenThrow )
{
    BOOST_REQUIRE( QString( qgetenv( STREAM_HOST_ENV_VAR )).isEmpty( ));
    BOOST_CHECK_THROW( std::make_shared<deflect::Stream>(),
                       std::runtime_error );
    BOOST_CHECK_THROW( deflect::Stream stream( "mystream", "" ),
                       std::runtime_error );
}

BOOST_AUTO_TEST_CASE( testWhenSteamHostProvidedThenNoThrow )
{
    BOOST_CHECK_NO_THROW( deflect::Stream stream( "mystream", "somehost" ));
    qputenv( STREAM_HOST_ENV_VAR, "somehost" );
    BOOST_CHECK_NO_THROW( std::make_shared<deflect::Stream>( ));
    BOOST_CHECK_NO_THROW( deflect::Stream stream( "mystream", "" ));
    qunsetenv( STREAM_HOST_ENV_VAR );
}

BOOST_AUTO_TEST_CASE( testWhenNoSteamIdProvidedThenARandomOneIsGenerated )
{
    BOOST_REQUIRE( QString( qgetenv( STREAM_ID_ENV_VAR )).isEmpty( ));
    {
        deflect::Stream stream( "", "somehost" );
        BOOST_CHECK( !stream.getId().empty( ));
        BOOST_CHECK_NE( deflect::Stream( "", "host").getId(),
                        deflect::Stream( "", "host").getId( ));
    }
    {
        qputenv( STREAM_HOST_ENV_VAR, "somehost" );
        deflect::Stream stream;
        BOOST_CHECK( !stream.getId().empty( ));
        BOOST_CHECK_NE( deflect::Stream().getId(), deflect::Stream().getId( ));
        qunsetenv( STREAM_HOST_ENV_VAR );
    }
}
