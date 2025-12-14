#include "./postgres.h"


namespace database
{

namespace postgres
{

void PlayerRepositoryImpl::Save(const domain::Player& player) 
{
    w_.exec_params(
            "INSERT INTO retired_players (id, name, score, play_time_ms) VALUES ($1, $2, $3, $4);"_zv,
                player.GetId().ToString(), player.GetName(), player.GetScore(), player.GetTotalActiveTime());
}

void PlayerRepositoryImpl::Delete(std::string_view name) 
{
    w_.exec_params("DELETE FROM retired_players WHERE name=$1;"_zv, name);
}

std::optional<domain::Player> PlayerRepositoryImpl::GetPlayerByName(std::string_view name) 
{

    auto query_text = "SELECT id, name, score, play_time_ms FROM retired_players WHERE name = $1;"_zv;
    auto [id, name_, score, play_time] = w_.query1<std::string, std::string, int, double>(query_text);
    if(id.empty())
    {
        return std::nullopt;
    }
    domain::Player player(domain::Player::PlayerId::FromString(id), name_, score, play_time);
    return player;
}

std::vector<domain::Player> PlayerRepositoryImpl::GetPlayersStat(uint64_t offset, uint64_t limit) 
{
    std::vector<domain::Player> players;
    auto query_text = "SELECT id, name, score, play_time_ms FROM retired_players LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset) + ";";
    for(auto [id, name, score, play_time] : w_.query<std::string, std::string, int, double>(query_text))
    {
        players.emplace_back(domain::Player::PlayerId::FromString(id), name, score, play_time);
    }
    return players;
}

    
} // namespace postgres


} // namespace database