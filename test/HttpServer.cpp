#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <fstream>
#include <csignal>
#include <cstdlib>
#include <map>
#include <mutex>
#include <condition_variable>
#include <random>
#include <atomic>
#include <queue>
#include <filesystem>

std::atomic<bool> interrupted(false);

std::map<int, std::thread> threadMap;
std::mutex mapMutex;
std::condition_variable cv;
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<int> dis(1, 1000);

int serverSocket;

void signalHandler(int signum) {
    if (signum == SIGINT) {
        std::cout << "Termination signal (Ctrl+C) received." << std::endl;
        close(serverSocket);
        interrupted = true;
    }
}

struct SynchronizedQueue {
    std::queue<int> queue;
    std::mutex mutex;

    void push(int item) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(item);
    }

    std::vector<int> drainAll() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<int> items;
        while (!queue.empty()) {
            items.push_back(queue.front());
            queue.pop();
        }
        return items;
    }
} completedThreads;


void handleGET(int clientSocket) {
    // Just send an HTTP response for the GET request
    const char* response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "\r\n"
                           "OK from the server!";
    send(clientSocket, response, strlen(response), 0);
}

void handlePOST(int clientSocket, const std::string& outfile) {
    std::vector<char> outputBuffer;

    // Read data in a loop until the client closes the connection
    char buffer[1024];
    int bytesRead;
    bool error = false;
    while (true) {
        bytesRead = static_cast<int>(recv(clientSocket, buffer, sizeof(buffer), 0));
        if (bytesRead == -1) {
            std::cerr << "Error receiving data." << std::endl;
            error = true;
            break;
        } else if (bytesRead == 0) {
            // Connection closed by the client
            break;
        }
        outputBuffer.insert(outputBuffer.end(), buffer, buffer + bytesRead);
    }
    if (!error) {
        std::ofstream out(outfile, std::ios::binary);
        std::cout << "Writing to file: " << outfile << std::endl;

        out.write(static_cast<const char*>(outputBuffer.data()), outputBuffer.size());

        out.close();
    }

    std::string status = error ? "500 Internal Server Error" : "200 OK";
    std::string status_message = "HTTP/1.1 " + status + "\r\nContent-Type: text/plain\r\n\r\n";
    const char* response = status_message.c_str();

    send(clientSocket, response, strlen(response), 0);
}

void handleClient(int clientSocket, int threadId, const std::string& outdir) {
    std::cerr << "client socket=" << clientSocket << " threadId=" << threadId << std::endl;
    // Read the client's HTTP request/header
    char buffer[1024];
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead == -1) {
        std::cerr << "Error reading client request." << std::endl;
    } else {
        buffer[bytesRead] = '\0';

        if (strstr(buffer, "GET") != nullptr) {
            handleGET(clientSocket);
        } else if (strstr(buffer, "POST") != nullptr) {
            if (strstr(buffer, "/upload") == nullptr) {
                std::cout << "Invalid path" << std::endl;
                const char *response = "HTTP/1.1 404 Not Found\r\n"
                                       "Content-Type: text/plain\r\n"
                                       "\r\n"
                                       "Not Found";
                send(clientSocket, response, strlen(response), 0);
            } else {
                std::string file = "noname";
                const char* patternStart = "?file=";
                const char* patternEnd = "HTTP";
                std::string str(buffer);
                size_t startPos = str.find(patternStart);
                bool err = false;
                if (startPos == std::string::npos) {
                    std::cout << "Start pattern not found." << std::endl;
                    err = true;
                }
                if (!err) {
                    // Find the position of the [end] pattern, starting from the position after [start]
                    size_t endPos = str.find(patternEnd, startPos);
                    if (endPos == std::string::npos) {
                        std::cout << "End pattern not found." << std::endl;
                        err = true;
                    }
                    if (!err) {
                        // Calculate the length of the substring
                        size_t substringLength = endPos - (startPos + strlen(patternStart));
                        // Extract the substring between [start] and [end] patterns
                        std::string substring = str.substr(startPos + strlen(patternStart), substringLength);
                        // Trim trailing whitespace
                        size_t end = substring.find_last_not_of(" \t\n\r");
                        if (end != std::string::npos) {
                            file = substring.substr(0, end + 1);
                        } else {
                            err = true;
                        }
                    }
                }
                if (err) {
                    // TODO - implement sendErrResponse(clientSocket, retCode)
                    const char *response = "HTTP/1.1 403 Invalid Format\r\n"
                                           "Content-Type: text/plain\r\n"
                                           "\r\n"
                                           "Invalid Format";
                    send(clientSocket, response, strlen(response), 0);
                } else {
                    const char *postData = strstr(buffer, "\r\n\r\n");
                    if (postData != nullptr) {
                        postData += 4; // Move past the "\r\n\r\n" delimiter
                        handlePOST(clientSocket, outdir + "/" + file);
                    } else {
                        // Invalid message format
                        const char *response = "HTTP/1.1 403 Invalid Format\r\n"
                                               "Content-Type: text/plain\r\n"
                                               "\r\n"
                                               "Method Not Allowed";
                        send(clientSocket, response, strlen(response), 0);
                    }
                }
            }
        } else {
            // Unsupported HTTP method
            const char *response = "HTTP/1.1 405 Method Not Allowed\r\n"
                                   "Content-Type: text/plain\r\n"
                                   "\r\n"
                                   "Method Not Allowed";
            send(clientSocket, response, strlen(response), 0);
        }
    }

    close(clientSocket);

    // Remove the completed thread from the map
    std::cout << "threadMap: removing thread with threadId=" << threadId << std:: endl;
    std::cout << "threadMap entries: " << threadMap.size() << std::endl;
    completedThreads.push(threadId);
    cv.notify_all();
}

