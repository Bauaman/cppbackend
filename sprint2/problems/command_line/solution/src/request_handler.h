#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/json.hpp>
#include <filesystem>
#include <chrono>
#include <variant>

#include "api_handler.h"
#include "http_server.h"

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace net = boost::asio;
namespace sys = boost::system;
using tcp = net::ip::tcp;

using ResponseVariant = std::variant<http::response<http::string_body>, http::response<http::file_body>>;

RequestData RequestParser(const std::string& req_target);
std::string toString(RequestType type);

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:
    explicit RequestHandler(net::io_context& ioc, GameServer& gs, net::strand<net::io_context::executor_type> api_strand) :
        ioc_(ioc),
        gs_(gs),
        strand_(api_strand),
        api_handler_(std::make_shared<ApiHandler>(gs)) {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {  
        try {
            std::string req_target = auxillary::UrlDecode(std::string(req.target()));
            RequestData r_data = RequestParser(req_target);
            auto version = req.version();
            auto keep_alive = req.keep_alive();
            if (r_data.type != RequestType::FILE /*запрос к API*/) {

                auto handle = [self = shared_from_this(), send,
                               req = std::forward<decltype(req)>(req), version, keep_alive, r_data] {
                    try {
                        assert(self->strand_.running_in_this_thread());
                        return send(self->api_handler_->HandleRequest(req, r_data));
                    } catch (...) {
                        json::object ex_resp {{"code", "badRequest"},
                                              {"message", "test"}};
                        send(MakeResponse(http::status::bad_request, json::serialize(ex_resp), version, keep_alive, ContentType::JSON));
                    }
                };

                return boost::asio::dispatch(strand_, handle);
            }
            
            return std::visit([&send](auto&& result) {
                            send(std::forward<decltype(result)>(result));
                            }, HandleFileRequest(req, r_data));
        } catch (const std::exception& ex) {
            std::string str(ex.what());
            json::object ex_resp {{"code", "badRequest"},
                                  {"message", str}};
            send(MakeResponse(http::status::bad_request, json::serialize(ex_resp), req.version(), req.keep_alive(), ContentType::JSON));
        }
    }

    template <typename Body, typename Allocator>
    ResponseVariant HandleFileRequest(http::request<Body, http::basic_fields<Allocator>>& req, RequestData& rd) {
        fs::path root = gs_.GetRootDir();
        fs::path req_path = fs::path(rd.r_target);
        fs::path filepath = fs::path(root.string() + "/" + req_path.string());
        if (fs::is_directory(filepath)) {
            filepath = filepath / "index.html";
        }
        if (!auxillary::IsSubPath(root, filepath)) {
            return MakeResponse(http::status::bad_request, "Bad Request: Requested file is outside of the root directory"sv, req.version(), req.keep_alive(), ContentType::PLAIN);
        }
        if (!fs::exists(filepath) || !fs::is_regular_file(filepath)) {
            return MakeResponse(http::status::not_found, "Bad Request: Requested file not found"sv, req.version(), req.keep_alive(), ContentType::PLAIN);
        }

        http::file_body::value_type file;
        if (boost::system::error_code ec; file.open(filepath.c_str(), boost::beast::file_mode::read, ec), ec) {
            throw std::logic_error("Wrong open file "s.append(ec.message()));
        }

        std::string extension = filepath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });
        std::string_view content_type = ContentType::DICT.count(extension) ? ContentType::DICT.at(extension) : ContentType::UNKNOWN;
        return MakeResponse(http::status::ok, file, req.version(), req.keep_alive(), content_type);

    }
    

private:
    net::io_context& ioc_;
    GameServer& gs_;
    net::strand<net::io_context::executor_type> strand_;
    std::shared_ptr<ApiHandler> api_handler_;

};

template <typename RequestHandler>
class LoggingRequestHandler {
public:
    LoggingRequestHandler(RequestHandler& handler) :
        decorated_(handler) {}
    
    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {

        std::chrono::high_resolution_clock timer;
        auto start_time = timer.now();

        auto uri = LogRequest(req);

        decorated_(std::move(req), [s = std::move(send), start_time, uri](auto&& response){
            std::string content_type = "null";
            auto code_result = response.result_int();
            if (response.find(http::field::content_type) != response.end()) {
                content_type = std::string(response.at(http::field::content_type));
            }
            s(response);
            auto stop_time = std::chrono::high_resolution_clock::now();
            auto dtime = std::chrono::duration_cast<std::chrono::microseconds>(stop_time - start_time).count();
            LogResponse(dtime, code_result, content_type, uri);
        });
        return;
    }

private:
    RequestHandler& decorated_;

    template <typename Body, typename Allocator>
    std::string LogRequest(http::request<Body, http::basic_fields<Allocator>>& req) {
        std::string host = static_cast<std::string>(req.at(http::field::host));
        host = host.substr(0, host.rfind(':'));

        boost::json::object obj{
            {"ip", host},
            {"URI", req.target()},
            {"method", req.method_string()}};
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, obj) << "request received"sv;
        std::string str(req.target());
        return str;
    }

    static void LogResponse(int delta, int code, std::string content, std::string uri) {
        if(content.empty()){
        content = "null";
        }
        boost::json::object obj;
        obj = {
            {"uri", uri},
            {"response_time", delta},
            {"code", code},
            {"content_type", content}};
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, obj) << "response sent"sv;
    }
};

}