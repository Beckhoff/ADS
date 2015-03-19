///////////////////////////////////////////////////////////////////////////////
// This is a part of the Beckhoff TwinCAT ADS API
// Copyright (C) Beckhoff Automation GmbH & Co. KG
// All rights reserved.
////////////////////////////////////////////////////////////////////////////////


#ifndef __ADSDEF_H__
#define __ADSDEF_H__

#include <stdint.h>

////////////////////////////////////////////////////////////////////////////////
// AMS Ports
#define	AMSPORT_LOGGER					100
#define	AMSPORT_R0_RTIME				200
#define	AMSPORT_R0_TRACE				(AMSPORT_R0_RTIME+90)
#define	AMSPORT_R0_IO					300
#define	AMSPORT_R0_SPS					400
#define	AMSPORT_R0_NC					500
#define	AMSPORT_R0_ISG					550
#define	AMSPORT_R0_PCS					600
#define	AMSPORT_R0_PLC					801
#define	AMSPORT_R0_PLC_RTS1				801
#define	AMSPORT_R0_PLC_RTS2				811
#define	AMSPORT_R0_PLC_RTS3				821
#define	AMSPORT_R0_PLC_RTS4				831
#define	AMSPORT_R0_PLC_TC3				851

////////////////////////////////////////////////////////////////////////////////
// ADS Cmd Ids
#define ADSSRVID_INVALID					0x00
#define ADSSRVID_READDEVICEINFO				0x01
#define ADSSRVID_READ         				0x02
#define ADSSRVID_WRITE        				0x03
#define ADSSRVID_READSTATE    				0x04
#define ADSSRVID_WRITECTRL    				0x05
#define ADSSRVID_ADDDEVICENOTE  			0x06
#define ADSSRVID_DELDEVICENOTE  			0x07
#define ADSSRVID_DEVICENOTE					0x08
#define ADSSRVID_READWRITE      			0x09

////////////////////////////////////////////////////////////////////////////////
// ADS reserved index groups
#define ADSIGRP_SYMTAB						0xF000
#define ADSIGRP_SYMNAME						0xF001
#define ADSIGRP_SYMVAL						0xF002

#define ADSIGRP_SYM_HNDBYNAME				0xF003
#define ADSIGRP_SYM_VALBYNAME				0xF004
#define ADSIGRP_SYM_VALBYHND				0xF005
#define ADSIGRP_SYM_RELEASEHND				0xF006
#define ADSIGRP_SYM_INFOBYNAME				0xF007
#define ADSIGRP_SYM_VERSION					0xF008
#define ADSIGRP_SYM_INFOBYNAMEEX			0xF009

#define ADSIGRP_SYM_DOWNLOAD				0xF00A
#define ADSIGRP_SYM_UPLOAD					0xF00B
#define ADSIGRP_SYM_UPLOADINFO				0xF00C
#define ADSIGRP_SYM_DOWNLOAD2				0xF00D
#define ADSIGRP_SYM_DT_UPLOAD				0xF00E
#define ADSIGRP_SYM_UPLOADINFO2				0xF00F

#define ADSIGRP_SYMNOTE						0xF010		// notification of named handle

#define ADSIGRP_SUMUP_READ					0xF080		// AdsRW  IOffs list size or 0 (=0 -> list size == WLength/3*sizeof(ULONG))
																	// W: {list of IGrp, IOffs, Length} 
																	// if IOffs != 0 then R: {list of results} and {list of data}
																	// if IOffs == 0 then R: only data (sum result)
#define ADSIGRP_SUMUP_WRITE					0xF081		// AdsRW  IOffs list size
																	// W: {list of IGrp, IOffs, Length} followed by {list of data} 
																	// R: list of results
#define ADSIGRP_SUMUP_READWRITE				0xF082		// AdsRW  IOffs list size 
																	// W: {list of IGrp, IOffs, RLength, WLength} followed by {list of data} 
																	// R: {list of results, RLength} followed by {list of data}
#define ADSIGRP_SUMUP_READEX				0xF083		// AdsRW  IOffs list size 
																	// W: {list of IGrp, IOffs, Length} 
#define ADSIGRP_SUMUP_READEX2				0xF084		// AdsRW  IOffs list size 
																	// W: {list of IGrp, IOffs, Length} 
																	// R: {list of results, Length} followed by {list of data (returned lengths)}
