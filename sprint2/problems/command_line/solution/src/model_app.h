#pragma once

#include <cassert>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <vector>
#include <unordered_map>

#include "types.h"

namespace model {

namespace detail {
struct TokenTag {};
}  // namespace detail

using namespace std::literals;

using Token = util::Tagged<std::string, detail::TokenTag>;
using TokenHasher = util::TaggedHasher<Token>;

class Player;
class GameSession;

Token GetToken();

class Dog {
public:
    explicit Dog(Token pl_tok) :
        dog_id_(++dog_id_counter_),
        token_(pl_tok),
        dog_position_(0., 0.),
        dog_speed_(0., 0.),
        dir_("U") {
        }

    ~Dog() {
    }

    int GetId() const;
    Token GetToken() const;
    bool SetToken(const Token& pl_token);
    const ParamPairDouble GetDogPosition() const;
    const ParamPairDouble GetDogSpeed() const;
    void SetPosition(const ParamPairDouble& position);
    void SetSpeed();
    void ResetSpeed();
    void SetDefaultSpeed(double default_dog_speed);
    const std::string& GetDogDirection() const;
    void SetDogDirection(const std::string& dir);
    void Move(const ParamPairDouble& move_dist);

private:
    Token token_;
    int dog_id_;
    static int dog_id_counter_;

    std::string dir_;
    ParamPairDouble dog_position_;
    ParamPairDouble dog_speed_;
    double default_dog_speed_ = 1.;
};

class Player {
public:
    Player(const Token& token, const std::string& name, std::shared_ptr<GameSession> sess) :
        player_token_(token), 
        player_name_(name),
        session_{sess},
        player_id_(++player_id_counter_){
            SetDog(std::make_shared<Dog>(token));
        }

    ~Player() {
    }

    Token GetPlayerToken() const;
    int GetId() const noexcept;
    const std::shared_ptr<GameSession> GetPlayersSession() const;
    std::shared_ptr<GameSession> GetPlayersSession();
    const std::string& GetName() const;
    void SetName(const std::string& name);
    std::shared_ptr<Dog> GetDog() const;
    void SetDog(std::shared_ptr<Dog> dog);
    
private:
    Token player_token_;
    std::string player_name_;
    std::shared_ptr<GameSession> session_;
    std::shared_ptr<Dog> dog_;
    int player_id_;
    static int player_id_counter_;
};

class PlayerList {
public:

    PlayerList() {
    }

    ~PlayerList() {
    }

    std::shared_ptr<Player> FindPlayer(const Token& token);
    std::shared_ptr<Player> AddPlayer(const std::string& name, std::shared_ptr<GameSession> session);
    const std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher>& GetPlayersList() const;
    auto begin();
    auto end();

private:
    std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher> players_;
};

}