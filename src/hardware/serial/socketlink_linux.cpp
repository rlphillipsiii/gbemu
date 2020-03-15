#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <cstring>
#include <thread>

#include "socketlink_linux.h"
#include "memorycontroller.h"
#include "interrupt.h"
#include "logging.h"
#include "configuration.h"

using std::thread;

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

    try {
        if (INVALID_FD != m_socket) {
            close(m_socket);
        }
    } catch (std::exception &) { }

    m_client.join();
    m_server.join();
}

void SocketLink::transfer(uint8_t value)
{
    (void)value;
    if (m_connected) {

    } else {

    }
}

void SocketLink::txSlave(uint8_t value)
{
    (void)value;
}

void SocketLink::txMaster(uint8_t value)
{
    (void)value;
}

void SocketLink::check()
{

}

void SocketLink::initServer(int port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) == INVALID_FD) {
        ERROR("Failed to bind socket to port %d\n", port);
        return;
    }

    if (listen(m_socket, 1) == INVALID_FD) {
        ERROR("%s\n", "Failed to listen on bound socket");
        return;
    }

    m_server = thread([&] { executeServer(m_socket); });
}

void SocketLink::executeServer(int socket)
{
    struct sockaddr_in addr;

    NOTE("%s\n", "Starting server thread");

    while (!m_interrupt.load()) {
        socklen_t length = sizeof(addr);

        int conn = accept(socket, (struct sockaddr*)&addr, &length);
        if (INVALID_FD == conn) { continue; }

        NOTE("%s\n", "Server connection established");

        m_connected = true;
        while (m_connected) {

        }

        NOTE("%s\n", "Server connection closed");

        close(conn);
    }
}

void SocketLink::initClient(int port)
{
    m_client = thread([&] { executeClient(m_socket, port); });
}

void SocketLink::executeClient(int socket, int port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1) {
        ERROR("%s\n", "Failed convert host IP");
        return;
    }

    NOTE("%s\n", "Starting client thread");

    while (!m_interrupt.load()) {
        int conn = connect(socket, (struct sockaddr*)&addr, sizeof(addr));
        if (INVALID_FD == conn) { continue; }

        NOTE("%s\n", "Client connection established");

        m_connected = true;
        while (m_connected) {

        }

        NOTE("%s\n", "Client connection closed");

        close(conn);
    }
}
