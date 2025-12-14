#include "listener.h"

namespace infrastructure
{
    Listener::Listener(milliseconds save_period, app::Application& application)
    : save_period_(save_period)
    , application_(application)
    {
        using namespace std::literals;

        conn1 = application_.DoOnTick([this](milliseconds delta) mutable {
            {
                if(time_since_previous_save_ >= save_period_)
                {
                    application_.SaveState();
                    time_since_previous_save_ = 0ms;
                }
                else
                {
                    time_since_previous_save_ += delta;
                }
            }
        });
    }
} //namespace infrastructure