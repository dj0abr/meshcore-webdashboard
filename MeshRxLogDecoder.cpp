#include "MeshRxLogDecoder.h"
#include "MeshDB.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <cctype>
#include <cstddef>

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

MeshRxLogDecoder::DecodedPacket MeshRxLogDecoder::Decode(const std::vector<uint8_t> &payload, const std::string &resolvedFromName)
{
    DecodedPacket pkt;

    pkt.rawBytes = payload;
    pkt.originalHex = BytesToHex(payload);

    DecodeBytes(pkt, resolvedFromName);

    pkt.valid = true;
    return pkt;
}

std::string MeshRxLogDecoder::FormatPath(const DecodedPacket &pkt)
{
    std::ostringstream oss;

    for (size_t i = 0; i < pkt.hops.size(); ++i)
    {
        if (i != 0)
        {
            oss << ",";
        }

        for (uint8_t b : pkt.hops[i])
        {
            oss << std::hex
                << std::setw(2)
                << std::setfill('0')
                << static_cast<unsigned>(b);
        }
    }

    return oss.str();
}

std::string MeshRxLogDecoder::BytesToHex(const std::vector<uint8_t> &data)
{
    std::ostringstream oss;

    for (uint8_t b : data)
    {
        oss << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<unsigned>(b);
    }

    return oss.str();
}

std::string MeshRxLogDecoder::ByteToHex(uint8_t b)
{
    std::ostringstream oss;
    oss << std::hex
        << std::setw(2)
        << std::setfill('0')
        << static_cast<unsigned>(b);
    return oss.str();
}

const char *MeshRxLogDecoder::RouteTypeToString(uint8_t routeType)
{
    switch (routeType)
    {
        case 0:
            return "TRANSPORT";

        case 1:
            return "FLOOD";

        case 2:
            return "DIRECT";

        case 3:
            return "TRANSPORT/DIRECT?";

        default:
            return "UNKNOWN";
    }
}

const char *MeshRxLogDecoder::PayloadTypeToString(uint8_t payloadType)
{
    switch (payloadType)
    {
        case 0:
            return "REQ";

        case 1:
            return "RESPONSE";

        case 2:
            return "TXT_MSG";

        case 3:
            return "ACK";

        case 4:
            return "ADVERT";

        case 5:
            return "GRP_TXT";

        case 6:
            return "GRP_DATA";

        case 7:
            return "ANON_REQ";

        case 8:
            return "PATH";

        case 9:
            return "TRACE";

        case 10:
            return "MULTIPART";

        case 11:
            return "CONTROL";

        case 15:
            return "RAW_CUSTOM";

        default:
            return "UNKNOWN";
    }
}

const char *MeshRxLogDecoder::AdvertRoleToString(uint8_t advertRole)
{
    switch (advertRole)
    {
        case 1:
            return "CHAT_NODE";

        case 2:
            return "REPEATER";

        case 3:
            return "ROOM_SERVER";

        case 4:
            return "SENSOR";

        default:
            return "UNKNOWN";
    }
}

uint32_t MeshRxLogDecoder::CalcPktHash(const std::vector<uint8_t> &pktPayload)
{
    uint8_t digest[SHA256_DIGEST_LENGTH];

    SHA256(pktPayload.data(), pktPayload.size(), digest);

    const uint32_t value =
        static_cast<uint32_t>(digest[0]) |
        (static_cast<uint32_t>(digest[1]) << 8) |
        (static_cast<uint32_t>(digest[2]) << 16) |
        (static_cast<uint32_t>(digest[3]) << 24);

    return value;
}

uint16_t MeshRxLogDecoder::ReadLe16(const std::vector<uint8_t> &data, size_t offset)
{
    if (offset + 2 > data.size())
    {
        throw std::runtime_error("ReadLe16 ausserhalb des Puffers");
    }

    return
        static_cast<uint16_t>(data[offset]) |
        (static_cast<uint16_t>(data[offset + 1]) << 8);
}

