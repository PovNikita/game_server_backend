#include "game.h"
#include "collision_detector.h"

#include <stdexcept>
#include <algorithm>
#include <unordered_set>

namespace model {
using namespace std::literals;

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

//GameSession
void GameSession::AddDog(std::shared_ptr<Dog> dog, bool is_random)
{
    auto dog_id = dog->GetId();
    model::Point pos;
    if(is_random == true)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        auto roads = map_->GetRoads();
        std::uniform_int_distribution<> distr(0, roads.size()-1);
        int road_num = distr(gen);
        if(auto road = roads.at(road_num); road.IsHorizontal())
        {
            pos.y = road.GetEnd().y;
            auto a = (road.GetStart().x < road.GetEnd().x) ? road.GetStart().x : road.GetEnd().x;
            auto b = (road.GetStart().x < road.GetEnd().x) ? road.GetEnd().x : road.GetStart().x;
            std::uniform_int_distribution<> distr1(a, b);
            pos.x = distr1(gen);
        }
        else
        {
            pos.x = road.GetEnd().x;
            auto a = (road.GetStart().y < road.GetEnd().y) ? road.GetStart().y : road.GetEnd().y;
            auto b = (road.GetStart().y < road.GetEnd().y) ? road.GetEnd().y : road.GetStart().y;
            std::uniform_int_distribution<> distr1(a, b);
            pos.y = distr1(gen);
        }
    }
    else
    {
        pos = map_->GetRoads()[0].GetStart();
    }
    dog_id_to_position_.insert({dog->GetId(), pos});
    std::shared_ptr<Dog> copy_dog = dog;
    dogs_.insert({dog->GetId(), copy_dog});
    dogs_.at(dog_id).get()->SetDogCoordinates(pos);
}

void GameSession::UpdateStateOfSession(uint64_t delta_time_ms, uint64_t retirement_time_ms)
{
    loot_controller_.Update(std::chrono::milliseconds{delta_time_ms}, dogs_.size(), loots_);
    
    std::vector<collision_detector::Gatherer> dogs_gatherers;
    dogs_gatherers.reserve(dogs_.size());
    for(auto [id, dog] : dogs_)
    {
        bool is_speed_zero = false;
        dog->SetGameTime(dog->GetGameTime_ms() + delta_time_ms);
        if(std::abs(dog->GetDogSpeed().dx_ - 0.0) < EPSILON && std::abs(dog->GetDogSpeed().dy_ - 0.0) < EPSILON)
        {
            is_speed_zero = true;
            dog->SetStandingTime(delta_time_ms + dog->GetStandingTime());
            if(dog->GetStandingTime() >= retirement_time_ms)
            {
                dog->SetRetirement();
            }
        }
        else
        {
            dog->SetStandingTime(0);
        }
        collision_detector::Gatherer gatherer;
        gatherer.start_pos = {.x = dog->GetDogCoordinates().x_, .y = dog->GetDogCoordinates().y_};
        motion_controller_.UpdateStateOfDog(delta_time_ms, dog);
        gatherer.end_pos = {.x = dog->GetDogCoordinates().x_, .y = dog->GetDogCoordinates().y_};
        gatherer.width = dog->GetWidth();
        gatherer.id_ = id;
        dogs_gatherers.push_back(gatherer);
    }

    std::vector<collision_detector::Item> items;
    auto all_loots = this->GetAllLoots();
    items.reserve(this->GetMap()->GetOffices().size() + this->GetLoots().size());
    uint64_t start_index_of_offices = 0;
    {
        size_t i = 0;
        for(const auto& loot : all_loots)
        {
            if(loot != nullptr && !loots_.IsBusyLoot(i))
            {
                items.push_back(collision_detector::Item{.position = {.x = loot->GetPosition().x, .y = loot->GetPosition().y}, .width = loot->GetWidth(), .id_ = i});
            }
            ++i;
        }
        start_index_of_offices = i;
        for(const auto& office : this->GetMap()->GetOffices())
        {
            items.emplace_back(collision_detector::Item{.position = {.x = static_cast<double>(office.GetPosition().x), .y = static_cast<double>(office.GetPosition().y)}, .width = office.GetWidth(), .id_ = i});
            ++i;
        }
    }

    auto collision_list = collision_detector::FindGatherEvents(collision_detector::ItemGathererProviderWrappler(items, dogs_gatherers), false);
    if(!collision_list.empty())
    {
        for(size_t i = 0; i < collision_list.size(); ++i)
        {
            auto dog = dogs_.at(collision_list.at(i).gatherer_id);
            if(auto item_id = collision_list.at(i).item_id; item_id < start_index_of_offices)
            {
                //id соответсвует loot
                if(dog->GetDogsBag().IsThereFreeSpace() && !loots_.IsBusyLoot(item_id) && loots_.GetAllLoots().at(item_id) != nullptr)
                {
                    //Есть место и лут ещё не был поднят и сдан
                    dog->GetDogsBag().AddIdOfTheLoot(item_id);
                    loots_.MarkBusy(item_id);
                }
            }
            else
            {
                //id соответсвует office
                if(!dog->GetDogsBag().IsEmpty())
                {
                    for(auto id : dog->GetDogsBag().GetAllIds())
                    {
                        auto score = map_->GetLootTypes().at(loots_.GetLoot(id).GetType()).GetValue();
                        dog->SetScore(dog->GetScore() + score);
                        loots_.PopLoot(id);
                    }
                    dog->GetDogsBag().ClearBag();
                }
            }
        }
    }
}

}  // namespace model
