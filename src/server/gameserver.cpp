#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>

#include <openssl/sha.h>

#include <cstring>
#include <cstdint>
#include <thread>
#include <array>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <fstream>
#include <optional>
#include <chrono>

#include "logging.h"
#include "configuration.h"
#include "gameserver.h"
#include "base64.h"
#include "gameboyinterface.h"

using std::array;
using std::string;
using std::vector;
using std::stringstream;
using std::string;
using std::unordered_map;
using std::ifstream;
using std::thread;
using std::optional;

using namespace std::chrono_literals;

const unordered_map<GameServer::ResponseCode, string> GameServer::RESPONSE_MSG = {
    { GameServer::HTTP_101, "Switching Protocols" },
    { GameServer::HTTP_200, "OK"                  },
    { GameServer::HTTP_204, "No Content"          },
    { GameServer::HTTP_400, "Bad Request"         },
    { GameServer::HTTP_404, "Not Found"           },

};

GameServer::GameServer()
    : GameServer(HTTP_PORT, WEBSOCK_PORT)
{
}

GameServer::GameServer(int http, int websock)
    : m_httpPort(http),
      m_websockPort(websock),
      m_interrupt(false),
      m_console(GameBoyInterface::Instance())
{
    string filename = Configuration::getString(ConfigKey::ROM);
    if (m_console) {
        m_console->load(filename);
        m_console->start();
    }
}

GameServer::~GameServer()
{
    stop();
}

bool GameServer::init(int & fd, int port)
{
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID == fd) {
        ERROR("%s\n", "Failed to open socket");
    }

    int option = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        ERROR("Failed to bind socket to port %d\n", port);
        return false;
    }

    if (listen(fd, 1) != 0) {
        ERROR("%s\n", "Failed to listen on bound socket");
        return false;
    }

    return true;
}

bool GameServer::start()
{
    if (!init(m_httpSock, m_httpPort)) {
        return false;
    }

    if (!init(m_websockSock, m_websockPort)) {
        return false;
    }

    m_websock = thread([&] { websocket(); });

    run();

    return true;
}

int GameServer::connect(int socket)
{
    struct sockaddr_in cli;
    socklen_t length = sizeof(cli);

    return accept(socket, (struct sockaddr*)&cli, &length);
}

void GameServer::run()
{
    while (!m_interrupt.load()) {
        http();
    }
}

optional<vector<string>> GameServer::wait(int conn)
{
    array<uint8_t, RECV_BUFFER_SIZE> buffer;

    vector<string> headers;

    stringstream stream;
    while (true) {
        ssize_t recvd = read(conn, buffer.data(), buffer.size());
        if (recvd <= 0) { return std::nullopt; }

        for (ssize_t i = 0; i < recvd; ++i) {
            if ('\n' == buffer.at(i)) {
                headers.push_back(stream.str());
                stream = stringstream();

                if ("\r" == headers.back()) {
                    return headers;
                }
            } else {
                stream << buffer.at(i);
            }
        }
    }

    return std::nullopt;
}

void GameServer::http()
{
    int conn = connect(m_httpSock);
    if (INVALID == conn) { return; }

    while (true) {
        auto headers = wait(conn);
        if (!headers) { break; }

        handleRequest(conn, *headers);
    }

    close(conn);
}

void GameServer::handleRequest(int conn, const vector<string> & request)
{
    vector<string> tokens = Configuration::split(request.at(0), " ");
    if (3 != tokens.size()) { return; }

    const auto & method   = tokens.at(0);
    const auto & protocol = tokens.at(2);

    string file = tokens.at(1);
    if ("/" == file) { file = "index.html"; }

    string response;
    if (("GET" != method) || ("HTTP/1.1" != protocol)) {
        response = getResponse(HTTP_400, "400.html");
    } else {
        if (file.find("event") != string::npos) {
            response = header(HTTP_204, 0).str();

            handleEvent(file);
        } else {
            response = getResponse(HTTP_200, file);
        }
    }

    if (INVALID == write(conn, response.data(), response.length())) {
        ERROR("Failed to respond to http request: \"%s\"\n", request.at(0).c_str());
    }
}

void GameServer::handleEvent(const string & request)
{
    vector<string> parts = Configuration::split(request, "?");
    if (parts.size() != 2) { return; }

    vector<string> tokens = Configuration::split(parts.at(1), "=");
    if (tokens.size() < 2) { return; }

    EventType event = EventType(std::stoi(tokens.at(0)));
    switch (event) {
    case KEY_UP: {
        GameBoyInterface::JoyPadButton button =
            GameBoyInterface::JoyPadButton(std::stoi(tokens.at(1)));
        m_console->clrButton(button);

        break;
    }

    case KEY_DOWN: {
        GameBoyInterface::JoyPadButton button =
            GameBoyInterface::JoyPadButton(std::stoi(tokens.at(1)));
        m_console->setButton(button);

        break;
    }

    default:
        WARN("Unrecognized event: %d\n", event);
        break;
    }
}

