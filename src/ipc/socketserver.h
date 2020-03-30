#ifndef _SOCKET_H
#define _SOCKET_H

#include <memory>

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#else

#endif

class SocketServer {
public:
    explicit SocketServer(int port);
    ~SocketServer() = default;
    
    inline bool ok() const { return m_ok; }

    bool start();
    
private:
    int m_port;

    bool m_ok;

#ifdef WIN32
    ////////////////////////////////////////////////////////////////////////////////
    class WsaHelper {
    public:
        WsaHelper();
        ~WsaHelper();

        inline bool ok() const { return m_ok; }
    private:
        WSADATA m_data;

        bool m_ok;
    };
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    class AddrInfo {
    public:
        AddrInfo() : m_result(nullptr) { }
        ~AddrInfo() { if (m_result) {freeaddrinfo(m_result); } }
    
        inline struct addrinfo **get() { return &m_result; }
    private:
        struct addrinfo *m_result;
    };

    ////////////////////////////////////////////////////////////////////////////////
    class SocketHandle {
    public:
        SocketHandle(AddrInfo & info);
        ~SocketHandle();

        inline bool ok() const { return m_ok; }

        inline SOCKET handle() const { return m_socket; }
    private:
        SOCKET m_socket;

        bool m_ok;
    };
    ////////////////////////////////////////////////////////////////////////////////

    std::unique_ptr<WsaHelper> m_wsa;
    std::unique_ptr<SocketHandle> m_socket;
#else

#endif
};

#endif
