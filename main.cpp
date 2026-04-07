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

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

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

int main(int argc, char** argv)
{
    const std::string port = DeterminePort(argc, argv);

    if (!InitDatabase())
    {
        return 1;
    }

    MeshCoreClient mc;
    AppRuntime runtime(mc);
    PushRouter pushRouter(mc, runtime);
    MessageRouter messageRouter(mc);

    pushRouter.Attach();
    messageRouter.Attach();

    if (!mc.connect(port))
    {
        std::cerr << "connect() failed for port " << port << "\n";
        return 1;
    }

    if (!runtime.InitializeClient())
    {
        return 1;
    }

    runtime.StartupSync();

    std::cout << "MeshCore Backend running on " << port << ". CTRL+C to exit.\n";


    runtime.CheckAndApplyCompanionConfig(true);

    /*runtime.CreatePublicChannel("abc123xyz");
    runtime.CreatePublicChannel("#berlin");
    runtime.CreatePrivateChannel("privat1", "00112233445566778899aabbccddeeff");*/

    while (mc.isConnected())
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

    return 0;
}