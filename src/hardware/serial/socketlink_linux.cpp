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

using namespace std::chrono_literals;

SocketLink::SocketLink(MemoryController & memory)
    : ConsoleLink(memory),
      m_interrupt(false),
      m_connected(false)
{
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_FD == m_socket) {
        ERROR("%s\n", "Failed to open socket");
        return;
    }

    Configuration & config = Configuration::instance();
    Configuration::Setting setting = config[ConfigKey::LINK_PORT];

    int port = (setting) ? setting->toInt() : PORT;

    NOTE("Socket server config: %s\n", (m_master) ? "master" : "slave");

    if (m_master) { initServer(port); }
    else          { initClient(port); }
}

SocketLink::~SocketLink()
{
    stop();
}

void SocketLink::stop()
{
    m_interrupt = true;

    if (INVALID_FD != m_socket) {
        close(m_socket);
    }

    stop(m_server);
    stop(m_client);
}

void SocketLink::stop(thread & t)
{
    if (!t.joinable()) { return; }

    pthread_kill(t.native_handle(), SIGINT);

    t.join();
}

void SocketLink::start(uint8_t value)
{
    if (m_connected) {
        lock_guard<mutex> guard(m_txLock);
        m_txQueue.push_back(value);

        //std::this_thread::sleep_for(20ms);
    } else {
        m_memory.write(SERIAL_CONTROL_ADDRESS, value & ~ConsoleLink::LINK_TRANSFER);

        m_memory.write(SERIAL_DATA_ADDRESS, 0xFF);
        Interrupts::set(m_memory, InterruptMask::SERIAL);
    }
}

void SocketLink::check()
{
    uint8_t recv;
    {
        lock_guard<mutex> guard(m_rxLock);

        if (m_rxQueue.empty()) { return; }

        recv = m_rxQueue.front();
        m_rxQueue.pop_front();
    }

    m_memory.write(SERIAL_DATA_ADDRESS, recv);

    uint8_t & current = m_memory.read(SERIAL_CONTROL_ADDRESS);
    current &= ~ConsoleLink::LINK_TRANSFER;

    Interrupts::set(m_memory, InterruptMask::SERIAL);
}

void SocketLink::initServer(int port)
{
    m_server = thread([&, port] { executeServer(m_socket, port); });
}

void SocketLink::executeServer(int socket, int port)
{
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(socket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        ERROR("Failed to bind socket to port %d\n", port);
        return;
    }

    if (listen(socket, 1) != 0) {
        ERROR("%s\n", "Failed to listen on bound socket");
        return;
    }

    struct sockaddr_in cli;

    NOTE("Starting server thread on port %d (0x%04x)\n", port, htons(port));

    while (!m_interrupt.load()) {
        socklen_t length = sizeof(cli);

        NOTE("%s\n", "Waiting for client connection");

        int conn = accept(socket, (struct sockaddr*)&cli, &length);
        if (INVALID_FD == conn) {
            ERROR("Accept failed with errno = %s\n", std::strerror(errno));
            continue;
        }

        NOTE("Server connection established: %d\n", conn);

        m_connected = true;
        while (m_connected) {
            m_connected = serverLoop(conn, m_txQueue, m_txLock, m_rxQueue, m_rxLock);
        }

        NOTE("%s\n", "Server connection closed");

        close(conn);
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

    while (!m_interrupt.load()) {
        if (connect(socket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
            continue;
        }

        NOTE("Client connection established: %d\n", socket);

        m_connected = true;
        while (m_connected) {
            m_connected = serverLoop(socket, m_txQueue, m_txLock, m_rxQueue, m_rxLock);
        }

        NOTE("%s\n", "Client connection closed");
    }

    NOTE("%s\n", "Client thread complete");
}

bool SocketLink::serverLoop(
    int conn,
    list<uint8_t> & tx,
    mutex & txLock,
    list<uint8_t> & rx,
    mutex & rxLock)
{
    int count;
    ioctl(conn, FIONREAD, &count);

    bool empty;

    uint8_t data = 0xFF;
    {
        lock_guard<mutex> guard(txLock);
        empty = tx.empty();

        if (!empty) {
            data = tx.front();
            tx.pop_front();
        }
    }

    auto rd = [&rxLock, &rx, conn] () -> bool {
        uint8_t data;
        if (read(conn, &data, 1) != 1)  {
            return false;
        }

        lock_guard<mutex> guard(rxLock);
        rx.push_back(data);
        return true;
    };

    if (!empty) {
        if (write(conn, &data, 1) != 1) { return false; }

        return rd();
    } else if (0 != count) {
        if (!rd()) { return false; }

        return (write(conn, &data, 1) == 1);
    }

    return true;
}
