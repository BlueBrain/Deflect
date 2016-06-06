/*********************************************************************/
/* Copyright (c) 2014-2015, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
/*                                                                   */
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
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

#include <deflect/Stream.h>

#include <string>
#include <iostream>

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <GLUT/glut.h> // GLUT is deprecated in 10.9
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#else
#  include <GL/gl.h>
#  include <GL/glut.h>
#endif

bool deflectInteraction = false;
bool deflectCompressImage = true;
unsigned int deflectCompressionQuality = 75;
char* deflectHost = NULL;
std::string deflectStreamId = "SimpleStreamer";
deflect::Stream* deflectStream = NULL;

void syntax( char* app );
void readCommandLineArguments( int argc, char** argv );
void initGLWindow( int argc, char** argv );
void initDeflectStream();
void display();
void reshape( int width, int height );


void cleanup()
{
    delete deflectStream;
}

int main( int argc, char** argv )
{
    readCommandLineArguments( argc, argv );

    if( deflectHost == NULL )
        syntax( argv[0] );

    initGLWindow( argc, argv );
    initDeflectStream();

    atexit( cleanup );
    glutMainLoop(); // enter the main loop

    return 0;
}

void readCommandLineArguments( int argc, char** argv )
{
    for( int i = 1; i < argc; ++i )
    {
        if( argv[i][0] == '-' )
        {
            switch( argv[i][1] )
            {
                case 'n':
                    if( i + 1 < argc )
                    {
                        deflectStreamId = argv[i+1];
                        ++i;
                    }
                    break;
                case 'i':
                    deflectInteraction = true;
                    break;
                case 'u':
                    deflectCompressImage = false;
                    break;
                default:
                    syntax( argv[0] );
            }
        }
        else if( i == argc - 1 )
            deflectHost = argv[i];
    }
}

void initGLWindow( int argc, char** argv )
{
    // setup GLUT / OpenGL
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
    glutInitWindowPosition( 0, 0 );
    glutInitWindowSize( 1024, 768 );

    glutCreateWindow( "SimpleStreamer" );

    // the display function will be called continuously
    glutDisplayFunc( display );
    glutIdleFunc( display );

    // the reshape function will be called on window resize
    glutReshapeFunc( reshape );

    glClearColor( 0.5, 0.5, 0.5, 1.0 );

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_LIGHTING) ;
    glEnable( GL_LIGHT0 );
}


void initDeflectStream()
{
    deflectStream = new deflect::Stream( deflectStreamId, deflectHost );
    if( !deflectStream->isConnected( ))
    {
        std::cerr << "Could not connect to host!" << std::endl;
        delete deflectStream;
        exit( 1 );
    }

    if( deflectInteraction && !deflectStream->registerForEvents( ))
    {
        std::cerr << "Could not register for events!" << std::endl;
        delete deflectStream;
        exit( 1 );
    }
}


void syntax( char* app )
{
    std::cerr << "syntax: " << app << " [options] <host>" << std::endl;
    std::cerr << "options:" << std::endl;
    std::cerr << " -n <stream id>     set stream identifier (default: 'SimpleStreamer')" << std::endl;
    std::cerr << " -i                 enable interaction events (default: OFF)" << std::endl;
    std::cerr << " -u                 enable uncompressed streaming (default: OFF)" << std::endl;

    exit( 1 );
}

void display()
{
    // angles of camera rotation and zoom factor
    static float angleX = 0.f;
    static float angleY = 0.f;
    static float zoom = 1.f;

    // Render the teapot
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    const float size = 2.f;
    glOrtho( -size, size, -size, size, -size, size );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    glRotatef( angleX, 0.0, 1.0, 0.0 );
    glRotatef( angleY, -1.0, 0.0, 0.0 );

    glScalef( zoom, zoom, zoom );
    glutSolidTeapot( 1.f );

    // Grab the frame from OpenGL
    const int windowWidth = glutGet( GLUT_WINDOW_WIDTH );
    const int windowHeight = glutGet( GLUT_WINDOW_HEIGHT );

    const size_t imageSize = windowWidth * windowHeight * 4;
    unsigned char* imageData = new unsigned char[imageSize];
    glReadPixels( 0, 0, windowWidth, windowHeight, GL_RGBA, GL_UNSIGNED_BYTE,
                  (GLvoid*)imageData );

    // Send the frame through the stream
    deflect::ImageWrapper deflectImage( (const void*)imageData, windowWidth,
                                        windowHeight, deflect::RGBA );
    deflectImage.compressionPolicy = deflectCompressImage ?
                deflect::COMPRESSION_ON : deflect::COMPRESSION_OFF;
    deflectImage.compressionQuality = deflectCompressionQuality;
    deflect::ImageWrapper::swapYAxis( (void*)imageData, windowWidth,
                                      windowHeight, 4 );
    const bool success = deflectStream->send( deflectImage );
    deflectStream->finishFrame();

    delete [] imageData;
    glutSwapBuffers();

    // increment rotation angle according to interaction, or by a constant rate
    // if interaction is not enabled. Note that mouse position is in normalized
    // window coordinates: (0,0) to (1,1).
    if( deflectStream->isRegisteredForEvents( ))
    {
        static float mouseX = 0.f;
        static float mouseY = 0.f;

        // Note: there is a risk of missing events since we only process the
        // latest state available. For more advanced applications, event
        // processing should be done in a separate thread.
        while( deflectStream->hasEvent( ))
        {
            const deflect::Event& event = deflectStream->getEvent();

            if( event.type == deflect::Event::EVT_CLOSE )
            {
                std::cout << "Received close..." << std::endl;
                exit( 0 );
            }

            const float newMouseX = event.mouseX;
            const float newMouseY = event.mouseY;

            if( event.mouseLeft )
            {
                angleX += (newMouseX - mouseX) * 360.f;
                angleY += (newMouseY - mouseY) * 360.f;
            }
            else if( event.mouseRight )
                zoom += (newMouseY - mouseY);

            mouseX = newMouseX;
            mouseY = newMouseY;
        }
    }
    else
    {
        angleX += 1.f;
        angleY += 1.f;
    }

    if( !success )
    {
        if( !deflectStream->isConnected( ))
        {
            std::cout << "Stream closed, exiting." << std::endl;
            exit( 0 );
        }
        else
        {
            std::cerr << "failure in deflectStreamSend()" << std::endl;
            exit( 1 );
        }
    }
}

void reshape( int width, int height )
{
    // reset the viewport
    glViewport( 0, 0, width, height );
}
