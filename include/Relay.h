#pragma once


#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <set>
#include "ThreadPool.h"

class Relay {
public:
    explicit Relay(int workers, std::string  host, int port, std::string path, std::string failedDir, int timeout);

    ~Relay() = default;

    void processFile(const std::string& filePath);

    static void
    upload(const std::string& serverIP, int serverPort, const std::string& filePath, const std::string& failedDir);

    void shutdown();

private:
    std::unique_ptr<ThreadPool> pool;
    std::string targetHost;
    int port;
    std::string path;
    std::string failedDir;
    bool shutdownStatus;

    std::vector<std::thread> workerThreads;
    std::condition_variable condition;
    std::queue<std::string> filesQueue;
    std::set<std::string> sentBundles;
};
