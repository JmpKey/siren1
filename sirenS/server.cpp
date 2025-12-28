#include "server.h"

void Server::take(
#ifdef _WIN32
    SOCKET
#else
    int
#endif
    _client_socket) {
    const int FILENAME_BUFFER_SIZE = 256; // Fixed size for file name
    char buffer[BUFFER_SIZE];

    while (true) {
        long long expected_size = 0;

        // 1. Get the expected file size (8 bytes)
        int size_status = recv_all(_client_socket, &expected_size, sizeof(expected_size));

        if (size_status <= 0) {
            // If the client disconnects or there is no more data to read, exit the loop
            if (size_status == 0) {
                std::cout << "Client disconnected gracefully." << std::endl;
            } else {
                std::cerr << "Error receiving file size or client disconnected. Error code: ";
#ifdef _WIN32
                std::cerr << WSAGetLastError() << std::endl;
#else
                std::cerr << errno << std::endl;
#endif
            }
            break;
        }

        if (expected_size == 0) {
            // If the size is 0, it can be an empty file, process it
            // Proceed to get the file name
        }

        // 2. Get the file name (fixed size)
        char filename_buffer[FILENAME_BUFFER_SIZE] = {0};
        if (recv_all(_client_socket, filename_buffer, FILENAME_BUFFER_SIZE) <= 0) {
            std::cerr << "Error receiving filename. Error code: ";
#ifdef _WIN32
            std::cerr << WSAGetLastError() << std::endl;
#else
            std::cerr << errno << std::endl;
#endif
            break;
        }

        // Check for invalid characters in the file name
        if (strchr(filename_buffer, '/') != nullptr || strchr(filename_buffer, '\\') != nullptr) {
            std::cerr << "Error: filename should not contain '/' or '\\'." << std::endl;
            // Continue reading to clear the buffer, but don't save the file
            // To simplify, just skip this file
            std::cerr << "Discarding malformed filename and remaining data for this file." << std::endl;
            // You need to read the remaining file data so as not to mess up the stream
            long long total_received_discard = 0;
            while (total_received_discard < expected_size) {
                long long remaining = expected_size - total_received_discard;
                size_t to_read = std::min((long long)BUFFER_SIZE, remaining);
                int bytes_received = recv(_client_socket, buffer, to_read, 0);
                if (bytes_received <= 0) {
                    std::cerr << "Error or client disconnected during discard of malformed file." << std::endl;
                    break;
                }
                total_received_discard += bytes_received;
            }
            continue; // skip to next file
        }

        std::string dir_name = "files";

        whatDir(dir_name);
        
#ifdef _WIN32
        std::experimental::filesystem::path filename_path = std::experimental::filesystem::path(dir_name) / std::experimental::filesystem::path(filename_buffer);
        std::string filename = filename_path.string();
#else
        std::filesystem::path filename_path = std::filesystem::path(dir_name) / std::filesystem::path(filename_buffer);
        std::string filename = filename_path.string();
#endif
        std::cout << "Starting reception of file: " << filename << " (Expected size: " << expected_size << " bytes)" << std::endl;

        std::ofstream outfile(filename, std::ios::binary);
        if (!outfile) {
            std::cerr << "Error creating file: " << filename << ". Error code: ";
#ifdef _WIN32
            std::cerr << GetLastError() << std::endl;
#else
            std::cerr << errno << std::endl;
#endif
            // If we can't create a file, we still have to accept the data,
            // so as not to spoil the flow, but we will simply discard them
        }

        // 3. Get the contents of the file by controlling the size
        long long total_received = 0;
        bool transfer_success = true;

        while (total_received < expected_size) {
            long long remaining = expected_size - total_received;
            // Determine how many bytes to read at a time (at least from BUFFER_SIZE and the remaining)
            size_t to_read = std::min((long long)BUFFER_SIZE, remaining);

            int bytes_received = recv(_client_socket, buffer, to_read, 0);

            if (bytes_received <= 0) {
                std::cerr << "Error: Connection closed prematurely during transfer of " << filename << ". Error code: ";
#ifdef _WIN32
                std::cerr << WSAGetLastError() << std::endl;
#else
                std::cerr << errno << std::endl;
#endif
                transfer_success = false;
                break;
            }

            if (outfile.is_open()) {
                outfile.write(buffer, bytes_received);
            }

            total_received += bytes_received;
        }

        if (outfile.is_open()) {
            outfile.close();
        }

        // Integrity check
        if (transfer_success && total_received == expected_size) {
            std::cout << "Successfully received file: " << filename << " (" << total_received << " bytes)." << std::endl;
        } else {
            std::cerr << "Transfer failed for " << filename << ". Received: " << total_received << ", Expected: " << expected_size << std::endl;
            // It would be possible to send the command "RESEND" to the client
            // But for the sake of simplicity in this example, we just log the error and move on to the next file (if there is one)
        }
    }

#ifdef _WIN32
    closesocket(_client_socket);
#else
    close(_client_socket);
#endif
}

