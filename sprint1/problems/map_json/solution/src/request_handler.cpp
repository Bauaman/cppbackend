#include "request_handler.h"

namespace http_handler {

    boost::json::value RequestHandler::PrepareResponce(const std::string& req_, model::Game& game_) {
        std::cout << "Prepare Resp: req_" << req_ << std::endl;
        const std::vector<model::Map>& maps = game_.GetMaps();
        boost::json::array response_text;
        if (req_ == "maps") {
            for (const auto& map : maps) {
                boost::json::object mapJson;
                mapJson["id"] = *map.GetId();
                mapJson["name"] = map.GetName();
                response_text.push_back(mapJson);
            }
        } else {
            std::cout << "Prepare Resp2: req_" << req_ << std::endl;
            model::Map::Id id_{req_};
            const model::Map* map = game_.FindMap(id_);
            if (map != nullptr) {
                boost::json::object mapJson;
                mapJson["id"] = *map->GetId();
                mapJson["name"] = map->GetName();
                boost::json::array roads;
                for (const auto& road : map->GetRoads()) {
                    boost::json::object road_;
                    if (road.IsHorizontal()) {
                        road_["x0"] = road.GetStart().x;
                        road_["y0"] = road.GetStart().y;
                        road_["x1"] = road.GetEnd().x;
                    }
                    if (road.IsVertical()) {
                        road_["x0"] = road.GetStart().x;
                        road_["y0"] = road.GetStart().y;
                        road_["y1"] = road.GetEnd().y;
                    }
                    roads.push_back(road_);
                }
                mapJson["roads"] = roads;
                boost::json::array buildings;
                for (const auto& building : map->GetBuildings()) {
                    boost::json::object build_;
                    build_["x"] = building.GetBounds().position.x;
                    build_["y"] = building.GetBounds().position.y;
                    build_["w"] = building.GetBounds().size.width;
                    build_["h"] = building.GetBounds().size.height;
                    buildings.push_back(build_);
                }
                mapJson["buildings"] = buildings;
                boost::json::array offices;
                for (const auto& office : map->GetOffices()) {
                    boost::json::object office_;
                    office_["id"] = *office.GetId();
                    office_["x"] = office.GetPosition().x;
                    office_["y"] = office.GetPosition().y;
                    office_["offsetX"] = office.GetOffset().dx;
                    office_["offsetY"] = office.GetOffset().dy;
                    offices.push_back(office_);
                }
                mapJson["offices"] = offices;
                response_text.push_back(mapJson);
            } else {
                boost::json::object mapJson;
                mapJson["code"] = "mapNotFound";
                mapJson["message"] = "Map not found";
                response_text.push_back(mapJson);
            }
        }
        return response_text;
    }

}  // namespace http_handler
