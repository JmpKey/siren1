#include "client.h"

Client::Client() {}

int Client::cli_start(int _argc, char* _argv[]) {
    if (_argc < 4) {
        std::cerr << "Usage: " << _argv[0] << " <IPv6 address> <port> <file1> [<file2> ...]" << std::endl;
        return 1;
    }

    const char* ipv6_address = _argv[1];
    int port = std::stoi(_argv[2]);

#ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }
#endif

    #ifdef _WIN32
    SOCKET sock; // On NT the socket handle is SOCKET
    #else
    int sock;     // On Unix the socket handle is int
    #endif
    struct sockaddr_in6 server_addr;

    sock = socket(AF_INET6, SOCK_STREAM, 0);
    #ifdef _WIN32
    if (sock == INVALID_SOCKET) {
        std::cerr << "Error creating socket! WSA error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    #else
    if (sock < 0) {
        std::cerr << "Error creating socket!" << std::endl;
        return 1;
    }
    #endif

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port);

    // inet_pton returns 1 on success, 0 if not valid IP, -1 on error
    // On Windows, in case of an error, 0 is returned and errno is not installed,
    // but WSAGetLastError() returns an error (for example, WSAEINVAL).
    if (inet_pton(AF_INET6, ipv6_address, &server_addr.sin6_addr) != 1) {
        std::cerr << "Error converting IPv6 address: " << ipv6_address;
        #ifdef _WIN32
        std::cerr << " WSA error: " << WSAGetLastError();
        #endif
        std::cerr << std::endl;
        #ifdef _WIN32
        closesocket(sock);
        WSACleanup();
        #else
        close(sock);
        #endif
        return 1;
    }

    #ifdef _WIN32
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Error connecting to server! WSA error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    #else
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error connecting to server!" << std::endl;
        close(sock);
        return 1;
    }
    #endif

    // Sorting through all the files
    for (int i = 3; i < _argc; ++i) {
        const std::string filename = _argv[i];
        if(filename == "IF") {std::cout << "!filename!" << std::endl; }

        // Find out the size
        std::ifstream infile(filename, std::ios::binary | std::ios::ate);
        if (!infile) {
            std::cerr << "Error opening file: " << filename << std::endl;
            continue;
        }

        // 1. Determine the file size
        long long file_size = infile.tellg();
        infile.seekg(0, std::ios::beg);

        std::cout << "Preparing to send file: " << filename << " (" << file_size << " bytes)" << std::endl;

        // 2. Send the file size (8 bytes)
        #ifdef _WIN32
        if (send_all(sock, &file_size, sizeof(file_size)) == SOCKET_ERROR) {
        #else
        if (send_all(sock, &file_size, sizeof(file_size)) < 0) {
        #endif
            std::cerr << "Error sending file size for " << filename;
            #ifdef _WIN32
            std::cerr << " WSA error: " << WSAGetLastError();
            #endif
            std::cerr << std::endl;
            infile.close();
            break;
        }

        // 3. Send the file name (fill the fixed buffer)
        char filename_buffer[FILENAME_BUFFER_SIZE] = {};
        char name_file_out[FILENAME_BUFFER_SIZE] = {};
        extract_filename(filename, name_file_out);
        strncpy(filename_buffer, name_file_out, FILENAME_BUFFER_SIZE - 1);

        #ifdef _WIN32
        if (send_all(sock, filename_buffer, FILENAME_BUFFER_SIZE) == SOCKET_ERROR) {
        #else
        if (send_all(sock, filename_buffer, FILENAME_BUFFER_SIZE) < 0) {
        #endif
            std::cerr << "Error sending filename for " << filename;
            #ifdef _WIN32
            std::cerr << " WSA error: " << WSAGetLastError();
            #endif
            std::cerr << std::endl;
            infile.close();
            break;
        }

        // 4. Sending the contents of the file
        char buffer[BUFFER_SIZE];
        long long bytes_sent = 0;
        bool send_success = true;

        while (bytes_sent < file_size) {
            infile.read(buffer, BUFFER_SIZE);
            ssize_t read_count = infile.gcount(); // How many bytes read

            if (read_count > 0) {
                ssize_t sent_count = send_all(sock, buffer, read_count);
                #ifdef _WIN32
                if (sent_count == SOCKET_ERROR) {
                #else
                if (sent_count < 0) {
                #endif
                    std::cerr << "Error sending data for " << filename;
                    #ifdef _WIN32
                    std::cerr << " WSA error: " << WSAGetLastError();
                    #endif
                    std::cerr << std::endl;
                    send_success = false;
                    break;
                }
                bytes_sent += sent_count;
            } else {
                if (infile.eof()) {
                    std::cerr << "Warning: Reached end of file " << filename << " unexpectedly early." << std::endl;
                } else if (infile.fail()) {
                    std::cerr << "Error reading file: " << filename << std::endl;
                }
                break;
            }
        }

        infile.close();

        if (send_success && bytes_sent == file_size) {
            std::cout << "File " << filename << " sent completely." << std::endl;
        } else {
            std::cerr << "Warning: File " << filename << " transfer incomplete or failed (sent " << bytes_sent << " of " << file_size << " bytes)." << std::endl;
        }
    }

    #ifdef _WIN32
    closesocket(sock);
    WSACleanup(); // Deinitializing Winsock
    #else
    close(sock);
    #endif

    std::cout << "Finished sending files." << std::endl;

    return 0;
}

ssize_t Client::send_all(
    #ifdef _WIN32
    SOCKET _sock,
    #else
    int _sock,
    #endif
    const void* _buffer, size_t _len) {

    size_t total_sent = 0;
    const char* buf = (const char*)_buffer;
    while (total_sent < _len) {
        #ifdef _WIN32
        // On Windows, send returns int, so _len - total_sent need to be cast to int
        int bytes_sent = send(_sock, buf + total_sent, static_cast<int>(_len - total_sent), 0);
        if (bytes_sent == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }
        #else
        ssize_t bytes_sent = send(_sock, buf + total_sent, _len - total_sent, 0);
        if (bytes_sent < 0) {
            return bytes_sent;
        }
        #endif
        total_sent += bytes_sent;
    }
    return total_sent;
}

void Client::extract_filename(const std::string& _full_path, char* _filename_buffer) {
    // Find the last path divider (Unix'/' and NT '\')
    size_t last_slash_pos = _full_path.find_last_of("/\\");

    // Extract the file name
    if (last_slash_pos != std::string::npos) {
        // If the separator is found, take the substring after it
        std::string filename = _full_path.substr(last_slash_pos + 1);
        strncpy(_filename_buffer, filename.c_str(), FILENAME_BUFFER_SIZE - 1);
        _filename_buffer[FILENAME_BUFFER_SIZE - 1] = '\0'; // Providing a null terminator
    } else {
        // Если разделителей нет, просто копируем полный путь
        strncpy(_filename_buffer, _full_path.c_str(), FILENAME_BUFFER_SIZE - 1);
        _filename_buffer[FILENAME_BUFFER_SIZE - 1] = '\0'; // Providing a null terminator
    }
}
