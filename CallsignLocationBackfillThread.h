#ifndef CALLSIGN_LOCATION_BACKFILL_THREAD_H
#define CALLSIGN_LOCATION_BACKFILL_THREAD_H

#include "GetLocation.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

class CallsignLocationBackfillThread
{
public:

    struct Config
    {
        GetLocation::Config locationConfig;
        std::chrono::hours runInterval = std::chrono::hours(24);
        std::chrono::milliseconds delayBetweenLookups = std::chrono::milliseconds(1000);
    };

    explicit CallsignLocationBackfillThread(const Config& config);
    ~CallsignLocationBackfillThread();

    void Start();
    void Stop();

private:

    void ThreadMain();
    void RunOnce();

    static std::optional<std::string> ExtractGermanCallsign(const std::string& text);
    static int32_t DegreesToE6(double value);

    Config m_config;
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stopRequested = false;
    bool m_running = false;
};

#endif