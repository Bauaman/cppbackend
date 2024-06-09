#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <chrono>
#include <filesystem>
#include <iostream>
#include "aux.h"
#include "content.h"
#include "http_server.h"
#include "model.h"

//#include "logger.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;
namespace logging = boost::log;

using namespace std::literals;

boost::json::value PrepareOfficesForResponce(const model::Map& map);
boost::json::value PrepareBuildingsForResponce(const model::Map& map);
boost::json::value PrepareRoadsForResponse(const model::Map& map);
RequestData RequestParser(const std::string& req_target);

template<typename RequestHandler>
class LoggingRequestHandler {

using ResponseVariant = std::variant<http::response<http::string_body>, http::response<http::file_body>>;

public:

    LoggingRequestHandler(RequestHandler&& handler) :
        decorated_(std::forward<RequestHandler>(handler)) {
        }

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        std::chrono::high_resolution_clock timer;
        auto start = timer.now();
        LogRequest(req);
        
        std::string content_type = "null";
        int code_result;

        decorated_(std::move(req), [s = std::move(send), &content_type, &code_result](auto&& response) {
        
        code_result = response.result_int();
        content_type = static_cast<std::string>(response.at(http::field::content_type));
        s(response);
        });
        
        auto stop = timer.now();
        auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();

        LogResponse(deltaTime, code_result, content_type);
        return;
    }

private:
    RequestHandler& decorated_;

    template <typename Body, typename Allocator>
    void LogRequest(const http::request<Body, http::basic_fields<Allocator>>& req, const std::string& address_string) {
        // Здесь вы можете добавить логирование запроса
        //std::cout << "Received request: " << req.target() << std::endl;
        boost::json::object add_data;
        add_data["ip"] = address_string;
        add_data["URI"] = std::string(req.target());
        //beast::string_view method_sv = http::to_string(req.method());
        std::string method_str(req.method_string());
        add_data["method"] = method_str;
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << "request received";
    }

    template <typename Body, typename Fields>
    void LogResponse(const http::response<Body, Fields>& resp, const std::string& address_string, long long resp_time) {
        // Здесь вы можете добавить логирование ответа
        //std::cout << "Sending response with status code: " << resp.result_int() << std::endl;
        boost::json::object add_data;
        add_data["ip"] = address_string;
        add_data["response_time"] = resp_time;
        add_data["code"] = resp.result_int();
        auto content_type = resp.find(http::field::content_type);
        if (content_type != resp.end()) {
            add_data["content_type"] = std::string(content_type->value());
        } else {
            add_data["content_type"] = "null";
        }
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << "response sent";
    }

    void LogResponse(const ResponseVariant& response, const std::string& address_string, long long resp_time) {
        std::visit([this, address_string, resp_time](const auto& resp) {
            LogResponse(resp, address_string, resp_time);
        }, response);
    }

    template <typename Body, typename Allocator>
    ResponseVariant PrepareResponse(http::request<Body, http::basic_fields<Allocator>>&& req, const std::string& root_dir) {
        return decorated_(std::move(req), root_dir);
    }
};

class RequestHandler {

    using ResponseVariant = std::variant<http::response<http::string_body>, http::response<http::file_body>>;
    using SendFunction = std::function<void(ResponseVariant)>;
    using Strand = net::strand<net::io_context::executor_type>;

public:
    explicit RequestHandler(net::io_context ioc, Strand api_strand, model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;


    template <typename Body, typename Allocator, typename Send>
    ResponseVariant operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        auto data = req.body();
        RequestData req_;
        std::string req_target = std::string(req.target());
        ResponseVariant response_v;
        if ((req.method() == http::verb::get) || (req.method() == http::verb::head)) {
            try {
                req_ = RequestParser(req_target);
                if (req_.req_type == RequestType::API) {
                    response_v = HandleAPIRequest(/*std::forward<Send>(send),*/ req, req_);
                }
                if (req_.req_type == RequestType::STATIC_FILE) {
                    response_v = HandleStaticFileRequest(/*std::forward<Send>(send),*/ req, req_, root_dir);
                }
            } catch (std::logic_error& ex) {
                http::response<http::string_body> resp(http::status::bad_request, req.version());
                resp.set(http::field::content_type, ContentType::JSON);
                PrepareResponseBadRequestInvalidMethod(resp);
                resp.keep_alive(req.keep_alive());
                resp.prepare_payload();
                return resp;
            }
        } else {
            http::response<http::string_body> resp(http::status::method_not_allowed, req.version());
            resp.set(http::field::content_type, ContentType::JSON);
            PrepareResponseBadRequestInvalidMethod(resp);
            resp.keep_alive(req.keep_alive());
            resp.prepare_payload();
            return resp;
        }
        return response_v;
    }

private:
    model::Game& game_;

