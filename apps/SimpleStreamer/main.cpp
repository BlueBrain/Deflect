/*********************************************************************/
/* Copyright (c) 2014-2017, EPFL/Blue Brain Project                  */
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

#include <cmath>
#include <iostream>
#include <string>

#ifdef __APPLE__
#include <GLUT/glut.h> // GLUT is deprecated in 10.9
#include <OpenGL/gl.h>
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif

bool deflectInteraction = false;
bool deflectCompressImage = true;
bool deflectStereoStreamLeft = false;
bool deflectStereoStreamRight = false;
unsigned int deflectCompressionQuality = 75;
std::string deflectHost;
std::string deflectStreamId = "SimpleStreamer";
std::unique_ptr<deflect::Stream> deflectStream;

void syntax(int exitStatus);
void readCommandLineArguments(int argc, char** argv);
void initGLWindow(int argc, char** argv);
void initDeflectStream();
void display();
void reshape(int width, int height);

void cleanup()
{
    deflectStream.reset();
}

int main(int argc, char** argv)
{
    readCommandLineArguments(argc, argv);

    if (deflectHost.empty())
        syntax(EXIT_FAILURE);

    initGLWindow(argc, argv);
    initDeflectStream();

    atexit(cleanup);
    glutMainLoop(); // enter the main loop

    return EXIT_SUCCESS;
}

void syntax(const int exitStatus)
{
    std::cout << "Usage: simplestreamer [options] <host>" << std::endl;
    std::cout << "Stream a GLUT teapot to a remote host\n" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << " -h, --help         display this help" << std::endl;
    std::cout << " -n <stream id>     set stream identifier (default: "
                 "'SimpleStreamer')"
              << std::endl;
    std::cout << " -i                 enable interaction events (default: OFF)"
              << std::endl;
    std::cout
        << " -u                 enable uncompressed streaming (default: OFF)"
        << std::endl;
    std::cout << " -s                 enable stereo streaming, equivalent to "
                 "-l -r (default: OFF)"
              << std::endl;
    std::cout << " -l                 enable stereo streaming, left image only "
                 "(default: OFF)"
              << std::endl;
    std::cout << " -r                 enable stereo streaming, right image "
                 "only (default: OFF)"
              << std::endl;
    exit(exitStatus);
}

void readCommandLineArguments(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--help")
            syntax(EXIT_SUCCESS);

        if (argv[i][0] == '-')
        {
            switch (argv[i][1])
            {
            case 'n':
                if (i + 1 < argc)
                {
                    deflectStreamId = argv[i + 1];
                    ++i;
                }
                break;
            case 'i':
                deflectInteraction = true;
                break;
            case 'u':
                deflectCompressImage = false;
                break;
            case 's':
                deflectStereoStreamLeft = true;
                deflectStereoStreamRight = true;
                break;
            case 'l':
                deflectStereoStreamLeft = true;
                break;
            case 'r':
                deflectStereoStreamRight = true;
                break;
            case 'h':
                syntax(EXIT_SUCCESS);
            default:
                std::cerr << "Unknown command line option: " << argv[i]
                          << std::endl;
                ::exit(EXIT_FAILURE);
            }
        }
        else if (i == argc - 1)
            deflectHost = argv[i];
    }
}

void initGLWindow(int argc, char** argv)
{
    // setup GLUT / OpenGL
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(1024, 768);

    glutCreateWindow("SimpleStreamer");

    // the display function will be called continuously
    glutDisplayFunc(display);
    glutIdleFunc(display);

    // the reshape function will be called on window resize
    glutReshapeFunc(reshape);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
}

void initDeflectStream()
{
    deflectStream.reset(new deflect::Stream(deflectStreamId, deflectHost));
    if (!deflectStream->isConnected())
    {
        std::cerr << "Could not connect to host!" << std::endl;
        deflectStream.reset();
        exit(EXIT_FAILURE);
    }

    if (deflectInteraction && !deflectStream->registerForEvents())
    {
        std::cerr << "Could not register for events!" << std::endl;
        deflectStream.reset();
        exit(EXIT_FAILURE);
    }
}

struct Camera
{
    // angles of camera rotation and zoom factor
    float angleX = 0.f;
    float angleY = 0.f;
    float offsetX = 0.f;
    float offsetY = 0.f;
    float zoom = 1.f;

    void apply()
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        const float size = 2.f;
        glOrtho(-size, size, -size, size, -size, size);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glTranslatef(offsetX, -offsetY, 0.f);

        glRotatef(angleX, 0.f, 1.f, 0.f);
        glRotatef(angleY, -1.f, 0.f, 0.f);

        glScalef(zoom, zoom, zoom);
    }
};

struct Image
{
    size_t width = 0;
    size_t height = 0;
    std::vector<char> data;

