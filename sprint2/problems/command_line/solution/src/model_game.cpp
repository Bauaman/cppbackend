#include "model_game.h"

namespace model {

void GameSession::UpdateDogsPosition(const double dt) {
    std::shared_ptr<Map> map = GetMap();
    for (auto dog : GetDogs()) {
        ParamPairDouble cur_dog_pos = dog->GetDogPosition();
        Point p_cur_dog_pos = {static_cast<Coord>(std::round(cur_dog_pos.x_)), static_cast<Coord>(std::round(cur_dog_pos.y_))};
        std::set<std::shared_ptr<Road>> roads_at_point = map->GetRoadsByCoords(p_cur_dog_pos);
        auto dog_speed = dog->GetDogSpeed();
        auto new_dog_pos = cur_dog_pos + (dog_speed * dt);
        if (CheckIfMovedProperly(roads_at_point, new_dog_pos)) {
            dog->SetPosition(new_dog_pos);
        } else {
            SetMaxMoveForTick(roads_at_point, new_dog_pos);
            dog->SetPosition(new_dog_pos);
            dog->ResetSpeed();
        }
    }
}

bool CheckIfMovedProperly(std::set<std::shared_ptr<Road>>& roads, ParamPairDouble& new_pos) {
    for (auto& road : roads) {
        if (road->PointIsOnRoad(new_pos)) {
            return true;
        }
    }
    return false;
}

RoadArea CreateMaxMovingCoords(std::set<std::shared_ptr<Road>>& roads) {
    ParamPairDouble min_c = roads.begin()->get()->GetRoadArea().left_bottom;
    ParamPairDouble max_c = roads.begin()->get()->GetRoadArea().right_top;
    for (auto& road : roads) {
        if (min_c.x_ > road->GetRoadArea().left_bottom.x_) {min_c.x_ = road->GetRoadArea().left_bottom.x_;}
        if (min_c.y_ > road->GetRoadArea().left_bottom.y_) {min_c.y_ = road->GetRoadArea().left_bottom.y_;}
        if (max_c.x_ < road->GetRoadArea().right_top.x_) {max_c.x_ = road->GetRoadArea().right_top.x_;}
        if (max_c.y_ < road->GetRoadArea().right_top.y_) {max_c.y_ = road->GetRoadArea().right_top.y_;}        
    }
    return {min_c, max_c};
}

void SetMaxMoveForTick(std::set<std::shared_ptr<Road>>& roads, ParamPairDouble& new_pos) {
    RoadArea max_possible_coords = CreateMaxMovingCoords(roads);
    if (new_pos.x_ < max_possible_coords.left_bottom.x_) {new_pos.x_ = max_possible_coords.left_bottom.x_;}
    if (new_pos.y_ < max_possible_coords.left_bottom.y_) {new_pos.y_ = max_possible_coords.left_bottom.y_;}
    if (new_pos.x_ > max_possible_coords.right_top.x_) {new_pos.x_ = max_possible_coords.right_top.x_;}
    if (new_pos.y_ > max_possible_coords.right_top.y_) {new_pos.y_ = max_possible_coords.right_top.y_;}
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

std::shared_ptr<Map> GameSession::GetMap() const {
    return map_;
}

void GameSession::AddDog(std::shared_ptr<Dog> dog, bool random_position) {
    dog->SetPosition(map_->GetStartPosition(random_position));
    dog->SetDefaultSpeed(map_->GetMapDogSpeed());
    dogs_.emplace_back(dog);
}

const std::vector<std::shared_ptr<Dog>> GameSession::GetDogs() {
    std::vector<std::shared_ptr<Dog>> result;
    result.reserve(dogs_.size());

    auto iter = dogs_.begin();
    while (iter != dogs_.end()) {
        if (iter->expired()) {
            iter = dogs_.erase(iter);
        } else {
            result.push_back(iter->lock());
            ++iter;
        }
    }
    return result;
}

const std::shared_ptr<Dog> GameSession::GetDog(const Token& token) const {
    for (const auto& weak_dog : dogs_) {
        if (auto dog = weak_dog.lock()) {
            if (dog->GetToken() == token) {
                return dog;
            }
        }
    }
    return nullptr;
}

const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
}

std::shared_ptr<Map> Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return std::make_shared<Map>(maps_.at(it->second));
    }
    return nullptr;
}

std::shared_ptr<GameSession> Game::GetGameSession(const Map::Id& id) {
    auto map = FindMap(id);
    if (map == nullptr) {
        throw std::invalid_argument("Map "s + *id + " doesn't exist"s);
    }

    auto it = std::find_if(game_sessions_.begin(), game_sessions_.end(),
                            [&id](const std::shared_ptr<GameSession>& session) {
                                return session->GetMap()->GetId() == id;
                            });
    
    if (it != game_sessions_.end()) {
        return *it;
    }

    auto game_session = std::make_shared<GameSession>(map);
    game_sessions_.push_back(game_session);

    return game_session;
}

void Game::SetDefaultDogSpeed(double speed) {
    default_dog_speed_ *= speed;
}

double Game::GetDefaultDogSpeed() const {
    return default_dog_speed_;
}

void Game::UpdateGame(const double dt) {
    for (auto& gs : game_sessions_) {
        gs->UpdateDogsPosition(dt);
    }
}

void Game::PrintMaps() {
    for (Map map : maps_) {
        std::cout << "Map: " << map.GetName() << std::endl;
        for (auto road : map.GetRoads()) {
            std::cout << "{" << road.GetStart().x << ", " << road.GetStart().y << "} - {" << road.GetEnd().x << ", " << road.GetEnd().y << "}" << std::endl;
        }
        std::cout << std::endl;
    }
}

}