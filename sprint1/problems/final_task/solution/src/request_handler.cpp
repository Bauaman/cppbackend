#include "request_handler.h"

namespace http_handler {

    boost::json::value RequestHandler::PrepareRoadsForResponse(const model::Map& map) {
        boost::json::array roads;
        for (const auto& road : map.GetRoads()) {
            boost::json::object road_;
            for (const std::string str : road.GetKeys()) {
                if (str == "x0") {road_["x0"] = road.GetStart().x;}
                if (str == "x1") {road_["x1"] = road.GetEnd().x;}
                if (str == "y0") {road_["y0"] = road.GetStart().y;}
                if (str == "y1") {road_["y1"] = road.GetEnd().y;}
            }
            roads.push_back(road_);
        }
        return roads;
    }

    boost::json::value RequestHandler::PrepareBuildingsForResponce(const model::Map& map) {
        boost::json::array buildings;
        for (const auto& building : map.GetBuildings()) {
            boost::json::object build_;
            for (const std::string str : building.GetKeys()) {
                if (str == "x") {build_["x"] = building.GetBounds().position.x;}
                if (str == "y") {build_["y"] = building.GetBounds().position.y;}
                if (str == "w") {build_["w"] = building.GetBounds().size.width;}
                if (str == "h") {build_["h"] = building.GetBounds().size.height;}
            }
            buildings.push_back(build_);
        }
        return buildings;
    }

    boost::json::value RequestHandler::PrepareOfficesForResponce(const model::Map& map) {
        boost::json::array offices;
        for (const auto& office : map.GetOffices()) {
            boost::json::object office_;
            for (const std::string str : office.GetKeys()) {
                if (str == "id") {office_["id"] = *office.GetId();}
                if (str == "x") {office_["x"] = office.GetPosition().x;}
                if (str == "y") {office_["y"] = office.GetPosition().y;}
                if (str == "offsetX") {office_["offsetX"] = office.GetOffset().dx;}
                if (str == "offsetY") {office_["offsetY"] = office.GetOffset().dy;}
            }
            offices.push_back(office_);
        }
        return offices;
    }

    boost::json::value RequestHandler::PrepareResponce(const std::string& req_, model::Game& game_) {
        boost::json::array response_text_arr;
        boost::json::object response_text_obj;
        //std::cout << "Prepare Response function: " << req_ << std::endl;
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
                //std::cout << "Map found" << std::endl;
                for (const auto& key : keys_in_map) {
                    std::cout << "Keys in map : " << key << std::endl;
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
                        response_text_obj["offices"]=offices;            
                    }
                }
                return response_text_obj;
            } else {
                //std::cout << "Map NOT found" << std::endl;
                response_text_obj["code"] = "mapNotFound";
                response_text_obj["message"] = "Map Not Found";
                return response_text_obj;
            }
        }
        return response_text_arr;
    }




























































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































}  // namespace http_handler
