#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <optional>
#include <deque>
#include <queue>
#include <memory>
#include <stdexcept>
#include <utility>

#include <boost/json.hpp>
#include <boost/serialization/access.hpp>

#include "../utils/tagged.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct DoublePoint {
    double x, y;

    bool operator==(const DoublePoint& rhs) const
    {
        return (x == rhs.x) && (y == rhs.y);
    }

    bool operator!=(const DoublePoint& rhs) const
    {
        return !(*this == rhs);
    }
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

class Road {
    struct HorizontalTag {
        HorizontalTag() = default;
    };

    struct VerticalTag {
        VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    constexpr static double width_ = 0.8;

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

    DoublePoint CalculateRightLowerPoint() const noexcept {
        double x = std::max(start_.x, end_.x) +  width_/2;
        double y = std::max(start_.y, end_.y) +  width_/2;
        return {x, y};
    }

    DoublePoint CalculateLeftTopPoint() const noexcept {
        double x = std::min(start_.x, end_.x) - width_/2;
        double y = std::min(start_.y, end_.y) - width_/2;
        return {x, y};
    }

    bool IsPointPartOfRoad(double x, double y) const noexcept;

private:
    Point start_;
    Point end_;
};

class Building {
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

class Office {
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

    double GetWidth() const noexcept {
        return width_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
    const double width_ = 0.5;
};

class Loot
{
public:
    Loot() = default;

    Loot(uint64_t type, DoublePoint position)
        : type_(type)
        , position_(position)
        {}

    Loot(Loot&& other)
    : position_(std::move(other.position_))
    , type_(std::move(other.type_))
    , width_(std::move(other.width_))
    {}

    Loot& operator=(Loot&& other)
    {
        std::swap(position_, other.position_);
        std::swap(type_, other.type_);
        std::swap(width_, other.width_);
        return *this;
    }

    Loot& SetPosition(DoublePoint new_position)
    {
        position_ = new_position;
        return *this;
    }

    Loot& SetType(uint64_t type)
    {
        type_ = type;
        return *this;
    }

    DoublePoint GetPosition() const
    {
        return position_;
    }

    uint64_t GetType() const
    {
        return type_;
    }

    double GetWidth() const noexcept {
        return width_;
    }

private:
    DoublePoint position_;
    uint64_t type_;
    double width_ = 0.0;

    template <typename Archive>
    friend void serialize(Archive& ar, model::Loot& loot, [[maybe_unused]] const unsigned version);
};

class LootGeneratorConf{
public:
    explicit LootGeneratorConf() = default;

    explicit LootGeneratorConf(double period, double propability)
        : period_(period)
        , propability_(propability)
        {}

    LootGeneratorConf& SetPeriod(double period)
    {
        period_ = period;
        return *this;
    }

    LootGeneratorConf& SetPropability(double propability)
    {
        propability_ = propability;
        return *this;
    }

    double GetPeriod() const
    {
        return period_;
    }

    double GetPropability() const
    {
        return propability_;
    }

private:
    double period_ = 0.0;
    double propability_ = 0.0;
};

class LootType {
public:
    LootType() = delete;

    LootType(int rotation, double scale, std::string name, std::string file_path, std::string type, std::string color)
        : rotation_(rotation)
        , scale_(scale)
        , name_(name)
        , file_path_(file_path)
        , type_(type)
        , color_(color)
        {}

    explicit LootType(boost::json::value loot_type)
    : loot_type_(loot_type)
    {}

    boost::json::value GetLootType() const
    {
        return loot_type_;
    }

