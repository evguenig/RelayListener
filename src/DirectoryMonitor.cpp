#include <iostream>
#include <sys/inotify.h>
#include <unistd.h>
#include <cstring>

#include "DirectoryMonitor.h"

DirectoryMonitor::DirectoryMonitor(std::unique_ptr<Relay> relayPtr, std::string directory)
            : relayPtr(std::move(relayPtr)) , directory(std::move(directory)) {
        if (!this->relayPtr || this->directory.empty()) {
            throw std::invalid_argument("Can not initialize DirectoryMonitor.");
        }
    }

void DirectoryMonitor::monitor() {
    std::cout << " Starting monitoring directory: " << directory << std::endl;
    // Create an inotify FD.
    int fd = inotify_init();
    if (fd < 0) {
        std::cerr << "Error opening inotify file descriptor" << std::endl;
        return;
    }

    // Create a watch descriptor for the directory to be monitored.
    int watch_descriptor = inotify_add_watch(fd, directory.c_str(),IN_MODIFY | IN_CREATE | IN_DELETE);
    if (watch_descriptor < 0) {
        std::cerr << "Error creating watch descriptor" << std::endl;
        return;
    }

    // Read events from the inotify file descriptor.
    while (true) {
        // Read from the inotify file descriptor
        char buffer[1024];
        auto n = read(fd, buffer, sizeof(buffer));
        if (n < 0) {
            std::cerr << "Error reading from inotify file descriptor: " << strerror(errno) << std::endl;
            return;
        }

        // Iterate over all the events
        for (int i = 0; i < n; i++) {
            auto *event = (struct inotify_event *)&buffer[i];

            // Process each event (now - only creation as we need only creation for Relay)
            switch (event->mask) {
                case IN_CREATE:
                    std::cout << "File created: " << event->name << std::endl;
                    if (relayPtr) {
                        std::string file = event->name;
                        relayPtr->processFile(directory + "/" + file);
                    }
                    break;
                case IN_MODIFY:
                    std::cout << "File modified: " << event->name << std::endl;
                    break;
                case IN_DELETE:
                    std::cout << "File deleted: " << event->name << std::endl;
                    break;
            }
        }
    }
}