uint32_t MeshRxLogDecoder::ReadLe32(const std::vector<uint8_t> &data, size_t offset)
{
    if (offset + 4 > data.size())
    {
        throw std::runtime_error("ReadLe32 ausserhalb des Puffers");
    }

    return
        static_cast<uint32_t>(data[offset]) |
        (static_cast<uint32_t>(data[offset + 1]) << 8) |
        (static_cast<uint32_t>(data[offset + 2]) << 16) |
        (static_cast<uint32_t>(data[offset + 3]) << 24);
}

int32_t MeshRxLogDecoder::ReadLe32Signed(const std::vector<uint8_t> &data, size_t offset)
{
    return static_cast<int32_t>(ReadLe32(data, offset));
}

std::vector<uint8_t> MeshRxLogDecoder::HexToBytes(const std::string &hex)
{
    if ((hex.size() % 2) != 0)
    {
        throw std::runtime_error("Hex-String hat ungerade Laenge");
    }

    std::vector<uint8_t> out;
    out.reserve(hex.size() / 2);

    for (size_t i = 0; i < hex.size(); i += 2)
    {
        const std::string byteStr = hex.substr(i, 2);
        const unsigned long value = std::stoul(byteStr, nullptr, 16);

        if (value > 0xFF)
        {
            throw std::runtime_error("Ungueltiger Byte-Wert im Hex-String");
        }

        out.push_back(static_cast<uint8_t>(value));
    }

    return out;
}

std::vector<uint8_t> MeshRxLogDecoder::Sha256(const std::vector<uint8_t> &data)
{
    std::vector<uint8_t> digest(SHA256_DIGEST_LENGTH, 0);

    SHA256(data.data(), data.size(), digest.data());

    return digest;
}

std::vector<uint8_t> MeshRxLogDecoder::HmacSha256(
    const std::vector<uint8_t> &key,
    const std::vector<uint8_t> &data
)
{
    unsigned int outLen = SHA256_DIGEST_LENGTH;
    std::vector<uint8_t> out(outLen, 0);

    HMAC(
        EVP_sha256(),
        key.data(),
        static_cast<int>(key.size()),
        data.data(),
        data.size(),
        out.data(),
        &outLen
    );

    out.resize(outLen);
    return out;
}

std::vector<uint8_t> MeshRxLogDecoder::Aes128EcbDecrypt(
    const std::vector<uint8_t> &key,
    const std::vector<uint8_t> &cipherText
)
{
    if (key.size() != 16)
    {
        throw std::runtime_error("AES-128 benoetigt 16 Byte Key");
    }

    if ((cipherText.size() % 16) != 0)
    {
        throw std::runtime_error("Ciphertext-Laenge ist kein AES-Blockvielfaches");
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr)
    {
        throw std::runtime_error("EVP_CIPHER_CTX_new fehlgeschlagen");
    }

    std::vector<uint8_t> plainText(cipherText.size() + 16, 0);

    int outLen1 = 0;
    int outLen2 = 0;

    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), nullptr, key.data(), nullptr) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_DecryptInit_ex fehlgeschlagen");
    }

    EVP_CIPHER_CTX_set_padding(ctx, 0);

    if (EVP_DecryptUpdate(
            ctx,
            plainText.data(),
            &outLen1,
            cipherText.data(),
            static_cast<int>(cipherText.size())
        ) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_DecryptUpdate fehlgeschlagen");
    }

    if (EVP_DecryptFinal_ex(ctx, plainText.data() + outLen1, &outLen2) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_DecryptFinal_ex fehlgeschlagen");
    }

    EVP_CIPHER_CTX_free(ctx);

    plainText.resize(static_cast<size_t>(outLen1 + outLen2));
    return plainText;
}

