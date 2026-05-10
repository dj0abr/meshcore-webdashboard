#include "RepeaterNodeSyncThread.h"
#include "MeshDB.h"

#include <curl/curl.h>

#include <cctype>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{
    std::string JsonEscape(const std::string& value)
    {
        std::ostringstream oss;

        for (unsigned char ch : value)
        {
            switch (ch)
            {
                case '"':
                {
                    oss << "\\\"";
                    break;
                }

                case '\\':
                {
                    oss << "\\\\";
                    break;
                }

                case '\b':
                {
                    oss << "\\b";
                    break;
                }

                case '\f':
                {
                    oss << "\\f";
                    break;
                }

                case '\n':
                {
                    oss << "\\n";
                    break;
                }

                case '\r':
                {
                    oss << "\\r";
                    break;
                }

                case '\t':
                {
                    oss << "\\t";
                    break;
                }

                default:
                {
                    if (ch < 0x20)
                    {
                        static const char* hex = "0123456789ABCDEF";
                        oss << "\\u00" << hex[(ch >> 4) & 0x0F] << hex[ch & 0x0F];
                    }
                    else
                    {
                        oss << static_cast<char>(ch);
                    }
                    break;
                }
            }
        }

        return oss.str();
    }

    void AppendJsonNullableString(std::ostringstream& oss, const std::string& name, const std::string& value)
    {
        oss << ",\"" << name << "\":";

        if (value.empty())
        {
            oss << "null";
        }
        else
        {
            oss << "\"" << JsonEscape(value) << "\"";
        }
    }

    void AppendJsonNullableInt(std::ostringstream& oss, const std::string& name, bool hasValue, int32_t value)
    {
        oss << ",\"" << name << "\":";

        if (!hasValue)
        {
            oss << "null";
        }
        else
        {
            oss << value;
        }
    }

    class JsonReader
    {
    public:
        explicit JsonReader(const std::string& text)
            : m_text(text)
        {
        }

        bool ParsePullResponse(std::vector<MeshDB::RepeaterNodeSyncRecord>& nodes)
        {
            SkipWhitespace();

            if (!Consume('{'))
            {
                return false;
            }

            while (true)
            {
                SkipWhitespace();

                if (Consume('}'))
                {
                    return true;
                }

                std::string key;
                if (!ParseString(key))
                {
                    return false;
                }

                SkipWhitespace();
                if (!Consume(':'))
                {
                    return false;
                }

                if (key == "nodes")
                {
                    if (!ParseNodesArray(nodes))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!SkipValue())
                    {
                        return false;
                    }
                }

                SkipWhitespace();

                if (Consume(','))
                {
                    continue;
                }

                if (Consume('}'))
                {
                    return true;
                }

                return false;
            }
        }


        bool ParseNodeAdvertPathsPullResponse(std::vector<MeshDB::NodeAdvertPathSyncRecord>& paths)
        {
            SkipWhitespace();

            if (!Consume('{'))
            {
                return false;
            }

            while (true)
            {
                SkipWhitespace();

                if (Consume('}'))
                {
                    return true;
                }

                std::string key;
                if (!ParseString(key))
                {
                    return false;
                }

                SkipWhitespace();
                if (!Consume(':'))
                {
                    return false;
                }

                if (key == "paths")
                {
                    if (!ParsePathsArray(paths))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!SkipValue())
                    {
                        return false;
                    }
                }

                SkipWhitespace();

                if (Consume(','))
                {
                    continue;
                }

                if (Consume('}'))
                {
                    return true;
                }

                return false;
            }
        }

    private:
        void SkipWhitespace()
        {
            while (m_pos < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[m_pos])))
            {
                m_pos++;
            }
        }

        bool Consume(char ch)
        {
            SkipWhitespace();

            if (m_pos < m_text.size() && m_text[m_pos] == ch)
            {
                m_pos++;
                return true;
            }

            return false;
        }

        bool ParseString(std::string& out)
        {
            SkipWhitespace();

            if (m_pos >= m_text.size() || m_text[m_pos] != '"')
            {
                return false;
            }

            m_pos++;
            out.clear();

            while (m_pos < m_text.size())
            {
                char ch = m_text[m_pos++];

                if (ch == '"')
                {
                    return true;
                }

                if (ch == '\\')
                {
                    if (m_pos >= m_text.size())
                    {
                        return false;
                    }

                    char esc = m_text[m_pos++];

                    switch (esc)
                    {
                        case '"': out.push_back('"'); break;
                        case '\\': out.push_back('\\'); break;
                        case '/': out.push_back('/'); break;
                        case 'b': out.push_back('\b'); break;
                        case 'f': out.push_back('\f'); break;
                        case 'n': out.push_back('\n'); break;
                        case 'r': out.push_back('\r'); break;
                        case 't': out.push_back('\t'); break;
                        case 'u':
                        {
                            if (m_pos + 4 > m_text.size())
                            {
                                return false;
                            }

                            // Die API liefert utf8mb4 meist direkt. \uXXXX kommt hier nur selten vor.
                            // Fuer ASCII-Codepoints dekodieren wir, alles andere bleibt als Ersatzzeichen erhalten.
                            unsigned code = 0;
                            for (int i = 0; i < 4; ++i)
                            {
                                char h = m_text[m_pos++];
                                code <<= 4;

                                if (h >= '0' && h <= '9') code += static_cast<unsigned>(h - '0');
                                else if (h >= 'a' && h <= 'f') code += static_cast<unsigned>(h - 'a' + 10);
                                else if (h >= 'A' && h <= 'F') code += static_cast<unsigned>(h - 'A' + 10);
                                else return false;
                            }

                            if (code <= 0x7F)
                            {
                                out.push_back(static_cast<char>(code));
                            }
                            else
                            {
                                out += "?";
                            }
                            break;
                        }
                        default:
                        {
                            return false;
                        }
                    }
                }
                else
                {
                    out.push_back(ch);
                }
            }

            return false;
        }

        bool ParseBool(bool& out)
        {
            SkipWhitespace();

            if (m_text.compare(m_pos, 4, "true") == 0)
            {
                m_pos += 4;
                out = true;
                return true;
            }

            if (m_text.compare(m_pos, 5, "false") == 0)
            {
                m_pos += 5;
                out = false;
                return true;
            }

            return false;
        }

        bool ParseNull()
        {
            SkipWhitespace();

            if (m_text.compare(m_pos, 4, "null") == 0)
            {
                m_pos += 4;
                return true;
            }

            return false;
        }

        bool ParseInt64(int64_t& out)
        {
            SkipWhitespace();

            std::size_t start = m_pos;

            if (m_pos < m_text.size() && m_text[m_pos] == '-')
            {
                m_pos++;
            }

            if (m_pos >= m_text.size() || !std::isdigit(static_cast<unsigned char>(m_text[m_pos])))
            {
                m_pos = start;
                return false;
            }

            while (m_pos < m_text.size() && std::isdigit(static_cast<unsigned char>(m_text[m_pos])))
            {
                m_pos++;
            }

            try
            {
                out = std::stoll(m_text.substr(start, m_pos - start));
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        bool ParseNullableString(std::string& out)
        {
            SkipWhitespace();

            if (ParseNull())
            {
                out.clear();
                return true;
            }

            return ParseString(out);
        }

        bool ParseNullableInt(bool& hasValue, int32_t& out)
        {
            SkipWhitespace();

            if (ParseNull())
            {
                hasValue = false;
                out = 0;
                return true;
            }

            int64_t tmp = 0;
            if (!ParseInt64(tmp))
            {
                return false;
            }

            hasValue = true;
            out = static_cast<int32_t>(tmp);
            return true;
        }

        bool SkipValue()
        {
            SkipWhitespace();

            if (m_pos >= m_text.size())
            {
                return false;
            }

            if (m_text[m_pos] == '"')
            {
                std::string dummy;
                return ParseString(dummy);
            }

            if (m_text[m_pos] == '{')
            {
                m_pos++;

                while (true)
                {
                    SkipWhitespace();
                    if (Consume('}'))
                    {
                        return true;
                    }

                    std::string key;
                    if (!ParseString(key))
                    {
                        return false;
                    }

                    if (!Consume(':') || !SkipValue())
                    {
                        return false;
                    }

                    if (Consume(','))
                    {
                        continue;
                    }

                    if (Consume('}'))
                    {
                        return true;
                    }

                    return false;
                }
            }

            if (m_text[m_pos] == '[')
            {
                m_pos++;

                while (true)
                {
                    SkipWhitespace();
                    if (Consume(']'))
                    {
                        return true;
                    }

                    if (!SkipValue())
                    {
                        return false;
                    }

                    if (Consume(','))
                    {
                        continue;
                    }

                    if (Consume(']'))
                    {
                        return true;
                    }

                    return false;
                }
            }

            bool b = false;
            if (ParseBool(b) || ParseNull())
            {
                return true;
            }

            int64_t n = 0;
            return ParseInt64(n);
        }


        bool ParsePathsArray(std::vector<MeshDB::NodeAdvertPathSyncRecord>& paths)
        {
            if (!Consume('['))
            {
                return false;
            }

            while (true)
            {
                SkipWhitespace();

                if (Consume(']'))
                {
                    return true;
                }

                MeshDB::NodeAdvertPathSyncRecord path;
                if (!ParsePathObject(path))
                {
                    return false;
                }

                paths.push_back(path);

                if (Consume(','))
                {
                    continue;
                }

                if (Consume(']'))
                {
                    return true;
                }

                return false;
            }
        }

        bool ParsePathObject(MeshDB::NodeAdvertPathSyncRecord& path)
        {
            if (!Consume('{'))
            {
                return false;
            }

            while (true)
            {
                SkipWhitespace();

                if (Consume('}'))
                {
                    return true;
                }

                std::string key;
                if (!ParseString(key))
                {
                    return false;
                }

                if (!Consume(':'))
                {
                    return false;
                }

                if (key == "public_key_hex")
                {
                    if (!ParseNullableString(path.publicKeyHex)) return false;
                }
                else if (key == "path_len")
                {
                    int32_t tmp = 0;
                    if (!ParseNullableInt(path.hasPathLen, tmp)) return false;
                    path.pathLen = static_cast<uint8_t>(tmp);
                }
                else if (key == "path_hash_size")
                {
                    int32_t tmp = 0;
                    if (!ParseNullableInt(path.hasPathHashSize, tmp)) return false;
                    path.pathHashSize = static_cast<uint8_t>(tmp);
                }
                else if (key == "path_text")
                {
                    if (!ParseNullableString(path.pathText)) return false;
                }
                else if (key == "last_seen_at")
                {
                    if (!ParseNullableString(path.lastSeenAt)) return false;
                }
                else
                {
                    if (!SkipValue()) return false;
                }

                if (Consume(','))
                {
                    continue;
                }

                if (Consume('}'))
                {
                    return true;
                }

                return false;
            }
        }

        bool ParseNodesArray(std::vector<MeshDB::RepeaterNodeSyncRecord>& nodes)
        {
            if (!Consume('['))
            {
                return false;
            }

            while (true)
            {
                SkipWhitespace();

                if (Consume(']'))
                {
                    return true;
                }

                MeshDB::RepeaterNodeSyncRecord node;
                if (!ParseNodeObject(node))
                {
                    return false;
                }

                nodes.push_back(node);

                if (Consume(','))
                {
                    continue;
                }

                if (Consume(']'))
                {
                    return true;
                }

                return false;
            }
        }

        bool ParseNodeObject(MeshDB::RepeaterNodeSyncRecord& node)
        {
            if (!Consume('{'))
            {
                return false;
            }

            while (true)
            {
                SkipWhitespace();

                if (Consume('}'))
                {
                    return true;
                }

                std::string key;
                if (!ParseString(key))
                {
                    return false;
                }

                if (!Consume(':'))
                {
                    return false;
                }

                if (key == "node_id")
                {
                    int32_t tmp = 0;
                    if (!ParseNullableInt(node.hasNodeId, tmp)) return false;
                    node.nodeId = static_cast<uint32_t>(tmp);
                }
                else if (key == "advert_type")
                {
                    int64_t v = 0;
                    if (!ParseInt64(v)) return false;
                    node.advertType = static_cast<uint8_t>(v);
                }
                else if (key == "advert_flags")
                {
                    int64_t v = 0;
                    if (!ParseInt64(v)) return false;
                    node.advertFlags = static_cast<uint8_t>(v);
                }
                else if (key == "name")
                {
                    if (!ParseNullableString(node.name)) return false;
                }
                else if (key == "public_key_hex")
                {
                    if (!ParseNullableString(node.publicKeyHex)) return false;
                }
                else if (key == "prefix6_hex")
                {
                    if (!ParseNullableString(node.prefix6Hex)) return false;
                }
                else if (key == "adv_lat_e6")
                {
                    if (!ParseNullableInt(node.hasAdvLatE6, node.advLatE6)) return false;
                }
                else if (key == "adv_lon_e6")
                {
                    if (!ParseNullableInt(node.hasAdvLonE6, node.advLonE6)) return false;
                }
                else if (key == "last_advert_at")
                {
                    if (!ParseNullableString(node.lastAdvertAt)) return false;
                }
                else if (key == "last_mod_at")
                {
                    if (!ParseNullableString(node.lastModAt)) return false;
                }
                else if (key == "first_seen_at")
                {
                    if (!ParseNullableString(node.firstSeenAt)) return false;
                }
                else
                {
                    if (!SkipValue()) return false;
                }

                if (Consume(','))
                {
                    continue;
                }

                if (Consume('}'))
                {
                    return true;
                }

                return false;
            }
        }

        const std::string& m_text;
        std::size_t m_pos = 0;
    };
}

