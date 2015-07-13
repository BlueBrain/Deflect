Changelog {#Changelog}
============

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
