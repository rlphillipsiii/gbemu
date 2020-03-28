#ifndef _GAME_SERVER_H
#define _GAME_SERVER_H

#include <thread>
#include <atomic>
#include <unordered_map>
#include <sstream>
#include <optional>
#include <vector>

#include "gameboyinterface.h"
#include "gbrgb.h"

class GameServer final {
public:
    GameServer();
    GameServer(int http, int websock);
    ~GameServer();

    bool start();
    void stop();

private:
    static constexpr int INVALID = -1;

    static constexpr int HTTP_PORT    = 8282;
    static constexpr int WEBSOCK_PORT = 1148;

    static constexpr int RECV_BUFFER_SIZE = 512;

    static constexpr int BYTES_PER_PIXEL = 4;
    static constexpr int CANVAS_SIZE =
        GameBoyInterface::HEIGHT * GameBoyInterface::WIDTH * BYTES_PER_PIXEL;

    static constexpr int ACK_LENGTH = 3;

    static constexpr const char *CRLF = "\r\n";

    static constexpr const char *SECRET_KEY = "Sec-WebSocket-Key";

    static constexpr const char *MAGIC_KEY
        = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    enum ResponseCode {
        HTTP_101 = 101,
        HTTP_200 = 200,
        HTTP_204 = 204,
        HTTP_400 = 400,
        HTTP_404 = 404,
    };

    enum EventType {
        KEY_UP   = 0,
        KEY_DOWN = 1,
    };

    static const std::unordered_map<ResponseCode, std::string> RESPONSE_MSG;

    int m_httpPort;
    int m_websockPort;

    std::atomic<bool> m_interrupt;

    std::thread m_websock;

    int m_httpSock;
    int m_websockSock;

    std::shared_ptr<GameBoyInterface> m_console;

    void run();

    bool init(int & fd, int port);
    void websocket();
    std::string getKey(const std::vector<std::string> & headers) const;
    std::vector<uint8_t> sha1(const std::string & key) const;
    bool handshake(int conn);
    bool send(int conn, const std::vector<GB::RGB> & packet);

    void handleEvent(const std::string & request);

    int connect(int socket);
    std::optional<std::vector<std::string>> wait(int conn);

    void http();
    void handleRequest(int conn, const std::vector<std::string> & request);

    std::stringstream header(ResponseCode code, int length, const std::string & mime) const;
    std::string getResponse(ResponseCode code, const std::string & file) const;
};

#endif
