#pragma once
#include <unordered_map>
#include <random>
#include <memory>
#include <deque>
#include <optional>
#include <algorithm>
#include <chrono>
#include <functional>
#include <list>
#include <filesystem>
#include <vector>

#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/signals2.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include "../game/game.h"
#include "../database/database.h"
#include "../database/use_cases.h"

#include "players.h"

namespace app{

namespace net = boost::asio;
namespace sys = boost::system;

class Ticker : public std::enable_shared_from_this<Ticker> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(std::chrono::milliseconds delta)>;

    // Функция handler будет вызываться внутри strand с интервалом period
    Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
        : strand_{strand}
        , period_{period}
        , handler_{std::move(handler)} {
    }

    void Start();

private:
    void ScheduleTick();

    void OnTick(sys::error_code ec);

    using Clock = std::chrono::steady_clock;

    Strand strand_;
    std::chrono::milliseconds period_;
    net::steady_timer timer_{strand_};
    Handler handler_;
    std::chrono::steady_clock::time_point last_tick_;
};

class Application {
public:
    static const uint64_t max_limit_records = 100;

    using milliseconds = std::chrono::milliseconds;
    using TickSignal = boost::signals2::signal<void(milliseconds delta)>;

    [[nodiscard]] boost::signals2::connection DoOnTick(const TickSignal::slot_type& handler) {
        return tick_signal_.connect(handler);
    }

    using Strand = net::strand<net::io_context::executor_type>;
    Application(model::Game& game, bool is_randomize_spawn_points, Strand& strand, database::Database& database);

    void RecoverFromFile(std::string_view state_file_path);
    void TurnOnAutoTickingMode(std::chrono::milliseconds tick_value_ms);

    const model::Map* FindMap(const model::Map::Id& id) const noexcept;
    const model::Game::Maps& GetMaps() const noexcept;

    std::pair<Token, uint64_t> JoinGame(model::Map::Id map_id, std::string user_name);

    std::shared_ptr<Player> FindPlayerByToken(Token token);

    const std::deque<std::shared_ptr<Player>> GetAllPlayersInSessionWithCurrentPlayer(Token current_player_token);
    const model::GameSession* GetGameSessionByPlayer(Token player_token);

    void UpdateApplication(uint64_t delta_time_ms);
    void UpdateApplication(std::chrono::milliseconds delta_time_ms);

    bool IsTickerRunning() const noexcept;

    void SaveState();
    void RecoverState();

    std::vector<database::domain::Player> GetPlayersStats(uint64_t offset, uint64_t limit)
    {
        database::app::UseCasesImpl use_cases(database_.GetUnitOfWorkImpl());
        return use_cases.GetPlayersStat(offset, limit);
    }
private:
    model::Game& game_;
    Strand& strand_;
    Players players_;
    std::shared_ptr<Ticker> ticker_;
    bool is_ticker_running_ = false;
    bool is_randomize_spawn_points_ = false;
    std::filesystem::path path_to_state_file_;
    TickSignal tick_signal_;

    database::Database& database_;

    void SerializePlayers(boost::json::object& file_json);
    void SerializeItems(boost::json::object& file_json);

    void RestorePlayers(const boost::json::array& players_json);
    void RestoreItems(boost::json::array& file_json);

    void HandleLeavedPlayers();
};

}