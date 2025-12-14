//std
#include <iostream>
#include <thread>
#include <filesystem>
#include <chrono>
#include <utility>

#include "sdk.h"
//boost
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>

//user
#include "./json_handler/json_loader.h"
#include "./request_handler/request_handler.h"
#include "./application/application.h"
#include "./logging/logger.h"
#include "command_line_parser.h"
#include "./game/game.h"
#include "./infrastructure/listener.h"
#include "./database/database.h"


using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

void InitListener(app::Application& app, std::optional<cl_pars::Args>& args)
{
    static infrastructure::Listener listener(std::chrono::milliseconds(args->save_state_period_ms_), app);
}

int main(int argc, const char* argv[]) {
    InitLogger();
    try {
        if(auto args = cl_pars::ParseCommandLine(argc, argv))
        {
            // 0. Инициализируем логер
            // 1. Загружаем карту из файла и строим модель игры
            model::Game game = json_loader::LoadGame(args->config_file_);

            //Add path to static files in game     
            game.SetPathToStaticFiles(args->www_root_);

            // 2. Инициализируем io_context
            const unsigned num_threads = std::thread::hardware_concurrency();
            net::io_context ioc(num_threads);

            // 2.1 Инициализируем подключение к БД
            const char* db_url = std::getenv(database::DB_ENV_NAME);
            if(!db_url) {
                throw std::runtime_error("GAME_DB_URL is not specified");
            }
            database::Database database(db_url, num_threads);

            // 2.2 Инициализируем приложение
            // strand, используемый для доступа к API
            auto api_strand = net::make_strand(ioc);

            app::Application application(game, args->is_randomize_spawn_points_, api_strand, database);

            // 2.3 Восстановление состояния
            if(args->state_file_ != "")
            {
                application.RecoverFromFile(args->state_file_);
            }

            // 2.4 Включение автоматического тактирования
            if(args->tick_ms_ != 0)
            {
                application.TurnOnAutoTickingMode(std::chrono::milliseconds(args->tick_ms_));
            }

            if(!args->state_file_.empty() && args->with_save_state_period)
            {
                InitListener(application, args);
            }
            // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
            net::signal_set signals(ioc, SIGTERM, SIGINT);
            signals.async_wait([&ioc, &application](const sys::error_code& ec, [[maybe_unused]]int signal_number) {
                boost::json::object data;
                if(!ec)
                {
                    ioc.stop();
                    data["code"] = EXIT_FAILURE;
                    
                }
                application.SaveState();
            });
            // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
            auto handler = std::make_shared<http_handler::RequestHandler>(game, application, api_strand);
            auto logging_handler = std::make_shared<http_handler::RequestHandlerWithLogger<http_handler::RequestHandler>>(handler);

            // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов

            const auto address = net::ip::make_address("0.0.0.0");
            constexpr net::ip::port_type port = 8080;

            http_server::ServeHttp(ioc, {address, port}, [&logging_handler](auto remote_endpoint, auto&& req, auto&& send) {
                                (*logging_handler)(remote_endpoint, std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });

            // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
            //std::cout << "Server has started..."sv << std::endl;

            boost::json::value data = {
                {"port", port},
                {"address", address.to_string()}
            };
            LogJson(data, "server started"sv);


            // 6. Запускаем обработку асинхронных операций
            RunWorkers(std::max(1u, num_threads), [&ioc] {
                ioc.run();
            });
        }
    } catch (const std::exception& ex) {
        //std::cerr << ex.what() << std::endl;
        boost::json::value data = {
            {"code", EXIT_FAILURE},
            {"exception", ex.what()}
        };
        LogJson(data, "server exited"sv);
        return EXIT_FAILURE;
    }
    boost::json::value data = {
        {"code", 0}
    };
    LogJson(data, "server exited"sv);
}
