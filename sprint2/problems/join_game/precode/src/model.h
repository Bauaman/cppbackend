#pragma once
#include <filesystem>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <boost/asio.hpp>
#include <boost/json.hpp>

#include "tagged.h"

namespace net = boost::asio;
namespace fs = std::filesystem;

using namespace std::literals;

namespace strconsts {

    static const std::string x_start = "x0";
    static const std::string x_end = "x1";
    static const std::string y_start = "y0";
    static const std::string y_end = "y1";
    static const std::string x_offset = "offsetX";
    static const std::string y_offset = "offsetY";
    static const std::string x_str = "x";
    static const std::string y_str = "y";
    static const std::string h_str = "h";
    static const std::string w_str = "w";
}

namespace model {

using Dimension = int;
using Coord = Dimension;

struct TokenTag {};
using Token = util::Tagged<std::string, TokenTag>;
using TokenHasher = util::TaggedHasher<Token>;

class Element {
public:
    void SetKeySequence(std::string str) {
        keys_.push_back(str);
    }

    std::vector<std::string> GetKeys() const {
        return keys_;
    }
private:
    std::vector<std::string> keys_;
};

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road : public Element {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building : public Element {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office : public Element {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
    std::vector<std::string> keys_;
};

class Map : public Element {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;
    using Value = std::variant<Id, std::string, std::vector<Road>, std::vector<Building>, std::vector<Office>>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class Dog {
public:
    explicit Dog(Token token) :
        player_token_(token),
        id_(++dogs_counter_) {
    }

    Token GetToken() const {
        return player_token_;
    }

    void SetDogName(std::string name) {
        dog_name_ = name;
    }

    int GetDogId() const noexcept {
        return id_;
    }

    const std::string GetDogName() {
        return dog_name_;
    }

private:
    int id_;
    std::string dog_name_;
    static int dogs_counter_;
    Token player_token_;

};

class GameSession {
    GameSession(const GameSession&) = delete;
    GameSession& operator=(const GameSession&) = delete;

public:
    explicit GameSession(const Map& map) :
        map_{map} {
    }

    void AddDog(Dog dog) {
        dogs_.push_back(dog);
    }

    const std::vector<Dog> GetDogs() const {
        return dogs_;
    }

private:
    const Map& map_;
    std::vector<Dog> dogs_;
};

class Player {
public:
    Player(const Token& token, const std::string& name, GameSession& game_session, Dog& dog) :
        token_(token),
        name_{name},
        game_session_(game_session), 
        dog_(dog),
        id_(players_counter_++) {
    }

    Token GetToken() const {
        return token_;
    }

    int GetPlayerId () const {
        return id_;
    }

    const GameSession& GetGameSession() const {
        return game_session_;
    }

    std::string GetPlayerName() const {
        return name_;
    }

private:
    std::string name_;
    GameSession& game_session_;
    Dog& dog_;
    Token token_;
    int id_;
    static int players_counter_;
};

class PlayerTokens {
 public:
  Token GetToken() {
    std::stringstream ss;
    ss << std::setw(16) << std::setfill('0') << std::hex << m_gen1();
    ss << std::setw(16) << std::setfill('0') << std::hex << m_gen2();
    std::string res = ss.str();
    assert(res.size() == 32);
    return Token(ss.str());
  }

  const Player* FindPlayer(const Token& token) const noexcept {
    if (auto it = player_id_to_index_.find(token); it != player_id_to_index_.end()) {
      return &players_.at(it->second);
    }

    return nullptr;
  }

  Player& AddPlayer(Player player);

  const std::vector<Player>& GetPlayers() const noexcept { return players_; }

 private:

  std::random_device m_random_device;
  std::mt19937_64 m_gen1{[this] {
    std::uniform_int_distribution<std::mt19937_64::result_type> dist;
    return dist(m_random_device);
  }()};
  std::mt19937_64 m_gen2{[this] {
    std::uniform_int_distribution<std::mt19937_64::result_type> dist;
    return dist(m_random_device);
  }()};
  // Чтобы сгенерировать токен, получите из generator1_ и generator2_
  // два 64-разрядных числа и, переведя их в hex-строки, склейте в одну.
  // Вы можете поэкспериментировать с алгоритмом генерирования токенов,
  // чтобы сделать их подбор ещё более затруднительным

  using PlayerIdToIndex = std::unordered_map<Token, size_t, TokenHasher>;

  std::vector<Player> players_;
  PlayerIdToIndex player_id_to_index_;
};


class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);
    void AddGameSession(GameSession session);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    GameSession& GetGameSession(const Map::Id& id) {
        if (sessions_.contains(id)) {
            return sessions_.at(id);
        }
        const Map* map = FindMap(id);
        if (!map) {
            throw std::invalid_argument("Map "s + *id + " doesn't exist.");
        }
        auto res = sessions_.emplace(id, *map);
        if (!res.second) {
            throw std::logic_error("Failed to add map " + *id + " to session");
        }
        return res.first->second;
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    std::unordered_map<Map::Id, GameSession, util::TaggedHasher<Map::Id>> sessions_;
};

class GameServer {

public:
    GameServer(net::io_context& ioc, fs::path root, Game& game);
    ~GameServer();

    const fs::path GetRootDir() const;
    const Map* FindMap(const Map::Id& id) const;
    const std::vector<Map>& GetMaps() const;
    Player& JoinGame(Map::Id id, const std::string& user_name);
    const Player* FindPlayer(const Token& token) const;
    const std::vector<Player>& GetPlayers() const;

private:
    net::io_context& ioc_;
    const fs::path root_;
    Game& game_;
    PlayerTokens player_tokens_;

};


}  // namespace model