#define ADSIGRP_SUMUP_ADDDEVNOTE			0xF085		// AdsRW  IOffs list size
																	// W: {list of IGrp, IOffs, Attrib} 
																	// R: {list of results, handles}
#define ADSIGRP_SUMUP_DELDEVNOTE			0xF086		// AdsRW  IOffs list size
																	// W: {list of handles} 
																	// R: {list of results, Length} followed by {list of data}

#define ADSIGRP_IOIMAGE_RWIB				0xF020		// read/write input byte(s)
#define ADSIGRP_IOIMAGE_RWIX				0xF021		// read/write input bit
#define ADSIGRP_IOIMAGE_RISIZE				0xF025		// read input size (in byte)
#define ADSIGRP_IOIMAGE_RWOB				0xF030		// read/write output byte(s)
#define ADSIGRP_IOIMAGE_RWOX				0xF031		// read/write output bit
#define ADSIGRP_IOIMAGE_CLEARI				0xF040		// write inputs to null
#define ADSIGRP_IOIMAGE_CLEARO				0xF050		// write outputs to null
#define ADSIGRP_IOIMAGE_RWIOB				0xF060		// read input and write output byte(s)

#define ADSIGRP_DEVICE_DATA					0xF100		// state, name, etc...
#define ADSIOFFS_DEVDATA_ADSSTATE			0x0000		// ads state of device
#define ADSIOFFS_DEVDATA_DEVSTATE			0x0002		// device state


////////////////////////////////////////////////////////////////////////////////
// Global Return codes
#define	ERR_GLOBAL							0x0000

#define GLOBALERR_NO_MEMORY					(0x19+ERR_GLOBAL)
#define GLOBALERR_TCP_SEND					(0x1A+ERR_GLOBAL)


////////////////////////////////////////////////////////////////////////////////
// Router Return codes
#define	ERR_ROUTER							0x0500

#define ROUTERERR_NOTREGISTERED				(0x07+ERR_ROUTER)
#define ROUTERERR_NOMOREQUEUES				(0x08+ERR_ROUTER)


////////////////////////////////////////////////////////////////////////////////
// ADS Return codes
#define ADSERR_NOERR						0x00
#define	ERR_ADSERRS							0x0700

