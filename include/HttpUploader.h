#ifndef HTTP_FILE_UPLOAD_H
#define HTTP_FILE_UPLOAD_H

#include <string>

// Function to upload a file via HTTP
bool uploadFile(const std::string& serverIP, int serverPort, const std::string& filePath, const std::string &failedDir);

#endif // HTTP_FILE_UPLOAD_H
