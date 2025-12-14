cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(game_model_lib STATIC
										src/game/game_items.h
										src/game/game_items.cpp
										src/game/game.h
										src/game/game.cpp
										src/game/motion.h
										src/game/motion.cpp
										src/utils/tagged.h
										src/game/loot_generator.h
    									src/game/loot_generator.cpp
										src/game/collision_detector.h
										src/game/collision_detector.cpp
										src/serialization/serialization.h
										src/serialization/serialization.cpp
)

# Добавляем сторонние библиотеки. Указываем видимость PUBLIC, т. к. 
# они должны быть ввидны и в библиотеке MyLib и в зависимостях.
target_include_directories(game_model_lib PUBLIC CONAN_PKG::boost)
target_link_libraries(game_model_lib PUBLIC Threads::Threads CONAN_PKG::boost)

# Добавляем цели, указываем только их собственные файлы.
add_executable(game_server
	src/database/connection_pool.h
	src/database/database.h
	src/database/database.cpp
	src/database/domain.h
	src/database/postgres.h
	src/database/postgres.cpp
	src/database/unit_of_work.h
	src/database/use_cases.h
	src/database/utils/tagged_uuid.cpp
	src/database/utils/tagged_uuid.h
	src/main.cpp
	src/server/http_server.cpp
	src/server/http_server.h
	src/sdk.h
	src/json_handler/boost_json.cpp
	src/json_handler/json_loader.h
	src/json_handler/json_loader.cpp
	src/request_handler/request_utils.h
	src/request_handler/request_utils.cpp
	src/request_handler/api_handler.h
	src/request_handler/api_handler.cpp
	src/request_handler/request_handler.cpp
	src/request_handler/request_handler.h
	src/logging/logger.cpp
	src/logging/logger.h
	src/application/application.cpp
	src/application/application.h
	src/application/players.cpp
	src/application/players.h
	src/command_line_parser.h
	src/infrastructure/listener.cpp
	src/infrastructure/listener.h
)

target_compile_definitions(game_server
    PRIVATE BOOST_BEAST_USE_STD_STRING_VIEW
)

target_compile_definitions(game_model_lib
    PRIVATE BOOST_BEAST_USE_STD_STRING_VIEW
)

add_executable(game_server_test
    tests/loot_generator_tests.cpp
	tests/collision_detector_tests.cpp
	tests/state_serialization_tests.cpp
)

target_link_libraries(game_server game_model_lib CONAN_PKG::libpqxx)
target_link_libraries(game_server_test CONAN_PKG::catch2 game_model_lib) 
