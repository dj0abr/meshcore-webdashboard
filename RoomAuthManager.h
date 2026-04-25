#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

class RoomAuthManager
{
public:

    enum class State : uint8_t
    {
        Unknown = 0,
        LoginPending,
        LoggedIn,
        LoginFailed
    };

    struct PendingLoginResolved
    {
        uint32_t roomNodeId = 0;
        unsigned long long txId = 0;
    };

    State GetState(uint32_t roomNodeId) const;

    void SetLoginPending(uint32_t roomNodeId);
    void SetLoggedIn(uint32_t roomNodeId);
    void SetLoginFailed(uint32_t roomNodeId);

    void Reset(uint32_t roomNodeId);
    void ResetAll();

    bool BeginLogin(
        uint32_t roomNodeId,
        unsigned long long txId,
        uint32_t nowSec,
        uint32_t timeoutSec);

    bool IsLoginTimedOut(
        uint32_t roomNodeId,
        uint32_t nowSec) const;

    void RefreshLoginDeadline(
        uint32_t roomNodeId,
        uint32_t nowSec,
        uint32_t timeoutSec);

    uint32_t GetLoginRemainingSeconds(
        uint32_t roomNodeId,
        uint32_t nowSec) const;

    std::optional<PendingLoginResolved> ResolvePendingLoginSuccess();
    std::optional<PendingLoginResolved> ResolvePendingLoginFail();

    bool HasPassword(uint32_t roomNodeId) const;
    std::optional<std::string> GetPassword(uint32_t roomNodeId) const;
    void SetPassword(uint32_t roomNodeId, const std::string& password);
    void ClearPassword(uint32_t roomNodeId);

    bool ShouldRequestPassword(uint32_t roomNodeId);
    void MarkPasswordRequested(uint32_t roomNodeId);
    void ClearPasswordRequested(uint32_t roomNodeId);

private:

    mutable std::mutex m_mutex;
    std::unordered_map<uint32_t, State> m_states;
    std::optional<uint32_t> m_pendingLoginRoomNodeId;
    std::optional<unsigned long long> m_pendingLoginTxId;
    std::unordered_map<uint32_t, uint32_t> m_loginDeadlines;

    std::unordered_map<uint32_t, std::string> m_passwords;
    std::unordered_set<uint32_t> m_passwordRequested;
};