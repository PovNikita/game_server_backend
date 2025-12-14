#pragma once

//TODO: Переименовать наименование файла motion в controllers

#include "game_items.h"
#include "loot_generator.h"
#include "collision_detector.h"

#include <cmath>
#include <optional>
#include <memory>
#include <chrono>
#include <random>
#include <deque>

namespace model {

static const double EPSILON = 1e-6;

//TODO: Переименовать класс Motion в DogMotionController
class Motion
{
public:
explicit Motion(const Map* map)
: map_(map)
{
    UpdateRoadMaps();
}

void SetMap(const Map* map)
{
    map_ = map;
    UpdateRoadMaps();
}

const Map* GetMap() const noexcept
{
    return map_;
}

void UpdateStateOfDog(uint64_t delta_time_ms, std::shared_ptr<Dog> dog);

private:
const Map* map_ = nullptr;
std::unordered_map<int, std::vector<const Road*>> horizontal_roads_;
std::unordered_map<int, std::vector<const Road*>> vertical_roads_;

void UpdateRoadMaps()
{
    horizontal_roads_.clear();
    vertical_roads_.clear();
    for(auto& road : map_->GetRoads())
    {
        if(road.IsHorizontal())
        {
            horizontal_roads_[road.GetStart().y].push_back(&road);
        }
        else
        {
            vertical_roads_[road.GetStart().x].push_back(&road);
        }
    }
}

double CalculateDistanceBetweenTwoPoints(model::DogCoordinates first_point, model::DogCoordinates end_point){
    return std::sqrt(std::pow((std::abs(first_point.x_ - end_point.x_)), 2) + std::pow((std::abs(first_point.y_ - end_point.y_)), 2)); 
};

model::DogCoordinates CalculateFurthestPointHorizontalRoad(const Road* road, DogCoordinates end_position, std::optional<model::DogCoordinates> &end_point_on_road,
                                            double& max_distance, double& distance_between_start_and_last_point, DogCoordinates dog_coordinates);

//Находит координату в случае если собака движется поперек дороги и нет других смежных дорог 
model::DogCoordinates CalculateFurthestPointOnOneHorizontalRoad(std::shared_ptr<Dog> dog, DogCoordinates end_position);

model::DogCoordinates CalculateFurthestPointVerticalRoad(const Road* road, DogCoordinates end_position, std::optional<model::DogCoordinates> &end_point_on_road,
                                            double& max_distance, double& distance_between_start_and_last_point, DogCoordinates dog_coordinates);

//Находит координату в случае если собака движется поперек дороги и нет других смежных дорог 
model::DogCoordinates CalculateFurthestPointOnOneVerticalRoad(std::shared_ptr<Dog> dog, DogCoordinates end_position);

};

class LootControllerInSession
{
public:
    explicit LootControllerInSession(const Map* map, double period, double propability)
        : map_(map)
        , loot_generator_(std::chrono::milliseconds{static_cast<int64_t>(period * 1000)}, propability) {}

    void Update(std::chrono::milliseconds time_delta, unsigned looter_count, 
                LootsWrappler& loots_to_update);

private:
    const Map* map_ = nullptr;
    loot_gen::LootGenerator loot_generator_;

    std::mt19937_64& GetRng();

    DoublePoint GetRandomPointOnTheMap();

    uint64_t GetRandomType(uint64_t start_value, uint64_t end_value);
};

}// namespace model