#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <optional>

#include "aux.h"
#include "game_server.h"
#include "response_maker.h"
#include "tagged.h"

namespace json = boost::json;
namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

namespace http_handler {

boost::json::value PrepareRoadsForResponse(const model::Map& map);
boost::json::value PrepareBuildingsForResponce(const model::Map& map);
boost::json::value PrepareOfficesForResponce(const model::Map& map);

template <typename Body, typename Allocator, typename Send>
class ApiHandler {
public:
    ApiHandler(http::request<Body, http::basic_fields<Allocator>>& req, Send& send, GameServer& gs, RequestData& rd) :
        req_(req),
        send_(send),
        gs_(gs),
        r_data_(rd) {}

    void HandleRequest() {
        unsigned version = req_.version();
        bool keep_alive = req_.keep_alive();
        try {
            if (r_data_.type == RequestType::API) {
                HandleMapRequest(version, keep_alive);
            }
            if (r_data_.type == RequestType::PLAYER) {
                if (r_data_.r_target == "players") {
                    HandlePlayersListRequest(version, keep_alive);
                } else if (r_data_.r_target == "join") {
                    HandlePlayerJoinRequest(version, keep_alive);
                } else if (r_data_.r_target == "state") {
                    HandleStateRequest(version, keep_alive);
                } else {
                    send_(MakeResponse(http::status::bad_request, Errors::BAD_REQ, req_.version(), req_.keep_alive(), ContentType::JSON));
                }

            }
        } catch (const std::exception& ex) {
            send_(MakeResponse(http::status::internal_server_error, "server error"sv, req_.version(), req_.keep_alive(), ContentType::HTML));
        }
        
    }

    void HandleMapRequest(unsigned version, bool keep_alive) {
        json::value message;
        if (req_.method() != http::verb::get) {
            send_(MakeResponse(http::status::method_not_allowed, Errors::GET_INVALID, version, keep_alive, ContentType::JSON, ""sv, "GET"sv));
            return;
        }
        
        if (r_data_.r_target.empty()) {
            send_(MakeResponse(http::status::bad_request, Errors::BAD_REQ, version, keep_alive, ContentType::JSON));
            return;
        }
        if (r_data_.r_target == "maps") {
            json::array array;
            for (auto& map : gs_.GetMaps()) {
                json::value val = {{"id",*map.GetId()}, {"name", map.GetName()}};
                array.push_back(val);
            }
            message = array;
            send_(MakeResponse(http::status::ok, json::serialize(message), version, keep_alive, ContentType::JSON));
            return;
        } else {
            json::object resp_message;
            model::Map::Id id_{r_data_.r_target};
            const model::Map* map = gs_.FindMap(id_);
            if (map != nullptr) {
                std::vector<std::string> keys_in_map = map->GetKeys();
                for (const auto& key : keys_in_map) {
                    if (key == "id") {
                        resp_message["id"] = *map->GetId();
                    } else if (key == "name") {
                        resp_message["name"] = map->GetName();
                    } else if (key == "roads") {
                        boost::json::array roads = PrepareRoadsForResponse(*map).as_array();
                        resp_message["roads"] = roads;
                    } else if(key == "buildings") {
                        boost::json::array buildings = PrepareBuildingsForResponce(*map).as_array();
                        resp_message["buildings"] = buildings;
                    } else if (key == "offices") {
                        boost::json::array offices = PrepareOfficesForResponce(*map).as_array();
                        resp_message["offices"] = offices;            
                    }
                }
                message = resp_message;
                send_(MakeResponse(http::status::ok, json::serialize(message), version, keep_alive, ContentType::JSON));
                return;
            } else {
                send_(MakeResponse(http::status::not_found, Errors::MAP_NOT_FOUND, version, keep_alive, ContentType::JSON));
                return;
            }
        }
    }

