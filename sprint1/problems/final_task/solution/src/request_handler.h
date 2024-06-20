#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW
#include <iostream>
#include "http_server.h"
#include "model.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

using namespace std::literals;

boost::json::value PrepareOfficesForResponce(const model::Map& map);
boost::json::value PrepareBuildingsForResponce(const model::Map& map);
boost::json::value PrepareRoadsForResponse(const model::Map& map);
std::string RequestParser(const std::string& req_target);

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view JSON = "application/json"sv;
    };

    boost::json::value PrepareResponce(const std::string& req_,const model::Game& game_);

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        auto data = req.body();
        http::status status;
        std::string text;
        std::string req_;
        std::string req_target = std::string(req.target());
        if ((req.method() == http::verb::get) || (req.method() == http::verb::head)) {
            try {
                req_ = RequestParser(req_target);
                boost::json::value response_body = PrepareResponce(req_, game_);
                status = http::status::ok;
                if (response_body.is_object()) {
                    if (response_body.as_object().find("code") != response_body.as_object().end() && 
                        response_body.as_object().at("code") == "mapNotFound") {
                        status = http::status::not_found;
                    }
                } else {
                    status = http::status::ok;
                }
                text = boost::json::serialize(response_body);
            } catch (std::logic_error& ex) {
                status = http::status::bad_request;
                boost::json::value jsonArr = {
                    {"code", "badRequest"},
                    {"message", "Bad request"}
                };
                text = boost::json::serialize(jsonArr);
            }
        } else {
            status = http::status::method_not_allowed;
            text = "Invalid method";
        }
        http::response<http::string_body> response(status, req.version());
        if (status == http::status::method_not_allowed) {
            response.set(http::field::allow, "GET, HEAD"sv);
        }
        response.set(http::field::content_type, ContentType::JSON);
        response.body() = text;
        response.content_length(text.size());
        response.keep_alive(req.keep_alive());
        response.prepare_payload();
        
        send(std::move(response));
    }

private:
    model::Game& game_;
};


}  // namespace http_handler


