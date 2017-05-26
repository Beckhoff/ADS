/**
   Copyright (c) 2015 Beckhoff Automation GmbH & Co. KG

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

#ifndef _AMSCONNECTION_H_
#define _AMSCONNECTION_H_

#include "AmsPort.h"
#include "Sockets.h"
#include "Router.h"

#include <atomic>

struct AmsRequest {
    Frame frame;
    const AmsAddr& destAddr;
    uint16_t port;
    uint16_t cmdId;
    uint32_t bufferLength;
    void* buffer;
    uint32_t* bytesRead;

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
};

struct AmsResponse {
    Frame frame;
    std::atomic<uint32_t> invokeId;
    uint32_t errorCode;

    AmsResponse();
    void Notify();

    // return true if notified before timeout
    bool Wait(uint32_t timeout_ms);
private:
    std::mutex mutex;
    std::condition_variable cv;
};

struct AmsConnection : AmsProxy {
    AmsConnection(Router& __router, IpV4 destIp = IpV4 { "" });
    ~AmsConnection();

    NotifyMapping CreateNotifyMapping(uint32_t hNotify, std::shared_ptr<Notification> notification);
    long DeleteNotification(const AmsAddr& amsAddr, uint32_t hNotify, uint32_t tmms, uint16_t port);

    template<class T> long AdsRequest(AmsRequest& request, uint32_t tmms)
    {
        AmsAddr srcAddr;
        const auto status = router.GetLocalAddress(request.port, &srcAddr);
        if (status) {
            return status;
        }
        AmsResponse* response = Write(request.frame, request.destAddr, srcAddr, request.cmdId);
        if (response) {
            if (response->Wait(tmms)) {
                const uint32_t bytesAvailable = std::min<uint32_t>(request.bufferLength,
                                                                   response->frame.size() - sizeof(T));
                T header(response->frame.data());
                memcpy(request.buffer, response->frame.data() + sizeof(T), bytesAvailable);
                if (request.bytesRead) {
                    *request.bytesRead = bytesAvailable;
                }
                const auto errorCode = response->errorCode;
                Release(response);
                return errorCode ? errorCode : header.result();
            }
            Release(response);
            return ADSERR_CLIENT_SYNCTIMEOUT;
        }
        return -1;
    }

private:
    friend struct AmsRouter;
    Router& router;
    TcpSocket socket;
    std::thread receiver;
    std::atomic<size_t> refCount;
    std::atomic<uint32_t> invokeId;
    std::array<AmsResponse, Router::NUM_PORTS_MAX> queue;

    Frame& ReceiveFrame(Frame& frame, size_t length) const;
    bool ReceiveNotification(const AoEHeader& header);
    void ReceiveJunk(size_t bytesToRead) const;
    void Receive(void* buffer, size_t bytesToRead) const;
    template<class T> void Receive(T& buffer) const { Receive(&buffer, sizeof(T)); }
    AmsResponse* Write(Frame& request, const AmsAddr dest, const AmsAddr srcAddr, uint16_t cmdId);

    void Recv();
    void TryRecv();
    uint32_t GetInvokeId();
    void Release(AmsResponse* response);
    AmsResponse* Reserve(uint32_t id, uint16_t port);
    AmsResponse* GetPending(uint32_t id, uint16_t port);

    std::map<VirtualConnection, std::shared_ptr<NotificationDispatcher> > dispatcherList;
    std::recursive_mutex dispatcherListMutex;
    std::shared_ptr<NotificationDispatcher> DispatcherListAdd(const VirtualConnection& connection);
    std::shared_ptr<NotificationDispatcher> DispatcherListGet(const VirtualConnection& connection);

public:
    const IpV4 destIp;
    const uint32_t ownIp;
};

#endif /* #ifndef _AMSCONNECTION_H_ */
