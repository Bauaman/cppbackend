#include "model.h"
#include <iostream>
#include <stdexcept>

namespace model {
using namespace std::literals;

int Dog::dogs_counter_ = 0;
int Player::players_counter_ = 0;

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

Player& PlayerTokens::AddPlayer(Player player) {
    const size_t index = players_.size();
    if (auto [iter, inserted] = player_id_to_index_.emplace(player.GetToken(), index); !inserted) {
        throw std::invalid_argument("Player with token "s + *player.GetToken() + " already exists"s);
    } else {
        try {
            return players_.emplace_back(std::move(player));
        } catch (...) {
            player_id_to_index_.erase(iter);
            throw;
        }
    }
}

GameServer::GameServer(net::io_context& ioc, fs::path root, Game& game) : 
    ioc_(ioc),
    root_(root),
    game_(game) {
        std::cout << "GameServer created" << std::endl;
}

GameServer::~GameServer() {}

const fs::path GameServer::GetRootDir() const {
    return root_;
}

const Map* GameServer::FindMap(const Map::Id& id) const {
    return game_.FindMap(id);
}

const std::vector<Map>& GameServer::GetMaps() const {
    return game_.GetMaps();
}

Player& GameServer::JoinGame(Map::Id id, const std::string& user_name) {
    auto& session = game_.GetGameSession(id);
    Token token = player_tokens_.GetToken();
    Dog dog(token);
    Player player(token, user_name, session, dog);
    return player_tokens_.AddPlayer(player);
}

const Player* GameServer::FindPlayer(const Token& token) const {
    return player_tokens_.FindPlayer(token);
}

const std::vector<Player>& GameServer::GetPlayers() const {
    return player_tokens_.GetPlayers();
}

}  // namespace model
