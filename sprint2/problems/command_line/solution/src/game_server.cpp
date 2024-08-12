#include "game_server.h"

std::shared_ptr<model::Player> GameServer::JoinGame(model::Map::Id id, const std::string& player_name) {
    auto session = game_.GetGameSession(id);
    if (!session) {
        throw std::runtime_error("Failed to create game session...");
    }
    auto player = player_list_.AddPlayer(player_name, session);
    auto dog = player->GetDog();
    session->AddDog(dog, spawn_dog_random);
    return player;
}