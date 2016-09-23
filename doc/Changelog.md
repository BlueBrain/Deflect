Changelog {#Changelog}
============

## Deflect 0.12

### 0.12.0 (git master)

* [130](https://github.com/BlueBrain/Deflect/pull/130)
  Replaced boost by C++11. Boost is now an optional dependency and it is used
  only by the tests. Some API changes were introduced by this change.
* [129](https://github.com/BlueBrain/Deflect/pull/129)
  Cleared Deflect from boost::serialization that was used exclusively by Tide.
* [128](https://github.com/BlueBrain/Deflect/pull/128)
  New events for transmitting all touch points in addition to existing gestures:
  - Gives the ability to handle more than one touch point in applications (e.g.
    draw with multiple fingers on a whiteboard).
  - Simplifies integration for applications that natively handle multitouch
    events, like Qml-based ones.
  - DeflectQt uses a new TouchInjector class for sending touch events to Qt
    (moved from Tide).
  In addition:
  - A new *pinch* event was added. Clients of Tide >= 1.2 have to adapt their
    code to use it instead of *wheel* events, which are no longer sent.
  - Minor additions to the simplestreamer demo application.
* [126](https://github.com/BlueBrain/Deflect/pull/126)
  DesktopStreamer: The list of default hosts can be configured using the CMake
  variable DEFLECT_DESKTOPSTREAMER_HOSTS.
* [124](https://github.com/BlueBrain/Deflect/pull/124):
  QmlStreamer: Users can now interact with WebGL content in a WebEngineView
  and no longer risk opening a system context menu with a long press (prevent
  crashes in certain offscreen applications).
* [123](https://github.com/BlueBrain/Deflect/pull/123):
  QmlStreamer is now compatible with Qml WebEngineView items. Users must call
  QtWebEngine::initialize() in their QApplication before creating the stream.
  To receive keyboard events, the objectName property of a WebEngineView must
  be set to "webengineview".
  QmlStreamer also exposes the Stream::sendData() function and swipe gestures
  are available in Qml from a "deflectgestures" context property.
* [119](https://github.com/BlueBrain/Deflect/pull/119):
  Added deflect::Stream::sendData() to allow sending user-defined information
  to the deflect::Server.
* [118](https://github.com/BlueBrain/Deflect/pull/118):
  New "pan" event for multi-finger pan gestures.
* [117](https://github.com/BlueBrain/Deflect/pull/117):
  Fix several issues with stream connection / disconnection [DISCL-375]
* [115](https://github.com/BlueBrain/Deflect/pull/115):
  Bugfix for possible endless loop of warnings in ServerWorker::_flushSocket():
  "QAbstractSocket::waitForBytesWritten() is not allowed in UnconnectedState"
* [114](https://github.com/BlueBrain/Deflect/pull/114):
  QmlStreamer: added support for keyboard events

## Deflect 0.11

### 0.11.1 (30-06-2016)

* [112](https://github.com/BlueBrain/Deflect/pull/112):
  DesktopStreamer improvements:
  - new "view" menu to show advanced options (stream id, fps), hidden by default
  - the experimental multi-window streaming on OSX can be selected at runtime
    through the view menu (no longer a CMake option)
  - improved resizing policy of the main window

### 0.11.0 (17-06-2016)

* [111](https://github.com/BlueBrain/Deflect/pull/111):
  DesktopStreamer: bugfix; stop streaming when the server closes the stream
  and the "remote control" option was not enabled.
* [110](https://github.com/BlueBrain/Deflect/pull/110):
  DesktopStreamer: Support for streaming multiple windows on OSX disabled by
  default, can be enabled with cmake -DDESKTOPSTREAMER_ENABLE_MULTIWINDOW=ON.
* [106](https://github.com/BlueBrain/Deflect/pull/106):
  DesktopStreamer: Rename 'interaction' -> 'remote control', off by default.
* [103](https://github.com/BlueBrain/Deflect/pull/103):
  DesktopStreamer: prevent AppNap of being re-enabled automatically
* [102](https://github.com/BlueBrain/Deflect/pull/102):
  DeflectQt: Continue rendering & streaming after updates for a while to
  compensate for running animations, fix spurious missing event handling
* [101](https://github.com/BlueBrain/Deflect/pull/101):
  DesktopStreamer: windows that are streamed independently are activated
  (i.e. sent to the foreground) before applying an interaction event. The mouse
  cursor is now rendered only on active windows or desktop.
* [100](https://github.com/BlueBrain/Deflect/pull/100):
  QmlStreamer: correcly quit application when stream is closed.
* [99](https://github.com/BlueBrain/Deflect/pull/99):
  Fix incomplete socket send under certain timing conditions
* [98](https://github.com/BlueBrain/Deflect/pull/98):
  Streams can be constructed based on the DEFLECT_ID and DEFLECT_HOST ENV_VARs.
* [97](https://github.com/BlueBrain/Deflect/pull/97):
  DesktopStreamer: the list of windows available for streaming is also updated
  after a window has been hidden or unhidden.
* [94](https://github.com/BlueBrain/Deflect/pull/94):
  Removed legacy SendCommand functionality from Stream to Server. It was only
  used internally by the old Tide Dock streamer and never meant to be public.
* [93](https://github.com/BlueBrain/Deflect/pull/93):
  Minor fixes for the Qml stream API:
  - Cleanup size handling, allow programmatic resizing of root Qml item
  - Do not send a size hints event if none were set by the user

## Deflect 0.10

### 0.10.1 (01-04-2016)
* [79](https://github.com/BlueBrain/Deflect/pull/79):
  DesktopStreamer: Grid-view like layout in apps listview

### 0.10.0 (07-03-2016)
* [79](https://github.com/BlueBrain/Deflect/pull/79):
  DesktopStreamer: Support multi-window streaming on OS X, fix mouse
  event handling for retina devices, fix crash on stream close from
  DisplayCluster
* [76](https://github.com/BlueBrain/Deflect/pull/76):
  DesktopStreamer: Try to recover from streaming errors

## Deflect 0.9

### 0.9.1 (03-12-2015)
* [66](https://github.com/BlueBrain/Deflect/pull/66):
  DesktopStreamer: Fix memleaks with app streaming on OSX
* [66](https://github.com/BlueBrain/Deflect/pull/66):
  DesktopStreamer: Use current hostname if manually typed w/o pressing 'Enter'

### 0.9.0 (17-11-2015)
* [65](https://github.com/BlueBrain/Deflect/pull/65):
  QmlStreamer: New 'streamname' command line option.
* [64](https://github.com/BlueBrain/Deflect/pull/64):
  QmlStreamer: Fix event forwarding, implement wheel event support
* [60](https://github.com/BlueBrain/Deflect/pull/60):
  Improved DesktopStreamer: removed selection rectangle, editable list of stream
  hostnames

## Deflect 0.8

### 0.8.0 (04-11-2015)
* Added DeflectQt library to create QML applications which render offscreen and
  stream and receive events via Deflect
* Added deflect::Stream::sendSizeHints()
* Added MTQueue class, moved from DisplayCluster
* Fix [54](https://github.com/BlueBrain/Deflect/issues/54): ZeroConf discovery
  in DesktopStreamer uses hostname instead of instance name
* Fix interaction with DesktopStreamer on OSX where one click resulted in two
  clicks
* Fix [45](https://github.com/BlueBrain/Deflect/issues/45): Mouse position for
  DesktopStreamer is rendered correctly on Retina displays
* Fix: send compressed JPEG images from correct thread, might fix crashes/hangs
* Fix: ZeroConf record for server announces hostname instead of (wrong) port

## Deflect 0.7

### 0.7.2 (03-09-2015)
* Add 'About' dialog to learn current used version
* Fix bundle generation on OSX, broken after 0.6.1

### 0.7.1 (01-09-2015)
* Fixed rare crash or hang together with stream and interaction

### 0.7.0 (19-08-2015)
* Added EVT_TAP_AND_HOLD EventType

## Deflect 0.6

### 0.6.1 (13-07-2015)
* DesktopStreamer: Minor fixes for desktop interaction, only show checkbox on
  supported platforms (OSX)
* DesktopStreamer OSX: zip the application for Puppet deployment

### 0.6.0 (28-06-2015)
* Many classes of the Server part have been renamed
* Various API fixes
* Project-wide codestyle cleanup

## Deflect 0.5

### 0.5.1 (12-05-2015)
* Streamers can register for events immediately, without waiting for the
  DisplayCluster host to have opened a window
* Desktopstreamer supports autodiscovery of DisplayCluster hosts using Zeroconf
* Add interaction from the Wall with the DesktopStreamer host (Mac OS X
  only)

## Deflect 0.4

### 0.4.1 (24-04-2015)
* DesktopStreamer properly handles AppNap on OSX 10.9.
* DesktopStreamer detects Retina displays automatically (no retina checkbox).

### 0.4.0 (07-01-2015)
* First release, based on the DisplayCluster 0.4 dcStream library
