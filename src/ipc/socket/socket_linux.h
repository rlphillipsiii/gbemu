#ifndef _SOCKET_LINK_H
#define _SOCKET_LINK_H

#include <atomic>
#include <thread>
#include <list>
#include <mutex>

#include "ipcinterface.h"

class SocketConnection final : public IpcInterface {
public:
    SocketConnection();
    ~SocketConnection();

    void stop() override;

    void connect();

private:
    static constexpr int PORT = 1148;

    static constexpr int INVALID_FD = -1;
    static constexpr int IO_ERROR   = -1;

    int m_socket;
    int m_connection;

    void initServer(int port);
    void initClient(int port);

    void executeServer(int socket);
    void executeClient(int socket, int port);

    void stop(std::thread & t);

    bool rd(uint8_t & data) override;
    bool wr(uint8_t data) override;

    int peek() override;
};

#endif
