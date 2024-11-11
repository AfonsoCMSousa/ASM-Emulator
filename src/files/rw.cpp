#include "rw.h"
#include <vector>
#include <string>
#include <iostream>
#include <cstring>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#else
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#define PATH_MAX 1024
#endif

#define BUFFER_SIZE 1024

// Function to open the file
intptr_t open_file(const char* filename) {

#ifdef _WIN32
    HANDLE hFile = CreateFileA(
        filename,    // file name with .s extension
        GENERIC_READ,            // open for reading
        0,                       // do not share
        NULL,                    // default security
        OPEN_EXISTING,           // open only if the file exists
        FILE_ATTRIBUTE_NORMAL,   // normal file
        NULL);                   // no attr. template
    
    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error_code = GetLastError();
        std::cerr << "Error opening file on Windows: " << error_code << std::endl;
        return -1;
    }
    return (intptr_t)hFile;
#else
#ifdef __APPLE__
    // macOS-specific way to get the executable path
    char result[PATH_MAX];
    ssize_t count = -1;
    uint32_t size = PATH_MAX;
    if (_NSGetExecutablePath(result, &size) != 0) {
        return -2;
    }
    count = strlen(result);
#else
    // Linux-specific way to get the executable path
    char result[PATH_MAX];
    ssize_t count = readlink("prco/self/exe", result, PATH_MAX);
    count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count == -1) {
        return -3;
    }
#endif

    // Find the directory of the executable
    std::string exePath = std::string(result, count);
    std::string exeDir = exePath.substr(0, exePath.find_last_of("/"));

    // Construct the full path to the file
    std::string fullPath = exeDir + "/" + filename;
    
    // Open the file in read-only mode
    int fd = open(fullPath.c_str(), O_RDONLY);
    if (fd == -1) {
        return -1;
    }

    return fd;
#endif
}

// Function to close the file
void close_file(intptr_t fd)
{
#ifdef _WIN32
    CloseHandle((HANDLE)fd);
#else
    close(fd);
#endif
}

// Function to read the file line by line
std::vector<std::string> read_lines(const char *filename)
{
    std::vector<std::string> lines;
    char buffer[BUFFER_SIZE];
    std::string current_line;

    intptr_t fd = open_file(filename);
    if (fd == -1)
    {
        throw std::runtime_error("Error: Unable to open file " + std::string(filename));
    }

#ifdef _WIN32
    DWORD bytes_read;
    while (ReadFile((HANDLE)fd, buffer, BUFFER_SIZE, &bytes_read, NULL))
    {
        if (bytes_read == 0)
        {
            break;
        }
        current_line.append(buffer, bytes_read);
        size_t pos;
        while ((pos = current_line.find('\n')) != std::string::npos)
        {
            lines.push_back(current_line.substr(0, pos));
            current_line.erase(0, pos + 1);
        }
    }
    if (GetLastError() != ERROR_SUCCESS && GetLastError() != ERROR_HANDLE_EOF)
    {
        close_file(fd);
        throw std::runtime_error("Error: Failed to read from file " + std::string(filename));
    }
#else
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0)
    {
        current_line.append(buffer, bytes_read);
        size_t pos;
        while ((pos = current_line.find('\n')) != std::string::npos)
        { 
            lines.push_back(current_line.substr(0, pos));
            current_line.erase(0, pos + 1);
        }
    }
    if (bytes_read == -1)
    {
        close_file(fd);
        throw std::runtime_error("Error: Failed to read from file " + std::string(filename));
    }
#endif

    close_file(fd);

    if (lines.empty())
    {
        throw std::runtime_error("Error: File " + std::string(filename) + " is empty or contains no valid lines");
    }

    return lines;
}