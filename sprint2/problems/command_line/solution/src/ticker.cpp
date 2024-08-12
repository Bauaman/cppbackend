#include "ticker.h"

void Ticker::Start() {
    net::dispatch(strand_, [self = shared_from_this(), this] {
        last_tick_ = Clock::now();
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

    if(!ec) {
        auto tick_this = Clock::now();
        auto delta = duration_cast<milliseconds>(tick_this - last_tick_);
        last_tick_ = tick_this;
        try {
            handler_(delta);
        } catch (...) {

        }
        ScheduleTick();
    }
}