    void HandlePlayerJoinRequest(unsigned version, bool keep_alive) {
        if (req_.method() != http::verb::post) {
            send_(MakeResponse(http::status::method_not_allowed, Errors::POST_INVALID, version, keep_alive, ContentType::JSON, "no-cache"sv, "POST"sv));
            return;
        }
        std::string user_name;
        std::string map_id;
        try {
            boost::json::value parsed_req = boost::json::parse(req_.body());
            if (parsed_req.as_object().find("userName") == parsed_req.as_object().end() || parsed_req.as_object().at("userName").as_string().empty()) {
                send_(MakeResponse(http::status::bad_request, Errors::USERNAME_EMPTY, version, keep_alive, ContentType::JSON, "no-cache"sv));
                return;
            }
            user_name = parsed_req.as_object().at("userName").as_string();
            map_id = parsed_req.as_object().at("mapId").as_string();
        } catch (...) {
            send_(MakeResponse(http::status::bad_request, Errors::PARSING_ERROR, version, keep_alive, ContentType::JSON, "no-cache"sv));
            return;
        }
        
        model::Map::Id mapId(map_id);
        auto map = gs_.FindMap(mapId);
        if (map == nullptr) {
            send_(MakeResponse(http::status::not_found, Errors::MAP_NOT_FOUND, version, keep_alive, ContentType::JSON, "no-cache"sv));
        }

        boost::json::object resp;
        try {
            const model::Player& player = gs_.JoinGame(model::Map::Id(mapId), user_name);
            resp = {{"authToken", *player.GetPlayerToken()},
                    {"playerId", player.GetId()}};
        } catch (const std::exception& ex) {
            send_(MakeResponse(http::status::internal_server_error, "Join game failed: "s + ex.what(), version, keep_alive, ContentType::HTML));
            return;
        }
        send_(MakeResponse(http::status::ok, boost::json::serialize(resp), version, keep_alive, ContentType::JSON, "no-cache"sv));
        return;
    }

    void HandlePlayersListRequest(unsigned version, bool keep_alive) {
        if (req_.method() != http::verb::get && req_.method() != http::verb::head) {
            send_(MakeResponse(http::status::method_not_allowed, Errors::INVALID_METHOD, version, keep_alive, ContentType::JSON, "no-cache"sv, "GET, HEAD"sv));
            return;
        } 

        const model::Player* player = AuthorizationCheck(version, keep_alive);
        /*
        std::string authorization;
        if (req_.count(http::field::authorization)) {
            authorization = req_.at(http::field::authorization);
        } else {
            send_(MakeResponse(http::status::unauthorized, Errors::AUTH_HEADER, version, keep_alive, ContentType::JSON, "no-cache"sv));
            return;
        }

        const std::string auth_prefix = "Bearer ";
        if (authorization.starts_with(auth_prefix)) {
            authorization = authorization.substr(auth_prefix.size());
        } else {
            send_(MakeResponse(http::status::unauthorized, Errors::INVALID_HEADER, version, keep_alive, ContentType::JSON, "no-cache"sv));
        }

        std::transform(authorization.begin(), authorization.end(), authorization.begin(), 
                        [](unsigned char c) {
                            return std::tolower(c);
                        });
        if (authorization.size() != 32 || authorization.find_first_not_of("0123456789abcdef") != std::string::npos) {
            send_(MakeResponse(http::status::unauthorized, Errors::INVALID_TOKEN, version, keep_alive, ContentType::JSON, "no-cache"sv));
            return;
        }
        const model::Player* player = gs_.FindPlayer(model::Token(authorization));
        if (player == nullptr) {
            send_(MakeResponse(http::status::unauthorized, Errors::UNKNOWN_TOKEN, version, keep_alive, ContentType::JSON, "no-cache"sv));
            return;
        }*/
        if (player == nullptr) {
            return;
        } else {
            boost::json::object resp;
            for (auto pl : gs_.GetPlayers()) {
                resp[std::to_string(pl.second.GetId())] = boost::json::object{{"name", pl.second.GetName()}};
            }
            send_(MakeResponse(http::status::ok, boost::json::serialize(resp), version, keep_alive, ContentType::JSON, "no-cache"sv));
            return;
        }
    }

