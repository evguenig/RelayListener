#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <boost/program_options.hpp>
#include <memory>
#include <map>

#include "Relay.h"
#include "DirectoryMonitor.h"

using Config = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

std::string& trim(std::string& s, const char* t = " \t\n\r") {
    s.erase(0, s.find_first_not_of(t));
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// Config from INI file
Config getConfig(const std::string& filename) {
    Config config;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return config;
    }

    std::string currentSection;
    std::string line;
    while (std::getline(file, line)) {
        trim(line);

        if (line.empty() || line[0] == ';') { // skip empty lines and comments
            continue;
        } else if (line[0] == '[' && line.back() == ']') {
            // Found a section
            currentSection = line.substr(1, line.size() - 2);
        } else {
            // Found a key-value pair
            size_t separatorPos = line.find('=');
            if (separatorPos != std::string::npos) {
                std::string key = line.substr(0, separatorPos);
                trim(key);
                std::string value = line.substr(separatorPos + 1);
                trim(value);

                config[currentSection][key] = value;
            }
        }
    }

    return config;
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <configFile>" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        Config config = getConfig(std::string(argv[1]));

        if (config.empty()) {
            std::cerr << "No config data - exiting..." <<std::endl;
            return EXIT_FAILURE;
        }

        std::string filesDir = config["local"]["filesDir"];
        std::string failedDir = config["local"]["failedDir"];
        int workers = std::stoi(config["local"]["workers"]);
        int timeout = std::stoi(config["local"]["timeout"]);

        std::string host = config["target"]["host"];
        int port = std::stoi(config["target"]["port"]);
        std::string path = config["target"]["path"];


        DirectoryMonitor directoryMonitor(
                std::make_unique<Relay>(workers, host, port, path, failedDir, timeout),
                filesDir);
        directoryMonitor.monitor();
    } catch (const std::invalid_argument &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
