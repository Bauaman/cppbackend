#include "request_handler.h"

namespace http_handler {

RequestData RequestParser(const std::string& req_target) {
    if (req_target.starts_with("/api")) {
        if (req_target.starts_with("/api/v1/")) {
            std::string req_ = req_target.substr(8);
            size_t next_slash_pos = req_.find('/');
            if (next_slash_pos != req_.length() && next_slash_pos != std::string::npos) {
                if (req_.starts_with("maps/")) {
                    req_ = req_.substr(next_slash_pos+1);
                    if (req_.find('/') == std::string::npos) {
                        return {RequestType::API, std::string(req_)};
                    } else {
                        throw std::logic_error("Invalid request (/api/v1/maps/id/?)"s);
                    }
                }
                if (req_.starts_with("game/")) {
                    req_ = req_.substr(next_slash_pos+1); 
                    if (req_.find('/') == std::string::npos) {
                        return {RequestType::PLAYER, std::string(req_)};
                    } else if (req_.starts_with("player/")) {
                        next_slash_pos = req_.find('/');
                        req_ = req_.substr(next_slash_pos+1);
                        return {RequestType::PLAYER, std::string(req_)};
                    } else {
                        throw std::logic_error("Invalid request (/api/v1/game/***/?)"s);
                    }
                }
            } else {
                if (req_ == "maps") {
                    return {RequestType::API,std::string(req_)};
                } else {
                    throw std::logic_error("Invalid request (/api/v1/?)"s);
                }
            }
        } else {
            throw std::logic_error("Invalid request (/api/?)"s);
        }
    } else {
        return {RequestType::FILE, req_target};
    }
    throw std::logic_error("Invalid request (unhandled case)"s);
}

std::string toString(RequestType type) {
    switch (type) {
        case RequestType::API: return "API";
        case RequestType::PLAYER: return "PLAYER";
        case RequestType::FILE: return "FILE";
        default: return "UNKNOWN";
    }
}

}