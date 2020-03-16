#ifndef _SOCKET_LINK_H
#define _SOCKET_LINK_H

#include <atomic>
#include <thread>
#include <list>
#include <mutex>

#include "consolelink.h"

class MemoryController;

class SocketLink : public ConsoleLink {
public:
    SocketLink(MemoryController & memory);
    ~SocketLink();

    void start(uint8_t value) override;
    void check() override;

    void stop() override;

private:
    static constexpr int PORT = 1148;

    static constexpr int INVALID_FD = -1;
    static constexpr int IO_ERROR   = -1;

    int m_socket;

    std::atomic<bool> m_interrupt;
    std::atomic<bool> m_connected;

    std::mutex m_txLock;
    std::mutex m_rxLock;

    std::list<uint8_t> m_rxQueue;
    std::list<uint8_t> m_txQueue;

    std::thread m_server;
    std::thread m_client;

    void initServer(int port);
    void initClient(int port);

    void executeServer(int socket, int port);
    void executeClient(int socket, int port);

    void stop(std::thread & t);

    bool serverLoop(
        int conn,
        std::list<uint8_t> & tx,
        std::mutex & txLock,
        std::list<uint8_t> & rx,
        std::mutex & rxLock);
};

#endif