string GameServer::getResponse(ResponseCode code, const string & file) const
{
    ifstream input("html/" + file);
    if (!input.good()) {
        return ("404.html" == file) ? "" : getResponse(HTTP_404, "404.html");
    }

    input.seekg(0, input.end);
    int length = int(input.tellg());
    input.seekg(0, input.beg);

    stringstream stream = header(code, length);

    string message;
    message.resize(length + 1);
    input.read(message.data(), message.capacity());

    stream << message;

    return stream.str();
}

stringstream GameServer::header(ResponseCode code, int length) const
{
    auto iterator = RESPONSE_MSG.find(code);

    stringstream stream;
    stream << "HTTP/1.1 " << code << " " << iterator->second << CRLF;
    stream << "Date: Mon, 27 Jul 2009 12:28:53 GMT" << CRLF;
    stream << "Server: GameBoy Trilogy" << CRLF;
    if (0 != length) {
        stream << "Last-Modified: Wed, 22 Jul 2019 19:15:56 GMT" << CRLF;
        stream << "Content-Length: " << length << CRLF;
        stream << "Content-Type: text/html" << CRLF;
    }
    stream << "Connection: Closed" << CRLF;
    stream << CRLF;

    return stream;
}

void GameServer::websocket()
{
    while (!m_interrupt.load()) {
        int conn = connect(m_websockSock);
        if (INVALID == conn) { continue; }

        if (handshake(conn)) {
            while (true) {
                std::this_thread::sleep_for(33ms);

                auto payload = m_console->getRGB();
                if (payload.empty()) { continue; }

                if (!send(conn, payload)) {
                    break;
                }
            }
        }

        close(conn);
    }
}

bool GameServer::handshake(int conn)
{
    auto headers = wait(conn);
    if (!headers) { return false; }

    string key = getKey(*headers);
    if (key.empty()) { return false; }

    key += MAGIC_KEY;

    vector<uint8_t> hash = sha1(key);

    string response = base64_encode(hash.data(), hash.size());

    string message = RESPONSE_MSG.find(HTTP_101)->second;

    stringstream stream;
    stream << "HTTP/1.1 " << HTTP_101 << " " << message << CRLF;
    stream << "Upgrade: websocket" << CRLF;
    stream << "Connection: Upgrade" << CRLF;
    stream << "Sec-WebSocket-Accept: " << response << CRLF;
    stream << CRLF;

    string payload = stream.str();

    ssize_t ret = write(conn, payload.data(), payload.size());
    return (INVALID != ret);
}

bool GameServer::send(int conn, const vector<GB::RGB> & payload)
{
    std::array<uint8_t, 10> header;
    header[0] = 0x82;
    header[1] = 0x7F;

    uint32_t length = payload.size() * 4;

    header[2] = header[3] = header[4] = header[5] = 0x00;
    header[6] = (length >> 24) & 0xFF;
    header[7] = (length >> 16) & 0xFF;
    header[8] = (length >> 8) & 0xFF;
    header[9] = length & 0xFF;

    if (INVALID == write(conn, header.data(), header.size())) {
        return false;
    }

    ssize_t ret = write(conn, payload.data(), length);
    return (INVALID != ret);
}

string GameServer::getKey(const vector<string> & headers) const
{
    for (auto it = std::next(headers.begin()); it != headers.end(); ++it) {
        auto kvp = Configuration::split(*it, ":");
        if (kvp.size() < 2) { continue; }

        const auto & key   = kvp.at(0);
        const auto & value = kvp.at(1);

        if (SECRET_KEY == key) {
            return Configuration::trim(value);
        }
    }
    return "";
}

vector<uint8_t> GameServer::sha1(const string & key) const
{
    vector<uint8_t> result(SHA_DIGEST_LENGTH);

    SHA_CTX context;
    SHA1_Init(&context);
    SHA1_Update(&context, key.data(), key.size());
    SHA1_Final(result.data(), &context);

    return result;
}

void GameServer::stop()
{
    m_interrupt = true;

    if (INVALID != m_httpSock) { close(m_httpSock); }

    if (m_websock.joinable()) {
        pthread_cancel(m_websock.native_handle());
        m_websock.join();
    }

    if (INVALID != m_websockSock) { close(m_websockSock); }
}

void GameServer::updateCanvas(uint8_t x, uint8_t y, GB::RGB pixel)
{
    (void)x; (void)y; (void)pixel;
}

void GameServer::renderCanvas()
{

}
