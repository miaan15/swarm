// AI-generated

module;

#ifdef ENABLE_BENCHMARK
#include <SDL3/SDL.h>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <algorithm>
#endif

#include <string>
#include <string_view>
#include <cstddef>

export module benchmark;

export namespace sw {

#ifdef ENABLE_BENCHMARK
inline bool _benchmark_enable = true;

struct BenchmarkTimerState {
    uint64_t start_ns{0};
    uint64_t accumulated_ns{0};
    uint32_t call_count{0};
    bool is_running{false};
};

struct BenchmarkSnapshotEntry {
    std::string name;
    double duration_ms{0.0};
    uint32_t call_count{0};
};

struct BenchmarkFrameRecord {
    uint64_t timestamp_ns{0};
    std::vector<BenchmarkSnapshotEntry> entries;
    double total_duration_ms{0.0};
};

inline std::vector<std::string> _benchmark_registered_order;
inline std::unordered_map<std::string, BenchmarkTimerState> _benchmark_active_timers;
inline std::vector<BenchmarkFrameRecord> _benchmark_history;

#else
inline bool _benchmark_enable = false;
#endif

inline void _benchmark_register([[maybe_unused]] const std::string& name) {
#ifdef ENABLE_BENCHMARK
    if (!_benchmark_enable) return;

    if (!_benchmark_active_timers.contains(name)) {
        _benchmark_registered_order.push_back(name);
        _benchmark_active_timers[name] = BenchmarkTimerState{};
    }
#endif
}

inline void _benchmark_start([[maybe_unused]] std::string_view name) {
#ifdef ENABLE_BENCHMARK
    if (!_benchmark_enable) return;

    auto it = _benchmark_active_timers.find(std::string(name));
    if (it != _benchmark_active_timers.end()) {
        it->second.start_ns = SDL_GetTicksNS();
        it->second.is_running = true;
    }
#endif
}

inline void _benchmark_end([[maybe_unused]] std::string_view name) {
#ifdef ENABLE_BENCHMARK
    if (!_benchmark_enable) return;

    const uint64_t end_ns = SDL_GetTicksNS();
    auto it = _benchmark_active_timers.find(std::string(name));
    if (it != _benchmark_active_timers.end() && it->second.is_running) {
        it->second.accumulated_ns += (end_ns - it->second.start_ns);
        it->second.call_count++;
        it->second.is_running = false;
    }
#endif
}

inline void _benchmark_update_registered() {
#ifdef ENABLE_BENCHMARK
    if (!_benchmark_enable) return;

    BenchmarkFrameRecord record;
    record.timestamp_ns = SDL_GetTicksNS();
    record.entries.reserve(_benchmark_registered_order.size());

    double total_ms = 0.0;
    for (const auto& name : _benchmark_registered_order) {
        auto& state = _benchmark_active_timers[name];
        double ms = static_cast<double>(state.accumulated_ns) / 1'000'000.0;
        total_ms += ms;

        record.entries.push_back(BenchmarkSnapshotEntry{
            .name = name,
            .duration_ms = ms,
            .call_count = state.call_count
        });

        state.accumulated_ns = 0;
        state.call_count = 0;
        state.is_running = false;
    }

    record.total_duration_ms = total_ms;
    _benchmark_history.push_back(std::move(record));
#endif
}

inline std::string _benchmark_info_table_str([[maybe_unused]] size_t n, [[maybe_unused]] size_t index_delta = 1) {
#ifdef ENABLE_BENCHMARK
    if (_benchmark_history.empty() || n == 0 || index_delta == 0) {
        return {};
    }

    const size_t total_records = _benchmark_history.size();

    // Collect sampled indices stepping backward from the latest frame
    std::vector<size_t> sampled_indices;
    sampled_indices.reserve(n);

    // Latest valid index (0-based)
    size_t curr = total_records - 1;

    while (sampled_indices.size() < n) {
        // Human 1-based frame record tag: (curr + 1)
        // If curr == 0, curr + 1 == 1. A record index must be > 0.
        sampled_indices.push_back(curr);

        if (curr < index_delta) {
            break; // Next step would result in <= 0 index
        }
        curr -= index_delta;
    }

    if (sampled_indices.empty()) {
        return {};
    }

    // Display left-to-right chronologically
    std::reverse(sampled_indices.begin(), sampled_indices.end());

    const size_t count = sampled_indices.size();

    // Dynamic width for Metric names
    size_t name_col_width = 10;
    for (const auto& name : _benchmark_registered_order) {
        name_col_width = std::max(name_col_width, name.length());
    }
    name_col_width += 2;

    constexpr size_t val_col_width = 20;

    std::ostringstream ss;

    auto print_separator = [&](char junction = '+') {
        ss << junction << std::string(name_col_width, '-') << junction;
        for (size_t i = 0; i < count; ++i) {
            ss << std::string(val_col_width, '-') << junction;
        }
        ss << '\n';
    };

    print_separator('+');

    // Header labels
    ss << "| " << std::left << std::setw(name_col_width - 1) << "Metric";
    for (size_t idx : sampled_indices) {
        std::string frame_tag = "#" + std::to_string(idx + 1) + " (ms / %)";
        ss << "| " << std::right << std::setw(val_col_width - 1) << frame_tag;
    }
    ss << "|\n";

    print_separator('+');

    // Data rows
    for (size_t reg_idx = 0; reg_idx < _benchmark_registered_order.size(); ++reg_idx) {
        const auto& metric_name = _benchmark_registered_order[reg_idx];
        ss << "| " << std::left << std::setw(name_col_width - 1) << metric_name;

        for (size_t idx : sampled_indices) {
            const auto& frame = _benchmark_history[idx];
            const auto& entry = frame.entries[reg_idx];

            double pct = (frame.total_duration_ms > 0.0)
                ? (entry.duration_ms / frame.total_duration_ms) * 100.0
                : 0.0;

            std::ostringstream cell;
            cell << std::fixed << std::setprecision(3) << entry.duration_ms
                 << " (" << std::setprecision(1) << pct << "%)";

            ss << "| " << std::right << std::setw(val_col_width - 1) << cell.str();
        }
        ss << "|\n";
    }

    print_separator('+');

    // TOTAL row
    ss << "| " << std::left << std::setw(name_col_width - 1) << "TOTAL";
    for (size_t idx : sampled_indices) {
        const auto& frame = _benchmark_history[idx];
        std::ostringstream cell;
        cell << std::fixed << std::setprecision(3) << frame.total_duration_ms << " (100.0%)";
        ss << "| " << std::right << std::setw(val_col_width - 1) << cell.str();
    }
    ss << "|\n";

    print_separator('+');

    return ss.str();
#else
    return {};
#endif
}

} // namespace sw
