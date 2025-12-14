#include "serialization.h"

namespace serialization
{
    DogRepr::DogRepr(const model::Dog& dog, const std::string& token, const std::string& map_id)
        : id_(dog.GetId())
        , name_(dog.GetName())
        , pos_(dog.GetDogCoordinates())
        , bag_capacity_(dog.GetBagCapacity())
        , speed_(dog.GetDogSpeed())
        , direction_(dog.GetDogDirection())
        , score_(dog.GetScore())
        , bag_content_(dog.GetDogsBag().GetAllIds())
        , game_time_ms_(dog.GetGameTime_ms())
        , standing_time_ms_(dog.GetStandingTime())
        , is_retirement_(dog.IsOnRetirement())
        , map_id_(map_id)
        , token_(token) {
    }

    [[nodiscard]] std::tuple<model::Dog, DogRepr::Token, DogRepr::MapID> DogRepr::Restore() const {
        model::Dog dog{name_, bag_capacity_};
        dog.SetId(id_);
        dog.SetDogCoordinates(pos_);
        dog.SetDogSpeed(speed_);
        dog.SetDogDirection(direction_);
        dog.SetScore(score_);
        dog.SetGameTime(game_time_ms_);
        dog.SetStandingTime(standing_time_ms_);
        dog.SetRetirement(is_retirement_);
        for (const auto& item_id : bag_content_) {
            dog.GetDogsBag().AddIdOfTheLoot(item_id);
        }
        return {dog, token_, map_id_};
    }
    
} // namespace serialization
