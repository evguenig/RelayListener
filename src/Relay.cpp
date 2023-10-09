#include <iostream>
#include <fstream>
#include <utility>
#include <thread>
#include <map>

#include "Relay.h"
#include "HttpUploader.h"


Relay::Relay(int workers, std::string host, int port, std::string path, std::string failedDir, int timeout)
        : pool(std::make_unique<ThreadPool>(workers)), targetHost(std::move(host)), port(port), path(std::move(path)),
          failedDir(std::move(failedDir)),
          shutdownStatus(false) {
}

void Relay::upload(const std::string& serverIP, int serverPort, const std::string& filePath, const std::string &failedDir) {
    if (uploadFile(serverIP, serverPort, filePath, failedDir)) {
        std::cout << "File " << filePath << " uploaded successfully." << std::endl;
    } else {
        std::cout << "Failed to upload file: " << filePath << std::endl;
    }
}

void Relay::processFile(const std::string &filePath) {
    std::cout << "Processing: " + filePath << std::endl;
    auto f = [this, filePath]() { Relay::upload(this->targetHost, this->port, filePath, this->failedDir); };
    pool->enqueue(f);
}

void Relay::shutdown() {
    if (!shutdownStatus) {
        shutdownStatus = true;
        condition.notify_all();
        for (auto& thread : workerThreads) {
            thread.join();
        }
    }
}

