#ifndef REPEATER_NODE_SYNC_THREAD_H
#define REPEATER_NODE_SYNC_THREAD_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

class RepeaterNodeSyncThread
{
public:

    struct Config
    {
        std::string baseUrl;
        std::string apiToken;
        std::chrono::seconds runInterval = std::chrono::seconds(600);
        long connectTimeoutSec = 10;
        long requestTimeoutSec = 60;
    };

    explicit RepeaterNodeSyncThread(const Config& config);
    ~RepeaterNodeSyncThread();

    void Start();
    void Stop();

private:

    void ThreadMain();
    void RunOnce();
    void SyncRepeaterNodes();
    void SyncNodeAdvertPaths();

    std::string BuildRepeaterNodesPushJson();
    std::string BuildNodeAdvertPathsPushJson();
    std::string HttpGet(const std::string& url);
    std::string HttpPostJson(const std::string& url, const std::string& body);

    static std::string JoinUrl(const std::string& baseUrl, const std::string& path);
    static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    Config m_config;
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stopRequested = false;
    bool m_running = false;
};

#endif
