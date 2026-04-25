#pragma once

#include "MeshCoreClient.h"

#include <string>

class MessageRouter
{
public:
    explicit MessageRouter(MeshCoreClient& client);

    void Attach();

private:
    void HandleMessage(const MeshCoreClient::RxMessage& msg, const std::string& fromName);
    void HandleTestChannelMessage(const MeshCoreClient::RxMessage& msg);

private:
    MeshCoreClient& m_client;
};