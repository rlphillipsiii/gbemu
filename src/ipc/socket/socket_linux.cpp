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
#include <string>

#include "socketlink_linux.h"
#include "logging.h"
#include "configuration.h"

using std::thread;
using std::string;

SocketConnection::SocketConnection()
{
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_FD == m_socket) {
        ERROR("%s\n", "Failed to open socket");
    }

    m_port   = Configuration::getInt(ConfigKey::LINK_PORT);
    m_master = Configuration::getBool(ConfigKey::LINK_MASTER);

    NOTE("Socket server config: %s\n", (m_master) ? "master" : "slave");
}

bool SocketConnection::start()
{
    return (m_master) ? startServer() : startClient();
}

void SocketConnection::stop(thread & t)
{
    if (INVALID_FD != m_socket) {
        close(m_socket);
    }
}

void SocketConnection::connect()
{
    if (m_master) {
        struct sockaddr_in cli;
        socklen_t length = sizeof(cli);

        NOTE("%s\n", "Waiting for client connection");

        m_connection = accept(socket, (struct sockaddr*)&cli, &length);
        if (INVALID_FD == m_connection) {
            ERROR("Accept failed with errno = %s\n", std::strerror(errno));
            continue;
        }
    } else {

    }
}

bool SocketConnection::startServer(int port)
{
    if (INVALID_FD == m_socket) { return false; }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        ERROR("Failed to bind socket to port %d\n", port);
        return false;
    }

    if (listen(m_socket, 1) != 0) {
        ERROR("%s\n", "Failed to listen on bound socket");
        return false;
    }

    NOTE("Starting server on port %d (0x%04x)\n", port, htons(port));
    return true;
}

bool SocketConnection::startClient()
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    string host = Configuration::getString(ConfigKey::LINK_ADDR);

    struct hostent *info = gethostbyname(host.c_str());
    if (!info) {
        ERROR("%s\n", "Failed convert host IP");
        return false;
    }

    std::memcpy((void*)&addr.sin_addr.s_addr, (void*)info->h_addr, info->h_length);

    LOG("Starting client on port %d (0x%04x)\n", port, htons(port));
    while (connect(m_socket, (struct sockaddr*)&addr, sizeof(addr)) == 0);
}

void SocketConnection::executeClient(int socket, int port)
{

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

bool SocketConnection::rd(uint8_t & data)
{
    return (read(m_connection, (void*)&data, 1) == 1);
}

bool SocketConnection::wr(uint8_t data)
{
    return (write(m_connection, (void*)&data, 1) == 1);
}
