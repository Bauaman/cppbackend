#pragma once

#include "content.h"
#include "aux.h"
#include "http_server.h"
#include "model.h"

#include <filesystem>

namespace http_handler {

namespace net = boost::asio;
using tcp = net::ip::tcp;    
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;
namespace logging = boost::log;

template <typename Body, typename Allocator, typename Send>
class ApiHandler {
public:
    ApiHandler(http::request<Body, http::basic_fields<Allocator>>& req, Send& send, model::GameServer& server, const RequestData& r_data) :
        req_{req},
        send_(std::move(send)),
        game_server_(game_server_),
        req_data_(r_data) {
    }

private:
http::request<Body, http::basic_fields<Allocator>>& req_;
Send send_;
model::GameServer& game_server_;
RequestData& req_data_;

};

} //namespace http_handler