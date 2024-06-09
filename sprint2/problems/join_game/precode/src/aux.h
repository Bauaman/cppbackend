#pragma once

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace auxillary {

std::string UrlDecode(const std::string& str);
bool IsSubPath(fs::path base, fs::path path);

}