std::vector<uint8_t> MeshRxLogDecoder::TrimPlaintextPadding(const std::vector<uint8_t> &plainText)
{
    std::vector<uint8_t> out = plainText;

    if (out.empty())
    {
        return out;
    }

    const uint8_t last = out.back();

    if (last != 0 && last <= 16 && out.size() >= last)
    {
        bool looksLikePkcs7 = true;

        for (size_t i = 0; i < last; ++i)
        {
            if (out[out.size() - 1 - i] != last)
            {
                looksLikePkcs7 = false;
                break;
            }
        }

        if (looksLikePkcs7)
        {
            out.resize(out.size() - last);
            return out;
        }
    }

    while (!out.empty() && out.back() == 0)
    {
        out.pop_back();
    }

    return out;
}

bool MeshRxLogDecoder::LooksLikeReadableUtf8(const std::vector<uint8_t> &textBytes)
{
    if (textBytes.empty())
    {
        return false;
    }

    size_t printableish = 0;

    for (uint8_t b : textBytes)
    {
        if (b == '\t' || b == '\n' || b == '\r')
        {
            ++printableish;
            continue;
        }

        if (b >= 0x20)
        {
            ++printableish;
        }
    }

    return (printableish * 100 / textBytes.size()) >= 85;
}

std::string MeshRxLogDecoder::SafeBytesToString(const std::vector<uint8_t> &textBytes)
{
    return std::string(textBytes.begin(), textBytes.end());
}

void MeshRxLogDecoder::DecodeBytes(DecodedPacket &pkt, const std::string &resolvedFromName)
{
    if (pkt.rawBytes.size() < 5)
    {
        throw std::runtime_error("Zu wenige Bytes fuer RX-Log-Decode");
    }

    size_t offset = 0;

    pkt.pushCode = pkt.rawBytes[offset++];

    pkt.snrQ4 = static_cast<int8_t>(pkt.rawBytes[offset++]);
    pkt.snrDb = static_cast<double>(pkt.snrQ4) / 4.0;

    pkt.rssiDbm = static_cast<int8_t>(pkt.rawBytes[offset++]);

    pkt.rfPacket.assign(pkt.rawBytes.begin() + offset, pkt.rawBytes.end());

    DecodeRfPacket(pkt);

    if (pkt.payloadType == 5)
    {
        DecodeGroupTextPayload(pkt, resolvedFromName);
    }
}

void MeshRxLogDecoder::DecodeRfPacket(DecodedPacket &pkt)
{
    if (pkt.rfPacket.size() < 2)
    {
        throw std::runtime_error("RF-Paket zu kurz");
    }

    size_t offset = 0;

    pkt.header = pkt.rfPacket[offset++];
    pkt.routeType = pkt.header & 0x03;
    pkt.payloadType = (pkt.header >> 2) & 0x0F;
    pkt.payloadVersion = (pkt.header >> 6) & 0x03;

    pkt.pathMeta = pkt.rfPacket[offset++];
    pkt.pathLen = pkt.pathMeta & 0x3F;
    pkt.pathHashSizeCode = (pkt.pathMeta >> 6) & 0x03;

    if (pkt.pathHashSizeCode == 3)
    {
        throw std::runtime_error("Reservierter/ungueltiger pathHashSizeCode = 3");
    }

    pkt.pathHashSize = static_cast<uint8_t>(pkt.pathHashSizeCode + 1);

    const size_t pathBytes =
        static_cast<size_t>(pkt.pathLen) * static_cast<size_t>(pkt.pathHashSize);

    if (pkt.rfPacket.size() < offset + pathBytes)
    {
        throw std::runtime_error("RF-Paket enthaelt nicht genug Bytes fuer den Pfad");
    }

    pkt.hops.clear();
    pkt.hops.reserve(pkt.pathLen);

    for (uint8_t i = 0; i < pkt.pathLen; ++i)
    {
        std::vector<uint8_t> hop;
        hop.insert(
            hop.end(),
            pkt.rfPacket.begin() + offset,
            pkt.rfPacket.begin() + offset + pkt.pathHashSize
        );
        pkt.hops.push_back(std::move(hop));
        offset += pkt.pathHashSize;
    }

    pkt.pktPayload.assign(pkt.rfPacket.begin() + offset, pkt.rfPacket.end());
    pkt.pktHash = CalcPktHash(pkt.pktPayload);

    if (pkt.payloadType == 4)
    {
        DecodeAdvertPayload(pkt);
    }
    else if (pkt.payloadType == 0)
    {
        DecodeReqPayload(pkt);
    }
}

