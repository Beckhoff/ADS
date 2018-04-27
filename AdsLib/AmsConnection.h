/**
   Copyright (c) 2015 - 2018 Beckhoff Automation GmbH & Co. KG

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
 */

#pragma once

#include "AmsPort.h"
#include "Sockets.h"
#include "Router.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <thread>

using Timepoint = std::chrono::steady_clock::time_point;
#define WAITING_FOR_RESPONSE ((uint32_t)0xFFFFFFFF)

struct AmsRequest {
    Frame frame;
    const AmsAddr& destAddr;
    uint16_t port;
    uint16_t cmdId;
    uint32_t bufferLength;
    void* buffer;
    uint32_t* bytesRead;
    Timepoint deadline;

    AmsRequest(const AmsAddr& ams,
               uint16_t       __port,
               uint16_t       __cmdId,
               uint32_t       __bufferLength = 0,
               void*          __buffer = nullptr,
               uint32_t*      __bytesRead = nullptr,
               size_t         payloadLength = 0)
        : frame(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + payloadLength),
        destAddr(ams),
        port(__port),
        cmdId(__cmdId),
        bufferLength(__bufferLength),
        buffer(__buffer),
        bytesRead(__bytesRead)
    {}

    void SetDeadline(uint32_t tmms)
    {
        deadline = std::chrono::steady_clock::now();
        deadline += std::chrono::milliseconds(tmms);
    }
};

struct AmsResponse {
    std::atomic<AmsRequest*> request;
    std::atomic<uint32_t> invokeId;

    AmsResponse();
    void Notify(uint32_t error);
    void Release();

    // wait for response or timeout and return received errorCode or ADSERR_CLIENT_SYNCTIMEOUT
    uint32_t Wait();

private:
    std::mutex mutex;
    std::condition_variable cv;
    uint32_t errorCode;
};

struct AmsConnection {
    AmsConnection(Router& __router, IpV4 destIp = IpV4 { "" });
    ~AmsConnection();

    SharedDispatcher CreateNotifyMapping(uint32_t hNotify, std::shared_ptr<Notification> notification);
    long DeleteNotification(const AmsAddr& amsAddr, uint32_t hNotify, uint32_t tmms, uint16_t port);
    long AdsRequest(AmsRequest& request, uint32_t timeout);

private:
    friend struct AmsRouter;
    Router& router;
    TcpSocket socket;
    std::thread receiver;
    std::atomic<size_t> refCount;
    std::atomic<uint32_t> invokeId;
    std::array<AmsResponse, Router::NUM_PORTS_MAX> queue;

    template<class T> void ReceiveFrame(AmsResponse* response, size_t length, uint32_t aoeError) const;
    bool ReceiveNotification(const AoEHeader& header);
    void ReceiveJunk(size_t bytesToRead) const;
    void Receive(void* buffer, size_t bytesToRead, timeval* timeout = nullptr) const;
    void Receive(void* buffer, size_t bytesToRead, const Timepoint& deadline) const;
    template<class T> void Receive(T& buffer) const { Receive(&buffer, sizeof(T)); }
    AmsResponse* Write(AmsRequest& request, const AmsAddr srcAddr);
    void Recv();
    void TryRecv();
    uint32_t GetInvokeId();
    AmsResponse* Reserve(AmsRequest* request, uint16_t port);
    AmsResponse* GetPending(uint32_t id, uint16_t port);

    std::map<VirtualConnection, SharedDispatcher> dispatcherList;
    std::recursive_mutex dispatcherListMutex;
    SharedDispatcher DispatcherListAdd(const VirtualConnection& connection);
    SharedDispatcher DispatcherListGet(const VirtualConnection& connection);

public:
    const IpV4 destIp;
    const uint32_t ownIp;
};
