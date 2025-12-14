#include "players.h"

#include <algorithm>

namespace app{
//Player

Player::Player(std::string dog_name, const model::GameSession* session)
    : dog_(std::make_shared<model::Dog>(dog_name, session->GetMap()->GetBagCapacity()))
    , session_(session)
    {}

std::shared_ptr<const model::Dog> Player::GetDog() const
{
    return dog_;
} 
const model::GameSession* Player::GetSession() const
{
    return session_;
}

std::shared_ptr<model::Dog> Player::GetDog()
{
    return dog_;
}

void Player::SetId(uint64_t id)
{
    dog_->SetId(id);
}

void Player::SetDirection(std::string_view direction)
{
    dog_->SetDogDirection(direction);
}

void Player::SetCoordinates(model::DogCoordinates coordinates)
{
    dog_->SetDogCoordinates(coordinates);
}

void Player::SetScore(uint64_t score)
{
    dog_->SetScore(score);
}

void Player::AddIdOfTheLoot(uint64_t id)
{
    dog_->GetDogsBag().AddIdOfTheLoot(id);
}

void Player::MoveAction(std::string_view move_parameter, double speed)
{
    model::DogSpeed current_dog_speed = dog_->GetDogSpeed();
    if(move_parameter == "U")
    {
        dog_->SetDogSpeed({0.0, -speed});
        dog_->SetDogDirection(model::Direction::NORTH);
    }
    else if(move_parameter == "D")
    {
        dog_->SetDogSpeed({0.0, speed});
        dog_->SetDogDirection(model::Direction::SOUTH);
    }
    else if(move_parameter == "L")
    {
        dog_->SetDogSpeed({-speed, 0.0});
        dog_->SetDogDirection(model::Direction::WEST);
    }
    else if(move_parameter == "R")
    {
        dog_->SetDogSpeed({speed, 0.0});
        dog_->SetDogDirection(model::Direction::EAST);
    }
    else
    {
        dog_->SetDogSpeed({0.0, 0.0});
    }
}


// PlayerTokens
std::shared_ptr<Player> PlayerTokens::FindPlayerByToken(Token token) {
    if(token_to_player_.find(token) != token_to_player_.end())
    {
        return token_to_player_.at(token);
    }
    return nullptr;
}

std::optional<Token> PlayerTokens::GetTokenByMapIdAndName(std::string user_name, std::string map_id)
{
    if(auto it = map_id_user_name_to_token_.find(map_id); it != map_id_user_name_to_token_.end())
    {
        if(it->second.find(user_name) != it->second.end())
        {
            return *it->second.at(user_name);
        }
    }
    return std::nullopt;
}

std::string GetTokenHexStringFromNumbers(uint64_t m_part, uint64_t l_part)
{
    std::stringstream str_stream;
    str_stream << std::hex
            << std::setw(16) << std::setfill('0') << m_part
            << std::setw(16) << std::setfill('0') << l_part;
    return str_stream.str();
}

Token PlayerTokens::AddPlayer(Player& player, std::string map_id, std::string_view token_str) {
    if(auto it = map_id_user_name_to_token_.find(map_id); it != map_id_user_name_to_token_.end())
    {
        if(it->second.find(player.GetDog()->GetName()) != it->second.end())
        {
            return *(it->second.at(player.GetDog()->GetName()));
        }
    }
    Token token("");
    if(token_str.empty())
    {
        *token = GetTokenHexStringFromNumbers(generator1_(), generator2_());
    }
    else
    {
        *token = std::string{token_str};
    }
    tokens_.push_back(token);
    token_to_player_.insert({tokens_.back(), std::make_shared<Player>(player)});
    map_id_user_name_to_token_[map_id].insert({player.GetDog()->GetName(), &tokens_.back()});
    ResetGenerators();
    return token;
}

void PlayerTokens::RemovePlayer(const std::string& user_name, const std::string& map_id)
{
    if(auto it = map_id_user_name_to_token_.find(map_id); it != map_id_user_name_to_token_.end())
    {
        if(auto it_second = it->second.find(user_name); it_second != it->second.end())
        {
            Token token = *(it_second->second);
            {
                auto it3 = token_to_player_.find(token);
                token_to_player_.erase(it3);
            }
            {
                auto it3 = std::find_if(tokens_.begin(), tokens_.end(), [&token](const auto& other) {return *other == *token;});
                tokens_.erase(it3);
            }
            {
                it->second.erase(it_second);
            }
        }
    }
}

void PlayerTokens::ResetGenerators() {
    auto make_seed = [this]() {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    };
    generator1_.seed(make_seed());
    generator2_.seed(make_seed());
}

//Players
std::pair<Player&, Token> Players::Add(std::string dog_name, const model::GameSession& session, std::string_view dog_token) {
    auto possible_token = players_tokens_.GetTokenByMapIdAndName(dog_name, *session.GetMap()->GetId());
    if(possible_token.has_value())
    {
        return {*(players_tokens_.FindPlayerByToken(possible_token.value())), possible_token.value()};
    }
    players_.push_back(std::make_shared<Player>(dog_name, &session));
    auto token = players_tokens_.AddPlayer(*(players_.back().get()), *session.GetMap()->GetId(), dog_token);
    session_to_players_[session.GetMap()->GetId()].push_back(players_.back());
    return {*(players_.back().get()), token};
}

std::shared_ptr<Player> Players::FindByToken(Token token) {
    return players_tokens_.FindPlayerByToken(token);
}

std::deque<std::shared_ptr<Player>> Players::GetAllPlayersInSessionWithCurrentPlayer(Token current_player_token)
{
    auto current_player = FindByToken(current_player_token);
    auto map_id = current_player->GetSession()->GetMap()->GetId();
    auto players = session_to_players_.at(map_id);
    auto it = std::find_if(players.begin(), players.end(), [current_player](std::shared_ptr<Player> player_to_compare){
        return (current_player->GetDog()->GetId() == player_to_compare->GetDog()->GetId());
    });
    //players.erase(it);
    std::sort(players.begin(), players.end(), [](std::shared_ptr<Player> lhs, std::shared_ptr<Player> rhs){
        return lhs->GetDog()->GetId() < rhs->GetDog()->GetId();
    });
    return players;
}

std::list<std::shared_ptr<Player>>& Players::GetAllPlayers()
{
    return players_;
}

std::optional<Token> Players::GetTokenByMapIdAndName(std::string user_name, std::string map_id)
{
    return players_tokens_.GetTokenByMapIdAndName(user_name, map_id);
}



std::deque<PlayerInfo> Players::RemoveRetiredPlayersInSession(const model::Map::Id& map_id, model::GameSession* game_session_ptr)
{
    std::deque<PlayerInfo> retired_players;
    if(auto it = session_to_players_.find(map_id); it != session_to_players_.end())
    {
        for(std::shared_ptr<Player> player : it->second)
        {
            if(player->GetDog()->IsOnRetirement())
            {
                uint64_t dog_id = player->GetDog()->GetId();
                std::optional<PlayerInfo> player_info = RemovePlayer(map_id, player->GetDog()->GetName());
                if(player_info.has_value())
                {
                    game_session_ptr->RemoveDog(dog_id);
                    retired_players.emplace_back(std::move(player_info.value()));
                }
            }
        }
    }
    return retired_players;
}

std::optional<PlayerInfo> Players::RemovePlayer(const model::Map::Id& map_id, std::string name)
{
    std::optional<PlayerInfo> player;
    if(auto it_session_to_players = session_to_players_.find(map_id); it_session_to_players != session_to_players_.end())
    {
        {
            //Remove player in session_to_players_
            auto it = std::find_if(it_session_to_players->second.begin(), it_session_to_players->second.end(), [&name](std::shared_ptr<Player> player) {
                return player->GetDog()->GetName() == name;
            });
            if(it != it_session_to_players->second.end())
            {
                it_session_to_players->second.erase(it);
            }
        }
        {
            //Remove player in Tokens
            players_tokens_.RemovePlayer(name, *map_id);
        }
        {
            static const double sec_to_ms = 0.001;
            //Remove player in players_
            auto it = std::find_if(players_.begin(), players_.end(), [&name](const auto& player) {return player->GetDog()->GetName() == name;});
            if(it != players_.end())
            {
                auto dog = it->get()->GetDog();
                PlayerInfo player_info = {.id = std::to_string(dog->GetId()), .name_ = dog->GetName(), .score_ = dog->GetScore(), .total_active_time_ms_ = static_cast<double>(dog->GetGameTime_ms())};
                player = std::move(player_info);
                players_.erase(it);
            }
        }
    }
    return player;
}

}