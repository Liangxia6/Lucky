#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "TimeStamp.h"


class EventLoop {

public:
    EventLoop();
    void UpdateinEventLoop(Channel *);
    void RemoveinEventLoop();

};