void MeshRxLogDecoder::DecodeAdvertPayload(DecodedPacket &pkt)
{
    pkt.advertValid = false;
    pkt.advertPublicKey.clear();
    pkt.advertSignature.clear();
    pkt.advertAppData.clear();
    pkt.advertName.clear();

    pkt.advertTimestamp = 0;
    pkt.advertFlags = 0;
    pkt.advertRole = 0;

    pkt.advertHasGps = false;
    pkt.advertHasBle = false;
    pkt.advertHasShortcut = false;
    pkt.advertHasName = false;

    pkt.advertIsChatNode = false;
    pkt.advertIsRepeater = false;
    pkt.advertIsRoomServer = false;
    pkt.advertIsSensor = false;

    pkt.advertLatitudeE6 = 0;
    pkt.advertLongitudeE6 = 0;
    pkt.advertLocationValid = false;

    pkt.advertBleVer = 0;
    pkt.advertBleValid = false;

    pkt.advertShortcutBits = 0;
    pkt.advertShortcutValid = false;

    if (pkt.pktPayload.size() < 100)
    {
        throw std::runtime_error("ADVERT-Payload zu kurz");
    }

    size_t offset = 0;

    pkt.advertPublicKey.assign(
        pkt.pktPayload.begin() + offset,
        pkt.pktPayload.begin() + offset + 32
    );
    offset += 32;

    pkt.advertTimestamp = ReadLe32(pkt.pktPayload, offset);
    offset += 4;

    pkt.advertSignature.assign(
        pkt.pktPayload.begin() + offset,
        pkt.pktPayload.begin() + offset + 64
    );
    offset += 64;

    if (offset >= pkt.pktPayload.size())
    {
        pkt.advertValid = true;
        return;
    }

    pkt.advertAppData.assign(pkt.pktPayload.begin() + offset, pkt.pktPayload.end());

    pkt.advertFlags = pkt.pktPayload[offset++];
    pkt.advertRole = pkt.advertFlags & 0x0F;

    pkt.advertHasGps = (pkt.advertFlags & 0x10) != 0;
    pkt.advertHasBle = (pkt.advertFlags & 0x20) != 0;
    pkt.advertHasShortcut = (pkt.advertFlags & 0x40) != 0;
    pkt.advertHasName = (pkt.advertFlags & 0x80) != 0;

    pkt.advertIsChatNode = (pkt.advertRole == 1);
    pkt.advertIsRepeater = (pkt.advertRole == 2);
    pkt.advertIsRoomServer = (pkt.advertRole == 3);
    pkt.advertIsSensor = (pkt.advertRole == 4);

    if (pkt.advertHasGps)
    {
        if (offset + 8 > pkt.pktPayload.size())
        {
            throw std::runtime_error("ADVERT-GPS-Daten unvollstaendig");
        }

        pkt.advertLatitudeE6 = ReadLe32Signed(pkt.pktPayload, offset);
        offset += 4;

        pkt.advertLongitudeE6 = ReadLe32Signed(pkt.pktPayload, offset);
        offset += 4;

        pkt.advertLocationValid = true;
    }

    if (pkt.advertHasBle)
    {
        if (offset + 2 > pkt.pktPayload.size())
        {
            throw std::runtime_error("ADVERT-BLE-Daten unvollstaendig");
        }

        pkt.advertBleVer = ReadLe16(pkt.pktPayload, offset);
        offset += 2;
        pkt.advertBleValid = true;
    }

    if (pkt.advertHasShortcut)
    {
        if (offset + 2 > pkt.pktPayload.size())
        {
            throw std::runtime_error("ADVERT-Shortcut-Daten unvollstaendig");
        }

        pkt.advertShortcutBits = ReadLe16(pkt.pktPayload, offset);
        offset += 2;
        pkt.advertShortcutValid = true;
    }

    if (pkt.advertHasName)
    {
        pkt.advertName.assign(pkt.pktPayload.begin() + offset, pkt.pktPayload.end());
    }

    pkt.advertValid = true;
}

