#pragma once

#include <string>
#include <vector>
#include <optional>

#include "./utils/tagged_uuid.h"

namespace database {

namespace domain {

namespace detail {
struct PlayerTag {};
}  // namespace detail

class Player {
public:
    using PlayerId = util::TaggedUUID<detail::PlayerTag>;

    Player(PlayerId id, std::string name, uint64_t score, double total_active_time)
    : id_(id)
    , name_(std::move(name))
    , score_(score)
    , total_active_time_(total_active_time)
    {}
    
    const std::string& GetName() const 
    {
        return name_;
    }

    uint64_t GetScore() const 
    {
        return score_;
    }

    double GetTotalActiveTime() const 
    {
        return total_active_time_;
    }

    const PlayerId& GetId() const 
    {
        return id_;
    }

private:
    PlayerId id_;
    std::string name_;
    uint64_t score_;
    double total_active_time_;
};

class PlayerRepository {
public:
    virtual void Save(const Player& player) = 0;
    virtual void Delete(std::string_view name) = 0;
    virtual std::optional<Player> GetPlayerByName(std::string_view name) = 0;
    virtual std::vector<Player> GetPlayersStat(uint64_t offset, uint64_t limit) = 0;

protected:
    ~PlayerRepository() = default;
};

}//namespace domain

}//namespace database