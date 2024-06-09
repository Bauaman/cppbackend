#include "aux.h"

namespace auxillary {

std::string UrlDecode(const std::string& str) {
    std::ostringstream decoded;
    std::istringstream encoded(str);
    encoded >> std::noskipws;

    char current;
    while (encoded >> current) {
        if (current == '%') {
            int hexCode;
            if (!(encoded >> std::hex >> hexCode)) {
                decoded << '%';
            } else {
                decoded << static_cast<char>(hexCode);
            }
        } else if (current == '+') {
            decoded << ' ';
        } else {
            decoded << current;
        }
    }
    return decoded.str();
}

bool IsSubPath(fs::path base, fs::path path) {
    fs::path combined_path = base / path;
    fs::path canonical_path = fs::weakly_canonical(path);

    return canonical_path.string().find(base.string()) == 0;
}

}