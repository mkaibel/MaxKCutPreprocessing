//
// Created by michaelk on 18.11.25.
//

#ifndef MKCP_IO_HPP
#define MKCP_IO_HPP

#include <string>
#include <filesystem>

inline std::string getFilename(const std::string& path) {
    std::filesystem::path file_path(path);
    return file_path.filename().string();
}

#endif //MKCP_IO_HPP