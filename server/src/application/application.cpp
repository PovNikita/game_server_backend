#include <sstream>
#include <string>

#include "application.h"
#include "fstream"
#include "../serialization/serialization.h"



namespace app
{
void Ticker::Start() {
    net::dispatch(strand_, [self = shared_from_this()] {
        self->last_tick_ = Clock::now();
        self->ScheduleTick();
    });
}

void Ticker::ScheduleTick() {
    assert(strand_.running_in_this_thread());
    timer_.expires_after(period_);
    timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
        self->OnTick(ec);
    });
}

void Ticker::OnTick(sys::error_code ec) {
    using namespace std::chrono;
    assert(strand_.running_in_this_thread());

    if (!ec) {
        auto this_tick = Clock::now();
        auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
        last_tick_ = this_tick;
        try {
            handler_(delta);
        } catch (...) {
        }
        ScheduleTick();
    }
}

Application::Application(model::Game& game, bool is_randomize_spawn_points, Strand& strand, database::Database& database)
: game_(game)
, is_randomize_spawn_points_(is_randomize_spawn_points)
, strand_(strand)
, database_(database)
{

}

void Application::RecoverFromFile(std::string_view state_file_path)
{
    path_to_state_file_ = std::filesystem::absolute(state_file_path);
    if(std::filesystem::exists(path_to_state_file_))
    {
        RecoverState();
    }
    else
    {
        std::ofstream file(path_to_state_file_);
        if(file.is_open())
        {
            file << "";
            file.close();
        }
    }

}

void Application::TurnOnAutoTickingMode(std::chrono::milliseconds tick_value_ms) {
    is_ticker_running_ = true;
    ticker_ = std::make_shared<Ticker>(
        strand_,
        tick_value_ms,
        [&](std::chrono::milliseconds delta) { UpdateApplication(delta); }
    );
    ticker_->Start();
}

const model::Map* Application::FindMap(const model::Map::Id& id) const noexcept {
    return game_.FindMap(id);
}

const model::Game::Maps& Application::GetMaps() const noexcept {
    return game_.GetMaps();
}

std::pair<Token, uint64_t> Application::JoinGame(model::Map::Id map_id, std::string user_name)
{
    const model::GameSession* game_session_ptr = game_.FindGameSession(map_id);
    if(game_session_ptr == nullptr)
    {
        game_session_ptr = game_.CreateGameSession(map_id);
    }
    //Is there with the same name
    auto [player, token] = players_.Add(user_name, *game_session_ptr);
    game_.GetGameSession(map_id)->AddDog(player.GetDog(), is_randomize_spawn_points_);
    return {token, player.GetDog()->GetId()};
}

std::shared_ptr<Player> Application::FindPlayerByToken(Token token) {
    return players_.FindByToken(token);
}

const std::deque<std::shared_ptr<Player>> Application::GetAllPlayersInSessionWithCurrentPlayer(Token current_player_token)
{
    return players_.GetAllPlayersInSessionWithCurrentPlayer(current_player_token);
}

const model::GameSession* Application::GetGameSessionByPlayer(Token player_token)
{
    return players_.FindByToken(player_token)->GetSession();
}

void Application::UpdateApplication(uint64_t delta_time_ms)
{
    if(delta_time_ms > 0)
    {
        game_.UpdateStateOfGame(delta_time_ms);
        tick_signal_(std::chrono::milliseconds(delta_time_ms));
        HandleLeavedPlayers();
    }
}

void Application::UpdateApplication(std::chrono::milliseconds delta_time_ms)
{
    UpdateApplication(delta_time_ms.count());
}

bool Application::IsTickerRunning() const noexcept {
    return is_ticker_running_;
}

void Application::SaveState()
{
    if(path_to_state_file_ != "")
    {
        using namespace std::literals;
        std::filesystem::path temp_file_path(path_to_state_file_.parent_path() / std::filesystem::path("temp.json"s));
        std::ofstream temp_file(temp_file_path);
        if(temp_file.is_open())
        {
            boost::json::object file_json;
            SerializePlayers(file_json);
            SerializeItems(file_json);
            temp_file << boost::json::serialize(file_json);
            temp_file.close();
            std::filesystem::rename(temp_file_path, path_to_state_file_);
        }
    }
}

void Application::SerializePlayers(boost::json::object& file_json)
{
    boost::json::array players;
    for(const auto& player : players_.GetAllPlayers())
    {
        
        std::stringstream string_stream;
        boost::archive::text_oarchive oa{string_stream};
        serialization::DogRepr ser_dog((*player->GetDog()), *(players_.GetTokenByMapIdAndName(player->GetDog()->GetName(), *(player->GetSession()->GetMap()->GetId())).value()),  
                                                    *(player->GetSession()->GetMap()->GetId()));
        oa << ser_dog;

        players.push_back(boost::json::string(string_stream.str()));
    }
    file_json.emplace("Players", players);
}

