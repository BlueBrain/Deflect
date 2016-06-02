/* Copyright (c) 2015, EPFL/Blue Brain Project
 *                     Daniel.Nachbaur@epfl.ch
 *
 * This file is part of Deflect <https://github.com/BlueBrain/Deflect>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

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

    QCommandLineOption streamHostOption( "host", "Stream target host "
                                                 "(default: localhost)",
                                         "host", "localhost" );
    parser.addOption( streamHostOption );

    // note: the 'name' command line option is already taken by QCoreApplication
    QCommandLineOption streamNameOption( "streamname", "Stream name (default: "
                                                 "Qml's root objectName "
                                                 "property or 'QmlStreamer')",
                                         "name" );
    parser.addOption( streamNameOption );

    parser.process( app );

    const QString qmlFile = parser.value( qmlFileOption );
    const QString streamHost = parser.value( streamHostOption );
    const QString streamName = parser.value( streamNameOption );

    try
    {
        std::unique_ptr< deflect::qt::QmlStreamer > streamer(
             new deflect::qt::QmlStreamer( qmlFile, streamHost.toStdString(),
                                           streamName.toStdString( )));
        app.connect( streamer.get(), &deflect::qt::QmlStreamer::streamClosed,
                     &app, &QCoreApplication::quit );
        return app.exec();
    }
    catch( const std::runtime_error& exception )
    {
        qWarning() << "QmlStreamer startup failed:" << exception.what();
        return EXIT_FAILURE;
    }
}
