Stereo Streaming
============

This document describes the stereo streaming support introduced in Deflect 0.13.

## Requirements

* Simple extension of the monoscopic Stream API
* No network protocol changes that break current Deflect clients or servers
* Support both screen space decompostion (sort-first) and left-right stereo
  decomposition modes for distributed rendering.

![Stereo streaming overview](stereo.png)

## API

New view enum in deflect/types.h:

    enum class View : std::int8_t { mono, left_eye, right_eye };

On the client side, no changes to the Stream API. The ImageWrapper takes an
additional View parameter.

On the server side, no changes to the Server API (except some cleanups). Each
Frame dispatched now contains the View information.

## Protocol

The Stream send an additional View information message before each image
payload. This message is silently ignored by older Servers.

## Examples

Example of a stereo 3D client application using the blocking Stream API:

    deflect::Stream stream( ... );

    /** ...synchronize start with other render clients (network barrier)... */

    renderLoop()
    {
    /** ...render left image... */

    deflect::ImageWrapper leftImage( data, width, height, deflect::RGBA );
    leftImage.view = deflect::View::left_eye;
    deflectStream->send( leftImage );

    /** ...render right image... */

    deflect::ImageWrapper rightImage( data, width, height, deflect::RGBA );
    rightImage.view = deflect::View::right_eye;
    deflectStream->send( rightImage );

    deflectStream->finishFrame();

    /** ...synchronize with other render clients (network barrier)... */
    }

Example of a stereo 3D client application using the asynchronous Stream API:

    deflect::Stream stream( ... );

    /** ...synchronize start with other render clients (network barrier)... */

    delfect::Stream::Future leftFuture, rightFuture;
    leftFuture = deflect::qt::make_ready_future<bool>( true );
    rightFuture = deflect::qt::make_ready_future<bool>( true );

    ImageData leftData, rightData; // must remain valid until sending completes

    renderLoop()
    {
    if( !leftFuture.valid() || !leftFuture.get( ))
       return;

    /** ...render left image... */

    deflect::ImageWrapper leftImage( leftData, width, height, deflect::RGBA );
    leftImage.view = deflect::View::left_eye;
    leftFuture = deflectStream->asyncSend( leftImage );

    if( !rightFuture.valid() || !rightFuture.get( ))
       return;

    /** ...render right image... */

    deflect::ImageWrapper rightImage( rightData, width, height, deflect::RGBA );
    rightImage.view = deflect::View::right_eye;
    rightFuture = deflectStream->send( rightImage );

    /** ...synchronize with other render clients (network barrier)... */
    }

For a complete code example, please refer to the SimpleStreamer application.

## Issues

### 1: Should clients be notified if they attempt to stream stereo content to a server which can only handle monoscopic content?

_Resolution: Not yet:_
For legacy implementation reasons, it is not possible to
send information from the Server to the Stream about the capabilities of the
server (such as stereo support) without rejecting all clients based on Deflect
< 0.12.1. A transition period is required before implementing this feature.