    static Image readGlBuffer()
    {
        Image image;
        image.width = glutGet(GLUT_WINDOW_WIDTH);
        image.height = glutGet(GLUT_WINDOW_HEIGHT);
        image.data.resize(image.width * image.height * 4);
        glReadPixels(0, 0, image.width, image.height, GL_RGBA, GL_UNSIGNED_BYTE,
                     (GLvoid*)image.data.data());

        deflect::ImageWrapper::swapYAxis(image.data.data(), image.width,
                                         image.height, 4);
        return image;
    }
};

bool send(const Image& image, const deflect::View view)
{
    deflect::ImageWrapper deflectImage(image.data.data(), image.width,
                                       image.height, deflect::RGBA);
    deflectImage.compressionPolicy = deflectCompressImage
                                         ? deflect::COMPRESSION_ON
                                         : deflect::COMPRESSION_OFF;
    deflectImage.compressionQuality = deflectCompressionQuality;
    deflectImage.view = view;
    return deflectStream->send(deflectImage) && deflectStream->finishFrame();
}

bool timeout(const float sec)
{
    using clock = std::chrono::system_clock;
    static clock::time_point start = clock::now();
    return std::chrono::duration<float>{clock::now() - start}.count() > sec;
}

void display()
{
    static Camera camera;
    camera.apply();

    bool success = false;
    bool waitToStart = false;
    static bool deflectFirstEventReceived = false;
    if (deflectStereoStreamLeft || deflectStereoStreamRight)
    {
        // Poor man's attempt to synchronise the start of separate stereo
        // streams (waiting on first event from server or 5 sec. timeout).
        // This does not prevent applications from going quickly out of sync.
        // Real-world applications need a dedicated synchronization channel to
        // ensure corresponding left-right views are rendered and sent together.
        if (!(deflectStereoStreamLeft && deflectStereoStreamRight))
            waitToStart = !(deflectFirstEventReceived || timeout(5));

        if (deflectStereoStreamLeft && !waitToStart)
        {
            glClearColor(0.7, 0.3, 0.3, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glutSolidTeapot(1.f);
            const auto leftImage = Image::readGlBuffer();
            success = send(leftImage, deflect::View::left_eye);
        }
        if (deflectStereoStreamRight && !waitToStart)
        {
            glClearColor(0.3, 0.7, 0.3, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glutSolidTeapot(1.f);
            const auto rightImage = Image::readGlBuffer();
            success = (!deflectStereoStreamLeft || success) &&
                      send(rightImage, deflect::View::right_eye);
        }
    }
    else
    {
        glClearColor(0.5, 0.5, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glutSolidTeapot(1.f);
        success = send(Image::readGlBuffer(), deflect::View::mono);
    }

    glutSwapBuffers();

    // increment rotation angle according to interaction, or by a constant rate
    // if interaction is not enabled. Note that mouse position is in normalized
    // window coordinates: (0,0) to (1,1).
    if (deflectStream->isRegisteredForEvents())
    {
        static float mouseX = 0.f;
        static float mouseY = 0.f;

        // Note: there is a risk of missing events since we only process the
        // latest state available. For more advanced applications, event
        // processing should be done in a separate thread.
        while (deflectStream->hasEvent())
        {
            const deflect::Event& event = deflectStream->getEvent();

            deflectFirstEventReceived = true;

            switch (event.type)
            {
            case deflect::Event::EVT_CLOSE:
                std::cout << "Received close..." << std::endl;
                exit(EXIT_SUCCESS);
            case deflect::Event::EVT_PINCH:
                camera.zoom += std::copysign(std::sqrt(event.dx * event.dx +
                                                       event.dy * event.dy),
                                             event.dx + event.dy);
                break;
            case deflect::Event::EVT_PRESS:
                mouseX = event.mouseX;
                mouseY = event.mouseY;
                break;
            case deflect::Event::EVT_MOVE:
            case deflect::Event::EVT_RELEASE:
                if (event.mouseLeft)
                {
                    camera.angleX += (event.mouseX - mouseX) * 360.f;
                    camera.angleY += (event.mouseY - mouseY) * 360.f;
                }
                mouseX = event.mouseX;
                mouseY = event.mouseY;
                break;
            case deflect::Event::EVT_PAN:
                camera.offsetX += event.dx;
                camera.offsetY += event.dy;
                mouseX = event.mouseX;
                mouseY = event.mouseY;
                break;
            case deflect::Event::EVT_KEY_PRESS:
                if (event.key == ' ')
                {
                    camera.angleX = 0.f;
                    camera.angleY = 0.f;
                    camera.offsetX = 0.f;
                    camera.offsetY = 0.f;
                    camera.zoom = 1.f;
                }
                break;
            default:
                break;
            };
        }
    }
    else
    {
        if (!waitToStart)
        {
            camera.angleX += 1.f;
            camera.angleY += 1.f;
        }
    }

    if (!success)
    {
        if (!deflectStream->isConnected())
        {
            std::cout << "Stream closed, exiting." << std::endl;
            exit(EXIT_SUCCESS);
        }
        else if (!waitToStart)
        {
            std::cerr << "failure in deflectStreamSend()" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

void reshape(int width, int height)
{
    // reset the viewport
    glViewport(0, 0, width, height);
}
