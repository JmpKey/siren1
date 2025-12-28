#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <fstream>
#include <string>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <cstring>
    #ifndef SSIZE_T
    #define SSIZE_T long long
    #endif
    typedef SSIZE_T ssize_t;
#else // Unix
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <cstring>
    #include <unistd.h> // Для close()
#endif

#define BUFFER_SIZE 1024

class Client
{
public:
    Client();
        int cli_start(int _argc, char* _argv[]);

private:
    static constexpr std::size_t FILENAME_BUFFER_SIZE = 256; // Must match the size on the server

    ssize_t send_all(
        #ifdef _WIN32
        SOCKET _sock,
        #else
        int _sock,
        #endif
        const void* _buffer, size_t _len);

    void extract_filename(const std::string& _full_path, char* _filename_buffer);
};

#endif // CLIENT_H
