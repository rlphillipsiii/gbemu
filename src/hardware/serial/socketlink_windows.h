#ifndef _SOCKET_LINK_H
#define _SOCKET_LINK_H

#include <atomic>
#include <thread>
#include <list>
#include <mutex>

#include "consolelink.h"

class MemoryController;

class SocketLink final : public ConsoleLink {
public:
    SocketLink(MemoryController & memory);
    ~SocketLink() { stop(); }

    void stop() override;

private:
    bool rd(uint8_t&) override { return false; }
    bool wr(uint8_t) override { return false; }

    int peek() override { return 0; }
};

#endif
