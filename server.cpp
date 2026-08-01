/*
 * Delhi Metro PathFinder - HTTP Backend Server
 *
 * Compile (Windows/MinGW):
 *   g++ -std=c++11 server.cpp -o server -lws2_32
 *
 * Run:
 *   ./server.exe
 *
 * Open browser: http://localhost:8080
 */

#include "httplib.h"
#include "metro.h"   // <-- all MetroSystem logic lives here

// ============================================================
// JSON Helpers (no external dependency)
// ============================================================

static string jsonEscape(const string& s) {
    string out;
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

static string buildStationsJson(const MetroSystem& metro) {
    vector<string> stations = metro.getAllStations();
    ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < stations.size(); i++) {
        if (i > 0) oss << ",";
        oss << "{\"name\":\"" << jsonEscape(stations[i]) << "\",\"lines\":[";
        set<string> lines = metro.getLinesForStation(stations[i]);
        bool first = true;
        for (const string& ln : lines) {
            if (!first) oss << ",";
            oss << "\"" << jsonEscape(ln) << "\"";
            first = false;
        }
        oss << "]}";
    }
    oss << "]";
    return oss.str();
}

static string buildRouteJson(const RouteResult& r, const string& from, const string& to) {
    ostringstream oss;

    if (r.totalTime == -1) {
        oss << "{\"error\":\"No route found between the specified stations.\","
            << "\"from\":\"" << jsonEscape(from) << "\","
            << "\"to\":\""   << jsonEscape(to)   << "\"}";
        return oss.str();
    }

    oss << "{"
        << "\"total_time\":"     << r.totalTime        << ","
        << "\"total_stations\":" << r.steps.size()     << ","
        << "\"changes\":"        << r.changes          << ","
        << "\"from\":\""         << jsonEscape(from)   << "\","
        << "\"to\":\""           << jsonEscape(to)     << "\",";

    // Full ordered station list
    oss << "\"stations\":[";
    for (size_t i = 0; i < r.steps.size(); i++) {
        if (i > 0) oss << ",";
        oss << "\"" << jsonEscape(r.steps[i].station) << "\"";
    }
    oss << "],";

    // Segments grouped by line
    oss << "\"segments\":[";
    if (!r.steps.empty()) {
        string curLine = r.steps.size() > 1 ? r.steps[1].line : "";
        vector<string> segStations;
        segStations.push_back(r.steps[0].station);

        bool firstSeg = true;
        for (size_t i = 1; i < r.steps.size(); i++) {
            string ln = r.steps[i].line;
            if (ln != curLine) {
                if (!firstSeg) oss << ",";
                firstSeg = false;
                oss << "{\"line\":\"" << jsonEscape(curLine) << "\",\"stations\":[";
                for (size_t j = 0; j < segStations.size(); j++) {
                    if (j > 0) oss << ",";
                    oss << "\"" << jsonEscape(segStations[j]) << "\"";
                }
                oss << "]}";
                segStations.clear();
                segStations.push_back(r.steps[i - 1].station); // overlap at interchange
                curLine = ln;
            }
            segStations.push_back(r.steps[i].station);
        }
        // last segment
        if (!segStations.empty()) {
            if (!firstSeg) oss << ",";
            oss << "{\"line\":\"" << jsonEscape(curLine) << "\",\"stations\":[";
            for (size_t j = 0; j < segStations.size(); j++) {
                if (j > 0) oss << ",";
                oss << "\"" << jsonEscape(segStations[j]) << "\"";
            }
            oss << "]}";
        }
    }
    oss << "]}";
    return oss.str();
}

// Simple JSON string extractor (avoids pulling in nlohmann/json)
static string extractJsonString(const string& body, const string& key) {
    string search = "\"" + key + "\"";
    size_t pos = body.find(search);
    if (pos == string::npos) return "";
    pos += search.size();
    while (pos < body.size() && (body[pos] == ' ' || body[pos] == ':')) pos++;
    if (pos >= body.size() || body[pos] != '"') return "";
    pos++;
    string result;
    while (pos < body.size() && body[pos] != '"') {
        if (body[pos] == '\\' && pos + 1 < body.size()) {
            pos++;
            if      (body[pos] == '"')  result += '"';
            else if (body[pos] == '\\') result += '\\';
            else if (body[pos] == 'n')  result += '\n';
            else result += body[pos];
        } else {
            result += body[pos];
        }
        pos++;
    }
    return result;
}

// ============================================================
// Main — HTTP Server
// ============================================================

int main() {
    MetroSystem metro;
    metro.setLineChangePenalty(5);
    initializeDelhiMetro(metro);

    httplib::Server svr;

    // Serve static files (index.html, style.css, app.js) from same directory
    svr.set_mount_point("/", ".");

    // CORS headers for all responses
    svr.set_pre_routing_handler([](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin",  "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        return httplib::Server::HandlerResponse::Unhandled;
    });

    svr.Options("/api/.*", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("", "text/plain");
    });

    // GET /api/stations  →  JSON array of all stations + their lines
    svr.Get("/api/stations", [&metro](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(buildStationsJson(metro), "application/json");
    });

    // POST /api/route  →  { "from": "...", "to": "..." }  →  JSON route
    svr.Post("/api/route", [&metro](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        string from = extractJsonString(req.body, "from");
        string to   = extractJsonString(req.body, "to");
        RouteResult result = metro.findRoute(from, to);
        res.set_content(buildRouteJson(result, from, to), "application/json");
    });

    cout << "==================================================" << endl;
    cout << "   Delhi Metro PathFinder  –  Web Server"         << endl;
    cout << "==================================================" << endl;
    cout << "   http://localhost:1158"                          << endl;
    cout << "   Press Ctrl+C to stop."                         << endl;
    cout << "==================================================" << endl;

    svr.listen("localhost", 1158);
    return 0;
}
