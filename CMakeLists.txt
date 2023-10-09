cmake_minimum_required(VERSION 3.25)
project(RelayListener)

message(
       	"Top level CMakeLists.txt"
        LANGUAGES CXX)

set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_STANDARD 20)

add_executable(RelayListener src/main.cpp
        src/DirectoryMonitor.cpp
		src/HttpUploader.cpp
		src/Relay.cpp
		src/ThreadPool.cpp
		)

target_include_directories(RelayListener
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)


add_executable(TestServer test/HttpServer.cpp)