void mapWorker() {
    while (!interrupted) {
        {
            std::unique_lock<std::mutex> lock(mapMutex);
            auto res = cv.wait_for(lock, std::chrono::seconds(5));
            if (res == std::cv_status::timeout)
                continue;
        }

        auto completedThreadIds = completedThreads.drainAll();
        std::unique_lock<std::mutex> lock(mapMutex);
        for (auto threadId: completedThreadIds) {
            std::cout << "Removing thread: " << threadId << std::endl;
            threadMap[threadId].join();
            threadMap.erase(threadId);
        }
        lock.unlock();
    }
    // check remaining threads
}

namespace fs = std::filesystem;
bool checkFolder(const std::string& path) {
    bool status = false;
    if (fs::exists(path)) {
        if (fs::is_directory(path)) {
            fs::perms permissions = std::filesystem::status(path).permissions();
            if ((permissions & std::filesystem::perms::owner_write) == std::filesystem::perms::none) {
                std::cout << "The path \'" << path << "\'exists but is not writable." << std::endl;
            } else {
                std::cout << "Using path \'" << path <<"\' for storing received files" << std::endl;
                status = true;
            }
        } else {
            std::cout << "The path: \'" << path << "\' is not a path." << std::endl;
        }
    } else {
        std::cout << "The path: \'" << path << "\' does not exist." << std::endl;
    }

    return status;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <port> <outFolder>" << std::endl;
        return EXIT_FAILURE;
    }

    int port = std::stoi(argv[1]);

    const std::string outFolder(argv[2]);

    if (!checkFolder(outFolder)) {
        std::cerr << "Error: invalid folder \'" << outFolder << "\'."<< std::endl;
        return 1;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket." << std::endl;
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error binding to port." << std::endl;
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 10) == -1) {
        std::cerr << "Error listening on socket." << std::endl;
        close(serverSocket);
        return 1;
    }

    std::thread mapWorkerThread(mapWorker);

    signal(SIGIO, signalHandler);

    std::cout << "HTTP server is running on port: " << std::to_string(port) << std::endl;
    std::cout << "Press Ctrl+C to terminate." << std::endl;

    while (!interrupted) {
        sockaddr_in clientAddress{};
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);

        if (clientSocket == -1) {
            // Handle the case where accept was interrupted
            if (errno == EINTR && interrupted) {
                std::cout << "Accept was interrupted." << std::endl;
                break;
            } else {
                std::cerr << "Error accepting client connection." << std::endl;
                continue;
            }
        }

        int threadId = dis(gen);

        std::thread clientThread(handleClient, clientSocket, threadId, outFolder);

        // Add the thread to the map
        std::unique_lock<std::mutex> lock(mapMutex);
        threadMap[threadId] = std::move(clientThread);
        lock.unlock();
    }

    std::cout << "Terminating ..." << std::endl;

    mapWorkerThread.join();

    return 0;
}
