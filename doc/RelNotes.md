Release Notes {#ReleaseNotes}
============

[TOC]

# Introduction {#Introduction}

Welcome to Deflect, a C++ library to develop applications that can send and
receive pixel streams from other Deflect-based applictions.

Deflect 0.4 is the first release, based on the version that was part of
DisplayCluster 0.4. Deflect 0.4 offers a stable API marked with version
1.0 for the streaming part.  The following applications are provided
which make use of the streaming API:

* DesktopStreamer: A small utility that lets you stream your desktop.
* SimpleStreamer: A simple example to demonstrate streaming of an OpenGL
  application.

Please file a [Bug Report](https://github.com/BlueBrain/Deflect/issues)
if you find any other issue with this release.

# Changes {#Changes}

## git master

* Add interaction from the Wall with the DesktopStreamer host (Mac OS X
  only)

## Deflect 0.4

* DesktopStreamer properly handles AppNap on OSX 10.9.
* DesktopStreamer detects Retina displays automatically (no retina checkbox).
