RelayListener - monitor directory for new files creation and upload them to Http Server.

The key components of this application are:

DirectoryMonitor: Monitors a specified directory for file creations and passes them to the Relay component for processing.\
Relay: Manages a pool of worker threads that handle file uploads. It ensures efficient and parallel processing.\
ThreadPool: Provides a thread pool for concurrent file upload tasks.\
HttpUploader: Contains functions for uploading files to a remote server over HTTP.

For test purposes HttpServer is provided.

Getting Started:

Create build folder and cd to it. Run:

cmake ../CMakeLists.txt 

Execution: Run the compiled binary, providing the configuration file as an argument.

./RelayListener config.ini

The application continuously monitors the local directory specified in the configuration file.
When a new file is created in the directory, it's processed and uploaded to the target server over HTTP.
If the upload is successful, the file is deleted from the local directory.
If the upload fails, the file is moved to the "failed" directory for later inspection.

Dependencies:

C++ compiler (e.g., g++), c++20 is currently used.\
Standard C++ libraries\
POSIX threads (libpthread)