int Server::run(int _argc, char* _argv[]) {
    if (_argc != 2) {
        std::cerr << "Usage: " << _argv[0] << " <port>" << std::endl;
        return 1;
    }

#ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }
#endif

    int port = std::stoi(_argv[1]); // port to int explicitly

#ifdef _WIN32
    SOCKET server_socket, client_socket; // socket descriptors (SOCKET on NT)
#else
    int server_socket, client_socket; // socket descriptors (int on Unix)
#endif

    struct sockaddr_in6 server_addr, client_addr; // Structures that know the addresses
    socklen_t client_addr_len = sizeof(client_addr); // Determine the length of the structure to run it across the network

    server_socket = socket(AF_INET6, SOCK_STREAM, 0); // SOCK_STREAM (TCP) the protocol of the three-way handshake, if the data does not reach the repeat, transfers the files
#ifdef _WIN32
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Error creating socket! Error code: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
#else
    if (server_socket < 0) {
        std::cerr << "Error creating socket!" << std::endl;
        return 1;
    }
#endif

    memset(&server_addr, 0, sizeof(server_addr)); // clear rubbish
    // установки ipv6
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port);
    server_addr.sin6_addr = in6addr_any;

#ifdef _WIN32
    // Setting the option to be able to reuse the address (useful for debugging)
    int opt = 1;
    //setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#else
#endif

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error binding socket! Error code: ";
#ifdef _WIN32
        std::cerr << WSAGetLastError() << std::endl;
        closesocket(server_socket);
        WSACleanup();
#else
        std::cerr << errno << std::endl;
        close(server_socket);
#endif
        return 1;
    }

    if (listen(server_socket, 5) < 0) { // 5 is the number of connections that are waiting
        std::cerr << "Error listening on socket! Error code: ";
#ifdef _WIN32
        std::cerr << WSAGetLastError() << std::endl;
        closesocket(server_socket);
        WSACleanup();
#else
        std::cerr << errno << std::endl;
        close(server_socket);
#endif
        return 1;
    }

    std::cout << "Server is listening on port " << port << "..." << std::endl;

    while (true) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);  // Picking up the client
#ifdef _WIN32
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Error accepting connection! Error code: " << WSAGetLastError() << std::endl;
            continue;
        }
#else
        if (client_socket < 0) {
            std::cerr << "Error accepting connection!" << std::endl;
            continue;
        }
#endif

        std::cout << "Client connected!" << std::endl;
        take(client_socket);
    }

#ifdef _WIN32
    closesocket(server_socket);
    WSACleanup();
#else
    close(server_socket);
#endif

    return 0;
}

// Auxiliary function for guaranteed reception of N bytes
// Returns the number of bytes successfully received, or SOCKET_ERROR/0
int Server::recv_all(
#ifdef _WIN32
    SOCKET
#else
    int
#endif
    _sock, void* _buffer, size_t _len) {
    size_t total_received = 0;
    char* buf = (char*)_buffer;
    while (total_received < _len) {
        int bytes_received = recv(_sock, buf + total_received, (int)(_len - total_received), 0); // Cast to int for Windows recv
        if (bytes_received <= 0) {
            return bytes_received; // Return 0 for closing, -1 (SOCKET_ERROR) for an error
        }
        total_received += bytes_received;
    }
    return (int)total_received; // Return the number of bytes received
}

void Server::whatDir(const std::string& _path) {
#ifdef _WIN32
    if (!std::experimental::filesystem::exists(_path)) {
        if (std::experimental::filesystem::create_directory(_path)) {
            std::cout << "Dir created: " << _path << std::endl;
        } else {
            std::cerr << "Error: dir not created: " << _path << std::endl;
        }
    }
#else
    if (!std::filesystem::exists(_path)) {
        if (std::filesystem::create_directory(_path)) {
            std::cout << "Dir created" << std::endl;
        } else { std::cout << "Err: dir not found" << std::endl; }
    }
#endif
}

// Will it come in handy? Hmm..
/*
std::string Server::charPtrToString(char* _cstr) {
    return std::string(_cstr);
}

char* Server::stringToCharPtr(const std::string& _str) {
    // THEN BE SURE TO FREE UP MEMORY!!
    char* cstr = new char[_str.length() + 1];
    std::strcpy(cstr, _str.c_str());
    return cstr;
}
*/
