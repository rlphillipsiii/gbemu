#ifndef _SOCKET_LINK_H
#define _SOCKET_LINK_H

#include <atomic>
#include <thread>

#include "consolelink.h"

class MemoryController;

class SocketLink : public ConsoleLink {
public:
    SocketLink(MemoryController & memory);
    ~SocketLink();

    void transfer(uint8_t value) override;
    void check() override;

    void stop() override;

private:
    static constexpr int PORT = 1148;

    static constexpr int INVALID_FD = -1;
    static constexpr int IO_ERROR   = -1;

    int m_socket;

    std::atomic<bool> m_interrupt;
    std::atomic<bool> m_connected;

    std::atomic<uint8_t> m_rx;
    std::atomic<uint8_t> m_tx;

    std::thread m_server;
    std::thread m_client;

    void initServer(int port);
    void initClient(int port);

    void executeServer(int socket);
    void executeClient(int socket, int port);

    void txMaster(uint8_t value);
    void txSlave(uint8_t value);
};

#endif
