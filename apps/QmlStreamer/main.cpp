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

#include <QGuiApplication>
#include <QCommandLineParser>
#include <deflect/qt/QmlStreamer.h>
#include <deflect/version.h>

int main( int argc, char** argv )
{
    QGuiApplication app( argc,argv );
    QGuiApplication::setApplicationVersion(
                QString::fromStdString( deflect::Version::getString( )));
    app.setQuitOnLastWindowClosed( true );

    QCommandLineParser parser;
    parser.setApplicationDescription( "QmlStreamer example" );
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption qmlFileOption( "qml", "QML file to load", "qml-file",
                                      "qrc:/qml/gui.qml" );
    parser.addOption( qmlFileOption );

    QCommandLineOption streamHostOption( "host", "Stream hostname", "hostname",
                                         "localhost" );
    parser.addOption( streamHostOption );

    parser.process( app );

    const QString qmlFile = parser.value( qmlFileOption );
    const QString streamHost = parser.value( streamHostOption );

    try
    {
        QScopedPointer< deflect::qt::QmlStreamer > streamer(
             new deflect::qt::QmlStreamer( qmlFile, streamHost.toStdString( )));
        return app.exec();
    }
    catch( const std::runtime_error& exception )
    {
        qWarning() << "QmlStreamer startup failed:" << exception.what();
        return EXIT_FAILURE;
    }
}