RepeaterNodeSyncThread::RepeaterNodeSyncThread(const Config& config)
    : m_config(config)
{
}

RepeaterNodeSyncThread::~RepeaterNodeSyncThread()
{
    Stop();
}

void RepeaterNodeSyncThread::Start()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_running)
    {
        return;
    }

    if (m_config.baseUrl.empty() || m_config.apiToken.empty())
    {
        std::cout << "[RepeaterNodeSync] disabled: URL or token missing\n";
        return;
    }

    m_stopRequested = false;
    m_thread = std::thread(&RepeaterNodeSyncThread::ThreadMain, this);
    m_running = true;
}

void RepeaterNodeSyncThread::Stop()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_running)
        {
            return;
        }

        m_stopRequested = true;
    }

    m_cv.notify_all();

    if (m_thread.joinable())
    {
        m_thread.join();
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = false;
    }
}

void RepeaterNodeSyncThread::ThreadMain()
{
    try
    {
        RunOnce();

        std::unique_lock<std::mutex> lock(m_mutex);

        while (!m_stopRequested)
        {
            if (m_cv.wait_for(lock, m_config.runInterval, [this]() { return m_stopRequested; }))
            {
                break;
            }

            lock.unlock();

            try
            {
                RunOnce();
            }
            catch (const std::exception& e)
            {
                std::cerr << "[RepeaterNodeSync] RunOnce exception: " << e.what() << "\n";
            }
            catch (...)
            {
                std::cerr << "[RepeaterNodeSync] RunOnce unknown exception\n";
            }

            lock.lock();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[RepeaterNodeSync] thread exception: " << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "[RepeaterNodeSync] thread unknown exception\n";
    }
}

void RepeaterNodeSyncThread::RunOnce()
{
    if (!MeshDB::IsReady())
    {
        std::cerr << "[RepeaterNodeSync] MeshDB not ready\n";
        return;
    }

    SyncRepeaterNodes();
    SyncNodeAdvertPaths();
}

void RepeaterNodeSyncThread::SyncRepeaterNodes()
{
    const std::string pushUrl = JoinUrl(m_config.baseUrl, "repeaternodes_push.php");
    const std::string pullUrl = JoinUrl(m_config.baseUrl, "repeaternodes_pull.php");

    const std::string pushBody = BuildRepeaterNodesPushJson();
    const std::string pushResponse = HttpPostJson(pushUrl, pushBody);

    if (pushResponse.find("\"success\":true") == std::string::npos)
    {
        std::cerr << "[RepeaterNodeSync] repeaternodes push failed: " << pushResponse.substr(0, 300) << "\n";
        return;
    }

    const std::string pullResponse = HttpGet(pullUrl);

    std::vector<MeshDB::RepeaterNodeSyncRecord> nodes;
    JsonReader reader(pullResponse);

    if (!reader.ParsePullResponse(nodes))
    {
        std::cerr << "[RepeaterNodeSync] could not parse repeaternodes pull response\n";
        return;
    }

    unsigned inserted = 0;
    unsigned ignored = 0;
    unsigned skipped = 0;

    if (!MeshDB::InsertMissingRepeaterNodesFromSync(nodes, &inserted, &ignored, &skipped))
    {
        std::cerr << "[RepeaterNodeSync] local repeaternodes insert failed\n";
        return;
    }

    std::cout
        << "[RepeaterNodeSync] repeaternodes ok, pulled=" << nodes.size()
        << ", inserted=" << inserted
        << ", ignored=" << ignored
        << ", skipped=" << skipped
        << "\n";
}

void RepeaterNodeSyncThread::SyncNodeAdvertPaths()
{
    const std::string pushUrl = JoinUrl(m_config.baseUrl, "node_advert_paths_push.php");
    const std::string pullUrl = JoinUrl(m_config.baseUrl, "node_advert_paths_pull.php");

    const std::string pushBody = BuildNodeAdvertPathsPushJson();
    const std::string pushResponse = HttpPostJson(pushUrl, pushBody);

    if (pushResponse.find("\"success\":true") == std::string::npos)
    {
        std::cerr << "[RepeaterNodeSync] node_advert_paths push failed: " << pushResponse.substr(0, 300) << "\n";
        return;
    }

    const std::string pullResponse = HttpGet(pullUrl);

    std::vector<MeshDB::NodeAdvertPathSyncRecord> paths;
    JsonReader reader(pullResponse);

    if (!reader.ParseNodeAdvertPathsPullResponse(paths))
    {
        std::cerr << "[RepeaterNodeSync] could not parse node_advert_paths pull response\n";
        return;
    }

    unsigned inserted = 0;
    unsigned ignored = 0;
    unsigned skipped = 0;

    if (!MeshDB::InsertMissingNodeAdvertPathsFromSync(paths, &inserted, &ignored, &skipped))
    {
        std::cerr << "[RepeaterNodeSync] local node_advert_paths insert failed\n";
        return;
    }

    std::cout
        << "[RepeaterNodeSync] node_advert_paths ok, pulled=" << paths.size()
        << ", inserted=" << inserted
        << ", ignored=" << ignored
        << ", skipped=" << skipped
        << "\n";
}

std::string RepeaterNodeSyncThread::BuildRepeaterNodesPushJson()
{
    const std::vector<MeshDB::RepeaterNodeSyncRecord> nodes = MeshDB::ListRepeaterNodesForSync();

    std::ostringstream oss;
    oss << "{\"nodes\":[";

    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        const MeshDB::RepeaterNodeSyncRecord& node = nodes[i];

        if (i != 0)
        {
            oss << ",";
        }

        oss << "{";
        oss << "\"node_id\":" << (node.hasNodeId ? std::to_string(node.nodeId) : "null");
        oss << ",\"advert_type\":" << unsigned(node.advertType);
        oss << ",\"advert_flags\":" << unsigned(node.advertFlags);
        oss << ",\"name\":\"" << JsonEscape(node.name) << "\"";
        oss << ",\"public_key_hex\":\"" << JsonEscape(node.publicKeyHex) << "\"";
        AppendJsonNullableString(oss, "prefix6_hex", node.prefix6Hex);
        AppendJsonNullableInt(oss, "adv_lat_e6", node.hasAdvLatE6, node.advLatE6);
        AppendJsonNullableInt(oss, "adv_lon_e6", node.hasAdvLonE6, node.advLonE6);
        AppendJsonNullableString(oss, "last_advert_at", node.lastAdvertAt);
        AppendJsonNullableString(oss, "last_mod_at", node.lastModAt);
        AppendJsonNullableString(oss, "first_seen_at", node.firstSeenAt);
        oss << "}";
    }

    oss << "]}";
    return oss.str();
}

