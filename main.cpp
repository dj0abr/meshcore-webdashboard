/*
Start-Prompt:

Ich habe ein MeshCore Dashboard für Linux bestehend aus einem c++ Backend und einem HTML/JS/php GUI.
Backend und GUI kommunizieren über eine Datenbank.
*/
#include "MeshCoreClient.h"
#include "MeshDB.h"
#include "AppRuntime.h"
#include "PushRouter.h"
#include "MessageRouter.h"
#include "CallsignLocationBackfillThread.h"
#include "RepeaterNodeSyncThread.h"

#include <cstdlib>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <csignal>

static std::atomic<bool> g_running(true);

namespace
{
    std::string DeterminePort(int argc, char** argv)
    {
        if (argc >= 2 && argv[1] != nullptr && argv[1][0] != '\0')
        {
            return argv[1];
        }

        return "/dev/ttyUSB0";
    }

    bool InitDatabase()
    {
        MeshDB::Config dbCfg;
        dbCfg.host = "localhost";
        dbCfg.user = "meshcore";
        dbCfg.password = "";
        dbCfg.database = "meshcore";
        dbCfg.socketPath = "/run/mysqld/mysqld.sock";
        dbCfg.port = 0;
        dbCfg.useUnixSocket = true;

        if (!MeshDB::Init(dbCfg))
        {
            std::cerr << "MeshDB konnte nicht initialisiert werden.\n";
            return false;
        }

        return true;
    }
}

void SignalHandler(int signum)
{
    (void)signum;
    g_running = false;
}

class CompanionConnectionGuard
{
public:
    CompanionConnectionGuard() = default;

    ~CompanionConnectionGuard()
    {
        MeshDB::StoreCompanionRadioConnected(false);
    }

    CompanionConnectionGuard(const CompanionConnectionGuard&) = delete;
    CompanionConnectionGuard& operator=(const CompanionConnectionGuard&) = delete;
};

int main(int argc, char** argv)
{
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    try {

        const std::string port = DeterminePort(argc, argv);

        if (!InitDatabase())
        {
            return 1;
        }

        CompanionConnectionGuard companionGuard;

        MeshCoreClient mc;
        AppRuntime runtime(mc);
        PushRouter pushRouter(mc, runtime);
        MessageRouter messageRouter(mc);

        pushRouter.Attach();
        messageRouter.Attach();

        if (!mc.connect(port))
        {
            std::cerr << "connect() failed for port " << port << "\n";
            MeshDB::StoreCompanionRadioConnected(false);
            return 1;
        }

        if (!runtime.InitializeClient())
        {
            return 1;
        }

        runtime.StartupSync();

        std::cout << "MeshCore Backend running on " << port << ". CTRL+C to exit.\n";

        runtime.CheckAndApplyCompanionConfig(true);

        // Starte Callsign Positionsergänzung
        CallsignLocationBackfillThread::Config backfillCfg;
        if (const char* v = std::getenv("MESHCORE_QRZ_USER"); v != nullptr)
            backfillCfg.locationConfig.qrzUsername = v;
        if (const char* v = std::getenv("MESHCORE_QRZ_PASS"); v != nullptr)
            backfillCfg.locationConfig.qrzPassword = v;
        backfillCfg.locationConfig.sessionCacheFile = "qrz_session_cache.txt";
        backfillCfg.runInterval = std::chrono::hours(24);
        backfillCfg.delayBetweenLookups = std::chrono::milliseconds(1500);
        CallsignLocationBackfillThread callsignBackfill(backfillCfg);
        callsignBackfill.Start();

        RepeaterNodeSyncThread::Config repeaterSyncCfg;
        if (const char* v = std::getenv("MESHCORE_REPEATER_SYNC_URL"); v != nullptr)
        {
            repeaterSyncCfg.baseUrl = v;
        }
        if (const char* v = std::getenv("MESHCORE_REPEATER_SYNC_TOKEN"); v != nullptr)
        {
            repeaterSyncCfg.apiToken = v;
        }
        if (const char* v = std::getenv("MESHCORE_REPEATER_SYNC_INTERVAL_SEC"); v != nullptr)
        {
            const long sec = std::strtol(v, nullptr, 10);

            if (sec > 0)
            {
                repeaterSyncCfg.runInterval = std::chrono::seconds(sec);
            }
        }

        RepeaterNodeSyncThread repeaterSync(repeaterSyncCfg);
        repeaterSync.Start();

        MeshDB::StoreCompanionRadioConnected(true);

        while (g_running && mc.isConnected())
        {
            try
            {
                runtime.CheckAndApplyCompanionConfig(false);
                runtime.Tick();
            }
            catch (const std::exception& e)
            {
                std::cerr << "[RUNTIME] error: " << e.what() << "\n";
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        std::cout << "Shutting down...\n";
        MeshDB::StoreCompanionRadioConnected(false);

        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Fatal exception: " << ex.what() << "\n";
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal unknown exception\n";
        return 1;
    }
}