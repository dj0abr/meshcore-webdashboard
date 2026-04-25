#include "RoomAuthManager.h"

RoomAuthManager::State RoomAuthManager::GetState(uint32_t roomNodeId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_states.find(roomNodeId);

    if (it == m_states.end())
    {
        return State::Unknown;
    }

    return it->second;
}

void RoomAuthManager::SetLoginPending(uint32_t roomNodeId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_states[roomNodeId] = State::LoginPending;
}

void RoomAuthManager::SetLoggedIn(uint32_t roomNodeId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_states[roomNodeId] = State::LoggedIn;
    m_loginDeadlines.erase(roomNodeId);
}

void RoomAuthManager::SetLoginFailed(uint32_t roomNodeId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_states[roomNodeId] = State::LoginFailed;
    m_loginDeadlines.erase(roomNodeId);
}

void RoomAuthManager::Reset(uint32_t roomNodeId)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_states.erase(roomNodeId);
    m_passwordRequested.erase(roomNodeId);
    m_loginDeadlines.erase(roomNodeId);

    if (m_pendingLoginRoomNodeId.has_value() &&
        *m_pendingLoginRoomNodeId == roomNodeId)
    {
        m_pendingLoginRoomNodeId.reset();
        m_pendingLoginTxId.reset();
    }
}

void RoomAuthManager::ResetAll()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_states.clear();
    m_pendingLoginRoomNodeId.reset();
    m_pendingLoginTxId.reset();
    m_passwordRequested.clear();
    m_loginDeadlines.clear();
}

bool RoomAuthManager::BeginLogin(
    uint32_t roomNodeId,
    unsigned long long txId,
    uint32_t nowSec,
    uint32_t timeoutSec)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_pendingLoginRoomNodeId.has_value())
    {
        if (*m_pendingLoginRoomNodeId == roomNodeId)
        {
            m_states[roomNodeId] = State::LoginPending;
            m_pendingLoginTxId = txId;
            m_loginDeadlines[roomNodeId] = nowSec + timeoutSec;
            return true;
        }

        return false;
    }

    m_pendingLoginRoomNodeId = roomNodeId;
    m_pendingLoginTxId = txId;
    m_states[roomNodeId] = State::LoginPending;
    m_loginDeadlines[roomNodeId] = nowSec + timeoutSec;
    return true;
}

bool RoomAuthManager::IsLoginTimedOut(
    uint32_t roomNodeId,
    uint32_t nowSec) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto stateIt = m_states.find(roomNodeId);

    if (stateIt == m_states.end() ||
        stateIt->second != State::LoginPending)
    {
        return false;
    }

    const auto deadlineIt = m_loginDeadlines.find(roomNodeId);

    if (deadlineIt == m_loginDeadlines.end())
    {
        return false;
    }

    return nowSec >= deadlineIt->second;
}

void RoomAuthManager::RefreshLoginDeadline(
    uint32_t roomNodeId,
    uint32_t nowSec,
    uint32_t timeoutSec)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto stateIt = m_states.find(roomNodeId);

    if (stateIt == m_states.end() ||
        stateIt->second != State::LoginPending)
    {
        return;
    }

    m_loginDeadlines[roomNodeId] = nowSec + timeoutSec;
}

uint32_t RoomAuthManager::GetLoginRemainingSeconds(
    uint32_t roomNodeId,
    uint32_t nowSec) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_loginDeadlines.find(roomNodeId);

    if (it == m_loginDeadlines.end())
    {
        return 0;
    }

    if (nowSec >= it->second)
    {
        return 0;
    }

    return it->second - nowSec;
}

std::optional<RoomAuthManager::PendingLoginResolved>
RoomAuthManager::ResolvePendingLoginSuccess()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_pendingLoginRoomNodeId.has_value() ||
        !m_pendingLoginTxId.has_value())
    {
        return std::nullopt;
    }

    PendingLoginResolved resolved {};
    resolved.roomNodeId = *m_pendingLoginRoomNodeId;
    resolved.txId = *m_pendingLoginTxId;

    m_pendingLoginRoomNodeId.reset();
    m_pendingLoginTxId.reset();
    m_loginDeadlines.erase(resolved.roomNodeId);
    m_states[resolved.roomNodeId] = State::LoggedIn;
    return resolved;
}

std::optional<RoomAuthManager::PendingLoginResolved>
RoomAuthManager::ResolvePendingLoginFail()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_pendingLoginRoomNodeId.has_value() ||
        !m_pendingLoginTxId.has_value())
    {
        return std::nullopt;
    }

    PendingLoginResolved resolved {};
    resolved.roomNodeId = *m_pendingLoginRoomNodeId;
    resolved.txId = *m_pendingLoginTxId;

    m_pendingLoginRoomNodeId.reset();
    m_pendingLoginTxId.reset();
    m_loginDeadlines.erase(resolved.roomNodeId);
    m_states[resolved.roomNodeId] = State::LoginFailed;
    return resolved;
}

bool RoomAuthManager::HasPassword(uint32_t roomNodeId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_passwords.find(roomNodeId) != m_passwords.end();
}

std::optional<std::string> RoomAuthManager::GetPassword(uint32_t roomNodeId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_passwords.find(roomNodeId);

    if (it == m_passwords.end())
    {
        return std::nullopt;
    }

    return it->second;
}

void RoomAuthManager::SetPassword(uint32_t roomNodeId, const std::string& password)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_passwords[roomNodeId] = password;
    m_passwordRequested.erase(roomNodeId);
}

void RoomAuthManager::ClearPassword(uint32_t roomNodeId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_passwords.erase(roomNodeId);
}

bool RoomAuthManager::ShouldRequestPassword(uint32_t roomNodeId)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_passwords.find(roomNodeId) != m_passwords.end())
    {
        return false;
    }

    return m_passwordRequested.find(roomNodeId) == m_passwordRequested.end();
}

void RoomAuthManager::MarkPasswordRequested(uint32_t roomNodeId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_passwordRequested.insert(roomNodeId);
}

void RoomAuthManager::ClearPasswordRequested(uint32_t roomNodeId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_passwordRequested.erase(roomNodeId);
}