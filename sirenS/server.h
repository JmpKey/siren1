#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <string>

// Windows-specific headers
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> // For WSAGetLastError, etc.
#include <experimental/filesystem> // C++17 Filesystem
#pragma comment(lib, "ws2_32.lib") // Link with ws2_32.lib
#else
// Unix headers
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <filesystem>
#endif

#define BUFFER_SIZE 1024

#ifdef _WIN32
    namespace fs = std::experimental::filesystem;
#endif

class Server
{
public:
    Server(){};
    int run(int _argc, char* _argv[]);

    void take(
#ifdef _WIN32
        SOCKET
#else
        int
#endif
        _client_socket);

    // Return type changed from ssize_t to int for cross-platform compatibility
    int recv_all(
#ifdef _WIN32
        SOCKET
#else
        int
#endif
        _sock, void* _buffer, size_t _len);

    void whatDir(const std::string& _path);

    // std::string charPtrToString(char* _cstr);
    // char* stringToCharPtr(const std::string& _str);
};

#endif // SERVER_H
