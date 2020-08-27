// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 -2020 Beckhoff Automation GmbH & Co. KG
   Author: Patrick Bruenn <p.bruenn@beckhoff.com>
 */

#pragma once

#include <condition_variable>
#include <mutex>

struct Semaphore {
    void release()
    {
        std::unique_lock<std::mutex> lock(mutex);
        ++count;
        cv.notify_one();
    }

    void acquire()
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&](){return count > 0; });
        --count;
    }

private:
    int count = 0;
    std::mutex mutex;
    std::condition_variable cv;
};
