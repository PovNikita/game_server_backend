#include "motion.h"

namespace model {

void Motion::UpdateStateOfDog(uint64_t delta_time_ms, std::shared_ptr<Dog> dog)
{
    static const double half_of_grid = 0.5;
    model::DogSpeed dog_speed = dog->GetDogSpeed();
    if(dog_speed.dx_ == 0.0 && dog_speed.dy_ == 0.0)
    {
        return;
    }
    static const double ms_to_sec = 0.001;
    model::DogCoordinates dog_coordinates = dog->GetDogCoordinates();
    model::DogCoordinates finish_point = dog_coordinates;
    model::DogCoordinates prev_finish_point = finish_point;
    auto it_horizontal_roads = horizontal_roads_.find(static_cast<int>(std::floor(dog_coordinates.y_ + half_of_grid)));
    auto it_vertical_roads = vertical_roads_.find(static_cast<int>(std::floor(dog_coordinates.x_ + half_of_grid)));
    model::DogCoordinates end_position(dog_coordinates.x_ + dog_speed.dx_ * (static_cast<double>(delta_time_ms) * ms_to_sec), 
                                        dog_coordinates.y_ + dog_speed.dy_ * (static_cast<double>(delta_time_ms) * ms_to_sec));
    bool is_point_calculated = false;
    while(!is_point_calculated) //Вычисляем координаты точки близкой к целевой(рассчетной) или точки, где столкнется игрок с грницей дороги
    {
        std::optional<model::DogCoordinates> end_point_on_road = dog_coordinates;
        if(dog->GetDogDirection() == model::Direction::EAST || dog->GetDogDirection() == model::Direction::WEST)
        {
            if(it_horizontal_roads != horizontal_roads_.end())
            {
                double max_distance = 0.0;
                double distance_between_start_and_last_point = 0.0;
                for(const auto* road : it_horizontal_roads->second)
                {
                    if(road->IsPointPartOfRoad(finish_point.x_, finish_point.y_))
                    {
                        finish_point = CalculateFurthestPointHorizontalRoad(road, end_position, end_point_on_road, max_distance, 
                                                             distance_between_start_and_last_point, dog_coordinates);
                    }
                }
            }
            else
            {
                finish_point = CalculateFurthestPointOnOneHorizontalRoad(dog, end_position);
                prev_finish_point = finish_point;
            }
        }
        else {
            if(it_vertical_roads != vertical_roads_.end())
            {
                double max_distance = 0.0;
                double distance_between_start_and_last_point = 0.0;
                for(const auto* road : it_vertical_roads->second)
                {
                    if(road->IsPointPartOfRoad(finish_point.x_, finish_point.y_))
                    {
                        finish_point = CalculateFurthestPointVerticalRoad(road, end_position, end_point_on_road, max_distance, 
                                                           distance_between_start_and_last_point, dog_coordinates);
                    }
                }
            }
            else
            {
                finish_point = CalculateFurthestPointOnOneVerticalRoad(dog, end_position);
                prev_finish_point = finish_point;
            }
        }
        it_horizontal_roads = horizontal_roads_.find(static_cast<int>(std::floor(finish_point.y_ + 0.5)));
        it_vertical_roads = vertical_roads_.find(static_cast<int>(std::floor(finish_point.x_ + 0.5)));
        if(std::abs(finish_point.x_ - end_position.x_) < EPSILON && std::abs(finish_point.y_ - end_position.y_) < EPSILON)
        {
            //целевая точка была достигнута
            is_point_calculated = true;
        }
        if(std::abs(finish_point.x_ - prev_finish_point.x_) < EPSILON && std::abs(finish_point.y_ - prev_finish_point.y_) < EPSILON)
        {
            //конец дороги был достигнут
            is_point_calculated = true;
        }
        prev_finish_point = finish_point;
    }
    dog->SetDogCoordinates(finish_point);
    if(finish_point != end_position)
    {
        dog->SetDogSpeed({0.0, 0.0});
    }
}

model::DogCoordinates Motion::CalculateFurthestPointHorizontalRoad(const Road* road, DogCoordinates end_position, std::optional<model::DogCoordinates> &end_point_on_road,
                                                double& max_distance, double& distance_between_start_and_last_point, DogCoordinates dog_coordinates)
{
    model::DogCoordinates point = {0.0, 0.0};
    end_point_on_road.value().x_ = std::clamp(end_position.x_, road->CalculateLeftTopPoint().x, road->CalculateRightLowerPoint().x);
    distance_between_start_and_last_point = CalculateDistanceBetweenTwoPoints(dog_coordinates, end_point_on_road.value());
    if(distance_between_start_and_last_point >= max_distance)
    {
        max_distance = distance_between_start_and_last_point;
        point = end_point_on_road.value();
    }
    return point;
}

//Находит координату в случае если собака движется поперек дороги и нет других смежных дорог 
model::DogCoordinates Motion::CalculateFurthestPointOnOneHorizontalRoad(std::shared_ptr<Dog> dog, DogCoordinates end_position)
{
    model::DogCoordinates point = {0.0, 0.0};
    point.y_ = dog->GetDogCoordinates().y_;
    if(std::abs(end_position.x_ - std::floor(dog->GetDogCoordinates().x_ + 0.5)) > Road::width_ / 2)
    {
        if(dog->GetDogDirection() == model::Direction::WEST)
        {
            point.x_ = std::floor(dog->GetDogCoordinates().x_ + 0.5) - Road::width_ / 2;
        }
        else
        {
            point.x_ = std::floor(dog->GetDogCoordinates().x_ + 0.5) + Road::width_ / 2;
        }
    }
    else
    {
        point.x_ = end_position.x_;
    }
    return point;
}

model::DogCoordinates Motion::CalculateFurthestPointVerticalRoad(const Road* road, DogCoordinates end_position, std::optional<model::DogCoordinates> &end_point_on_road,
                                            double& max_distance, double& distance_between_start_and_last_point, DogCoordinates dog_coordinates)
{
    model::DogCoordinates point = {0.0, 0.0};
    end_point_on_road.value().y_ = std::clamp(end_position.y_, road->CalculateLeftTopPoint().y, road->CalculateRightLowerPoint().y);
    distance_between_start_and_last_point = CalculateDistanceBetweenTwoPoints(dog_coordinates, end_point_on_road.value());
    if(distance_between_start_and_last_point >= max_distance)
    {
        max_distance = distance_between_start_and_last_point;
        point = end_point_on_road.value();
    }
    return point;
}

//Находит координату в случае если собака движется поперек дороги и нет других смежных дорог 
model::DogCoordinates Motion::CalculateFurthestPointOnOneVerticalRoad(std::shared_ptr<Dog> dog, DogCoordinates end_position)
{
    model::DogCoordinates point = {0.0, 0.0};
    point.x_ = dog->GetDogCoordinates().x_;
    if(std::abs(end_position.y_ - std::floor(dog->GetDogCoordinates().y_ + 0.5)) > Road::width_ / 2)
    {
        if(dog->GetDogDirection() == model::Direction::NORTH)
        {
            point.y_ = std::floor(dog->GetDogCoordinates().y_ + 0.5) - Road::width_ / 2;
        }
        else
        {
            point.y_ = std::floor(dog->GetDogCoordinates().y_ + 0.5) + Road::width_ / 2;
        }
    }
    else
    {
        point.y_ = end_position.y_;
    }
    return point;
}


void LootControllerInSession::Update(std::chrono::milliseconds time_delta, unsigned looter_count, 
            LootsWrappler& loots_to_update)
{
    unsigned current_number_of_loot = loots_to_update.GetSize();
    auto loot_count = loot_generator_.Generate(time_delta, current_number_of_loot, looter_count);
    for(unsigned i = 0; i < loot_count; ++i)
    {
        auto position = GetRandomPointOnTheMap();
        loots_to_update.AddLoot(GetRandomType(0, map_->GetLootTypes().size()-1), position);
    } 
}

std::mt19937_64& LootControllerInSession::GetRng() {
    static std::mt19937_64 gen(std::random_device{}());
    return gen;
}

DoublePoint LootControllerInSession::GetRandomPointOnTheMap()
{
    DoublePoint pos;
    auto roads = map_->GetRoads();
    std::uniform_int_distribution<int> distr(0, roads.size()-1);
    int road_num = distr(GetRng());
    if(auto road = roads.at(road_num); road.IsHorizontal())
    {
        pos.y = road.GetEnd().y;
        auto a = (road.GetStart().x < road.GetEnd().x) ? road.GetStart().x : road.GetEnd().x;
        auto b = (road.GetStart().x < road.GetEnd().x) ? road.GetEnd().x : road.GetStart().x;
        std::uniform_int_distribution<int> distr1(a, b);
        pos.x = distr1(GetRng());
    }
    else
    {
        pos.x = road.GetEnd().x;
        auto a = (road.GetStart().y < road.GetEnd().y) ? road.GetStart().y : road.GetEnd().y;
        auto b = (road.GetStart().y < road.GetEnd().y) ? road.GetEnd().y : road.GetStart().y;
        std::uniform_int_distribution<int> distr1(a, b);
        pos.y = distr1(GetRng());
    }
    return pos;
}

uint64_t LootControllerInSession::GetRandomType(uint64_t start_value, uint64_t end_value)
{
    std::uniform_int_distribution<int> distr(start_value, end_value);
    return distr(GetRng());
}

}