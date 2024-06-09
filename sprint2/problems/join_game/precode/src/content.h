#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

using namespace std::literals;

enum class RequestType {
    MAP,
    PLAYER,
    STATIC_FILE
};

struct RequestData {
    RequestType req_type;
    std::string req_data;
};

struct Errors {
    constexpr static std::string_view MAP_NOT_FOUND = R"({"code": "mapNotFound", "message": "Map not found"})"sv;
    constexpr static std::string_view BAD_REQ = R"({"code": "badRequest", "message": "Bad request"})"sv;
    constexpr static std::string_view PARSING_ERROR = R"({"code": "invalidArgument", "message": "Join request parsing failed"})"sv;
    constexpr static std::string_view USERNAME_EMPTY = R"({"code": "invalidArgument", "message": "Invalid name"})"sv;
    constexpr static std::string_view POST_INVALID = R"({"code": "invalidMethod", "message": "Only POST method is expected"})"sv;
    constexpr static std::string_view GET_INVALID = R"({"code": "invalidMethod", "message": "Only GET method is expected"})"sv;
    constexpr static std::string_view INVALID_METHOD = R"({"code": "invalidMethod", "message": "GET or HEAD method is expected"})"sv;
    constexpr static std::string_view AUTH_HEADER = R"({"code": "invalidToken", "message": "No authorization header in request"})"sv;
    constexpr static std::string_view BAD_TOKEN = R"({"code": "unknownToken", "message": "Player token has not been found"})"sv;
};

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view PLAIN = "text/plain"sv;
    constexpr static std::string_view JSON = "application/json"sv;
    constexpr static std::string_view MEDIA_UNKNOWN = "application/octet-stream"sv;
    static const std::unordered_map<std::string, std::string_view> DICT;
};

