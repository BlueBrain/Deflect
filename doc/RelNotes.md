Release Notes {#mainpage}
============

[TOC]

# Introduction {#Introduction}

Welcome to Deflect, a C++ library to develop applications that can send
and receive pixel streams from other Deflect-based applications, for
example DisplayCluster. Deflect offers a stable API marked with version
1.0 for the streaming part. The following applications are provided
which make use of the streaming API:

* DesktopStreamer: A small utility that lets you stream your desktop.
* SimpleStreamer: A simple example to demonstrate streaming of an OpenGL
  application.

Please file a [Bug Report](https://github.com/BlueBrain/Deflect/issues)
if you find any other issue with this release.

# Changes {#Changes}

## git master (0.6)

* Many classes of the Server part have been renamed
* Various API fixes
* Project-wide codestyle cleanup

## Deflect 0.5

* Streamers can register for events immediately, without waiting for the
  DisplayCluster host to have opened a window
* Desktopstreamer supports autodiscovery of DisplayCluster hosts using Zeroconf
* Add interaction from the Wall with the DesktopStreamer host (Mac OS X
  only)

## Deflect 0.4

* DesktopStreamer properly handles AppNap on OSX 10.9.
* DesktopStreamer detects Retina displays automatically (no retina checkbox).
* First release, based on the DisplayCluster 0.4 dcStream library
