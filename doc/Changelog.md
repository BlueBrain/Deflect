Changelog {#Changelog}
============

## Deflect 0.11

### 0.11.0 (git master)

* [102](https://github.com/BlueBrain/Deflect/pull/102):
  DeflectQt: Continue rendering & streaming after updates for a while to
  compensate for running animations, fix spurious missing event handling  
* [100](https://github.com/BlueBrain/Deflect/pull/100):
  QmlStreamer: correcly quit application when stream is closed.
* [99](https://github.com/BlueBrain/Deflect/pull/99):
  Fix incomplete socket send under certain timing conditions
* [98](https://github.com/BlueBrain/Deflect/pull/98):
  Streams can be constructed based on the DEFLECT_ID and DEFLECT_HOST ENV_VARs.
* [95](https://github.com/BlueBrain/Deflect/pull/95):
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
