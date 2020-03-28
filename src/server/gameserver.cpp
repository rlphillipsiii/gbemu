#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

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
    signal(SIGPIPE, SIG_IGN);

    // Get the HTTP server and the WebSocket server ready to run.  If
    // initialization of either fails here, then we can't continue, so
    // just bail and let the calling code decide how to proceed.
    if (!init(m_httpSock, m_httpPort)) { return false; }
    if (!init(m_websockSock, m_websockPort)) { return false; }

    // Start a background thread the runs the websocket server.  This
    // thread is going to be responsible for the WebSocket handshake and
    // then streaming the GameBoy's framebuffer to the client at a set
    // interval (if the client can keep up).
    m_websock = thread([&] { websocket(); });

    // Start the HTTP server in the calling thread.
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
    // Run the HTTP server loop until someone says not to.
    while (!m_interrupt.load()) {
        http();
    }
}

optional<vector<string>> GameServer::wait(int conn)
{
    array<uint8_t, RECV_BUFFER_SIZE> buffer;

    vector<string> headers;

    string stream;

    // We are going to block until we've received a full set of HTTP
    // headers or the connection is lost.
    while (true) {
        // Execute a blocking read on the file descriptor.  If we receive
        // a 0 or a negative number, then our connection is no longer
        // alive, so we need to return nothing back to the caller.
        ssize_t recvd = read(conn, buffer.data(), buffer.size());
        if (recvd <= 0) { return std::nullopt; }

        // At this point, we've received some data, so we need to loop over
        // what we received and process it byte by byte.  Each header is
        // separated by a CRLF, so we can consider a newline character to
        // be the end of a line that defines a key value pair in the header.
        for (ssize_t i = 0; i < recvd; ++i) {
            if ('\n' == buffer.at(i)) {
                // We are at the end of the line, so add the header to our
                // vector and reset the string back to its empty state.
                headers.emplace_back(std::move(stream));
                stream.clear();

                // If we've gotten a line with nothing but a carriage return,
                // then we found the end of the header, and we can return our
                // vector.
                if ("\r" == headers.back()) {
                    return headers;
                }
            } else {
                stream += buffer.at(i);
            }
        }
    }
    // Not supposed to be able to get here
    return std::nullopt;
}

void GameServer::http()
{
    // Block until we've received a connection on our HTTP socket.
    int conn = connect(m_httpSock);
    if (INVALID == conn) { return; }

    while (true) {
        // Block again waiting to receive a full HTTP header via the
        // connection that we just established.  If the header object
        // that we get back is empty, then our connection is no bueno,
        // so break out of our infinite loop and close the fd.
        auto headers = wait(conn);
        if (!headers) { break; }

        // We've got a full HTTP request from the connected client, so
        // hand it off to be processed and responded to.
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
            response = header(HTTP_204, 0, "text/html").str();

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
    // The request comes from the host in the form of "event?key=value", so
    // the first thing that we want to do here is strip the event piece.  A
    // straightforward way to do that is to split on the ? and return the
    // second entry.  Well formed requests will only contain 1 question mark
    // so make sure that there are only 2 entries after we split.
    vector<string> parts = Configuration::split(request, "?");
    if (parts.size() != 2) { return; }

    // TODO: not necessary yet, but might want to split on "&" and loop
    //       over each entry

    // Now that we have isolated the data, split on the "=", which should
    // give use a key and 1 or more entries corresonding to the value.
    vector<string> tokens = Configuration::split(parts.at(1), "=");
    if (tokens.size() < 2) { return; }

    // Well formed requests will contain an integer key corresponding to an
    // EventType, so convert the string to an int and then to an EventType.
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

    string mime = (file.find(".css") != string::npos) ? "text/css" : "text/html";
    stringstream stream = header(code, length, mime);

    string message;
    message.resize(length + 1);
    input.read(message.data(), message.capacity());

    stream << message;

    return stream.str();
}

stringstream GameServer::header(ResponseCode code, int length, const string & mime) const
{
    // TODO: actually send a real date

    auto iterator = RESPONSE_MSG.find(code);

    stringstream stream;
    stream << "HTTP/1.1 " << code << " " << iterator->second << CRLF;
    stream << "Date: Mon, 27 Jul 2009 12:28:53 GMT" << CRLF;
    stream << "Server: GameBoy Trilogy" << CRLF;
    if (0 != length) {
        stream << "Last-Modified: Wed, 22 Jul 2019 19:15:56 GMT" << CRLF;
        stream << "Content-Length: " << length << CRLF;
        stream << "Content-Type: " << mime << CRLF;
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
                auto payload = m_console->getRGB();
                if (payload.empty()) { continue; }

                if (!send(conn, payload)) {
                    break;
                }

                std::this_thread::sleep_for(33ms);

                std::array<uint8_t, ACK_LENGTH> ack;
                ssize_t resp = read(conn, ack.data(), ack.size());
                if (resp <= 0) { break; }
            }
        }

        close(conn);
    }
}

