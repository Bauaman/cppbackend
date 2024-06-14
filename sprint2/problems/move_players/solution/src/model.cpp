#include "model.h"

namespace model {
using namespace std::literals;


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

}