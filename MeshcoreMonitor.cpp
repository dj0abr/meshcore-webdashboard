#include "MeshcoreMonitor.h"

#include "MeshDB.h"

#include <mutex>

namespace
{
    std::mutex g_monitorMutex;
}

void MeshcoreMonitor(
    const std::string &source,
    uint8_t code,
    const std::vector<uint8_t> &payload,
    const MeshRxLogDecoder::DecodedPacket *pkt
)
{
    std::lock_guard<std::mutex> lock(g_monitorMutex);

    MeshDB::MonitorRecord record {};
    record.source = source;
    record.pushCode = code;
    record.payload = payload;
    record.packet = pkt;

    MeshDB::StoreMeshcoreMonitor(record);
}
