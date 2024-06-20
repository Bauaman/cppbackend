#pragma once

#include "json_loader.h"

#include <boost/asio/io_context.hpp>


namespace net = boost::asio;
namespace fs = std::filesystem;

using namespace std::literals;

class GameServer {
    GameServer() = delete;
    GameServer(const GameServer&) = delete;
    GameServer(GameServer&&) = delete;
    GameServer& operator=(const GameServer&) = delete;
    GameServer& operator=(GameServer&&) = delete;

public:
    GameServer(net::io_context& ioc, fs::path config, fs::path root) :
        ioc_(ioc),
        root_dir_(root) {
            game_ = json_loader::LoadGame(config);
        }

    const fs::path& GetRootDir() const noexcept {
        return root_dir_;
    }

    const model::Map* FindMap(const model::Map::Id& id) const noexcept {
        return game_.FindMap(id);
    }

    const std::vector<model::Map>& GetMaps() const noexcept {
        return game_.GetMaps();
    }

    model::Player& JoinGame(model::Map::Id id, const std::string& player_name) {
        model::GameSession& session = game_.GetGameSession(id);
        model::Token token = player_tokens_.GetToken();
        model::Dog dog(token);
        model::Player player(token, player_name, session, dog);
        return player_tokens_.AddPlayer(player);
    }

    const model::Player* FindPlayer(const model::Token& token) {
        return player_tokens_.FindPlayer(token);
    }

    const std::vector<model::Player>& GetPlayers() const noexcept {
        return player_tokens_.GetPlayers();
    }

private:
    net::io_context& ioc_;
    const fs::path root_dir_;
    model::Game game_;
    model::PlayerToken player_tokens_;

};