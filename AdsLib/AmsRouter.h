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

#ifndef _AMS_ROUTER_H_
#define _AMS_ROUTER_H_

#include "AmsConnection.h"

struct AmsRouter : Router {
    AmsRouter(AmsNetId netId = AmsNetId {});

    uint16_t OpenPort();
    long ClosePort(uint16_t port);
    long GetLocalAddress(uint16_t port, AmsAddr* pAddr);
    void SetLocalAddress(AmsNetId netId);
    long GetTimeout(uint16_t port, uint32_t& timeout);
    long SetTimeout(uint16_t port, uint32_t timeout);
    long AddNotification(AmsRequest& request, uint32_t* pNotification, std::shared_ptr<Notification> notify);
    long DelNotification(uint16_t port, const AmsAddr* pAddr, uint32_t hNotification);

    long AddRoute(AmsNetId ams, const IpV4& ip);
    void DelRoute(const AmsNetId& ams);
    AmsConnection* GetConnection(const AmsNetId& pAddr);
    long AdsRequest(AmsRequest& request);

private:
    AmsNetId localAddr;
    std::recursive_mutex mutex;
    std::map<IpV4, std::unique_ptr<AmsConnection> > connections;
    std::map<AmsNetId, AmsConnection*> mapping;

    std::map<IpV4, std::unique_ptr<AmsConnection> >::iterator __GetConnection(const AmsNetId& pAddr);
    void DeleteIfLastConnection(const AmsConnection* conn);

    std::array<AmsPort, NUM_PORTS_MAX> ports;
};
#endif /* #ifndef _AMS_ROUTER_H_ */
