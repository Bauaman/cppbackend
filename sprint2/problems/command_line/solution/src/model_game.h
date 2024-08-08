#pragma once

#include <cmath>
#include <optional>

#include "model_app.h"
#include "model.h"

namespace model {

bool CheckIfMovedProperly(std::set<std::shared_ptr<Road>>& roads, ParamPairDouble& new_pos);
void SetMaxMoveForTick(std::set<std::shared_ptr<Road>>& roads, ParamPairDouble& new_pos);
RoadArea CreateMaxMovingCoords(std::set<std::shared_ptr<Road>>& roads);

class GameSession {
    GameSession(const GameSession&) = delete;
    GameSession& operator=(const GameSession&) = delete;

public:
    explicit GameSession(std::shared_ptr<Map> map) :
        map_(map) {}

    std::shared_ptr<Map> GetMap() const;
    void AddDog(std::shared_ptr<Dog> dog, bool random_position);
    void UpdateDogsPosition(const double dt);
    const std::vector<std::shared_ptr<Dog>> GetDogs();
    const std::shared_ptr<Dog> GetDog(const Token& token) const;

private:
    std::shared_ptr<Map> map_;
    std::vector<std::weak_ptr<Dog>> dogs_;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);
    const Maps& GetMaps() const noexcept;
    std::shared_ptr<Map> FindMap(const Map::Id& id) const noexcept;
    std::shared_ptr<GameSession> GetGameSession(const Map::Id& id);
    void SetDefaultDogSpeed(double speed);
    double GetDefaultDogSpeed() const;
    void UpdateGame(const double dt);
    void PrintMaps();

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;

    std::vector<std::shared_ptr<GameSession>> game_sessions_;

    double default_dog_speed_ = 1.;

};

}