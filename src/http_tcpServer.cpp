#include "http_tcpServer.h"
#include <cerrno>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <unistd.h>

namespace {
    const int BUFFER_SIZE = 30720;

    void log(const std::string &msg) {
        std::cout << msg << std::endl;
    }

    void exitWithSystemError(const char *errMsg) {
        perror(errMsg);
        exit(1);
    }

    void exitWithError(const std::string &errMsg) {
        log("ERROR: " + errMsg);
        exit(1);
    }
}

namespace http {
    TcpServer::TcpServer(std::string ip_address, int port) :
        m_ip_address(ip_address),
        m_port(port),
        m_socket(),
        m_new_socket(),
        // m_incommingMessage(),
        m_socketAddress(),
        m_socketAddress_len(sizeof(m_socketAddress)),
        m_serverMessage(buildResponse()) 
    {
        m_socketAddress.sin_family = AF_INET;
        m_socketAddress.sin_port = htons(m_port);
        m_socketAddress.sin_addr.s_addr = inet_addr(m_ip_address.c_str());
        
        if (startServer() != 0) {
            std::ostringstream ss;
            ss << "Failed to start server with PORT: " << ntohs(m_socketAddress.sin_port);
            log(ss.str());
        }
    }

    TcpServer::~TcpServer() {
        closeServer();
    }

    int TcpServer::startServer() {
        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket < 0) {
            std::ostringstream errMsg;
            errMsg << "Cannot create socket. errno: " << strerror(errno);
            exitWithError(errMsg.str());
            return 1;
        }

        if (bind(m_socket, (sockaddr *)&m_socketAddress, m_socketAddress_len) < 0) {
            exitWithError("Cannot connect socket to address");
            return 1;
        }

        std::ostringstream ss;
        ss << "Socket created.\n";
        log(ss.str());

        return 0;        
    }

    void TcpServer::closeServer() {
        close(m_socket);
        close(m_new_socket);
        exit(0);
    }

    void TcpServer::startListen() {
        if (listen(m_socket, 20) < 0) {
            // std::ostringstream errMsg;
            // errMsg << "Socket listen failed. errno: " << strerror(errno);
            // exitWithError(errMsg.str());

            const char *errMsg = "ERROR: Socket listen";
            exitWithSystemError(errMsg);
        }

        std::ostringstream ss;
        ss << "\n** Listening on ADDRESS: "
            << inet_ntoa(m_socketAddress.sin_addr)
            << " PORT: " << ntohs(m_socketAddress.sin_port)
            << " ***\n\n";
        log(ss.str());

        int bytesReceived;
        while (true) {
            log("----- Waiting for a new connection ----- \n\n\n");
            acceptConnection(m_new_socket);

            char buffer[BUFFER_SIZE] = {0};
            bytesReceived = read(m_new_socket, buffer, BUFFER_SIZE);
            if (bytesReceived < 0) {
                exitWithError("Failed to read bytes from client socket connection");
            }

            std::ostringstream ss;
            ss << "----- Received Request from client ----- \n\n";
            log(ss.str());

            sendResponse();

            close(m_new_socket);
        }
    }

    void TcpServer::acceptConnection(int &new_socket) {
        new_socket = accept(m_socket, (sockaddr *)&m_socketAddress, &m_socketAddress_len);

        if (new_socket < 0) {
            std::ostringstream ss;
            ss << "Server failed to accept incomming connection from ADDRESS: "
                << inet_ntoa(m_socketAddress.sin_addr) << "PORT: "
                << ntohs(m_socketAddress.sin_port);

            exitWithError(ss.str());
        }
    }

    std::string TcpServer::buildResponse() {
        std::string htmlFile = "<!DOCTYPE html><html lang=\"en\"><body><h1>Web Server Test</h1><p>Yep.</p></body></html>";
        std::ostringstream ss;
        ss << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " << htmlFile.size() << "\n\n"
            << htmlFile;

        return ss.str();
    }

    void TcpServer::sendResponse() {
        long bytesSent;
        bytesSent = write(m_new_socket, m_serverMessage.c_str(), m_serverMessage.size());

        if (bytesSent == m_serverMessage.size()) {
            log("----- Server Response sent to client ----- \n\n");
        } else {
            log("Error sending response to client");
        }
    }
} // namespace http
