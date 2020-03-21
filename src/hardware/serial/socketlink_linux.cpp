#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ioctl.h>

#include <cstring>
#include <thread>
#include <mutex>
#include <list>
#include <chrono>

#include "socketlink_linux.h"
#include "memorycontroller.h"
#include "interrupt.h"
#include "logging.h"
#include "configuration.h"
#include "memmap.h"

using std::thread;
using std::lock_guard;
using std::mutex;
using std::list;
using std::unique_lock;

SocketLink::SocketLink(MemoryController & memory)
    : ConsoleLink(memory)
{
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_FD == m_socket) {
        ERROR("%s\n", "Failed to open socket");
        return;
    }

    int port = Configuration::getInt(ConfigKey::LINK_PORT);

    NOTE("Socket server config: %s\n", (m_master) ? "master" : "slave");

    if (m_master) { initServer(port); }
    else          { initClient(port); }
}

void SocketLink::stop()
{
    if (INVALID_FD != m_socket) {
        close(m_socket);
    }

    stop(m_server);
    stop(m_client);

    ConsoleLink::stop();
}

void SocketLink::stop(thread & t)
{
    if (!t.joinable()) { return; }

    pthread_kill(t.native_handle(), SIGINT);
}

void SocketLink::initServer(int port)
{
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        ERROR("Failed to bind socket to port %d\n", port);
        return;
    }

    if (listen(m_socket, 1) != 0) {
        ERROR("%s\n", "Failed to listen on bound socket");
        return;
    }

    NOTE("Starting server thread on port %d (0x%04x)\n", port, htons(port));

    m_server = thread([&, port] { executeServer(m_socket); });
}

void SocketLink::executeServer(int socket)
{
    while (!m_interrupt.load()) {
        struct sockaddr_in cli;
        socklen_t length = sizeof(cli);

        NOTE("%s\n", "Waiting for client connection");

        m_connection = accept(socket, (struct sockaddr*)&cli, &length);
        if (INVALID_FD == m_connection) {
            ERROR("Accept failed with errno = %s\n", std::strerror(errno));
            continue;
        }

        NOTE("Server connection established: %d\n", m_connection);

        m_connected = true;
        while (m_connected) {
            m_connected = serverLoop();
        }

        NOTE("%s\n", "Server connection closed");

        close(m_connection);
    }

    NOTE("%s\n", "Server thread complete");
}

void SocketLink::initClient(int port)
{
    m_client = thread([&, port] { executeClient(m_socket, port); });
}

void SocketLink::executeClient(int socket, int port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    struct hostent *info = gethostbyname("127.0.0.1");
    if (!info) {
        ERROR("%s\n", "Failed convert host IP");
        return;
    }

    std::memcpy((void*)&addr.sin_addr.s_addr, (void*)info->h_addr, info->h_length);

    NOTE("Starting client thread on port %d (0x%04x)\n", port, htons(port));

    m_connection = socket;
    while (!m_interrupt.load()) {
        if (connect(socket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
            continue;
        }

        NOTE("Client connection established: %d\n", socket);

        m_connected = true;
        while (m_connected) {
            m_connected = serverLoop();
        }

        NOTE("%s\n", "Client connection closed");
    }

    NOTE("%s\n", "Client thread complete");
}

bool SocketLink::rd(uint8_t & data)
{
    return (read(m_connection, (void*)&data, 1) == 1);
}

bool SocketLink::wr(uint8_t data)
{
    return (write(m_connection, (void*)&data, 1) == 1);
}

int SocketLink::peek()
{
    int count;
    ioctl(m_connection, FIONREAD, &count);

    return count;
}
