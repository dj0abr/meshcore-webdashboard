#include "GetLocation.hpp"

#include <ctime>
#include <cctype>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <curl/curl.h>
#include <tinyxml2.h>

using namespace tinyxml2;

GetLocation::GetLocation(Config cfg) : cfg_(std::move(cfg)) {
    if (cfg_.qrzUsername.empty() || cfg_.qrzPassword.empty()) {
        throw std::runtime_error("QRZ Username/Password fehlen in Config.");
    }
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

static size_t curlWriteCb(void* contents, size_t size, size_t nmemb, void* userp) {
    const size_t total = size * nmemb;
    auto* out = static_cast<std::string*>(userp);
    out->append(static_cast<char*>(contents), total);
    return total;
}

std::string GetLocation::httpGet(const std::string& url, const Config& cfg) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init fehlgeschlagen");

    std::string resp;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, cfg.userAgent.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, cfg.timeoutSec);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, cfg.connectTimeoutSec);

    // HTTPS
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK) {
        std::string err = curl_easy_strerror(rc);
        curl_easy_cleanup(curl);
        throw std::runtime_error("HTTP GET Fehler: " + err);
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (httpCode < 200 || httpCode >= 300) {
        std::ostringstream oss;
        oss << "HTTP Status " << httpCode;
        throw std::runtime_error(oss.str());
    }
    return resp;
}

std::string GetLocation::urlEncode(const std::string& s) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init für urlEncode fehlgeschlagen");

    char* enc = curl_easy_escape(curl, s.c_str(), static_cast<int>(s.size()));
    if (!enc) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("curl_easy_escape fehlgeschlagen");
    }

    std::string out(enc);
    curl_free(enc);
    curl_easy_cleanup(curl);
    return out;
}

std::optional<std::string> GetLocation::loadCachedSessionKey() const {
    std::ifstream in(cfg_.sessionCacheFile);
    if (!in) return std::nullopt;

    long long ts = 0;
    std::string key;
    in >> ts;
    in >> key;
    if (!in || key.empty()) return std::nullopt;

    const long long now = static_cast<long long>(std::time(nullptr));
    if (now - ts >= cfg_.sessionTtlSec) return std::nullopt;

    return key;
}

void GetLocation::saveCachedSessionKey(const std::string& key) const {
    std::ofstream out(cfg_.sessionCacheFile, std::ios::trunc);
    if (!out) return; // Cache ist optional
    out << static_cast<long long>(std::time(nullptr)) << "\n" << key << "\n";
}

void GetLocation::invalidateCachedSessionKey() const {
    std::ofstream out(cfg_.sessionCacheFile, std::ios::trunc);
    // leeren reicht
}

std::string GetLocation::getSessionKey() {
    if (auto cached = loadCachedSessionKey(); cached && !cached->empty()) {
        return *cached;
    }

    const std::string url =
        cfg_.endpoint +
        "?username=" + urlEncode(cfg_.qrzUsername) +
        "&password=" + urlEncode(cfg_.qrzPassword) +
        "&agent=" + urlEncode("GetLocation-Cpp");

    const std::string xml = httpGet(url, cfg_);

    XMLDocument doc;
    if (doc.Parse(xml.c_str()) != XML_SUCCESS) {
        throw std::runtime_error("Ungültiges XML von QRZ (Login).");
    }

    // QRZDatabase/Session/Key
    XMLElement* root = doc.FirstChildElement("QRZDatabase");
    if (!root) throw std::runtime_error("QRZ XML: Root 'QRZDatabase' fehlt.");

    XMLElement* sess = root->FirstChildElement("Session");
    if (!sess) throw std::runtime_error("QRZ XML: 'Session' fehlt.");

    if (auto* err = sess->FirstChildElement("Error"); err && err->GetText()) {
        throw std::runtime_error(std::string("QRZ Login Fehler: ") + err->GetText());
    }

    XMLElement* keyEl = sess->FirstChildElement("Key");
    const char* keyTxt = keyEl ? keyEl->GetText() : nullptr;
    if (!keyTxt || std::string(keyTxt).empty()) {
        throw std::runtime_error("QRZ Login fehlgeschlagen: Session Key fehlt.");
    }

    std::string key = keyTxt;
    saveCachedSessionKey(key);
    return key;
}

