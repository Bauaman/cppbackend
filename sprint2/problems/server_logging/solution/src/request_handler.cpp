#include "request_handler.h"

namespace http_handler {

    //template <typename Body, typename Allocator>
    /*std::string*/RequestData RequestParser(const std::string& req_target) {
        if (req_target.find("/api") == 0) {
            size_t pos = req_target.find("/api/v1/");
            if (pos != std::string::npos) { //проверка на правильный префикс
                std::string req_ = req_target.substr(pos + 8);
                size_t next_slash_pos = req_.find('/');

                if (next_slash_pos != req_.length() && next_slash_pos != std::string::npos) {
                    req_ = req_.substr(next_slash_pos+1);
                    if (req_.find('/') == std::string::npos) {
                        return {RequestType::API, std::string(req_)};
                    } else {
                        throw std::logic_error("Invalid request (/api/v1/maps/id/?)"s);
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
            return {RequestType::STATIC_FILE, req_target};
        }
    }

    using namespace strconsts;
    boost::json::value PrepareRoadsForResponse(const model::Map& map) {
        boost::json::array roads;
        for (const auto& road : map.GetRoads()) {
            boost::json::object road_;
            for (const std::string& str : road.GetKeys()) {
                if (str == x_start) {road_[x_start] = road.GetStart().x;}
                if (str == x_end) {road_[x_end] = road.GetEnd().x;}
                if (str == y_start) {road_[y_start] = road.GetStart().y;}
                if (str == y_end) {road_[y_end] = road.GetEnd().y;}
            }
            roads.push_back(road_);
        }
        return roads;
    }

    boost::json::value PrepareBuildingsForResponce(const model::Map& map) {
        boost::json::array buildings;
        for (const auto& building : map.GetBuildings()) {
            boost::json::object build_;
            for (const std::string& str : building.GetKeys()) {
                if (str == x_str) {build_[x_str] = building.GetBounds().position.x;}
                if (str == y_str) {build_[y_str] = building.GetBounds().position.y;}
                if (str == w_str) {build_[w_str] = building.GetBounds().size.width;}
                if (str == h_str) {build_[h_str] = building.GetBounds().size.height;}
            }
            buildings.push_back(build_);
        }
        return buildings;
    }

    boost::json::value PrepareOfficesForResponce(const model::Map& map) {
        boost::json::array offices;
        for (const auto& office : map.GetOffices()) {
            boost::json::object office_;
            for (const std::string& str : office.GetKeys()) {
                if (str == "id") {office_["id"] = *office.GetId();}
                if (str == x_str) {office_[x_str] = office.GetPosition().x;}
                if (str == y_str) {office_[y_str] = office.GetPosition().y;}
                if (str == x_offset) {office_[x_offset] = office.GetOffset().dx;}
                if (str == y_offset) {office_[y_offset] = office.GetOffset().dy;}
            }
            offices.push_back(office_);
        }
        return offices;
    }

    boost::json::value RequestHandler::PrepareAPIResponce(const std::string& req_, const model::Game& game_) {
        boost::json::array response_text_arr;
        boost::json::object response_text_obj;
        const std::vector<model::Map>& maps = game_.GetMaps();
        
        if (req_.empty()) {
            throw std::logic_error("bad request /api/v1/maps/...");
        }
        if (req_ == "maps") {
            for (auto& map : maps) {
                boost::json::object map_object;
                map_object["id"] = *map.GetId();
                map_object["name"] = map.GetName();
                response_text_arr.push_back(map_object);
            }
            return response_text_arr;
        } else {
            model::Map::Id  id_{req_};
            const model::Map* map = game_.FindMap(id_);
            if (map != nullptr) {
                std::vector<std::string> keys_in_map = map->GetKeys();
                for (const auto& key : keys_in_map) {
                    if (key == "id") {
                        response_text_obj["id"] = *map->GetId();
                    } else if (key == "name") {
                        response_text_obj["name"] = map->GetName();
                    } else if (key == "roads") {
                        boost::json::array roads = PrepareRoadsForResponse(*map).as_array();
                        response_text_obj["roads"] = roads;
                    } else if(key == "buildings") {
                        boost::json::array buildings = PrepareBuildingsForResponce(*map).as_array();
                        response_text_obj["buildings"] = buildings;
                    } else if (key == "offices") {
                        boost::json::array offices = PrepareOfficesForResponce(*map).as_array();
                        response_text_obj["offices"] = offices;            
                    }
                }
                return response_text_obj;
            } else {
                response_text_obj["code"] = "mapNotFound";
                response_text_obj["message"] = "Map Not Found";
                return response_text_obj;
            }
        }
        return response_text_arr;
    }




























































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































}  // namespace http_handler
