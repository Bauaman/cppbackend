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

}