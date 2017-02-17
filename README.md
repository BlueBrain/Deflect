# Deflect

Welcome to Deflect, a C++ library for streaming pixels to other Deflect-based
applications, for example [Tide](https://github.com/BlueBrain/Tide).
Deflect offers a stable API marked with version 1.6 (for the client part).

## Features

Deflect provides the following functionality:

* Stream pixels to a remote Server from one or multiple sources
* Stream stereo images from a distributed 3D application
* Receive input events from the Server and send data to it
* Transmitted events include keyboard, mouse and multi-point touch gestures
* Compressed or uncompressed streaming
* Fast multi-threaded JPEG compression (using libjpeg-turbo)

DeflectQt (optional) provides the following additional functionality:

* Create QML applications which render offscreen and stream and receive events
  via Deflect

The following applications are provided which make use of the streaming API:

* DesktopStreamer: A small utility that lets you stream your desktop.
* SimpleStreamer: A simple example to demonstrate streaming of an OpenGL
  application.
* QmlStreamer (optional): An offscreen application to stream any given qml file.

## Building from source

~~~
  git clone https://github.com/BlueBrain/Deflect.git
  mkdir Deflect/build
  cd Deflect/build
  cmake -GNinja ..
  ninja
~~~

## ChangeLog

To keep track of the changes between releases check the @ref Changelog.

## About

Deflect is a cross-platform library, designed to run on any modern operating
system, including all Unix variants. Deflect uses CMake to create a
platform-specific build environment. The following platforms and build
environments are tested:

* Linux: Ubuntu 16.04 and RHEL 6 (Makefile, Ninja; x64)
* Mac OS X: 10.7 - 10.10 (Makefile, Ninja; x86_64)

The [latest API documentation](http://bluebrain.github.io/Deflect-0.12/index.html)
can be found on [bluebrain.github.io](http://bluebrain.github.io).