void MeshRxLogDecoder::DecodeReqPayload(DecodedPacket &pkt)
{
    pkt.reqValid = false;
    pkt.reqDstHash = 0;
    pkt.reqSrcHash = 0;
    pkt.reqMac = 0;
    pkt.reqCipherText.clear();

    if (pkt.pktPayload.size() < 4)
    {
        throw std::runtime_error("REQ-Payload zu kurz");
    }

    size_t offset = 0;

    pkt.reqDstHash = pkt.pktPayload[offset++];
    pkt.reqSrcHash = pkt.pktPayload[offset++];

    pkt.reqMac = ReadLe16(pkt.pktPayload, offset);
    offset += 2;

    pkt.reqCipherText.assign(pkt.pktPayload.begin() + offset, pkt.pktPayload.end());

    pkt.reqValid = true;
}

std::vector<uint8_t> MeshRxLogDecoder::ResolveGroupChannelKey(
    DecodedPacket &pkt,
    const std::string &resolvedFromName)
{
    pkt.grpResolvedChannelName.clear();
    pkt.grpResolvedChannelKeyHex.clear();

    if (!resolvedFromName.empty())
    {
        if (auto rec = MeshDB::FindChannelByName(resolvedFromName);
            rec.has_value() && !rec->keyHex.empty())
        {
            pkt.grpResolvedChannelName = rec->name;
            pkt.grpResolvedChannelKeyHex = rec->keyHex;
            return HexToBytes(rec->keyHex);
        }
    }

    const std::vector<MeshDB::ChannelRecord> channels = MeshDB::ListChannels(true);

    for (const auto &rec : channels)
    {
        if (rec.keyHex.empty())
        {
            continue;
        }

        const std::vector<uint8_t> candidateKey = HexToBytes(rec.keyHex);

        if (candidateKey.empty())
        {
            continue;
        }

        const std::vector<uint8_t> candidateSha = Sha256(candidateKey);

        if (!candidateSha.empty() && (candidateSha[0] == pkt.grpChannelHash))
        {
            pkt.grpResolvedChannelName = rec.name;
            pkt.grpResolvedChannelKeyHex = rec.keyHex;
            return candidateKey;
        }
    }

    return {};
}

