#pragma once
#include <string>
#include <tuple>
#include <algorithm>
#include <queue>

#include <boost/serialization/deque.hpp>
#include <boost/serialization/unordered_set.hpp>
#include <boost/serialization/queue.hpp>

#include <boost/archive/binary_iarchive.hpp> 

#include "../game/game_items.h"

namespace model {

template <typename Archive>
void serialize(Archive& ar, model::DogCoordinates& dog_coord, [[maybe_unused]] const unsigned version) {
    ar& dog_coord.x_;
    ar& dog_coord.y_;
}

template <typename Archive>
void serialize(Archive& ar, model::DoublePoint& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, model::DogSpeed& dog_speed, [[maybe_unused]] const unsigned version) {
    ar& dog_speed.dx_;
    ar& dog_speed.dy_;
}

template <typename Archive>
void serialize(Archive& ar, model::Loot& loot, [[maybe_unused]] const unsigned version) {
    ar& loot.position_;
    ar& loot.type_;
    ar& loot.width_;
}

template <typename Archive>
void serialize(Archive& ar, model::LootsWrappler& loots_wrappler, [[maybe_unused]] const unsigned version) {
    ar& loots_wrappler.loots_;
    ar& loots_wrappler.freed_ids_;
    ar& loots_wrappler.busy_loots_id_;

    if constexpr (Archive::is_loading::value) {
        // Восстанавливаем loots_ptr_
        loots_wrappler.loots_ptr_.clear();
        for(size_t i = 0; i < loots_wrappler.loots_.size(); ++i)
        {
            loots_wrappler.loots_ptr_.push_back(&(loots_wrappler.loots_.at(i)));
        }
        std::queue<size_t> copy_of_freed_ids_(loots_wrappler.freed_ids_);
        while(!copy_of_freed_ids_.empty())
        {
            size_t id = copy_of_freed_ids_.front();
            copy_of_freed_ids_.pop();
            loots_wrappler.loots_ptr_.at(id) = nullptr;
        }
    }
}

}  // namespace model

namespace serialization {

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;
    using Token = std::string;
    using MapID = std::string;

    explicit DogRepr(const model::Dog& dog, const std::string& token, const std::string& map_id);

    [[nodiscard]] std::tuple<model::Dog, Token, MapID> Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& name_;
        ar& pos_;
        ar& bag_capacity_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_content_;
        ar& map_id_;
        ar& token_;
    }

private:
    uint64_t id_ = 0;
    std::string name_;
    model::DogCoordinates pos_;
    uint64_t bag_capacity_ = 0;
    model::DogSpeed speed_;
    model::Direction direction_ = model::Direction::NORTH;
    uint64_t score_ = 0;
    std::deque<uint64_t> bag_content_;
    std::string map_id_;
    std::string token_;
    uint64_t game_time_ms_ = 0;
    uint64_t standing_time_ms_ = 0;
    bool is_retirement_ = false;
};

}  // namespace serialization
