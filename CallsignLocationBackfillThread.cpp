#include "CallsignLocationBackfillThread.h"
#include "MeshDB.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <exception>
#include <iostream>
#include <regex>
#include <thread>
#include <vector>

CallsignLocationBackfillThread::CallsignLocationBackfillThread(const Config& config)
    : m_config(config)
{
}

CallsignLocationBackfillThread::~CallsignLocationBackfillThread()
{
    Stop();
}

void CallsignLocationBackfillThread::Start()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_running)
    {
        return;
    }

    m_stopRequested = false;
    m_thread = std::thread(&CallsignLocationBackfillThread::ThreadMain, this);
    m_running = true;
}

void CallsignLocationBackfillThread::Stop()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_running)
        {
            return;
        }

        m_stopRequested = true;
    }

    m_cv.notify_all();

    if (m_thread.joinable())
    {
        m_thread.join();
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = false;
    }
}

void CallsignLocationBackfillThread::ThreadMain()
{
    try
    {
        RunOnce();

        std::unique_lock<std::mutex> lock(m_mutex);

        while (!m_stopRequested)
        {
            if (m_cv.wait_for(lock, m_config.runInterval, [this]() { return m_stopRequested; }))
            {
                break;
            }

            lock.unlock();

            try
            {
                RunOnce();
            }
            catch (const std::exception& e)
            {
                std::cerr << "[CallsignLocationBackfill] RunOnce exception: " << e.what() << "\n";
            }
            catch (...)
            {
                std::cerr << "[CallsignLocationBackfill] RunOnce unknown exception\n";
            }

            lock.lock();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[CallsignLocationBackfill] thread exception: " << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "[CallsignLocationBackfill] thread unknown exception\n";
    }
}

void CallsignLocationBackfillThread::RunOnce()
{
    if (!MeshDB::IsReady())
    {
        std::cerr << "[CallsignLocationBackfill] MeshDB not ready\n";
        return;
    }

    if (m_config.locationConfig.qrzUsername.empty() || m_config.locationConfig.qrzPassword.empty())
    {
        std::cerr << "[CallsignLocationBackfill] QRZ credentials missing, skip run\n";
        return;
    }

    std::vector<std::string> names = MeshDB::ListNodeNamesMissingAdvertLocation();

    if (names.empty())
    {
        std::cout << "[CallsignLocationBackfill] no nodes without location\n";
        return;
    }

    std::vector<std::pair<std::string, std::string>> candidates;
    candidates.reserve(names.size());

    for (const std::string& name : names)
    {
        const std::optional<std::string> callsign = ExtractGermanCallsign(name);

        if (callsign.has_value())
        {
            candidates.push_back({name, *callsign});
        }
    }

    if (candidates.empty())
    {
        std::cout << "[CallsignLocationBackfill] no callsigns found in node names\n";
        return;
    }

    std::cout
        << "[CallsignLocationBackfill] checking "
        << candidates.size()
        << " candidate names\n";

    GetLocation locator(m_config.locationConfig);

    for (std::size_t i = 0; i < candidates.size(); ++i)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_stopRequested)
            {
                break;
            }
        }

        const std::string& nodeName = candidates[i].first;
        const std::string& callsign = candidates[i].second;

        try
        {
            const GetLocation::Result result = locator.lookup(callsign);

            const int32_t latE6 = DegreesToE6(result.lat);
            const int32_t lonE6 = DegreesToE6(result.lon);

            if (MeshDB::UpdateNodeAdvertLocationByName(nodeName, latE6, lonE6))
            {
                std::cout
                    << "[CallsignLocationBackfill] updated '"
                    << nodeName
                    << "' from callsign "
                    << callsign
                    << " -> "
                    << latE6
                    << ", "
                    << lonE6
                    << "\n";
            }
            else
            {
                std::cerr
                    << "[CallsignLocationBackfill] DB update failed for '"
                    << nodeName
                    << "'\n";
            }
        }
        catch (const std::exception& e)
        {
            std::cerr
                << "[CallsignLocationBackfill] lookup failed for '"
                << nodeName
                << "' using callsign "
                << callsign
                << ": "
                << e.what()
                << "\n";
        }

        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_stopRequested)
        {
            break;
        }

        m_cv.wait_for(
            lock,
            m_config.delayBetweenLookups,
            [this]() { return m_stopRequested; });
    }
}

std::optional<std::string> CallsignLocationBackfillThread::ExtractGermanCallsign(const std::string& text)
{
    std::string normalized;
    normalized.reserve(text.size());

    for (unsigned char ch : text)
    {
        if (std::isalnum(ch))
        {
            normalized.push_back(static_cast<char>(std::toupper(ch)));
        }
        else
        {
            normalized.push_back(' ');
        }
    }

    /*
        Deutsche Calls grob:
        DB0SL
        DD1GR
        DO5FLV
        DL1ABC
        DM2XYZ
        Y25ABC

        Diese Regex ist absichtlich eher konservativ.
    */
    static const std::regex rx(
        R"(\b(?:[A-Z0-9]{1,3}/)?([A-Z0-9]{1,3}[0-9][A-Z]{1,3})(?:/[A-Z]{1,3})?\b)",
        std::regex::ECMAScript);

    std::smatch match;

    if (std::regex_search(normalized, match, rx) && (match.size() >= 2))
    {
        return match[1].str();
    }

    return std::nullopt;
}

int32_t CallsignLocationBackfillThread::DegreesToE6(double value)
{
    const double scaled = value * 1000000.0;
    return static_cast<int32_t>(std::llround(scaled));
}