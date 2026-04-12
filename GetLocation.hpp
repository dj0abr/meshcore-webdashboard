#pragma once

#include <string>
#include <optional>
#include <stdexcept>

class GetLocation {
public:
    struct Result {
        std::string callsign;
        std::string grid;   // Maidenhead locator, z.B. "JO62QM"
        double lat = 0.0;
        double lon = 0.0;
    };

    struct Config {
        std::string qrzUsername;                       // QRZ user
        std::string qrzPassword;                       // QRZ password
        std::string endpoint = "https://xmldata.qrz.com/xml/current/";
        std::string userAgent = "GetLocation/1.0";
        std::string sessionCacheFile = "qrz_session_cache.txt";
        int connectTimeoutSec = 8;
        int timeoutSec = 15;
        int sessionTtlSec = 1800;                      // 30 Minuten
    };

    explicit GetLocation(Config cfg);

    // Führt Lookup aus, wirft bei Fehlern std::runtime_error.
    Result lookup(const std::string& callsign);

    // Maidenhead->Lat/Lon auch separat nutzbar
    static std::pair<double, double> maidenheadToLatLon(const std::string& grid);

private:
    Config cfg_;

    static std::string httpGet(const std::string& url, const Config& cfg);
    static std::string urlEncode(const std::string& s);

    std::optional<std::string> loadCachedSessionKey() const;
    void saveCachedSessionKey(const std::string& key) const;
    void invalidateCachedSessionKey() const;

    std::string getSessionKey();
};
