#include "model_app.h"

namespace model {

int Dog::dog_id_counter_ = 0;
int Player::player_id_counter_ = 0;

Token GetToken() {
    std::random_device random_device_;
    std::mt19937_64 generator1_{[&random_device_] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[&random_device_] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::stringstream ss;
    ss << std::setw(16) << std::setfill('0') << std::hex << generator1_(); 
    ss << std::setw(16) << std::setfill('0') << std::hex << generator2_(); 
    std::string result = ss.str();
    assert(result.size() == 32);
    return Token(ss.str());
}

int Dog::GetId() const {
    return dog_id_;
}

Token Dog::GetToken() const {
    return token_;
}

bool Dog::SetToken(const Token& pl_token) {
    if ((*token_).empty()) {
        token_ = pl_token;
        return true;
    }
    return false;
}

const ParamPairDouble Dog::GetDogPosition() const {
    return dog_position_;
}

const ParamPairDouble Dog::GetDogSpeed() const {
    return dog_speed_;
}

void Dog::SetPosition(const ParamPairDouble& position) {
    dog_position_.x_ = position.x_;
    dog_position_.y_ = position.y_;
}

void Dog::SetSpeed() {
    if (dir_ == "U") {
        dog_speed_ = {0., -default_dog_speed_};
    } else if (dir_ == "R") {
        dog_speed_ = {default_dog_speed_, 0.};
    } else if (dir_ == "D") {
        dog_speed_ = {0., default_dog_speed_};
    } else if (dir_ == "L") {
        dog_speed_ = {-default_dog_speed_, 0.};
    } else if (dir_ == "") {
        dog_speed_ = {0., 0.};
    } else {
        assert(false && "Unexpexted direction");
    }
}

void Dog::ResetSpeed() {
    dog_speed_ = {0., 0.};
}

void Dog::SetDefaultSpeed(double default_dog_speed) {
    default_dog_speed_ = default_dog_speed;
}

const std::string& Dog::GetDogDirection() const {
    return dir_;
}

void Dog::SetDogDirection(const std::string& dir) {
    if (dir != "U" && dir != "R" && dir != "L" && dir != "D" && dir != "") {
        throw std::invalid_argument("Unknown direction...");
    }
    dir_ = dir;
    SetSpeed();
}

void Dog::Move(const ParamPairDouble& move_dist) {
    dog_position_.x_ += move_dist.x_;
    dog_position_.y_ += move_dist.y_;
}

Token Player::GetPlayerToken() const {
    return player_token_;
}

int Player::GetId() const noexcept {
    return player_id_;
}

const std::shared_ptr<GameSession> Player::GetPlayersSession() const {
    return session_;
}

std::shared_ptr<GameSession> Player::GetPlayersSession() {
    return session_;
}

const std::string& Player::GetName() const {
    return player_name_;
}

void Player::SetName(const std::string& name) {
    if (!player_name_.empty()) {
        throw std::logic_error("Cannot rename player...");
    }
    if (name.empty()) {
        throw std::logic_error("Cannot set empty name...");
    }
    player_name_ = name;
}

std::shared_ptr<Dog> Player::GetDog() const {
    return dog_;
}

void Player::SetDog(std::shared_ptr<Dog> dog) {
    if (dog_) {
        throw std::logic_error("Player already owns a dog...");
    }
    if (!dog) {
        throw std::logic_error("Dog not specified...");
    }
    dog_ = dog;
}

std::shared_ptr<Player> PlayerList::FindPlayer(const Token& token) {
    if (players_.contains(token)) {
        return players_.at(token);
    }
    return nullptr;
}

std::shared_ptr<Player> PlayerList::AddPlayer(const std::string& name, std::shared_ptr<GameSession> session) {
    Token token = GetToken();
    auto p = players_.emplace(token, std::make_shared<Player>(token, name, session));
    if (p.second) {
        return players_.at(token);
    }
    throw std::runtime_error("Failed to add player...");
}

const std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher>& PlayerList::GetPlayersList() const {
    return players_;
}

auto PlayerList::begin() {
    return players_.begin();
}

auto PlayerList::end() {
    return players_.end();
}
}