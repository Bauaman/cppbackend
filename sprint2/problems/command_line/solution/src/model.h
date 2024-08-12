#pragma once

#include "aux.h"
#include "types.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace model {

using namespace std::literals;
class Element {
public:
    void SetKeySequence(std::string str);
    std::vector<std::string> GetKeys() const;

private:
    std::vector<std::string> keys_;
};

class Road : public Element {
    struct  HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };
    
public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept :
        start_{start},
        end_{end_x, start.y} {
            SetRoadArea();
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept :
        start_{start},
        end_{start.x, end_y} {
            SetRoadArea();
    }

    bool IsHorizontal() const noexcept;
    bool IsVertical() const noexcept;
    Point GetStart() const noexcept;
    Point GetEnd() const noexcept;
    bool PointIsOnRoad(ParamPairDouble& p);
    RoadArea GetRoadArea() const;

private:
    Point start_;
    Point end_;

    RoadArea road_area_;
    void SetRoadArea();
};

class Building : public Element {
public:
    explicit Building(Rectangle bounds) noexcept :
        bounds_{bounds} {}

    const Rectangle& GetBounds() const noexcept;

private:
    Rectangle bounds_;
};

class Office : public Element {
public:
    using Id = util::Tagged<std::string, Office>;
    Office(Id id, Point pos, Offset offset) noexcept :
        id_{id},
        position_{pos},
        offset_{offset} {}

    const Id& GetId() const noexcept;
    Point GetPosition() const noexcept;
    Offset GetOffset() const noexcept;

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

    Map(Id id, std::string name) noexcept :
        id_(std::move(id)),
        name_(std::move(name)) {}

    const Id& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    const Buildings& GetBuildings() const noexcept;
    const Roads& GetRoads() const noexcept;
    const Offices& GetOffices() const noexcept;
    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
    void AddOffice(Office office);
    ParamPairDouble GetRandomDogPosition() const;
    ParamPairDouble GetStartPosition(bool random) const;
    void SetMapDogSpeed(double ds);
    double GetMapDogSpeed() const;
    std::set<std::shared_ptr<Road>> GetRoadsByCoords(Point p) const;
    void PrintCoordToRoad() const;

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;
    Id id_;
    std::string name_;

    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    double map_dog_speed_;

    std::unordered_map<Point, std::set<std::shared_ptr<Road>>, PointHash> coord_to_road;
};


} //namespace model