This library is intended to provide easy use as ADS client applications running on non-windows systems (e.g. FreeBSD, Linux, ...) to communicate with TwinCAT devices via TCP/IP.

To build this library a recent compiler with C++14 support is required.

Currently (2023-01-26) tested with:
===================================

host (amd64)     | target| compiler
-----------------|-------|-------------
TC/BSD 13        | amd64 | clang 13.0.0
Alpine 3.16      | amd64 | gcc 11.2.1
Arch Linux       | amd64 | gcc 12.2.1
Debian Bullseye  | amd64 | clang 11.0.1
Debian Bullseye  | amd64 | gcc 10.2.1
Debian Bullseye  | i686  | gcc 10.2.1
Debian Bullseye  | mips  | gcc 10.2.1
Debian Bullseye  | win32 | gcc 5.5.0
Windows 10       | win64 | msvc 19.31.31107

Compile & usage
===============
```shell
# clone the repository
git clone https://github.com/Beckhoff/ADS.git
# change into root of the cloned repository
cd ADS
# configure meson to build the library into "build" dir
meson setup build
# let ninja build the library
ninja -C build
```

Prepare your target to run the example
======================================
- Download your PLC project to your target e.g. "PLC-TestProject" of our GitHub repository.
- Authorise your ADS client for the TwinCAT target by adding an AMS route.
    - TwinCAT Engineering:
      Go to the tree item SYSTEM/Routes and add a static route.

    - TwinCAT Systray:
      Open the context menue by right click the TwinCAT systray icon. (not available on Windows CE devices) 

    - TC2:
      Go to Properties/AMS Router/Remote Computers and restart TwinCAT
    - TC3:
      Go to  Router/Edit routes.
    - TcAmsRemoteMgr:
      Windows CE devices can be configured locally (TC2 requires a TwinCAT restart). Tool location: `/Hard Disk/System/TcAmsRemoteMgr.exe`
    - IPC Diagnose:
      Beckhoff IPC’s provide a web interface for diagnose and configuration. Further information: http://infosys.beckhoff.de/content/1033/devicemanager/index.html?id=286

Sample AMS route:
```
  Name:           MyAdsClient
  AMS Net Id:     192.168.0.1.1.1 # NetId of your ADS client, derived from its IP address or set by bhf::ads:SetLocalAdress().
  Address:        192.168.0.1     # Use the IP of the ADS client, which is connected to your TwinCAT target
  Transport Type: TCP/IP
  Remote Route:   None / Server
  Unidirectional: false
  Secure ADS:     false
```

Prepare your client to run the example/example.cpp
==================================================
- set "remoteNetId" to the AMS NetId of your TwinCAT target (this is the AMS NetId found in the "About TwinCAT System window" e.g. "192.168.0.2.1.1").
- set "remoteIpV4" to the IP Address of your TwinCAT target (e.g. 192.168.0.2)
- (optional) enable bhf::ads::SetLocalAddress() and set to the AMS NetId you choose for the ADS client (e.g. 192.168.0.1.1.1).

```shell
# configure meson to build example into "build" dir
meson example/build example
# let ninja build the example
ninja -C example/build
# and run the example
./example/build/example
```
---
ADS/AMS Specification: https://infosys.beckhoff.com/content/1033/tc3_ads_intro/index.html
