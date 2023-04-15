#pragma once
#include <chrono>
#include <sstream>
#include <string>

namespace util {

    template<typename... _Args>
    inline static std::string inner_string(const _Args&... args) {
        std::stringstream ss;
        (void)std::initializer_list<int>{
            [&](const auto& arg) {
                ss << arg;
                return 0;
            }(args)...
        };

        return ss.str();
    }

    template<typename _Duration, class _Clock = std::chrono::steady_clock>
    inline uint64_t ticks() {
        return std::chrono::duration_cast<_Duration>(
            _Clock::now().time_since_epoch()
            ).count();
    }

    inline uint64_t sequence_faster(uint8_t type) {
        static constexpr uint64_t _spot = 1672502400000ull;     // 2023-01-01
        static constexpr uint64_t _sequence_max = 0x3FFFFull;
        static uint64_t _sequence = 0;
        const uint64_t mill_now = ticks<std::chrono::milliseconds>();
        const uint64_t mill_end = mill_now - _spot;
        _sequence += 1;
        _sequence >= _sequence_max ? _sequence = 0 : _sequence;
        return ((mill_end << 23) | (type << 18) | (_sequence & _sequence_max));
    }

}; // end namespace util
