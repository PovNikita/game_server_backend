#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <cstdint>
#include <optional>
#include <memory>
#include <random>
#include <deque>

#include "../utils/tagged.h"
#include "game_items.h"
#include "motion.h"

namespace model {

class GameSession {
public:
GameSession() = delete;

explicit GameSession(const Map* map, double period, double propability)
    : map_(map)
    , motion_controller_(map_)
    , loot_controller_(map_, period, propability) {
    }

void AddDog(std::shared_ptr<Dog> dog, bool is_random = false);

void AddMap(const Map* map)
{
    if(map_ != nullptr)
    {
        throw std::runtime_error("GameSession can't own 2 maps");
    }
    map_ = map;
    motion_controller_.SetMap(map_);
}

void AddLoot(uint64_t type, DoublePoint position)
{
    loots_.AddLoot(type, position);
}


 const std::deque<const Loot*> GetLoots() const
{
    return loots_.GetLoots();
}

const std::deque<Loot*>& GetAllLoots() const
{
    return loots_.GetAllLoots();
}

bool IsBusyLoot(uint64_t loot_id) const
{
    return loots_.IsBusyLoot(loot_id);
}

void ChangeMap(const Map* map)
{
    map_ = map;
    motion_controller_.SetMap(map_);
}

const Map* GetMap() const
{
    return map_;
}

const Dog* GetDog(int id)
{
    if(dogs_.find(id) != dogs_.end())
    {
        return (dogs_.at(id)).get();
    }
}

void RemoveDog(uint64_t id)
{
    if(auto it = dogs_.find(id); it != dogs_.end())
    {
        dogs_.erase(it);
    }
}

void UpdateStateOfSession(uint64_t delta_time_ms, uint64_t retirement_time_ms);

LootsWrappler& GetLootsInSession()
{
    return loots_;
}

private:
std::unordered_map<uint64_t, std::shared_ptr<Dog>> dogs_;
std::unordered_map<uint64_t, Point> dog_id_to_position_;
const Map* map_ = nullptr;
Motion motion_controller_;
LootControllerInSession loot_controller_;
LootsWrappler loots_;

};

class MapIdHasher {
public:
    size_t operator()(const Map::Id& map_id) const {
        return std::hash<std::string>{}(*map_id);
    }
};

static const uint64_t default_bag_capacity = 3;
static const double default_retirement_time_s = 60;
class Game {
public:
    using Maps = std::vector<Map>;

    void SetPathToStaticFiles(std::string_view path_str_from_cur_dir)
    {
        path_to_static_files_ = std::move(
                std::filesystem::current_path() / std::filesystem::path(path_str_from_cur_dir)
            );
    }

    const std::string GetPathToStaticFiles()
    {
        return path_to_static_files_.string();
    }

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    const GameSession* CreateGameSession(Map::Id map_id)
    {
        auto map = FindMap(map_id);
        if(map == nullptr)
        {
            return nullptr;
        }
        map_id_to_game_session_.insert({map_id, std::make_unique<GameSession>(map, loot_generator_config_->GetPeriod(), loot_generator_config_->GetPropability())});
        return map_id_to_game_session_.at(map_id).get();
    }

    const GameSession* FindGameSession(Map::Id map_id)
    {
        return GetGameSession(map_id);
    }

    GameSession* GetGameSession(Map::Id map_id)
    {
        if(map_id_to_game_session_.find(map_id) != map_id_to_game_session_.end())
        {
            return map_id_to_game_session_.at(map_id).get();
        }
        return nullptr;
    }

    void UpdateStateOfGame(uint64_t delta_time_ms)
    {
        for(auto& [id, game_session] : map_id_to_game_session_)
        {
            game_session->UpdateStateOfSession(delta_time_ms, GetRetirementTimeMs());
        }
    }

    void SetLootGeneratorConf(double period, double propability)
    {
        loot_generator_config_->SetPeriod(period).SetPropability(propability);
    }

    std::shared_ptr<LootGeneratorConf> GetLootGeneratorConf()
    {
        return loot_generator_config_;
    }

    void SetRetirementTime(double time_s)
    {
        retirement_time_s_ = time_s;
    }

    double GetRetirementTime() const
    {
        return retirement_time_s_;
    }

    uint64_t GetRetirementTimeMs() const
    {
        return static_cast<uint64_t>(retirement_time_s_ * 1000.0);
    }


private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    std::filesystem::path path_to_static_files_;
    std::unordered_map<Map::Id, std::unique_ptr<GameSession>, MapIdHasher> map_id_to_game_session_;
    std::shared_ptr<LootGeneratorConf> loot_generator_config_ = std::make_shared<LootGeneratorConf>();
    double retirement_time_s_ = default_retirement_time_s;
};

}  // namespace model
