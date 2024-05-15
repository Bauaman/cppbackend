#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <iostream>
#include "http_server.h"
#include "model.h"
#include <filesystem>
//#include <boost/filesystem.hpp>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;

using namespace std::literals;


enum class RequestType {
    API,
    STATIC_FILE
};

struct RequestData {
    RequestType req_type;
    std::string req_data;
};

boost::json::value PrepareOfficesForResponce(const model::Map& map);
boost::json::value PrepareBuildingsForResponce(const model::Map& map);
boost::json::value PrepareRoadsForResponse(const model::Map& map);
RequestData RequestParser(const std::string& req_target);

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
        constexpr static std::string_view CSS = "text/css"sv;
        constexpr static std::string_view PLAIN = "text/plain"sv;
        constexpr static std::string_view JAVASCRIPT = "text/javascript"sv;
        constexpr static std::string_view XML = "application/xml"sv;
        constexpr static std::string_view PNG = "image/png"sv;
        constexpr static std::string_view JPG = "image/jpeg"sv;
        constexpr static std::string_view GIF = "image/gif"sv;
        constexpr static std::string_view BMP = "image/bmp"sv;
        constexpr static std::string_view ICO = "image/vnd.microsoft.icon"sv;
        constexpr static std::string_view TIFF = "image/tiff"sv;
        constexpr static std::string_view SVG = "image/svg+xml"sv;
        constexpr static std::string_view MP3 = "audio/mpeg"sv;
        constexpr static std::string_view UNKNOWN = "application/octet-stream"sv;
    };

    bool IsSubPath(fs::path base, fs::path path) {
        path = fs::weakly_canonical(path);
        base = fs::weakly_canonical(base);

        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
            if (p == path.end() || *p != *b) {
                return false;
            }
        }
        return true;
    }

    std::string_view ExtensionToContentType(fs::path filepath) {
        std::string extension = filepath.extension().string();
        if (extension == ".htm" || extension == ".html")
                return ContentType::TEXT_HTML;
            else if (extension == ".json")
                return ContentType::JSON;
            else if (extension == ".css")
                return ContentType::CSS;
            else if (extension == ".txt")
                return ContentType::PLAIN;
            else if (extension == ".js")
                return ContentType::JAVASCRIPT;
            else if (extension == ".xml")
                return ContentType::XML;
            else if (extension == ".png")
                return ContentType::PNG;
            else if (extension == ".jpg" || extension == ".jpeg" || extension == ".jpe")
                return ContentType::JPG;
            else if (extension == ".gif")
                return ContentType::GIF;
            else if (extension == ".bmp")
                return ContentType::BMP;
            else if (extension == ".ico")
                return ContentType::ICO;
            else if (extension == ".tiff" || extension == ".tif")
                return ContentType::TIFF;
            else if (extension == ".svg" || extension == ".svgz")
                return ContentType::SVG;
            else if (extension == ".mp3")
                return ContentType::MP3;
            else
                return ContentType::UNKNOWN;
    }

    boost::json::value PrepareAPIResponce(const std::string& req_,const model::Game& game_);

    void ReadStaticFile(const std::string& filepath, http::response<http::file_body>& response) {
        beast::error_code ec;
        http::file_body::value_type file;
        file.open(filepath.c_str(), beast::file_mode::read, ec);

        if (ec) {
            throw std::logic_error("Failed to open file: " + filepath);
        }
        
        std::cout << "ExtensionToContentType: " << ExtensionToContentType(filepath) << std::endl;
        response.body() = std::move(file);
        response.set(http::field::content_type, ExtensionToContentType(filepath)/*ContentType::TEXT_HTML*/);
        response.content_length(response.body().size());
        response.keep_alive(true);
    }

    void PrepareResponseBadRequestInvalidMethod(/*http::status status, */http::response<http::string_body>& response) {
        //response.result(status);
        //if (status == http::status::bad_request) {
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
        //if (status == http::status::method_not_allowed) {
        if (response.result() == http::status::method_not_allowed) {
            response.body() = "Invalid method";
            response.content_length(response.body().size());
            response.set(http::field::allow, "GET, HEAD"sv);
        }
        
    }
    template <typename Send>
    void HandleStaticFileRequest(Send&& send, const http::request<http::basic_string_body<char>>& req, const RequestData& req_data, const std::string& root_dir) {
        try {
            std::string normalized_path = fs::weakly_canonical(fs::path(req_data.req_data)).string();
            std::cout << "normalized path: " << normalized_path << std::endl;
            std::string filepath = root_dir + normalized_path;
            std:: cout << "filepath: " << filepath << std::endl;
            if (fs::is_directory(filepath)) {
                filepath += "index.htm";
            }
            if (!IsSubPath(fs::path(root_dir), filepath)/*filepath.find(root_dir) != 0*/) {
                http::response<http::string_body> response(http::status::bad_request, req.version());
                response.set(http::field::content_type, ContentType::PLAIN);
                PrepareResponseBadRequestInvalidMethod(/*http::status::bad_request, */response);
                response.keep_alive(req.keep_alive());
                response.prepare_payload();
                send(std::move(response));
            } else {
                http::response<http::file_body> response(http::status::ok, req.version());
                ReadStaticFile(filepath, response);
                send(std::move(response));
            }
        } catch (const std::exception& ex) {
            http::response<http::string_body> response(
            http::status::not_found, req.version());
            response.set(http::field::content_type, ContentType::PLAIN);
            response.keep_alive(req.keep_alive());
            response.body() = "File not found: " + req_data.req_data;
            response.content_length(response.body().size());
            response.prepare_payload();

            send(std::move(response));
        }
    }

    template <typename Send>
    void HandleAPIRequest(Send&& send, const http::request<http::basic_string_body<char>>& req, const RequestData& req_data) {
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
        
        send(std::move(response));
    }

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, const std::string& root_dir) {
        // Обработать запрос request и отправить ответ, используя send
        auto data = req.body();
        RequestData req_;
        std::string req_target = std::string(req.target());
        if ((req.method() == http::verb::get) || (req.method() == http::verb::head)) {
            try {
                req_ = RequestParser(req_target);
                if (req_.req_type == RequestType::API) {
                    HandleAPIRequest(std::forward<Send>(send), req, req_);
                }
                if (req_.req_type == RequestType::STATIC_FILE) {
                    HandleStaticFileRequest(std::forward<Send>(send), req, req_, root_dir);
                }
                // Заменено на HandleAPIRequest
                /*
                boost::json::value response_body = PrepareResponce(req_.req_data, game_);
                status = http::status::ok;
                if (response_body.is_object()) {
                    if (response_body.as_object().find("code") != response_body.as_object().end() && 
                        response_body.as_object().at("code") == "mapNotFound") {
                        status = http::status::not_found;
                    }
                } else {
                    status = http::status::ok;
                }
                text = boost::json::serialize(response_body);*/
            } catch (std::logic_error& ex) {
                //Заменено на функцию PrepareResponseBadRequestInvalidMethod
                /*
                status = http::status::bad_request;
                boost::json::value jsonArr = {
                    {"code", "badRequest"},
                    {"message", "Bad request"}
                };
                text = boost::json::serialize(jsonArr);*/
                http::response<http::string_body> resp(http::status::bad_request, req.version());
                resp.set(http::field::content_type, ContentType::JSON);
                PrepareResponseBadRequestInvalidMethod(/*http::status::bad_request, */resp);
                resp.keep_alive(req.keep_alive());
                resp.prepare_payload();
                send(std::move(resp));
            }
        } else {
            /*
            status = http::status::method_not_allowed;
            text = "Invalid method"; */
            http::response<http::string_body> resp(http::status::method_not_allowed, req.version());
            resp.set(http::field::content_type, ContentType::JSON);
            PrepareResponseBadRequestInvalidMethod(/*http::status::method_not_allowed, */resp);
            resp.keep_alive(req.keep_alive());
            resp.prepare_payload();
            send(std::move(resp));
        }
        /*
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
        */
    }

private:
    model::Game& game_;
};


}  // namespace http_handler


