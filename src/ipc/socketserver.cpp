#include <cassert>
#include <string>

#include "socketserver.h"

#ifdef ERROR
#undef ERROR
#endif

#include "logging.h"

using std::unique_ptr;
using std::string;

#ifdef WIN32

SocketServer::WsaHelper::WsaHelper()
{
    m_ok = (WSAStartup(MAKEWORD(2,2), &m_data) == 0);
}

SocketServer::WsaHelper::~WsaHelper()
{
    WSACleanup();
}

SocketServer::SocketHandle::SocketHandle(AddrInfo & info)
{
    auto *result = *info.get();
    
    m_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    m_ok = (INVALID_SOCKET == m_socket);
}

SocketServer::SocketHandle::~SocketHandle()
{
    if (ok()) {
        closesocket(m_socket);
    }
}


#else

#endif

////////////////////////////////////////////////////////////////////////////////
SocketServer::SocketServer(int port)
    : m_port(port),
      m_ok(false)
{

}

#ifdef WIN32
bool SocketServer::start()
{
    auto wsa = std::make_unique<WsaHelper>();
    assert(wsa);
    
    if (!wsa->ok()) {
        ERROR("%s\n", "Failed WSAStartup");
        return false;
    }

    struct addrinfo hints;    
    ZeroMemory(&hints, sizeof(hints));

    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags    = AI_PASSIVE;

    string port = std::to_string(m_port);
    
    AddrInfo info;
    if (getaddrinfo(NULL, port.c_str(), &hints, info.get()) != 0) {
        ERROR("%s\n", "getaddrinfo failed");
        return false;
    }

    auto sock = std::make_unique<SocketHandle>(info);
    assert(sock);
    
    if (!sock->ok()) {
        ERROR("socket failed with error: %d\n", WSAGetLastError());
        return false;
    }

    auto *result = *info.get();
    if (bind(sock->handle(), result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        ERROR("bind failed with error: %d\n", WSAGetLastError());
        return false;
    }

    if (listen(sock->handle(), 1) == SOCKET_ERROR) {
        ERROR("listen failed with error: %d\n", WSAGetLastError());
        return false;
    }

    m_wsa    = std::move(wsa);
    m_socket = std::move(sock);
    return true;
}
#else
bool SocketServer::start()
{
    return false;
}
#endif
