#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <sys/inotify.h>
#include <unistd.h>
#include <cstring>

#include "Relay.h"

class DirectoryMonitor {
public:
    DirectoryMonitor(std::unique_ptr<Relay> relayPtr, std::string directory);

    void monitor();

private:
    std::unique_ptr<Relay> relayPtr;
    const std::string directory;
};
