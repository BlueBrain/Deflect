# Deflect

Welcome to Deflect, a C++ library for streaming pixels to other Deflect-based
applications, for example
[DisplayCluster](https://github.com/BlueBrain/DisplayCluster).
Deflect offers a stable API marked with version 1.0 (for the client part).

## Features

Deflect provides the following functionality:

* Stream pixels to a remote Server from one or multiple sources
* Register for receiving events from the Server
* Compressed or uncompressed streaming
* Fast multi-threaded JPEG compression (using libjpeg-turbo)

The following applications are provided which make use of the streaming API:

* DesktopStreamer: A small utility that lets you stream your desktop.
* SimpleStreamer: A simple example to demonstrate streaming of an OpenGL
  application.

## Building from source

~~~
  git clone https://github.com/BlueBrain/Deflect.git
  mkdir Deflect/build
  cd Deflect/build
  cmake ..
  make
~~~

## ChangeLog

To keep track of the changes between releases check the
[changelog](doc/Changelog.md).

## About

Deflect is a cross-platform library, designed to run on any modern operating
system, including all Unix variants. Deflect uses CMake to create a
platform-specific build environment. The following platforms and build
environments are tested:

* Linux: Ubuntu 14.04 and RHEL 6 (Makefile, x64)
* Mac OS X: 10.7 - 10.10 (Makefile, x86_64)

The [latest API documentation](http://bluebrain.github.io/Deflect-0.6/index.html)
can be found on [bluebrain.github.io](http://bluebrain.github.io).
