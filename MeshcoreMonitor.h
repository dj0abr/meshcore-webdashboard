#ifndef MESHCORE_MONITOR_H
#define MESHCORE_MONITOR_H

#include "MeshRxLogDecoder.h"

#include <cstdint>
#include <string>
#include <vector>

void MeshcoreMonitor(
    const std::string &source,
    uint8_t code,
    const std::vector<uint8_t> &payload,
    const MeshRxLogDecoder::DecodedPacket *pkt = nullptr
);

#endif
