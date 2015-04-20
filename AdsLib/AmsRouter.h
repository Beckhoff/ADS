#ifndef _AMS_ROUTER_H_
#define _AMS_ROUTER_H_

#include "AmsConnection.h"

struct AmsRouter : Router {
    AmsRouter(AmsNetId netId = AmsNetId {});

    uint16_t OpenPort();
    long ClosePort(uint16_t port);
    long GetLocalAddress(uint16_t port, AmsAddr* pAddr);
    long GetTimeout(uint16_t port, uint32_t& timeout);
    long SetTimeout(uint16_t port, uint32_t timeout);
    long AddNotification(uint16_t                     port,
                         const AmsAddr*               pAddr,
                         uint32_t                     indexGroup,
                         uint32_t                     indexOffset,
                         const AdsNotificationAttrib* pAttrib,
                         PAdsNotificationFuncEx       pFunc,
                         uint32_t                     hUser,
                         uint32_t*                    pNotification);
    long DelNotification(uint16_t port, const AmsAddr* pAddr, uint32_t hNotification);

    long AddRoute(AmsNetId ams, const IpV4& ip);
    void DelRoute(const AmsNetId& ams);
    void SetNetId(AmsNetId ams);
    AmsConnection* GetConnection(const AmsNetId& pAddr);

    template<class T> long AdsRequest(AmsRequest& request)
    {
        if (request.bytesRead) {
            *request.bytesRead = 0;
        }

        auto ads = GetConnection(request.destAddr.netId);
        if (!ads) {
            return GLOBALERR_MISSING_ROUTE;
        }
        return ads->AdsRequest<T>(request, ports[request.port - Router::PORT_BASE].tmms);
    }

private:
    AmsNetId localAddr;
    std::recursive_mutex mutex;
    std::map<IpV4, std::unique_ptr<AmsConnection> > connections;
    std::map<AmsNetId, AmsConnection*> mapping;

    std::map<IpV4, std::unique_ptr<AmsConnection> >::iterator __GetConnection(const AmsNetId& pAddr);
    void DeleteIfLastConnection(const AmsConnection* conn);
    void Recv();

    std::array<AmsPort, NUM_PORTS_MAX> ports;
};
#endif /* #ifndef _AMS_ROUTER_H_ */