std::string RepeaterNodeSyncThread::BuildNodeAdvertPathsPushJson()
{
    const std::vector<MeshDB::NodeAdvertPathSyncRecord> paths = MeshDB::ListNodeAdvertPathsForSync();

    std::ostringstream oss;
    oss << "{\"paths\":[";

    for (std::size_t i = 0; i < paths.size(); ++i)
    {
        const MeshDB::NodeAdvertPathSyncRecord& path = paths[i];

        if (i != 0)
        {
            oss << ",";
        }

        oss << "{";
        oss << "\"public_key_hex\":\"" << JsonEscape(path.publicKeyHex) << "\"";
        oss << ",\"path_len\":" << (path.hasPathLen ? std::to_string(unsigned(path.pathLen)) : "null");
        oss << ",\"path_hash_size\":" << (path.hasPathHashSize ? std::to_string(unsigned(path.pathHashSize)) : "null");
        AppendJsonNullableString(oss, "path_text", path.pathText);
        AppendJsonNullableString(oss, "last_seen_at", path.lastSeenAt);
        oss << "}";
    }

    oss << "]}";
    return oss.str();
}

std::string RepeaterNodeSyncThread::HttpGet(const std::string& url)
{
    CURL* curl = curl_easy_init();

    if (curl == nullptr)
    {
        throw std::runtime_error("curl_easy_init failed");
    }

    std::string response;
    struct curl_slist* headers = nullptr;
    const std::string tokenHeader = "X-API-Token: " + m_config.apiToken;
    headers = curl_slist_append(headers, tokenHeader.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "meshcore-repeaternodes-sync/1.0");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, m_config.connectTimeoutSec);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, m_config.requestTimeoutSec);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode rc = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK)
    {
        throw std::runtime_error(std::string("curl GET failed: ") + curl_easy_strerror(rc));
    }

    if (httpCode < 200 || httpCode >= 300)
    {
        throw std::runtime_error("HTTP GET failed with code " + std::to_string(httpCode));
    }

    return response;
}