void MeshRxLogDecoder::DecodeGroupTextPayload(DecodedPacket &pkt, const std::string &resolvedFromName)
{
    pkt.grpTxtValid = false;
    pkt.grpChannelHash = 0;
    pkt.grpMac = 0;
    pkt.grpCipherText.clear();

    pkt.grpExpectedChannelHash = 0;
    pkt.grpKeyHashMatch = false;

    pkt.grpDecryptTried = false;
    pkt.grpDecryptOk = false;
    pkt.grpMacVerified = false;

    pkt.grpTimestamp = 0;
    pkt.grpTxtType = 0;
    pkt.grpPlainText.clear();
    pkt.grpText.clear();

    if (pkt.pktPayload.size() < 3)
    {
        throw std::runtime_error("GRP_TXT-Payload zu kurz");
    }

    size_t offset = 0;

    pkt.grpChannelHash = pkt.pktPayload[offset++];
    pkt.grpMac = ReadLe16(pkt.pktPayload, offset);
    offset += 2;

    pkt.grpCipherText.assign(pkt.pktPayload.begin() + offset, pkt.pktPayload.end());
    pkt.grpTxtValid = true;

    const std::vector<uint8_t> channelKey = ResolveGroupChannelKey(pkt, resolvedFromName);
    if (channelKey.empty())
    {
        return;
    }

    const std::vector<uint8_t> channelKeySha = Sha256(channelKey);

    if (!channelKeySha.empty())
    {
        pkt.grpExpectedChannelHash = channelKeySha[0];
        pkt.grpKeyHashMatch = (pkt.grpExpectedChannelHash == pkt.grpChannelHash);
    }

    pkt.grpDecryptTried = true;

    if (!pkt.grpKeyHashMatch)
    {
        return;
    }

    if ((pkt.grpCipherText.size() % 16) != 0)
    {
        return;
    }

    try
    {
        const std::vector<uint8_t> hmac = HmacSha256(channelKey, pkt.grpCipherText);

        if (hmac.size() >= 2)
        {
            const uint16_t macLe =
                static_cast<uint16_t>(hmac[0]) |
                (static_cast<uint16_t>(hmac[1]) << 8);

            if (macLe == pkt.grpMac)
            {
                pkt.grpMacVerified = true;
            }
        }

        std::vector<uint8_t> plainText = Aes128EcbDecrypt(channelKey, pkt.grpCipherText);
        plainText = TrimPlaintextPadding(plainText);

        if (plainText.size() < 5)
        {
            return;
        }

        const uint32_t timestamp = ReadLe32(plainText, 0);
        const uint8_t txtType = plainText[4];

        std::vector<uint8_t> textBytes(plainText.begin() + 5, plainText.end());

        while (!textBytes.empty() && textBytes.back() == 0)
        {
            textBytes.pop_back();
        }

        if (!LooksLikeReadableUtf8(textBytes))
        {
            return;
        }

        pkt.grpTimestamp = timestamp;
        pkt.grpTxtType = txtType;
        pkt.grpPlainText = std::move(plainText);
        pkt.grpText = SafeBytesToString(textBytes);
        pkt.grpDecryptOk = true;
    }
    catch (const std::exception &)
    {
        pkt.grpDecryptOk = false;
    }
}

