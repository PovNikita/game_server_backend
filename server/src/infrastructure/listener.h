#pragma once

#include <chrono>

#include <boost/signals2.hpp>

#include "../application/application.h"

namespace infrastructure
{

class Listener
{
using milliseconds = std::chrono::milliseconds;
public:
    Listener(milliseconds save_period, app::Application& application);

    Listener& operator=(Listener&&) = default;


private:
    app::Application& application_;
    boost::signals2::scoped_connection conn1;

    milliseconds time_since_previous_save_;
    milliseconds save_period_;
    bool do_on_every_tick_ = false;
};

} //namespace infrastructure