std::string RepeaterNodeSyncThread::HttpPostJson(const std::string& url, const std::string& body)
{
    CURL* curl = curl_easy_init();

    if (curl == nullptr)
    {
        throw std::runtime_error("curl_easy_init failed");
    }

    std::string response;
    struct curl_slist* headers = nullptr;
    const std::string tokenHeader = "X-API-Token: " + m_config.apiToken;
    headers = curl_slist_append(headers, tokenHeader.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "meshcore-repeaternodes-sync/1.0");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, m_config.connectTimeoutSec);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, m_config.requestTimeoutSec);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode rc = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK)
    {
        throw std::runtime_error(std::string("curl POST failed: ") + curl_easy_strerror(rc));
    }

    if (httpCode < 200 || httpCode >= 300)
    {
        throw std::runtime_error("HTTP POST failed with code " + std::to_string(httpCode));
    }

    return response;
}

std::string RepeaterNodeSyncThread::JoinUrl(const std::string& baseUrl, const std::string& path)
{
    if (baseUrl.empty())
    {
        return path;
    }

    if (baseUrl.back() == '/')
    {
        return baseUrl + path;
    }

    return baseUrl + "/" + path;
}

size_t RepeaterNodeSyncThread::CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    const size_t total = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);

    response->append(static_cast<const char*>(contents), total);
    return total;
}