    boost::json::value PrepareAPIResponce(const std::string& req_,const model::Game& game_);

    void ReadStaticFile(const std::string& filepath, http::response<http::file_body>& response) {
        beast::error_code ec;
        http::file_body::value_type file;
        file.open(filepath.c_str(), beast::file_mode::read, ec);

        if (ec) {
            throw std::logic_error("Failed to open file: " + filepath);
        }
        
        response.body() = std::move(file);
        //response.set(http::field::content_type, ExtensionToContentType(filepath));
        response.content_length(response.body().size());
        response.keep_alive(true);
    }

    void PrepareResponseBadRequestInvalidMethod(http::response<http::string_body>& response) {
        if (response.result() == http::status::bad_request) {
            if (response.find(http::field::content_type)->value() == ContentType::JSON) {
                boost::json::value json_arr = {
                    {"code" , "badRequest"},
                    {"message" , "Bad Request"}
                };
                response.body() = boost::json::serialize(json_arr);
            }
            if (response.find(http::field::content_type)->value() == ContentType::PLAIN) {
                response.body() = "Bad Request: Requested file is outside of the root directory";
            }
            response.content_length(response.body().size());
        }
        if (response.result() == http::status::method_not_allowed) {
            response.body() = "Invalid method";
            response.content_length(response.body().size());
            response.set(http::field::allow, "GET, HEAD"sv);
        }
        
    }

    ResponseVariant HandleStaticFileRequest(const http::request<http::basic_string_body<char>>& req, const RequestData& req_data, const std::string& root_dir) {
        try {
            std::string normalized_path = fs::weakly_canonical(fs::path(req_data.req_data)).string();
            std::string filepath = root_dir + normalized_path;
            if (fs::is_directory(filepath)) {
                filepath = filepath + "index.html";
            }
            if (!auxillary::IsSubPath(fs::path(root_dir), filepath)) {
                http::response<http::string_body> response(http::status::bad_request, req.version());
                response.set(http::field::content_type, ContentType::PLAIN);
                PrepareResponseBadRequestInvalidMethod(response);
                response.keep_alive(req.keep_alive());
                response.prepare_payload();
                
                return response;
            } else {
                http::response<http::file_body> response(http::status::ok, req.version());
                ReadStaticFile(filepath, response);
                
                return response;
            }
        } catch (const std::exception& ex) {
            http::response<http::string_body> response(
            http::status::not_found, req.version());
            response.set(http::field::content_type, ContentType::PLAIN);
            response.keep_alive(req.keep_alive());
            response.body() = "File not found: " + req_data.req_data;
            response.content_length(response.body().size());
            response.prepare_payload();

            return response;
        }
    }

    http::response<http::string_body> HandleAPIRequest(const http::request<http::basic_string_body<char>>& req, const RequestData& req_data) {
        http::response<http::string_body> response(http::status::ok, req.version());
        boost::json::value response_body = PrepareAPIResponce(req_data.req_data, game_);
        if (response_body.is_object()) {
            if (response_body.as_object().find("code") != response_body.as_object().end() && 
                response_body.as_object().at("code") == "mapNotFound") {
                response.result(http::status::not_found);
            }
        }
        response.body() = boost::json::serialize(response_body);
        response.keep_alive(req.keep_alive());
        response.set(http::field::content_type, ContentType::JSON);
        response.content_length(response.body().size());
        response.prepare_payload();
        
        return response;
        
    }

};

}  // namespace http_handler


