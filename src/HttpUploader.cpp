#include <fstream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>

#include <arpa/inet.h>

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

// Function to upload a file via HTTP
bool uploadFile(const std::string& serverIP, int serverPort, const std::string& filePath, const std::string &failedDir) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in remoteaddr;
    remoteaddr.sin_family = AF_INET;
    remoteaddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
    remoteaddr.sin_port = htons(serverPort);

    bool err = false;
    if (connect(sockfd, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr)) < 0) {
        std::cout << "Unable to connect to server." << std::endl;
        err = true;
    }
    fs::path path(filePath);
    std::string fileName = path.filename();
    if (!err) {
        std::cout << "Connected" << std::endl;

        std::ifstream ifs(filePath, std::ios::binary);
        if (!ifs) {
            std::cout << "Unable to open file `" << filePath << "`." << std::endl;
            err = true;
        } else {
            std::vector<char> outputBuffer;
            char buffer[4096];
            while (!ifs.eof()) {
                ifs.read(buffer, sizeof(buffer));
                int bytesRead = ifs.gcount();  // Get the number of bytes read in this chunk
                if (bytesRead > 0) {
                    outputBuffer.insert(outputBuffer.end(), buffer, buffer + bytesRead);
                }
            }

            // Create an HTTP request.
            std::stringstream request;
            request << "POST /upload?file=" << fileName << " HTTP/1.1\r\n";
            request << "Host: localhost\r\n";
            request << "Content-Length: " << outputBuffer.size() << "\r\n";
            request << "Content-Type: application/x-binary\r\n";
            request << "\r\n";

            bool err = false;
            // Send the headers to the server.
            if (send(sockfd, request.str().c_str(), request.str().size(), 0) < 0) {
                std::cout << "Unable to send heaader request." << std::endl;
                err = true;
            }
            if (!err) {
                std::cout << "Sent header request" << std::endl;

                // Send the file content to the server.
                if (send(sockfd, static_cast<const char*>(outputBuffer.data()), outputBuffer.size(), 0) < 0) {
                    std::cerr << "Unable to send file contents." << std::endl;
                    err = true;
                }
            }
        }
    }
    close(sockfd);

    if (!err) {
        if (std::remove(filePath.c_str()) != 0) {
            std::cerr << "Error deleting the file: " << filePath << std::endl;
        }
        // TODO - rework to return status
        return true;
    } else {
        // Attempt to move the file
        if (std::rename(filePath.c_str(), (failedDir + "/" + fileName).c_str()) != 0) {
            std::cerr << "Error moving the file:" << filePath << std::endl;
        }
        return false;
    }

}