bool GameServer::handshake(int conn)
{
    // Wait for a regular HTTP request to come in.  The websocket protocol
    // starts w/ a normal HTTP exchange before switching over to the
    // actual websocket stream.
    auto headers = wait(conn);
    if (!headers) { return false; }

    // Now that we've got a request, we need to retrieve the key that was
    // sent by the client so that we can use it to derive the key tha we
    // will send back to complete the handshake.
    string key = getKey(*headers);
    if (key.empty()) { return false; }

    // The protocol dictates that we need to concatenate this "magic key"
    // to whatever the client sent us.
    key += MAGIC_KEY;

    // Compute our own response key by taking the sha1 hash of the key
    // that we just built from the client's request and then applying a
    // base64 encoding algorithm to the sha1.  The result is our key.
    vector<uint8_t> hash = sha1(key);
    string response = base64_encode(hash.data(), hash.size());

    // Build the response header, which should be an HTTP 101 protocol
    // switch header.
    string message = RESPONSE_MSG.find(HTTP_101)->second;

    string payload;
    payload += "HTTP/1.1 " + std::to_string(HTTP_101) + " " + message + CRLF;
    payload += "Upgrade: websocket" + string(CRLF);
    payload += "Connection: Upgrade" + string(CRLF);
    payload += "Sec-WebSocket-Accept: " + response + CRLF;
    payload += CRLF;

    // Send our header to the client to complete the handshake.
    ssize_t ret = write(conn, payload.data(), payload.size());
    return (INVALID != ret);
}

bool GameServer::send(int conn, const vector<GB::RGB> & payload)
{
    std::array<uint8_t, 10> header;

    // The 1st two bytes of the frame are information.  We are going to set
    // it up as a single transmission of binary data w/ a length of 10 bytes
    // for the header, which implies 8 bytes for the length of the payload
    // (64 bit unsigned integer).
    header[0] = 0x82;
    header[1] = 0x7F;

    // The length of the payload that we are trying to send is the size of
    // the vector multiplied by 4.  Each entry in the vector contains a
    // "pixel" in the form of RGB8888, so there are 4 bytes per entry.
    uint32_t length = payload.size() * 4;

    // In practice, our payload has a set length of 144 * 160 * 4, which
    // means that we really only need to fill in 24 bits to be able to
    // represent our length.
    header[2] = header[3] = header[4] = header[5] = header[6] = 0x00;
    header[7] = (length >> 16) & 0xFF;
    header[8] = (length >> 8) & 0xFF;
    header[9] = length & 0xFF;

    // Send the frame and the send the payload.  GB::RGB is a struct of
    // 4 bytes, RGBA, so we can treat it like a byte stream and send it
    // out directly.
    if (INVALID == write(conn, header.data(), header.size())) {
        return false;
    }
    ssize_t ret = write(conn, payload.data(), length);
    return (INVALID != ret);
}

string GameServer::getKey(const vector<string> & headers) const
{
    // The first entry in the headers isn't an actual header, so we should
    // start the iterator at the first entry after begin() (i.e. next())
    for (auto it = std::next(headers.begin()); it != headers.end(); ++it) {
        // HTTP headers are key value pairs that are separated by a colon, so
        // split on the colon and make sure we have at least two entries.
        auto kvp = Configuration::split(*it, ":");
        if (kvp.size() < 2) { continue; }

        const auto & key   = kvp.at(0);
        const auto & value = kvp.at(1);

        // Check for the key corresponding to the key from the client.  If we
        // found it trim the whitespace off of the value and return it back to
        // the caller.
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