void MeshRxLogDecoder::PrintDecodedPacket(const MeshRxLogDecoder::DecodedPacket &pkt)
{
    std::cout << "Original empfangene Daten:" << std::endl;
    std::cout << "  " << pkt.originalHex << std::endl;
    std::cout << std::endl;

    std::cout << "RX-Log / Companion-Teil:" << std::endl;
    std::cout << "  pushCode         : 0x" << MeshRxLogDecoder::ByteToHex(pkt.pushCode) << std::endl;
    std::cout << "  snrQ4            : " << static_cast<int>(pkt.snrQ4) << std::endl;
    std::cout << "  snrDb            : " << pkt.snrDb << " dB" << std::endl;
    std::cout << "  rssiDbm          : " << static_cast<int>(pkt.rssiDbm) << " dBm" << std::endl;
    std::cout << "  rfPacket         : " << MeshRxLogDecoder::BytesToHex(pkt.rfPacket) << std::endl;
    std::cout << std::endl;

    std::cout << "Dekodiertes Mesh-Paket:" << std::endl;
    std::cout << "  header           : 0x" << MeshRxLogDecoder::ByteToHex(pkt.header)
              << " (" << static_cast<int>(pkt.header) << ")" << std::endl;
    std::cout << "  routeType        : " << static_cast<int>(pkt.routeType)
              << " (" << MeshRxLogDecoder::RouteTypeToString(pkt.routeType) << ")" << std::endl;
    std::cout << "  payloadType      : " << static_cast<int>(pkt.payloadType)
              << " (" << MeshRxLogDecoder::PayloadTypeToString(pkt.payloadType) << ")" << std::endl;
    std::cout << "  payloadVersion   : " << static_cast<int>(pkt.payloadVersion) << std::endl;
    std::cout << std::endl;

    std::cout << "Pfad-Information:" << std::endl;
    std::cout << "  pathMeta         : 0x" << MeshRxLogDecoder::ByteToHex(pkt.pathMeta)
              << " (" << static_cast<int>(pkt.pathMeta) << ")" << std::endl;
    std::cout << "  pathLen          : " << static_cast<int>(pkt.pathLen) << " hops" << std::endl;
    std::cout << "  pathHashSizeCode : " << static_cast<int>(pkt.pathHashSizeCode) << std::endl;
    std::cout << "  pathHashSize     : " << static_cast<int>(pkt.pathHashSize) << " Byte pro Hop" << std::endl;
    std::cout << "  path             : " << MeshRxLogDecoder::FormatPath(pkt) << std::endl;
    std::cout << std::endl;

    std::cout << "Einzelne Hops:" << std::endl;
    for (size_t i = 0; i < pkt.hops.size(); ++i)
    {
        std::cout << "  Hop " << (i + 1) << "            : ";

        for (uint8_t b : pkt.hops[i])
        {
            std::cout << MeshRxLogDecoder::ByteToHex(b);
        }

        std::cout << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Restlicher Paket-Payload:" << std::endl;
    std::cout << "  pktPayload       : " << MeshRxLogDecoder::BytesToHex(pkt.pktPayload) << std::endl;
    std::cout << "  pktPayloadLen    : " << pkt.pktPayload.size() << " Bytes" << std::endl;
    std::cout << "  pktHash          : " << pkt.pktHash
              << " (0x" << std::hex << std::setw(8) << std::setfill('0') << pkt.pktHash
              << std::dec << ")" << std::endl;
    std::cout << std::endl;

    if (pkt.payloadType == 4)
    {
        std::cout << "ADVERT-Payload:" << std::endl;

        if (!pkt.advertValid)
        {
            std::cout << "  advertValid      : nein" << std::endl;
            std::cout << std::endl;
            return;
        }

        std::cout << "  advertValid      : ja" << std::endl;
        std::cout << "  publicKey        : " << MeshRxLogDecoder::BytesToHex(pkt.advertPublicKey) << std::endl;
        std::cout << "  timestamp        : " << pkt.advertTimestamp
                  << " (0x" << std::hex << std::setw(8) << std::setfill('0') << pkt.advertTimestamp
                  << std::dec << ")" << std::endl;
        std::cout << "  signature        : " << MeshRxLogDecoder::BytesToHex(pkt.advertSignature) << std::endl;
        std::cout << "  appData          : " << MeshRxLogDecoder::BytesToHex(pkt.advertAppData) << std::endl;
        std::cout << "  flags            : 0x" << MeshRxLogDecoder::ByteToHex(pkt.advertFlags) << std::endl;
        std::cout << "  role             : " << static_cast<int>(pkt.advertRole)
                  << " (" << MeshRxLogDecoder::AdvertRoleToString(pkt.advertRole) << ")" << std::endl;
        std::cout << "  hasGps           : " << (pkt.advertHasGps ? "ja" : "nein") << std::endl;
        std::cout << "  hasBle           : " << (pkt.advertHasBle ? "ja" : "nein") << std::endl;
        std::cout << "  hasShortcut      : " << (pkt.advertHasShortcut ? "ja" : "nein") << std::endl;
        std::cout << "  hasName          : " << (pkt.advertHasName ? "ja" : "nein") << std::endl;

        if (pkt.advertLocationValid)
        {
            std::cout << "  latitudeE6       : " << pkt.advertLatitudeE6 << std::endl;
            std::cout << "  longitudeE6      : " << pkt.advertLongitudeE6 << std::endl;
        }

        if (pkt.advertBleValid)
        {
            std::cout << "  bleVersion       : " << pkt.advertBleVer
                      << " (0x" << std::hex << std::setw(4) << std::setfill('0') << pkt.advertBleVer
                      << std::dec << ")" << std::endl;
        }

        if (pkt.advertShortcutValid)
        {
            std::cout << "  shortcutBits     : " << pkt.advertShortcutBits
                      << " (0x" << std::hex << std::setw(4) << std::setfill('0') << pkt.advertShortcutBits
                      << std::dec << ")" << std::endl;
        }

        if (pkt.advertHasName)
        {
            std::cout << "  name             : " << pkt.advertName << std::endl;
        }

        std::cout << std::endl;
    }

    if (pkt.payloadType == 0)
    {
        std::cout << "REQ-Payload:" << std::endl;

        if (!pkt.reqValid)
        {
            std::cout << "  reqValid         : nein" << std::endl;
            std::cout << std::endl;
            return;
        }

        std::cout << "  reqValid         : ja" << std::endl;
        std::cout << "  dstHash          : 0x" << MeshRxLogDecoder::ByteToHex(pkt.reqDstHash) << std::endl;
        std::cout << "  srcHash          : 0x" << MeshRxLogDecoder::ByteToHex(pkt.reqSrcHash) << std::endl;
        std::cout << "  mac              : 0x"
                  << std::hex << std::setw(4) << std::setfill('0') << pkt.reqMac
                  << std::dec << std::endl;
        std::cout << "  cipherText       : " << MeshRxLogDecoder::BytesToHex(pkt.reqCipherText) << std::endl;
        std::cout << "  cipherTextLen    : " << pkt.reqCipherText.size() << " Bytes" << std::endl;
        std::cout << std::endl;
    }

    if (pkt.payloadType == 5)
    {
        std::cout << "GRP_TXT-Payload:" << std::endl;

        if (!pkt.grpTxtValid)
        {
            std::cout << "  grpTxtValid      : nein" << std::endl;
            std::cout << std::endl;
            return;
        }

        std::cout << "  grpTxtValid      : ja" << std::endl;
        std::cout << "  channelHash      : 0x" << MeshRxLogDecoder::ByteToHex(pkt.grpChannelHash) << std::endl;
        std::cout << "  resolvedFromName : [" << pkt.grpResolvedChannelName << "]" << std::endl;
        std::cout << "  expectedHash     : 0x" << MeshRxLogDecoder::ByteToHex(pkt.grpExpectedChannelHash) << std::endl;
        std::cout << "  keyHashMatch     : " << (pkt.grpKeyHashMatch ? "ja" : "nein") << std::endl;
        std::cout << "  mac              : 0x"
                  << std::hex << std::setw(4) << std::setfill('0') << pkt.grpMac
                  << std::dec << std::endl;
        std::cout << "  cipherTextLen    : " << pkt.grpCipherText.size() << " Bytes" << std::endl;
        std::cout << "  cipherText       : " << MeshRxLogDecoder::BytesToHex(pkt.grpCipherText) << std::endl;
        std::cout << "  decryptTried     : " << (pkt.grpDecryptTried ? "ja" : "nein") << std::endl;
        std::cout << "  decryptOk        : " << (pkt.grpDecryptOk ? "ja" : "nein") << std::endl;
        std::cout << "  macVerified      : " << (pkt.grpMacVerified ? "ja" : "nein") << std::endl;

        if (pkt.grpDecryptOk)
        {
            std::cout << "  timestamp        : " << pkt.grpTimestamp
                      << " (0x" << std::hex << std::setw(8) << std::setfill('0') << pkt.grpTimestamp
                      << std::dec << ")" << std::endl;
            std::cout << "  txtType          : " << static_cast<int>(pkt.grpTxtType) << std::endl;
            std::cout << "  plainText        : " << MeshRxLogDecoder::BytesToHex(pkt.grpPlainText) << std::endl;
            std::cout << "  text             : " << pkt.grpText << std::endl;
        }

        std::cout << std::endl;
    }
}