    void HandleStateRequest(unsigned version, bool keep_alive) {
        if (req_.method() != http::verb::get && req_.method() != http::verb::head) {
            send_(MakeResponse(http::status::method_not_allowed, Errors::INVALID_METHOD, version, keep_alive, ContentType::JSON, "no-cache"sv, "GET, HEAD"sv));
            return;
        }

        const model::Player* player = AuthorizationCheck(version, keep_alive);
        /*
       //================================================================================================================
       std::string authorization;
        if (req_.count(http::field::authorization)) {
            authorization = req_.at(http::field::authorization);
        } else {
            send_(MakeResponse(http::status::unauthorized, Errors::AUTH_HEADER, version, keep_alive, ContentType::JSON, "no-cache"sv));
            return;
        }

        const std::string auth_prefix = "Bearer ";
        if (authorization.starts_with(auth_prefix)) {
            authorization = authorization.substr(auth_prefix.size());
        } else {
            send_(MakeResponse(http::status::unauthorized, Errors::INVALID_HEADER, version, keep_alive, ContentType::JSON, "no-cache"sv));
        }

        std::transform(authorization.begin(), authorization.end(), authorization.begin(), 
                        [](unsigned char c) {
                            return std::tolower(c);
                        });
        if (authorization.size() != 32 || authorization.find_first_not_of("0123456789abcdef") != std::string::npos) {
            send_(MakeResponse(http::status::unauthorized, Errors::INVALID_TOKEN, version, keep_alive, ContentType::JSON, "no-cache"sv));
            return;
        }
        const model::Player* player = gs_.FindPlayer(model::Token(authorization));
        if (player == nullptr) {
            send_(MakeResponse(http::status::unauthorized, Errors::UNKNOWN_TOKEN, version, keep_alive, ContentType::JSON, "no-cache"sv));
            return;
        }
        //===============================================================================================================
        */
        if (player == nullptr) {
            return;
        } else {
            boost::json::object resp;
            for (auto pl : gs_.GetPlayers()) {
                if (pl.second.GetPlayersSession() == player->GetPlayersSession()) {
                    resp[std::to_string(pl.second.GetId())] = {
                        {"pos", {(double)pl.second.GetDog()->GetDogPosition().x_, (double)pl.second.GetDog()->GetDogPosition().y_}},
                        {"speed", {(double)pl.second.GetDog()->GetDogSpeed().x_, (double)pl.second.GetDog()->GetDogSpeed().y_}},
                        {"dir", pl.second.GetDog()->GetDogDirection()} 
                    };
                }
            }
            
            boost::json::object resp_message {{"players", resp}};
            send_(MakeResponse(http::status::ok, json::serialize(resp_message), version, keep_alive, ContentType::JSON, "no-cache"sv));
            return;
        }
    }


private:
    const http::request<Body, http::basic_fields<Allocator>>& req_;
    Send& send_;
    GameServer& gs_;
    RequestData& r_data_;

    const model::Player* AuthorizationCheck(unsigned version, bool keep_alive) {
        std::string authorization;
        if (req_.count(http::field::authorization)) {
            authorization = req_.at(http::field::authorization);
        } else {
            send_(MakeResponse(http::status::unauthorized, Errors::AUTH_HEADER, version, keep_alive, ContentType::JSON, "no-cache"sv));
            return nullptr;
        }

        const std::string auth_prefix = "Bearer ";
        if (authorization.starts_with(auth_prefix)) {
            authorization = authorization.substr(auth_prefix.size());
        } else {
            send_(MakeResponse(http::status::unauthorized, Errors::INVALID_HEADER, version, keep_alive, ContentType::JSON, "no-cache"sv));
            return nullptr;
        }
        std::transform(authorization.begin(), authorization.end(), authorization.begin(), 
                [](unsigned char c) {
                    return std::tolower(c);
                });
        if (authorization.size() != 32 || authorization.find_first_not_of("0123456789abcdef") != std::string::npos) {
            send_(MakeResponse(http::status::unauthorized, Errors::INVALID_TOKEN, version, keep_alive, ContentType::JSON, "no-cache"sv));
            return nullptr;
        }
        const model::Player* player = gs_.FindPlayer(model::Token(authorization));
        if (player == nullptr) {
            send_(MakeResponse(http::status::unauthorized, Errors::UNKNOWN_TOKEN, version, keep_alive, ContentType::JSON, "no-cache"sv));
            return nullptr;
        }
        return player;
    }
};

}