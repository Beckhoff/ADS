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

    NotificationId CreateNotifyMapping(uint32_t hNotify, Notification& notification);
    long DeleteNotification(const AmsAddr& amsAddr, uint32_t hNotify, uint32_t tmms, uint16_t port);

    AmsResponse* Write(Frame& request, const AmsAddr dest, const AmsAddr srcAddr, uint16_t cmdId);
    void Release(AmsResponse* response);
    AmsResponse* GetPending(uint32_t id, uint16_t port);

    template<class T>
    long AdsRequest(Frame&         request,
                    const AmsAddr& destAddr,
                    uint32_t       tmms,
                    uint16_t       port,
                    uint16_t       cmdId,
                    uint32_t       bufferLength = 0,
                    void*          buffer = nullptr,
                    uint32_t*      bytesRead = nullptr)
    {
        AmsAddr srcAddr;
        const auto status = router.GetLocalAddress(port, &srcAddr);
        if (status) {
            return status;
        }
        AmsResponse* response = Write(request, destAddr, srcAddr, cmdId);
        if (response) {
            if (response->Wait(tmms)) {
                const uint32_t bytesAvailable = std::min<uint32_t>(bufferLength, response->frame.size() - sizeof(T));
                T header(response->frame.data());
                memcpy(buffer, response->frame.data() + sizeof(T), bytesAvailable);
                if (bytesRead) {
                    *bytesRead = bytesAvailable;
                }
                Release(response);
                return header.result();
            }
            Release(response);
            return ADSERR_CLIENT_SYNCTIMEOUT;
        }
        return -1;
    }

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
                Release(response);
                return header.result();
            }
            Release(response);
            return ADSERR_CLIENT_SYNCTIMEOUT;
        }
        return -1;
    }

    const IpV4 destIp;
private:
    Router& router;
    TcpSocket socket;
    std::thread receiver;
    std::atomic<uint32_t> invokeId;
    std::array<AmsResponse, Router::NUM_PORTS_MAX> queue;

    Frame& ReceiveFrame(Frame& frame, size_t length) const;
    bool ReceiveNotification(const AoEHeader& header);
    void ReceiveJunk(size_t bytesToRead) const;
    void Receive(void* buffer, size_t bytesToRead) const;
    template<class T> void Receive(T& buffer) const { Receive(&buffer, sizeof(T)); }

    void Recv();
    void TryRecv();
    uint32_t GetInvokeId();
    AmsResponse* Reserve(uint32_t id, uint16_t port);

    struct DispatcherList {
        std::shared_ptr<NotificationDispatcher> Add(const VirtualConnection& connection, AmsProxy& proxy);
        std::shared_ptr<NotificationDispatcher> Get(const VirtualConnection& connection);

private:
        std::map<VirtualConnection, std::shared_ptr<NotificationDispatcher> > list;
        std::recursive_mutex mutex;
    } dispatcherList;
};

#endif /* #ifndef _AMSCONNECTION_H_ */