    uint64_t GetValue() const
    {
        boost::json::value val = loot_type_.as_object().at("value");
        if(val.is_int64())
        {
            return static_cast<uint64_t>(val.as_int64());
        }
        else if (val.is_uint64())
        {
            return val.as_uint64();
        }
        return 0;
    }

private:
    boost::json::value loot_type_;
    int rotation_ = 0;
    double scale_ = 0.0;
    std::string name_ = "";
    std::string file_path_ = "";
    std::string type_ = "";
    std::string color_ = "";
};

class LootsWrappler
{
public:

LootsWrappler() = default;

LootsWrappler(LootsWrappler&& other)
:loots_(std::move(other.loots_))
, loots_ptr_(std::move(other.loots_ptr_))
, busy_loots_id_(std::move(other.busy_loots_id_))
, freed_ids_(std::move(other.freed_ids_))
{
}

LootsWrappler& operator=(LootsWrappler&& other)
{
    loots_ = std::move(other.loots_);
    loots_ptr_ = std::move(other.loots_ptr_);
    busy_loots_id_ = std::move(other.busy_loots_id_);
    freed_ids_ = std::move(other.freed_ids_);
    return *this;
}

void AddLoot(uint64_t type, DoublePoint position)
{
    //Is there are free ids
    if(freed_ids_.size() > 0)
    {
        uint64_t id = freed_ids_.front();
        freed_ids_.pop();
        SetLootType(type, id);
        SetLootsCoordinates(position, id);
        loots_ptr_.at(id) = &(loots_.at(id));
    }
    //There are not free ids
    else
    {
        loots_.emplace_back(type, position);
        loots_ptr_.push_back(&(loots_.back()));
    }
}

size_t GetSize() const
{
    return loots_.size() - busy_loots_id_.size() - freed_ids_.size();
}

void PopLoot(uint64_t id)
{
    if(!IsValidId(id))
    {
        throw std::out_of_range("Invalid id of loot_item");
    }
    MakeIndexFree(id);
    if(IsBusyLoot(id))
    {
        MarkNotBusy(id);
    }
}

std::deque<const Loot*> GetLoots() const
{
    std::deque<const Loot*> items;

    for(size_t i = 0; i < loots_ptr_.size(); ++i)
    {
        Loot* item_ptr = loots_ptr_.at(i);
        if(item_ptr != nullptr) //is not free index
        {
            if(!IsBusyLoot(i))
            {
                items.push_back(item_ptr);
            }
        }
    }
    return items;
}

const std::deque<Loot*>& GetAllLoots() const
{
    return loots_ptr_;
}

const Loot& GetLoot(uint64_t id) const
{
    if(!IsValidId(id))
    {
        throw std::out_of_range("Invalid id of loot_item");
    }
    else
    {
        return loots_.at(id);
    }
}

void MarkBusy(uint64_t index)
{
    busy_loots_id_.insert(index);
}

void MarkNotBusy(uint64_t index)
{
    auto it = busy_loots_id_.find(index);
    busy_loots_id_.erase(it);
}

bool IsBusyLoot(uint64_t index) const
{
    if(busy_loots_id_.find(index) != busy_loots_id_.end())
    {
        return true;
    }
    return false;
}

LootsWrappler& SetLootType(uint64_t type, uint64_t id)
{
    loots_.at(id).SetType(type);
    return *this;
}

LootsWrappler& SetLootsCoordinates(DoublePoint position, uint64_t id)
{
    loots_.at(id).SetPosition(position);
    return *this;
}

private:
std::deque<Loot> loots_;
std::deque<Loot*> loots_ptr_;
std::unordered_set<uint64_t> busy_loots_id_; //indexes in bags
std::queue<uint64_t> freed_ids_; //содержит информацию о свободных индексах, если пустой, значит свободны все индексы

bool IsValidId(uint64_t id) const
{
    if(id >= loots_.size() || loots_ptr_.at(id) == nullptr)
    {
        return false;
    }
    else
    {
        return true;
    }
}

template <typename Archive>
friend void serialize(Archive& ar, model::LootsWrappler& loots_wrappler, const unsigned version);

void MakeIndexFree(uint64_t id)
{
    loots_ptr_.at(id) = nullptr;
    freed_ids_.push(id);
}

};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

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

    void SetDogSpeed(double dog_speed)
    {
        dog_speed_ = dog_speed;
    }

    double GetDogSpeed() const noexcept
    {
        return dog_speed_;
    }

    void AddLootType(LootType&& loot_type)
    {
        loot_types_.push_back(std::move(loot_type));
    }

    const std::vector<LootType>& GetLootTypes() const
    {
        return loot_types_;
    }

    void SetBagCapacity(uint64_t bag_capacity)
    {
        bag_capacity_ = bag_capacity;
    }

    uint64_t GetBagCapacity() const
    {
        return bag_capacity_;
    }

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    std::vector<LootType> loot_types_;

    double dog_speed_ = 0.0;
    uint64_t bag_capacity_ = 0;
};

static uint64_t id_counter = 0;

struct DogCoordinates {
    DogCoordinates() = default;
    DogCoordinates(double x, double y) 
    : x_(x)
    , y_(y) {}
    DogCoordinates(Point point) 
    : x_(static_cast<double>(point.x)) 
    , y_(static_cast<double>(point.y)) {};
    bool operator==(const DogCoordinates& rhs) const 
    { 
        return ((x_ == rhs.x_) && (y_ == rhs.y_)); 
    };
    bool operator!=(const DogCoordinates& rhs) const 
    { 
        return ((x_ != rhs.x_) && (y_ != rhs.y_)); 
    };
    double x_, y_;
};

struct DogSpeed {
    double dx_, dy_ = 0.0;

