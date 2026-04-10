#ifndef MESH_RX_LOG_DECODER_H
#define MESH_RX_LOG_DECODER_H

#include <cstdint>
#include <string>
#include <vector>

class MeshRxLogDecoder
{
public:
    struct DecodedPacket
    {
        bool valid = false;

        std::string originalHex;
        std::vector<uint8_t> rawBytes;

        uint8_t pushCode = 0;

        int8_t snrQ4 = 0;
        double snrDb = 0.0;
        int8_t rssiDbm = 0;

        std::vector<uint8_t> rfPacket;

        uint8_t header = 0;
        uint8_t routeType = 0;
        uint8_t payloadType = 0;
        uint8_t payloadVersion = 0;

        uint8_t pathMeta = 0;
        uint8_t pathLen = 0;
        uint8_t pathHashSizeCode = 0;
        uint8_t pathHashSize = 0;

        std::vector<std::vector<uint8_t>> hops;
        std::vector<uint8_t> pktPayload;

        uint32_t pktHash = 0;

        bool advertValid = false;
        std::vector<uint8_t> advertPublicKey;
        uint32_t advertTimestamp = 0;
        std::vector<uint8_t> advertSignature;
        std::vector<uint8_t> advertAppData;

        uint8_t advertFlags = 0;
        uint8_t advertRole = 0;

        bool advertHasGps = false;
        bool advertHasBle = false;
        bool advertHasShortcut = false;
        bool advertHasName = false;

        bool advertIsChatNode = false;
        bool advertIsRepeater = false;
        bool advertIsRoomServer = false;
        bool advertIsSensor = false;

        int32_t advertLatitudeE6 = 0;
        int32_t advertLongitudeE6 = 0;
        bool advertLocationValid = false;

        uint16_t advertBleVer = 0;
        bool advertBleValid = false;

        uint16_t advertShortcutBits = 0;
        bool advertShortcutValid = false;

        std::string advertName;

        bool reqValid = false;
        uint8_t reqDstHash = 0;
        uint8_t reqSrcHash = 0;
        uint16_t reqMac = 0;
        std::vector<uint8_t> reqCipherText;

        bool grpTxtValid = false;
        uint8_t grpChannelHash = 0;
        uint16_t grpMac = 0;
        std::vector<uint8_t> grpCipherText;

        bool grpKeyHashMatch = false;
        uint8_t grpExpectedChannelHash = 0;

        bool grpDecryptTried = false;
        bool grpDecryptOk = false;
        bool grpMacVerified = false;

        uint32_t grpTimestamp = 0;
        uint8_t grpTxtType = 0;
        std::vector<uint8_t> grpPlainText;
        std::string grpResolvedChannelName;
        std::string grpResolvedChannelKeyHex;
        std::string grpText;
    };

public:
    static DecodedPacket Decode(const std::vector<uint8_t> &payload, const std::string &resolvedFromName = "");
    static std::string FormatPath(const DecodedPacket &pkt);
    static std::string BytesToHex(const std::vector<uint8_t> &data);
    static std::string ByteToHex(uint8_t b);
    static const char *RouteTypeToString(uint8_t routeType);
    static const char *PayloadTypeToString(uint8_t payloadType);
    static const char *AdvertRoleToString(uint8_t advertRole);
    static uint32_t CalcPktHash(const std::vector<uint8_t> &pktPayload);
    static void PrintDecodedPacket(const MeshRxLogDecoder::DecodedPacket &pkt);

private:
    static void DecodeBytes(DecodedPacket &pkt, const std::string &resolvedFromName);
    static void DecodeRfPacket(DecodedPacket &pkt);
    static void DecodeAdvertPayload(DecodedPacket &pkt);
    static void DecodeReqPayload(DecodedPacket &pkt);
    static void DecodeGroupTextPayload(DecodedPacket &pkt, const std::string &resolvedFromName);

    static uint16_t ReadLe16(const std::vector<uint8_t> &data, size_t offset);
    static uint32_t ReadLe32(const std::vector<uint8_t> &data, size_t offset);
    static int32_t ReadLe32Signed(const std::vector<uint8_t> &data, size_t offset);

    static std::vector<uint8_t> HexToBytes(const std::string &hex);
    static std::vector<uint8_t> Sha256(const std::vector<uint8_t> &data);
    static std::vector<uint8_t> HmacSha256(
        const std::vector<uint8_t> &key,
        const std::vector<uint8_t> &data
    );
    static std::vector<uint8_t> Aes128EcbDecrypt(
        const std::vector<uint8_t> &key,
        const std::vector<uint8_t> &cipherText
    );
    static std::vector<uint8_t> TrimPlaintextPadding(const std::vector<uint8_t> &plainText);
    static bool LooksLikeReadableUtf8(const std::vector<uint8_t> &textBytes);
    static std::string SafeBytesToString(const std::vector<uint8_t> &textBytes);
    static std::vector<uint8_t> ResolveGroupChannelKey(DecodedPacket &pkt, const std::string &resolvedFromName);
};

#endif