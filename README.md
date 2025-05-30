This library is intended to provide easy use as ADS client applications running on non-windows systems (e.g. FreeBSD, Linux, ...) to communicate with TwinCAT devices via TCP/IP.

To build this library a recent compiler with C++14 support is required.

Currently (2024-12-13) tested with:
===================================

host os    | host  | target| compiler
-----------|-------|-------|-------------
Alpine 3.21| amd64 | amd64 | gcc 14.2.0
Arch Linux | amd64 | amd64 | clang 18.1.8
Arch Linux | amd64 | amd64 | gcc 14.2.1
Debian 12  | amd64 | amd64 | clang 14.0.6
Debian 12  | amd64 | amd64 | gcc 12.2.0-14
Debian 12  | amd64 | i686  | gcc 12.2.0-14
Debian 12  | amd64 | mips  | gcc 12.2.0-14
Debian 12  | amd64 | win32 | gcc 10.2.1-6
Debian 12  | amd64 |riscv64| gcc 12.2.0-13
Debian 12  | arm64 | arm64 | gcc 12.2.0-14
TC/BSD 14  | amd64 | amd64 | clang 18.1.5
Windows 10 | amd64 | win64 | msvc 19.36.33134

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
      Beckhoff IPC’s provide a web interface for diagnose and configuration. Further information: http://infosys.beckhoff.de/content/1033/devicemanager/index.html

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
## Without TwinCAT on the client
Use this option for system with no TwinCAT 3.1 installtion:
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
## On TC/BSD with TwinCAT installed, you can use the local TwinCAT router
```shell
# Install toolchain
doas install os-generic-userland-devtools
# configure meson to build example into "build" dir
meson example/build example --native-file meson.native.tcbsd
# let ninja build the example
ninja -C example/build
# and run the example
./example/build/example
```
## On Beckhoff RT Linux  with TwinCAT installed, you can use the local TwinCAT router
```shell
# configure meson to build example into "build" dir
meson example/build example --native-file meson.native.tclur
# let ninja build the example
ninja -C example/build
# and run the example
./example/build/example
```

Note: the name of the binaries which use the TwinCAT ADS Router have a different naming scheme

binary     | standalone | use TwinCAT Router
-----------|------------|-------------------
AdsLib     |     x      | 
TcAdsLib   |            | x 
adstool    |     x      | 
tcadstool  |            | x 
example    |     x      | 
tcexample  |            | x 
---
ADS/AMS Specification: https://infosys.beckhoff.com/content/1033/tc3_ads_intro/index.html