    bool operator==(const DogSpeed& other) const
    {
        return dx_ == other.dx_ && dy_ == other.dy_;
    }
};

enum class Direction {
    NORTH,
    SOUTH,
    WEST,
    EAST
};

class DogsBag {
public:
    DogsBag() = delete;

    explicit DogsBag(size_t capacity_of_bag)
    : capacity_(capacity_of_bag)
    {}

    void AddIdOfTheLoot(uint64_t id)
    {
        if(IsThereFreeSpace())
        {
            id_of_items_.push_back(id);
        }
    }

    const std::deque<uint64_t>& GetAllIds() const
    {
        return id_of_items_;
    }

    size_t GetNumberOfItems() const
    {
        return id_of_items_.size();
    }

    bool IsThereFreeSpace() const
    {
        return capacity_ > id_of_items_.size();
    }

    bool IsEmpty() const
    {
        return id_of_items_.empty();
    }

    void ClearBag()
    {
        id_of_items_.clear();
    }

    uint64_t GetCapacity() const
    {
        return capacity_;
    }

    void SetDogsBagCapacity(uint64_t capacity)
    {
        capacity_ = capacity;
    }


private:
    std::deque<uint64_t> id_of_items_;
    size_t capacity_ = 0;
};

class Dog {
public:

    Dog() = delete;
    explicit Dog(std::string name, size_t bag_capacity)
    : name_(name)
    , bag_(bag_capacity) {}

    uint64_t GetId() const
    {
        return dog_id_;
    }

    void SetId(uint64_t id)
    {
        dog_id_ = id;
    }

    void SetName(std::string&& name)
    {
        name_ = std::move(name);
    }

    void SetName(const std::string& name)
    {
        name_ = name;
    }

    const std::string& GetName() const
    {
        return name_;
    }

    void SetDogCoordinates(DogCoordinates dog_coordinates) 
    {
        dog_coordinates_ = dog_coordinates;
    }

    DogCoordinates GetDogCoordinates() const {
        return dog_coordinates_;
    }

    void SetDogSpeed(DogSpeed dog_speed)
    {
        dog_speed_ = dog_speed;
    }

    DogSpeed GetDogSpeed() const {
        return dog_speed_;
    }

    void SetDogDirection(Direction dog_direction)
    {
        dog_direction_ = dog_direction;
    }

    void SetDogDirection(std::string_view dog_direction)
    {
        if(dog_direction == "R")
        {
            dog_direction_ = Direction::EAST;
        }
        else if(dog_direction == "L")
        {
            dog_direction_ = Direction::WEST;
        }
        else if(dog_direction == "U")
        {
            dog_direction_ = Direction::NORTH;
        }
        else
        {
            dog_direction_ = Direction::SOUTH;
        }
    }

    Direction GetDogDirection() const {
        return dog_direction_;
    }

    std::string GetDogDirectionString() const {
        switch (dog_direction_)
        {
            case Direction::EAST:
                return "R";
            case Direction::WEST:
                return "L";
            case Direction::NORTH:
                return "U";
            case Direction::SOUTH:
                return "D";
            default:
                return "S";
        }

    }

    double GetWidth() const noexcept {
        return width_;
    }

    DogsBag& GetDogsBag()
    {
        return bag_;
    }

    const DogsBag& GetDogsBag() const
    {
        return bag_;
    }

    uint64_t GetScore() const
    {
        return score_;
    }

    void SetScore(uint64_t new_score)
    {
        score_ = new_score;
    }

    uint64_t GetBagCapacity() const
    {
        return bag_.GetCapacity();
    }

    void SetDogsBagCapacity(uint64_t capacity)
    {
        bag_.SetDogsBagCapacity(capacity);
    }

    void SetGameTime(uint64_t game_time_ms)
    {
        game_time_ms_ = game_time_ms;
    }

    uint64_t GetGameTime_ms() const
    {
        return game_time_ms_;
    }

    void SetStandingTime(uint64_t standing_time_ms)
    {
        standing_time_ms_ = standing_time_ms;
    }

    uint64_t GetStandingTime() const
    {
        return standing_time_ms_;
    }

    void SetRetirement(bool flag = true)
    {
        is_retirement_ = flag;
    }

    bool IsOnRetirement() const
    {
        return is_retirement_;
    }

private:
    uint64_t dog_id_ = id_counter++;
    std::string name_ = "";
    DogCoordinates dog_coordinates_;
    DogSpeed dog_speed_ = {0.0, 0.0};
    Direction dog_direction_ = Direction::NORTH;
    DogsBag bag_;
    const double width_ = 0.6;
    uint64_t score_ = 0;
    uint64_t game_time_ms_ = 0;
    uint64_t standing_time_ms_ = 0;
    bool is_retirement_ = false;

};
}