#define ADSERR_DEVICE_ERROR					(0x00+ERR_ADSERRS) // Error class < device error >
#define ADSERR_DEVICE_SRVNOTSUPP			(0x01+ERR_ADSERRS) // Service is not supported by server
#define ADSERR_DEVICE_INVALIDGRP 			(0x02+ERR_ADSERRS) // invalid indexGroup
#define ADSERR_DEVICE_INVALIDOFFSET			(0x03+ERR_ADSERRS) // invalid indexOffset
#define ADSERR_DEVICE_INVALIDACCESS			(0x04+ERR_ADSERRS) // reading/writing not permitted
#define ADSERR_DEVICE_INVALIDSIZE			(0x05+ERR_ADSERRS) // parameter size not correct
#define ADSERR_DEVICE_INVALIDDATA			(0x06+ERR_ADSERRS) // invalid parameter value(s)
#define ADSERR_DEVICE_NOTREADY				(0x07+ERR_ADSERRS) // device is not in a ready state
#define ADSERR_DEVICE_BUSY					(0x08+ERR_ADSERRS) // device is busy
#define ADSERR_DEVICE_INVALIDCONTEXT		(0x09+ERR_ADSERRS) // invalid context (must be InWindows)
#define ADSERR_DEVICE_NOMEMORY				(0x0A+ERR_ADSERRS) // out of memory
#define ADSERR_DEVICE_INVALIDPARM			(0x0B+ERR_ADSERRS) // invalid parameter value(s)
#define ADSERR_DEVICE_NOTFOUND				(0x0C+ERR_ADSERRS) // not found (files, ...)
#define ADSERR_DEVICE_SYNTAX				(0x0D+ERR_ADSERRS) // syntax error in comand or file
#define ADSERR_DEVICE_INCOMPATIBLE			(0x0E+ERR_ADSERRS) // objects do not match
#define ADSERR_DEVICE_EXISTS				(0x0F+ERR_ADSERRS) // object already exists
#define ADSERR_DEVICE_SYMBOLNOTFOUND		(0x10+ERR_ADSERRS) // symbol not found
#define ADSERR_DEVICE_SYMBOLVERSIONINVALID	(0x11+ERR_ADSERRS) // symbol version invalid
#define ADSERR_DEVICE_INVALIDSTATE			(0x12+ERR_ADSERRS) // server is in invalid state
#define ADSERR_DEVICE_TRANSMODENOTSUPP		(0x13+ERR_ADSERRS) // AdsTransMode not supported
#define ADSERR_DEVICE_NOTIFYHNDINVALID		(0x14+ERR_ADSERRS) // Notification handle is invalid
#define ADSERR_DEVICE_CLIENTUNKNOWN			(0x15+ERR_ADSERRS) // Notification client not registered	
#define ADSERR_DEVICE_NOMOREHDLS			(0x16+ERR_ADSERRS) // no more notification handles	
#define ADSERR_DEVICE_INVALIDWATCHSIZE		(0x17+ERR_ADSERRS) // size for watch to big	
#define ADSERR_DEVICE_NOTINIT				(0x18+ERR_ADSERRS) // device not initialized	
#define ADSERR_DEVICE_TIMEOUT				(0x19+ERR_ADSERRS) // device has a timeout
#define ADSERR_DEVICE_NOINTERFACE			(0x1A+ERR_ADSERRS) // query interface failed
#define ADSERR_DEVICE_INVALIDINTERFACE		(0x1B+ERR_ADSERRS) // wrong interface required
#define ADSERR_DEVICE_INVALIDCLSID			(0x1C+ERR_ADSERRS) // class ID is invalid
#define ADSERR_DEVICE_INVALIDOBJID			(0x1D+ERR_ADSERRS) // object ID is invalid
#define ADSERR_DEVICE_PENDING				(0x1E+ERR_ADSERRS) // request is pending
#define ADSERR_DEVICE_ABORTED				(0x1F+ERR_ADSERRS) // request is aborted
#define ADSERR_DEVICE_WARNING				(0x20+ERR_ADSERRS) // signal warning
#define ADSERR_DEVICE_INVALIDARRAYIDX		(0x21+ERR_ADSERRS) // invalid array index
#define ADSERR_DEVICE_SYMBOLNOTACTIVE		(0x22+ERR_ADSERRS) // symbol not active -> release handle and try again
#define ADSERR_DEVICE_ACCESSDENIED			(0x23+ERR_ADSERRS) // access denied
#define ADSERR_DEVICE_LICENSENOTFOUND		(0x24+ERR_ADSERRS) // no license found
#define ADSERR_DEVICE_LICENSEEXPIRED		(0x25+ERR_ADSERRS) // license expired
#define ADSERR_DEVICE_LICENSEEXCEEDED		(0x26+ERR_ADSERRS) // license exceeded
#define ADSERR_DEVICE_LICENSEINVALID		(0x27+ERR_ADSERRS) // license invalid 
#define ADSERR_DEVICE_LICENSESYSTEMID		(0x28+ERR_ADSERRS) // license invalid system id
#define ADSERR_DEVICE_LICENSENOTIMELIMIT	(0x29+ERR_ADSERRS) // license not time limited
#define ADSERR_DEVICE_LICENSEFUTUREISSUE	(0x2A+ERR_ADSERRS) // license issue time in the future
#define ADSERR_DEVICE_LICENSETIMETOLONG		(0x2B+ERR_ADSERRS) // license time period to long
#define ADSERR_DEVICE_EXCEPTION				(0x2C+ERR_ADSERRS) // exception in device specific code
#define ADSERR_DEVICE_LICENSEDUPLICATED		(0x2D+ERR_ADSERRS) // license file read twice
#define ADSERR_DEVICE_SIGNATUREINVALID		(0x2E+ERR_ADSERRS) // invalid signature
#define ADSERR_DEVICE_CERTIFICATEINVALID	(0x2F+ERR_ADSERRS) // public key certificate
//
#define ADSERR_CLIENT_ERROR					(0x40+ERR_ADSERRS) // Error class < client error >
#define ADSERR_CLIENT_INVALIDPARM			(0x41+ERR_ADSERRS) // invalid parameter at service call
#define ADSERR_CLIENT_LISTEMPTY				(0x42+ERR_ADSERRS) // polling list	is empty
#define ADSERR_CLIENT_VARUSED				(0x43+ERR_ADSERRS) // var connection already in use
#define ADSERR_CLIENT_DUPLINVOKEID			(0x44+ERR_ADSERRS) // invoke id in use
#define ADSERR_CLIENT_SYNCTIMEOUT			(0x45+ERR_ADSERRS) // timeout elapsed
#define ADSERR_CLIENT_W32ERROR				(0x46+ERR_ADSERRS) // error in win32 subsystem
#define ADSERR_CLIENT_TIMEOUTINVALID		(0x47+ERR_ADSERRS) // ?
#define ADSERR_CLIENT_PORTNOTOPEN			(0x48+ERR_ADSERRS) // ads dll
#define ADSERR_CLIENT_NOAMSADDR				(0x49+ERR_ADSERRS) // ads dll 
#define ADSERR_CLIENT_SYNCINTERNAL			(0x50+ERR_ADSERRS) // internal error in ads sync 
#define ADSERR_CLIENT_ADDHASH				(0x51+ERR_ADSERRS) // hash table overflow 
#define ADSERR_CLIENT_REMOVEHASH			(0x52+ERR_ADSERRS) // key not found in hash table 
#define ADSERR_CLIENT_NOMORESYM				(0x53+ERR_ADSERRS) // no more symbols in cache 
#define ADSERR_CLIENT_SYNCRESINVALID		(0x54+ERR_ADSERRS) // invalid response received 
#define ADSERR_CLIENT_SYNCPORTLOCKED		(0x55+ERR_ADSERRS) // sync port is locked 

