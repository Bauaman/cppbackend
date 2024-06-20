#include "model.h"

namespace model {
using namespace std::literals;

void Map::AddRoad(const Road& road) {
    roads_.emplace_back(road);
    std::shared_ptr<Road> road_ptr = std::make_shared<Road>(roads_.back());
    //std::cout << "road emplaced: " << road_ptr << std::endl;
    
    if (road.IsHorizontal()) {
        //std::cout << "Adding Horizontal road " << std::endl;
        for (Coord x = std::min(road_ptr->GetStart().x, road_ptr->GetEnd().x); x <= std::max(road_ptr->GetStart().x, road_ptr->GetEnd().x); x++) {
            //std::cout << "adding point: {" << x << ", " << road_ptr->GetStart().y << "} " << road_ptr << std::endl; 
            coord_to_road[{x, road_ptr->GetStart().y}].emplace(road_ptr);
        }
    }
    if (road.IsVertical()) {
        //std::cout << "Adding Vertical road " << std::endl;
        for (Coord y = std::min(road_ptr->GetStart().y, road_ptr->GetEnd().y); y <= std::max(road_ptr->GetStart().y, road_ptr->GetEnd().y); y++) {
            //std::cout << "adding point: {" << road_ptr->GetStart().x << ", " << y << "} " << road_ptr << std::endl; 
            coord_to_road[{road_ptr->GetStart().x, y}].emplace(road_ptr);
        }        
    }
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }
    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
        try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

std::set<std::shared_ptr<Road>> Map::GetRoadsByCoords(Point p) const {
    if (coord_to_road.contains(p)) {
        return coord_to_road.at(p);
    }
    return {};    
}
/*
ParamPairDouble Map::GetRandomDogPosition() const {
    if (roads_.empty()) {
        throw std::runtime_error("No roads to put dog on...");
    }
    const Road& road = roads_[auxillary::GetRandomNumber(0, roads_.size()-1)];
    Point road_start = road.GetStart();
    Point road_end = road.GetEnd();

    if (road.IsHorizontal()) {
        double random_x = auxillary::GetRandomNumber((double)road_start.x, (double)road_end.x);
        return ParamPairDouble{random_x, (double)road_start.y};
    }
    if (road.IsVertical()) {
        double random_y = auxillary::GetRandomNumber((double)road_start.y, (double)road_end.y);
        return ParamPairDouble{(double)road_start.x, random_y};
    }
    return {(double)road_start.x, (double)road_start.y}; //never happens
}
*/
void Map::PrintCoordToRoad() const {
    for (auto [point, road] : coord_to_road) {
        
        std::cout << "{" << point.x << ", " << point.y << "} " << ": " << std::endl;
        for (auto r : road) {
            std::cout << "road ptr " << r << std::endl;
        }
    }
}

}