GetLocation::Result GetLocation::lookup(const std::string& callsignIn) {
    std::string callsign;
    callsign.reserve(callsignIn.size());
    for (char c : callsignIn) callsign.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));

    // 1) Session-Key holen
    std::string key = getSessionKey();

    // 2) Callsign Lookup
    const std::string url =
        cfg_.endpoint +
        "?s=" + urlEncode(key) +
        "&callsign=" + urlEncode(callsign);

    std::string xml;
    try {
        xml = httpGet(url, cfg_);
    } catch (...) {
        throw;
    }

    XMLDocument doc;
    if (doc.Parse(xml.c_str()) != XML_SUCCESS) {
        throw std::runtime_error("Ungültiges XML von QRZ (Lookup).");
    }

    XMLElement* root = doc.FirstChildElement("QRZDatabase");
    if (!root) throw std::runtime_error("QRZ XML: Root 'QRZDatabase' fehlt.");

    XMLElement* sess = root->FirstChildElement("Session");
    if (sess) {
        if (auto* err = sess->FirstChildElement("Error"); err && err->GetText()) {
            // häufig: "Session Timeout" o.ä. -> Cache killen
            invalidateCachedSessionKey();
            throw std::runtime_error(std::string("QRZ Session Fehler: ") + err->GetText());
        }
    }

    XMLElement* cs = root->FirstChildElement("Callsign");
    if (!cs) throw std::runtime_error("QRZ XML: 'Callsign' Block fehlt.");

    XMLElement* gridEl = cs->FirstChildElement("grid");
    std::string grid = (gridEl && gridEl->GetText()) ? gridEl->GetText() : "";
    if (grid.empty()) throw std::runtime_error("Kein QTH Locator (grid) gefunden.");

    auto [lat, lon] = maidenheadToLatLon(grid);

    Result r;
    r.callsign = callsign;
    r.grid = grid;
    r.lat = lat;
    r.lon = lon;
    return r;
}

// Maidenhead Locator -> Mittelpunkt (Lat/Lon) in Grad
std::pair<double, double> GetLocation::maidenheadToLatLon(const std::string& gridIn) {
    // Entferne Spaces, normalisiere
    std::string g;
    for (char c : gridIn) {
        if (!std::isspace(static_cast<unsigned char>(c)))
            g.push_back(c);
    }
    if (g.size() < 4 || (g.size() % 2) != 0) {
        throw std::runtime_error("Ungültiger Maidenhead Locator: muss Länge 4/6/8/... (gerade) haben.");
    }

    // Validieren/Normalisieren: 1. Paar Buchstaben, 2. Paar Ziffern, 3. Paar Buchstaben, 4. Paar Ziffern, ...
    // Wir unterstützen bis 10 Zeichen problemlos; länger geht auch iterativ (siehe unten).
    auto up = [](char c){ return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); };

    double lon = -180.0;
    double lat =  -90.0;

    double sizeLon = 360.0;
    double sizeLat = 180.0;

    for (size_t i = 0; i < g.size(); i += 2) {
        const size_t pairIndex = i / 2;
        char a = g[i];
        char b = g[i + 1];

        const bool expectLetters = (pairIndex % 2 == 0);
        if (expectLetters) {
            a = up(a); b = up(b);
            if (a < 'A' || a > 'X' || b < 'A' || b > 'X') {
                // im 1. Paar eigentlich nur A-R gültig, aber wir checken unten separat
                // hier grob
            }

            if (pairIndex == 0) {
                // Field: A-R
                if (a < 'A' || a > 'R' || b < 'A' || b > 'R') {
                    throw std::runtime_error("Ungültiger Maidenhead Locator: Field muss A-R sein.");
                }
                sizeLon = 20.0;
                sizeLat = 10.0;
                lon += (a - 'A') * sizeLon;
                lat += (b - 'A') * sizeLat;
            } else {
                // Subsquare (A-X) teilt jeweils durch 24
                if (a < 'A' || a > 'X' || b < 'A' || b > 'X') {
                    throw std::runtime_error("Ungültiger Maidenhead Locator: Letter-Paar muss A-X sein.");
                }
                sizeLon /= 24.0;
                sizeLat /= 24.0;
                lon += (a - 'A') * sizeLon;
                lat += (b - 'A') * sizeLat;
            }
        } else {
            // Digits: 0-9
            if (a < '0' || a > '9' || b < '0' || b > '9') {
                throw std::runtime_error("Ungültiger Maidenhead Locator: Digit-Paar muss 0-9 sein.");
            }
            // Square teilt durch 10
            sizeLon /= 10.0;
            sizeLat /= 10.0;
            lon += (a - '0') * sizeLon;
            lat += (b - '0') * sizeLat;
        }
    }

    // Mittelpunkt der Zelle
    lon += sizeLon / 2.0;
    lat += sizeLat / 2.0;

    // Clamp (optional, aber nett)
    if (lon < -180.0) lon = -180.0;
    if (lon >  180.0) lon =  180.0;
    if (lat <  -90.0) lat =  -90.0;
    if (lat >   90.0) lat =   90.0;

    return {lat, lon};
}
