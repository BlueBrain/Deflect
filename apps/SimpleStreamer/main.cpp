#include <string>
#include <iostream>

#ifdef __APPLE__
    #include <OpenGL/gl.h>
    #include <GLUT/glut.h>

    // GLUT is deprecated in 10.9
#   pragma clang diagnostic ignored "-Wdeprecated-declarations"
#   pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#else
    #include <GL/gl.h>
    #include <GL/glut.h>
#endif

// DisplayCluster streaming
#include <deflect/Stream.h>

bool deflectInteraction = false;
bool deflectCompressImage = true;
unsigned int deflectCompressionQuality = 75;
char * deflectHostname = NULL;
std::string deflectStreamName = "SimpleStreamer";
deflect::Stream* deflectStream = NULL;

void syntax(char * app);
void readCommandLineArguments(int argc, char **argv);
void initGLWindow(int argc, char **argv);
void initDeflectStream();
void display();
void reshape(int width, int height);


void cleanup()
{
    delete deflectStream;
}

int main(int argc, char **argv)
{
    readCommandLineArguments(argc, argv);

    if(deflectHostname == NULL)
    {
        syntax(argv[0]);
    }

    initGLWindow(argc, argv);

    initDeflectStream();

    atexit( cleanup );

    // enter the main loop
    glutMainLoop();

    return 0;
}

void readCommandLineArguments(int argc, char **argv)
{
    for(int i=1; i<argc; i++)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
                case 'n':
                    if(i+1 < argc)
                    {
                        deflectStreamName = argv[i+1];
                        i++;
                    }
                    break;
                case 'i':
                    deflectInteraction = true;
                    break;
                case 'u':
                    deflectCompressImage = false;
                    break;
                default:
                    syntax(argv[0]);
            }
        }
        else if(i == argc-1)
        {
            deflectHostname = argv[i];
        }
    }
}


void initGLWindow(int argc, char **argv)
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

    glClearColor(0.5, 0.5, 0.5, 1.);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
}


void initDeflectStream()
{
    // connect to DisplayCluster
    deflectStream = new deflect::Stream( deflectStreamName, deflectHostname );
    if( !deflectStream->isConnected( ))
    {
        std::cerr << "Could not connect to host!" << std::endl;
        delete deflectStream;
        exit(1);
    }

    if( deflectInteraction && !deflectStream->registerForEvents( ))
    {
        std::cerr << "Could not register for events!" << std::endl;
        delete deflectStream;
        exit(1);
    }
}


void syntax(char * app)
{
    std::cerr << "syntax: " << app << " [options] <hostname>" << std::endl;
    std::cerr << "options:" << std::endl;
    std::cerr << " -n <stream name>     set stream name (default SimpleStreamer)" << std::endl;
    std::cerr << " -i                   enable interaction events (default disabled)" << std::endl;
    std::cerr << " -u                   enable uncompressed streaming (default disabled)" << std::endl;

    exit(1);
}

void display()
{
    // angles of camera rotation
    static float angleX = 0.;
    static float angleY = 0.;

    // zoom factor
    static float zoom = 1.;

    // clear color / depth buffers and setup view
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float size = 2.;
    glOrtho(-size, size, -size, size, -size, size);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(angleX, 0.0, 1.0, 0.0);
    glRotatef(angleY, -1.0, 0.0, 0.0);

    glScalef(zoom, zoom, zoom);

    // render the teapot
    glutSolidTeapot(1.);

    // send to DisplayCluster

    // get current window dimensions
    int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
    int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

    // grab the image data from OpenGL
    const size_t imageSize = windowWidth * windowHeight * 4;
    unsigned char* imageData = new unsigned char[imageSize];
    glReadPixels(0,0,windowWidth,windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)imageData);

    deflect::ImageWrapper deflectImage((const void*)imageData, windowWidth, windowHeight, deflect::RGBA);
    deflectImage.compressionPolicy = deflectCompressImage ? deflect::COMPRESSION_ON : deflect::COMPRESSION_OFF;
    deflectImage.compressionQuality = deflectCompressionQuality;
    deflect::ImageWrapper::swapYAxis((void*)imageData, windowWidth, windowHeight, 4);
    const bool success = deflectStream->send(deflectImage);
    deflectStream->finishFrame();

    // and free the allocated image data
    delete [] imageData;

    glutSwapBuffers();

    // increment rotation angle according to interaction, or by a constant rate
    // if interaction is not enabled. Note that mouse position is in normalized
    // window coordinates: (0,0) to (1,1).
    if( deflectStream->isRegisteredForEvents( ))
    {
        static float mouseX = 0.;
        static float mouseY = 0.;

        // Note: there is a risk of missing events since we only process the
        // latest state available. For more advanced applications, event
        // processing should be done in a separate thread.
        while( deflectStream->hasEvent( ))
        {
            const deflect::Event& event = deflectStream->getEvent();

            if( event.type == deflect::Event::EVT_CLOSE )
            {
                std::cout << "Received close..." << std::endl;
                exit(0);
            }

            const float newMouseX = event.mouseX;
            const float newMouseY = event.mouseY;

            if( event.mouseLeft )
            {
                angleX += (newMouseX - mouseX) * 360.;
                angleY += (newMouseY - mouseY) * 360.;
            }
            else if( event.mouseRight )
                zoom += (newMouseY - mouseY);

            mouseX = newMouseX;
            mouseY = newMouseY;
        }
    }
    else
    {
        angleX += 1.;
        angleY += 1.;
    }

    if(!success)
    {
        if (!deflectStream->isConnected())
        {
            std::cout << "Stream closed, exiting." << std::endl;
            exit(0);
        }
        else
        {
            std::cerr << "failure in deflectStreamSend()" << std::endl;
            exit(1);
        }
    }
}

void reshape(int width, int height)
{
    // reset the viewport
    glViewport(0, 0, width, height);
}
