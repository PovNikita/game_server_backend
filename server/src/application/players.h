#pragma once
#include <string>
#include <memory>
#include <list>
#include <random>
#include <deque>
#include <optional>



#include "../game/game.h"
#include "../utils/tagged.h"

namespace detail {
struct TokenTag {};

};

namespace app {

using Token = util::Tagged<std::string, detail::TokenTag>;

class Player
{
public:
    Player() = delete;
    explicit Player(std::string dog_name, const model::GameSession* session);
    //const model::Dog* GetDog() const;
    const model::GameSession* GetSession() const;

    std::shared_ptr<const model::Dog> GetDog() const;
    std::shared_ptr<model::Dog> GetDog();

    void SetId(uint64_t id);

    void SetDirection(std::string_view direction);

    void SetCoordinates(model::DogCoordinates coordinates);

    void SetScore(uint64_t score);

    void AddIdOfTheLoot(uint64_t id);

    void MoveAction(std::string_view move_parameter, double speed);

private:
    std::shared_ptr<model::Dog> dog_;
    const model::GameSession* session_;
};

class PlayerTokens
{
public:
    std::shared_ptr<Player> FindPlayerByToken(Token token);
    std::optional<Token> GetTokenByMapIdAndName(std::string user_name, std::string map_id);
    Token AddPlayer(Player& player, std::string map_id, std::string_view token_str = "");

    void RemovePlayer(const std::string& user_name, const std::string& map_id);
    
private:
std::list<Token> tokens_;
std::unordered_map<Token, std::shared_ptr<Player>, util::TaggedHasher<Token>> token_to_player_;
std::unordered_map<std::string, std::unordered_map<std::string_view, Token*>> map_id_user_name_to_token_;

std::random_device random_device_;
std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};

    void ResetGenerators();
};

struct PlayerInfo {
    std::string id;
    std::string name_;
    uint64_t score_;
    double total_active_time_ms_;
};

class Players {
public:
    std::pair<Player&, Token> Add(std::string dog_name, const model::GameSession& session, std::string_view dog_token = "");
    std::shared_ptr<Player> FindByToken(Token token);
    std::deque<std::shared_ptr<Player>> GetAllPlayersInSessionWithCurrentPlayer(Token current_player_token);
    std::list<std::shared_ptr<Player>>& GetAllPlayers();
    std::optional<Token> GetTokenByMapIdAndName(std::string user_name, std::string map_id);
    std::deque<PlayerInfo> RemoveRetiredPlayersInSession(const model::Map::Id& map_id, model::GameSession* game_session_ptr);
    std::optional<PlayerInfo> RemovePlayer(const model::Map::Id& map_id, std::string name);

private:
    std::list<std::shared_ptr<Player>> players_;
    std::unordered_map<model::Map::Id, std::deque<std::shared_ptr<Player>>, util::TaggedHasher<model::Map::Id>> session_to_players_;
    PlayerTokens players_tokens_;
};
}