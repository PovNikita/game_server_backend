#pragma once
#include <boost/program_options.hpp>
#include <optional>
#include <iostream>

namespace cl_pars {
struct Args {
    uint64_t tick_ms_ = 0;
    std::string config_file_;
    std::string www_root_;
    bool is_randomize_spawn_points_ = false;
    std::string state_file_ = "";
    uint64_t save_state_period_ms_ = 0;
    bool with_save_state_period = true;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    using namespace std::literals;
    namespace po = boost::program_options;
    Args args;
    po::options_description desc{"All options"s};
    desc.add_options()
        // Добавляем опцию --help и её короткую версию -h
        ("help,h", "Show help")
        // Опция --src files, сохраняющая свои аргументы в поле args.source
        ("tick-period,t",           po::value(&args.tick_ms_)->multitoken()->value_name("milliseconds"s),           "Set tick value")
        ("config-file,c",           po::value(&args.config_file_)->value_name("file"),                              "Set path to config file")
        ("www-root,w",              po::value(&args.www_root_)->value_name("dir"),                                  "Set path to static files")
        ("randomize-spawn-points",  po::value(&args.is_randomize_spawn_points_)->value_name("bool"),                "Set dog spawn mode(random/not random)")
        ("state-file",              po::value(&args.state_file_)->value_name("file"),                               "Set path to state file")
        ("save-state-period",       po::value(&args.save_state_period_ms_)->value_name("milliseconds"s),            "Set save state period");
    
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help")) {
        std::cout << desc;
    }

    if (!vm.contains("config-file")) {
        throw std::runtime_error("There is not path to config-file");
    }

    if (!vm.contains("www-root")) {
        throw std::runtime_error("There is not path to static files");
    }

    if (!vm.contains("save-state-period")) {
        args.with_save_state_period = false;
    }

    return args;
}


}