void Application::SerializeItems(boost::json::object& file_json)
{
    boost::json::array items_json;
    for(auto map : game_.GetMaps())
    {
        if(model::GameSession* game_session = game_.GetGameSession(map.GetId()); game_session != nullptr)
        {
            std::stringstream string_stream;
            boost::archive::text_oarchive oa{string_stream};
            oa << game_session->GetLootsInSession();
            boost::json::object loots_in_map{
                {"map_id",       *(map.GetId())},
                {"loots_in_map", string_stream.str()}};

            items_json.push_back(loots_in_map);
        }
    }
    file_json.emplace("Items", items_json);
}

void Application::RecoverState()
{
    using namespace std::literals;
    std::ifstream json_file(path_to_state_file_);
    if(!json_file.is_open())
    {
        throw std::runtime_error("Can't open json state file");
    }
    std::string json_string = ""s;
    std::string temp;
    while(getline(json_file, temp)) {
        json_string += temp;
    }
    json_file.close();
    boost::json::value parsed_json;
    try
    {
        if(!json_string.empty())
        {
            parsed_json = boost::json::parse(json_string);
            if(parsed_json.is_object())
            {
                RestorePlayers(parsed_json.as_object().at("Players").as_array());
                RestoreItems(parsed_json.as_object().at("Items").as_array());
            }

        }
    }
    catch(const std::exception& e)
    {
        throw;
    }
}

uint64_t GetUint64_tFromJson(const boost::json::value& val)
{
    if(val.is_int64())
    {
        return static_cast<uint64_t>(val.as_int64());
    }
    else
    {
        return val.as_uint64();
    }
}


void Application::RestorePlayers(const boost::json::array& players_json)
{
    uint64_t max_id_dog = 0;
    for(auto json_player : players_json)
    {
        std::stringstream string_stream;
        string_stream.str(json_player.as_string().c_str());
        boost::archive::text_iarchive ia{string_stream};
        serialization::DogRepr ser_dog;
        ia >> ser_dog;
        auto[dog, dog_token, map_id] = ser_dog.Restore();
        const model::GameSession* game_session_ptr = game_.FindGameSession(model::Map::Id{map_id});
        if(game_session_ptr == nullptr)
        {
            game_session_ptr = game_.CreateGameSession(model::Map::Id{map_id});
        }
        //Is there with the same name
        auto [player, token] = players_.Add(dog.GetName(), *game_session_ptr, dog_token);
        game_.GetGameSession(model::Map::Id{map_id})->AddDog(player.GetDog());
        if(dog.GetId() > max_id_dog)
        {
            max_id_dog = dog.GetId();
        }
        player.SetId(dog.GetId());
        player.SetCoordinates(dog.GetDogCoordinates());
        player.SetDirection(dog.GetDogDirectionString());
        player.SetScore(dog.GetScore());
        for(const auto& loot_id : dog.GetDogsBag().GetAllIds())
        {
            player.AddIdOfTheLoot(loot_id);
        }
    }
    model::id_counter = max_id_dog + 1;
}

void Application::RestoreItems(boost::json::array& file_json)
{
    for(auto& json_obj : file_json)
    {
        std::stringstream string_stream;
        string_stream.str(json_obj.as_object().at("loots_in_map").as_string().c_str());
        boost::archive::text_iarchive ia{string_stream};
        model::LootsWrappler loots_wrappler;
        ia >> loots_wrappler;
        std::string map_id(json_obj.as_object().at("map_id").as_string().c_str());
        game_.GetGameSession(model::Map::Id{map_id})->GetLootsInSession() = std::move(loots_wrappler);
    }
}

void Application::HandleLeavedPlayers()
{
    for(auto map : game_.GetMaps())
    {
        model::GameSession* game_session_ptr = game_.GetGameSession(map.GetId());
        if(game_session_ptr != nullptr)
        {
            auto retired_players = players_.RemoveRetiredPlayersInSession(map.GetId(), game_session_ptr);
            if(!retired_players.empty())
            {
                database::app::UseCasesImpl use_cases(database_.GetUnitOfWorkImpl());
                for(auto player_info : retired_players)
                {
                    use_cases.SavePlayer(database::domain::Player(database::domain::Player::PlayerId::New(), player_info.name_, player_info.score_, player_info.total_active_time_ms_));
                }
            }
        }

    }
}

}