#pragma	pack( push, 1)
typedef struct AmsNetId_
{
	uint8_t b[6];
#ifdef __cplusplus
	bool operator <(const AmsNetId_& ref) const
	{
		for (int i = 0; i < sizeof(ref.b); ++i) {
			if (b[i] != ref.b[i]) {
				return b[i] < ref.b[i];
			}
		}
		return false;
	}
#endif
} AmsNetId, *PAmsNetId;

#include <cstring> // for memcmp()
typedef struct AmsAddr_
{
	AmsNetId netId;
	uint16_t port;
#ifdef __cplusplus
	bool operator <(const AmsAddr_& ref) const
	{
		if (memcmp(&netId, &ref.netId, sizeof(netId))) {
			return netId < ref.netId;
		}
		return port < ref.port;
	}
#endif
} AmsAddr, *PAmsAddr;

typedef	struct
{
	uint8_t version;
	uint8_t revision;
	uint16_t build;
} AdsVersion;

typedef enum nAdsTransMode
{
	ADSTRANS_NOTRANS = 0,
	ADSTRANS_CLIENTCYCLE = 1,
	ADSTRANS_CLIENTONCHA = 2,
	ADSTRANS_SERVERCYCLE = 3,
	ADSTRANS_SERVERONCHA = 4,
}ADSTRANSMODE;

typedef enum nAdsState
{
	ADSSTATE_INVALID = 0,
	ADSSTATE_IDLE = 1,
	ADSSTATE_RESET = 2,
	ADSSTATE_INIT = 3,
	ADSSTATE_START = 4,
	ADSSTATE_RUN = 5,
	ADSSTATE_STOP = 6,
	ADSSTATE_SAVECFG = 7,
	ADSSTATE_LOADCFG = 8,
	ADSSTATE_POWERFAILURE = 9,
	ADSSTATE_POWERGOOD = 10,
	ADSSTATE_ERROR = 11,
	ADSSTATE_SHUTDOWN = 12,
	ADSSTATE_SUSPEND = 13,
	ADSSTATE_RESUME = 14,
	ADSSTATE_CONFIG = 15,
	ADSSTATE_RECONFIG = 16,
	ADSSTATE_STOPPING = 17,
	ADSSTATE_MAXSTATES
} ADSSTATE;

typedef struct
{
	uint32_t cbLength;
	uint32_t nTransMode;
	uint32_t nMaxDelay;
	union
	{
		uint32_t nCycleTime;
		uint32_t dwChangeFilter;
	};
} AdsNotificationAttrib, *PAdsNotificationAttrib;

typedef struct
{
	uint32_t hNotification;
	uint64_t nTimeStamp;
	uint32_t cbSampleSize;
	uint8_t data[];
} AdsNotificationHeader, *PAdsNotificationHeader;

typedef void(*PAdsNotificationFuncEx)(const AmsAddr* pAddr, const AdsNotificationHeader* pNotification, uint32_t hUser);

#pragma	pack( pop )
#endif	// __ADSDEF_H__
