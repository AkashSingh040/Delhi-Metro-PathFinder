#pragma once

/*
 * metro.h — Delhi Metro shared data structures, graph, and pathfinding logic.
 * Included by both:
 *   - Delhi-Metro-PathFinder.cpp  (CLI app)
 *   - server.cpp                  (HTTP backend)
 */

#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <string>
#include <climits>
#include <algorithm>
#include <iomanip>
#include <sstream>

using namespace std;

// ============================================================
// Structures
// ============================================================

struct Connection {
    string destination;
    int time;
    string line;
    Connection(string dest, int t, string l) : destination(dest), time(t), line(l) {}
};

struct Node {
    int distance;
    string station;
    string currentLine;
    Node(int d, string s, string l) : distance(d), station(s), currentLine(l) {}
    bool operator>(const Node& other) const { return distance > other.distance; }
};

// Used by the HTTP server for structured JSON responses
struct RouteStep {
    string station;
    string line; // empty for start station
};

struct RouteResult {
    int totalTime;   // -1 if no route found
    int changes;
    vector<RouteStep> steps;
};

// ============================================================
// MetroSystem Class
// ============================================================

class MetroSystem {
private:
    map<string, vector<Connection>> graph;
    map<string, set<string>>        stationLines;
    int LINE_CHANGE_PENALTY;

public:
    MetroSystem() : LINE_CHANGE_PENALTY(5) {}

    // ---- Graph building ----

    void addConnection(string from, string to, int time, string line) {
        graph[from].push_back(Connection(to, time, line));
        graph[to].push_back(Connection(from, time, line));
        stationLines[from].insert(line);
        stationLines[to].insert(line);
    }

    void setLineChangePenalty(int penalty) {
        LINE_CHANGE_PENALTY = penalty;
    }

    // ---- Dijkstra (returns total time + ordered station path) ----

    pair<int, vector<string>> findShortestPath(string start, string end) {
        map<string, int> dist;
        map<string, pair<string, string>> parent;
        priority_queue<Node, vector<Node>, greater<Node>> pq;

        for (auto& kv : graph) dist[kv.first] = INT_MAX;
        dist[start] = 0;
        pq.push(Node(0, start, ""));

        while (!pq.empty()) {
            Node current = pq.top(); pq.pop();

            if (current.distance > dist[current.station]) continue;

            vector<Connection>& connections = graph[current.station];
            for (size_t i = 0; i < connections.size(); i++) {
                Connection& conn = connections[i];
                int edgeWeight = conn.time;
                if (!current.currentLine.empty() && current.currentLine != conn.line)
                    edgeWeight += LINE_CHANGE_PENALTY;

                int newDist = current.distance + edgeWeight;
                if (newDist < dist[conn.destination]) {
                    dist[conn.destination] = newDist;
                    parent[conn.destination] = make_pair(current.station, conn.line);
                    pq.push(Node(newDist, conn.destination, conn.line));
                }
            }
        }

        vector<string> path;
        if (dist[end] == INT_MAX) return make_pair(-1, path);

        string cur = end;
        while (cur != start) {
            path.push_back(cur);
            cur = parent[cur].first;
        }
        path.push_back(start);
        reverse(path.begin(), path.end());
        return make_pair(dist[end], path);
    }

    // ---- CLI display (used by Delhi-Metro-PathFinder.cpp) ----

    void displayRoute(string start, string end) {
        pair<int, vector<string>> result = findShortestPath(start, end);
        int totalTime = result.first;
        vector<string>& path = result.second;

        if (totalTime == -1) {
            cout << "\n[X] No route found from " << start << " to " << end << endl;
            return;
        }

        cout << "\n========================================================\n";
        cout << "           DELHI METRO ROUTE FINDER                  \n";
        cout << "========================================================\n";
        cout << "\nFROM: " << start << "\n";
        cout << "TO: "   << end   << "\n";
        cout << "TOTAL TIME: "     << totalTime   << " minutes\n";
        cout << "TOTAL STATIONS: " << path.size() << "\n";

        // Re-run Dijkstra to get line info for display
        map<string, pair<string, string>> parent;
        map<string, int> dist;
        priority_queue<Node, vector<Node>, greater<Node>> pq;

        for (auto& kv : graph) dist[kv.first] = INT_MAX;
        dist[start] = 0;
        pq.push(Node(0, start, ""));

        while (!pq.empty()) {
            Node current = pq.top(); pq.pop();
            if (current.distance > dist[current.station]) continue;
            vector<Connection>& connections = graph[current.station];
            for (size_t i = 0; i < connections.size(); i++) {
                Connection& conn = connections[i];
                int edgeWeight = conn.time;
                if (!current.currentLine.empty() && current.currentLine != conn.line)
                    edgeWeight += LINE_CHANGE_PENALTY;
                int newDist = current.distance + edgeWeight;
                if (newDist < dist[conn.destination]) {
                    dist[conn.destination] = newDist;
                    parent[conn.destination] = make_pair(current.station, conn.line);
                    pq.push(Node(newDist, conn.destination, conn.line));
                }
            }
        }

        cout << "\n--------------------------------------------------------\n";
        cout << "DETAILED ROUTE:\n";
        cout << "--------------------------------------------------------\n\n";

        string currentLine = "";
        int stationCount = 0;

        for (size_t i = 0; i < path.size(); i++) {
            if (i == 0) {
                cout << "START: " << path[i] << "\n";
            } else {
                string line = parent[path[i]].second;
                if (line != currentLine) {
                    if (!currentLine.empty()) {
                        cout << "\n   *** CHANGE LINE ***\n";
                        cout << "   From: " << currentLine << " Line\n";
                        cout << "   To: "   << line        << " Line\n";
                        cout << "   (Add " << LINE_CHANGE_PENALTY << " min for transfer)\n\n";
                    } else {
                        cout << "\nBoard: " << line << " Line\n\n";
                    }
                    currentLine = line;
                    stationCount = 0;
                }
                stationCount++;
                if (i == path.size() - 1) {
                    cout << "END: " << path[i] << " (" << currentLine << " Line)\n";
                } else {
                    cout << "   " << stationCount << ". " << path[i];
                    if (stationLines[path[i]].size() > 1) {
                        cout << " [INTERCHANGE: ";
                        bool first = true;
                        for (auto& ln : stationLines[path[i]]) {
                            if (!first) cout << ", ";
                            cout << ln;
                            first = false;
                        }
                        cout << "]";
                    }
                    cout << "\n";
                }
            }
        }
        cout << "\n--------------------------------------------------------\n\n";
    }

    void listAllStations() {
        cout << "\n========================================================\n";
        cout << "           ALL DELHI METRO STATIONS                     \n";
        cout << "========================================================\n\n";

        vector<string> stations;
        for (auto& kv : graph) stations.push_back(kv.first);
        sort(stations.begin(), stations.end());

        int count = 1;
        for (size_t i = 0; i < stations.size(); i++) {
            cout << setw(3) << count++ << ". " << setw(35) << left << stations[i];
            cout << " [";
            bool first = true;
            for (auto& ln : stationLines[stations[i]]) {
                if (!first) cout << ", ";
                cout << ln;
                first = false;
            }
            cout << "]\n";
        }
        cout << "\nTotal Stations: " << stations.size() << "\n\n";
    }

    // ---- API helpers (used by server.cpp) ----

    vector<string> getAllStations() const {
        vector<string> stations;
        for (auto& kv : graph) stations.push_back(kv.first);
        sort(stations.begin(), stations.end());
        return stations;
    }

    set<string> getLinesForStation(const string& station) const {
        auto it = stationLines.find(station);
        return (it != stationLines.end()) ? it->second : set<string>();
    }

    bool stationExists(const string& name) const {
        return graph.find(name) != graph.end();
    }

    // Returns a RouteResult with per-step line information (for JSON response)
    RouteResult findRoute(const string& start, const string& end) const {
        RouteResult result;
        result.totalTime = -1;
        result.changes = 0;

        if (!stationExists(start) || !stationExists(end)) return result;

        map<string, int> dist;
        map<string, pair<string, string>> parent;
        priority_queue<Node, vector<Node>, greater<Node>> pq;

        for (auto& kv : graph) dist[kv.first] = INT_MAX;
        dist[start] = 0;
        pq.push(Node(0, start, ""));

        while (!pq.empty()) {
            Node current = pq.top(); pq.pop();
            if (current.distance > dist[current.station]) continue;

            const vector<Connection>& connections = graph.at(current.station);
            for (size_t i = 0; i < connections.size(); i++) {
                const Connection& conn = connections[i];
                int edgeWeight = conn.time;
                if (!current.currentLine.empty() && current.currentLine != conn.line)
                    edgeWeight += LINE_CHANGE_PENALTY;
                int newDist = current.distance + edgeWeight;
                if (newDist < dist[conn.destination]) {
                    dist[conn.destination] = newDist;
                    parent[conn.destination] = make_pair(current.station, conn.line);
                    pq.push(Node(newDist, conn.destination, conn.line));
                }
            }
        }

        if (dist[end] == INT_MAX) return result;

        // Reconstruct path with line info
        vector<pair<string, string>> pathWithLines;
        string cur = end;
        while (cur != start) {
            pathWithLines.push_back(make_pair(cur, parent[cur].second));
            cur = parent[cur].first;
        }
        pathWithLines.push_back(make_pair(start, ""));
        reverse(pathWithLines.begin(), pathWithLines.end());

        // Count changes
        string prevLine = "";
        int changes = 0;
        for (size_t i = 1; i < pathWithLines.size(); i++) {
            string ln = pathWithLines[i].second;
            if (!prevLine.empty() && ln != prevLine) changes++;
            prevLine = ln;
        }

        result.totalTime = dist[end];
        result.changes = changes;
        for (auto& p : pathWithLines) {
            RouteStep step;
            step.station = p.first;
            step.line    = p.second;
            result.steps.push_back(step);
        }
        return result;
    }
};

// ============================================================
// Initialize complete Delhi Metro network
// ============================================================

void initializeDelhiMetro(MetroSystem& metro) {

    // RED LINE (Line 1)
    metro.addConnection("Rithala", "Rohini West", 2, "Red");
    metro.addConnection("Rohini West", "Rohini East", 2, "Red");
    metro.addConnection("Rohini East", "Pitampura", 2, "Red");
    metro.addConnection("Pitampura", "Kohat Enclave", 2, "Red");
    metro.addConnection("Kohat Enclave", "Netaji Subhash Place", 2, "Red");
    metro.addConnection("Netaji Subhash Place", "Keshav Puram", 2, "Red");
    metro.addConnection("Keshav Puram", "Kanhaiya Nagar", 2, "Red");
    metro.addConnection("Kanhaiya Nagar", "Inderlok", 2, "Red");
    metro.addConnection("Inderlok", "Shastri Nagar", 2, "Red");
    metro.addConnection("Shastri Nagar", "Pratap Nagar", 2, "Red");
    metro.addConnection("Pratap Nagar", "Pulbangash", 2, "Red");
    metro.addConnection("Pulbangash", "Tis Hazari", 2, "Red");
    metro.addConnection("Tis Hazari", "Kashmere Gate", 2, "Red");
    metro.addConnection("Kashmere Gate", "Shastri Park", 3, "Red");
    metro.addConnection("Shastri Park", "Seelampur", 2, "Red");
    metro.addConnection("Seelampur", "Welcome", 2, "Red");
    metro.addConnection("Welcome", "Shahdara", 2, "Red");
    metro.addConnection("Shahdara", "Mansarovar Park", 2, "Red");
    metro.addConnection("Mansarovar Park", "Jhilmil", 2, "Red");
    metro.addConnection("Jhilmil", "Dilshad Garden", 2, "Red");
    metro.addConnection("Dilshad Garden", "Shaheed Sthal", 3, "Red");

    // YELLOW LINE (Line 2)
    metro.addConnection("Samaypur Badli", "Rohini Sector 18", 2, "Yellow");
    metro.addConnection("Rohini Sector 18", "Haiderpur Badli Mor", 2, "Yellow");
    metro.addConnection("Haiderpur Badli Mor", "Jahangirpuri", 2, "Yellow");
    metro.addConnection("Jahangirpuri", "Adarsh Nagar", 2, "Yellow");
    metro.addConnection("Adarsh Nagar", "Azadpur", 2, "Yellow");
    metro.addConnection("Azadpur", "Model Town", 2, "Yellow");
    metro.addConnection("Model Town", "GTB Nagar", 2, "Yellow");
    metro.addConnection("GTB Nagar", "Vishwavidyalaya", 2, "Yellow");
    metro.addConnection("Vishwavidyalaya", "Vidhan Sabha", 2, "Yellow");
    metro.addConnection("Vidhan Sabha", "Civil Lines", 2, "Yellow");
    metro.addConnection("Civil Lines", "Kashmere Gate", 2, "Yellow");
    metro.addConnection("Kashmere Gate", "Chandni Chowk", 2, "Yellow");
    metro.addConnection("Chandni Chowk", "Chawri Bazar", 2, "Yellow");
    metro.addConnection("Chawri Bazar", "New Delhi", 2, "Yellow");
    metro.addConnection("New Delhi", "Rajiv Chowk", 2, "Yellow");
    metro.addConnection("Rajiv Chowk", "Patel Chowk", 2, "Yellow");
    metro.addConnection("Patel Chowk", "Central Secretariat", 2, "Yellow");
    metro.addConnection("Central Secretariat", "Udyog Bhawan", 2, "Yellow");
    metro.addConnection("Udyog Bhawan", "Lok Kalyan Marg", 2, "Yellow");
    metro.addConnection("Lok Kalyan Marg", "Jor Bagh", 2, "Yellow");
    metro.addConnection("Jor Bagh", "INA", 2, "Yellow");
    metro.addConnection("INA", "AIIMS", 2, "Yellow");
    metro.addConnection("AIIMS", "Green Park", 2, "Yellow");
    metro.addConnection("Green Park", "Hauz Khas", 2, "Yellow");
    metro.addConnection("Hauz Khas", "Malviya Nagar", 2, "Yellow");
    metro.addConnection("Malviya Nagar", "Saket", 2, "Yellow");
    metro.addConnection("Saket", "Qutab Minar", 2, "Yellow");
    metro.addConnection("Qutab Minar", "Chattarpur", 2, "Yellow");
    metro.addConnection("Chattarpur", "Sultanpur", 2, "Yellow");
    metro.addConnection("Sultanpur", "Ghitorni", 2, "Yellow");
    metro.addConnection("Ghitorni", "Arjan Garh", 2, "Yellow");
    metro.addConnection("Arjan Garh", "Guru Dronacharya", 2, "Yellow");
    metro.addConnection("Guru Dronacharya", "Sikanderpur", 2, "Yellow");
    metro.addConnection("Sikanderpur", "MG Road", 2, "Yellow");
    metro.addConnection("MG Road", "IFFCO Chowk", 2, "Yellow");
    metro.addConnection("IFFCO Chowk", "HUDA City Centre", 2, "Yellow");

    // BLUE LINE (Line 3 & 4)
    metro.addConnection("Dwarka Sector 21", "Dwarka Sector 8", 2, "Blue");
    metro.addConnection("Dwarka Sector 8", "Dwarka Sector 9", 2, "Blue");
    metro.addConnection("Dwarka Sector 9", "Dwarka Sector 10", 2, "Blue");
    metro.addConnection("Dwarka Sector 10", "Dwarka Sector 11", 2, "Blue");
    metro.addConnection("Dwarka Sector 11", "Dwarka Sector 12", 2, "Blue");
    metro.addConnection("Dwarka Sector 12", "Dwarka Sector 13", 2, "Blue");
    metro.addConnection("Dwarka Sector 13", "Dwarka Sector 14", 2, "Blue");
    metro.addConnection("Dwarka Sector 14", "Dwarka", 2, "Blue");
    metro.addConnection("Dwarka", "Dwarka Mor", 2, "Blue");
    metro.addConnection("Dwarka Mor", "Nawada", 2, "Blue");
    metro.addConnection("Nawada", "Uttam Nagar West", 2, "Blue");
    metro.addConnection("Uttam Nagar West", "Uttam Nagar East", 2, "Blue");
    metro.addConnection("Uttam Nagar East", "Janakpuri West", 2, "Blue");
    metro.addConnection("Janakpuri West", "Janakpuri East", 2, "Blue");
    metro.addConnection("Janakpuri East", "Tilak Nagar", 2, "Blue");
    metro.addConnection("Tilak Nagar", "Subhash Nagar", 2, "Blue");
    metro.addConnection("Subhash Nagar", "Tagore Garden", 2, "Blue");
    metro.addConnection("Tagore Garden", "Rajouri Garden", 2, "Blue");
    metro.addConnection("Rajouri Garden", "Ramesh Nagar", 2, "Blue");
    metro.addConnection("Ramesh Nagar", "Moti Nagar", 2, "Blue");
    metro.addConnection("Moti Nagar", "Kirti Nagar", 2, "Blue");
    metro.addConnection("Kirti Nagar", "Shadipur", 2, "Blue");
    metro.addConnection("Shadipur", "Patel Nagar", 2, "Blue");
    metro.addConnection("Patel Nagar", "Rajendra Place", 2, "Blue");
    metro.addConnection("Rajendra Place", "Karol Bagh", 2, "Blue");
    metro.addConnection("Karol Bagh", "Jhandewalan", 2, "Blue");
    metro.addConnection("Jhandewalan", "Ramakrishna Ashram Marg", 2, "Blue");
    metro.addConnection("Ramakrishna Ashram Marg", "Rajiv Chowk", 2, "Blue");
    metro.addConnection("Rajiv Chowk", "Barakhamba Road", 2, "Blue");
    metro.addConnection("Barakhamba Road", "Mandi House", 2, "Blue");
    metro.addConnection("Mandi House", "Supreme Court", 2, "Blue");
    metro.addConnection("Supreme Court", "Indraprastha", 2, "Blue");
    metro.addConnection("Indraprastha", "Yamuna Bank", 2, "Blue");
    metro.addConnection("Yamuna Bank", "Akshardham", 2, "Blue");
    metro.addConnection("Akshardham", "Mayur Vihar Phase 1", 2, "Blue");
    metro.addConnection("Mayur Vihar Phase 1", "Mayur Vihar Extension", 2, "Blue");
    metro.addConnection("Mayur Vihar Extension", "New Ashok Nagar", 2, "Blue");
    metro.addConnection("New Ashok Nagar", "Noida Sector 15", 2, "Blue");
    metro.addConnection("Noida Sector 15", "Noida Sector 16", 2, "Blue");
    metro.addConnection("Noida Sector 16", "Noida Sector 18", 2, "Blue");
    metro.addConnection("Noida Sector 18", "Botanical Garden", 2, "Blue");
    metro.addConnection("Botanical Garden", "Golf Course", 2, "Blue");
    metro.addConnection("Golf Course", "Noida City Centre", 2, "Blue");
    // Blue Line Branch - Vaishali
    metro.addConnection("Yamuna Bank", "Laxmi Nagar", 2, "Blue");
    metro.addConnection("Laxmi Nagar", "Nirman Vihar", 2, "Blue");
    metro.addConnection("Nirman Vihar", "Preet Vihar", 2, "Blue");
    metro.addConnection("Preet Vihar", "Karkarduma", 2, "Blue");
    metro.addConnection("Karkarduma", "Anand Vihar", 2, "Blue");
    metro.addConnection("Anand Vihar", "Kaushambi", 2, "Blue");
    metro.addConnection("Kaushambi", "Vaishali", 2, "Blue");

    // GREEN LINE (Line 5)
    metro.addConnection("Kirti Nagar", "Satguru Ram Singh Marg", 2, "Green");
    metro.addConnection("Satguru Ram Singh Marg", "Inderlok", 2, "Green");
    metro.addConnection("Inderlok", "Ashok Park Main", 2, "Green");
    metro.addConnection("Ashok Park Main", "Punjabi Bagh", 2, "Green");
    metro.addConnection("Punjabi Bagh", "Shivaji Park", 2, "Green");
    metro.addConnection("Shivaji Park", "Madipur", 2, "Green");
    metro.addConnection("Madipur", "Paschim Vihar East", 2, "Green");
    metro.addConnection("Paschim Vihar East", "Paschim Vihar West", 2, "Green");
    metro.addConnection("Paschim Vihar West", "Peera Garhi", 2, "Green");
    metro.addConnection("Peera Garhi", "Udyog Nagar", 2, "Green");
    metro.addConnection("Udyog Nagar", "Maharaja Surajmal Stadium", 2, "Green");
    metro.addConnection("Maharaja Surajmal Stadium", "Nangloi", 2, "Green");
    metro.addConnection("Nangloi", "Nangloi Railway Station", 2, "Green");
    metro.addConnection("Nangloi Railway Station", "Rajdhani Park", 2, "Green");
    metro.addConnection("Rajdhani Park", "Mundka", 2, "Green");
    metro.addConnection("Mundka", "Mundka Industrial Area", 2, "Green");
    metro.addConnection("Mundka Industrial Area", "Ghevra", 2, "Green");
    metro.addConnection("Ghevra", "Tikri Kalan", 2, "Green");
    metro.addConnection("Tikri Kalan", "Tikri Border", 2, "Green");
    metro.addConnection("Tikri Border", "Pandit Shree Ram Sharma", 2, "Green");
    metro.addConnection("Pandit Shree Ram Sharma", "Bahadurgarh City", 2, "Green");
    metro.addConnection("Bahadurgarh City", "Brigadier Hoshiar Singh", 2, "Green");

    // VIOLET LINE (Line 6)
    metro.addConnection("Kashmere Gate", "Lal Quila", 2, "Violet");
    metro.addConnection("Lal Quila", "Jama Masjid", 2, "Violet");
    metro.addConnection("Jama Masjid", "Delhi Gate", 2, "Violet");
    metro.addConnection("Delhi Gate", "ITO", 2, "Violet");
    metro.addConnection("ITO", "Mandi House", 2, "Violet");
    metro.addConnection("Mandi House", "Janpath", 2, "Violet");
    metro.addConnection("Janpath", "Central Secretariat", 2, "Violet");
    metro.addConnection("Central Secretariat", "Khan Market", 2, "Violet");
    metro.addConnection("Khan Market", "Jawaharlal Nehru Stadium", 2, "Violet");
    metro.addConnection("Jawaharlal Nehru Stadium", "Jangpura", 2, "Violet");
    metro.addConnection("Jangpura", "Lajpat Nagar", 2, "Violet");
    metro.addConnection("Lajpat Nagar", "Moolchand", 2, "Violet");
    metro.addConnection("Moolchand", "Kailash Colony", 2, "Violet");
    metro.addConnection("Kailash Colony", "Nehru Place", 2, "Violet");
    metro.addConnection("Nehru Place", "Kalkaji Mandir", 2, "Violet");
    metro.addConnection("Kalkaji Mandir", "Govindpuri", 2, "Violet");
    metro.addConnection("Govindpuri", "Okhla", 2, "Violet");
    metro.addConnection("Okhla", "Jasola", 2, "Violet");
    metro.addConnection("Jasola", "Sarita Vihar", 2, "Violet");
    metro.addConnection("Sarita Vihar", "Mohan Estate", 2, "Violet");
    metro.addConnection("Mohan Estate", "Tughlakabad", 2, "Violet");
    metro.addConnection("Tughlakabad", "Badarpur", 2, "Violet");
    metro.addConnection("Badarpur", "Sarai", 2, "Violet");
    metro.addConnection("Sarai", "NHPC Chowk", 2, "Violet");
    metro.addConnection("NHPC Chowk", "Mewala Maharajpur", 2, "Violet");
    metro.addConnection("Mewala Maharajpur", "Sector 28", 2, "Violet");
    metro.addConnection("Sector 28", "Badkal Mor", 2, "Violet");
    metro.addConnection("Badkal Mor", "Old Faridabad", 2, "Violet");
    metro.addConnection("Old Faridabad", "Neelam Chowk Ajronda", 2, "Violet");
    metro.addConnection("Neelam Chowk Ajronda", "Bata Chowk", 2, "Violet");
    metro.addConnection("Bata Chowk", "Escorts Mujesar", 2, "Violet");
    metro.addConnection("Escorts Mujesar", "Sant Surdas", 2, "Violet");
    metro.addConnection("Sant Surdas", "Raja Nahar Singh", 2, "Violet");

    // PINK LINE (Line 7)
    metro.addConnection("Majlis Park", "Azadpur", 2, "Pink");
    metro.addConnection("Azadpur", "Shalimar Bagh", 2, "Pink");
    metro.addConnection("Shalimar Bagh", "Netaji Subhash Place", 2, "Pink");
    metro.addConnection("Netaji Subhash Place", "Shakurpur", 2, "Pink");
    metro.addConnection("Shakurpur", "Punjabi Bagh West", 2, "Pink");
    metro.addConnection("Punjabi Bagh West", "ESI Hospital", 2, "Pink");
    metro.addConnection("ESI Hospital", "Rajouri Garden", 2, "Pink");
    metro.addConnection("Rajouri Garden", "Mayapuri", 2, "Pink");
    metro.addConnection("Mayapuri", "Naraina Vihar", 2, "Pink");
    metro.addConnection("Naraina Vihar", "Delhi Cantt", 2, "Pink");
    metro.addConnection("Delhi Cantt", "Durgabai Deshmukh South Campus", 2, "Pink");
    metro.addConnection("Durgabai Deshmukh South Campus", "Sir Vishweshwaraiah Moti Bagh", 2, "Pink");
    metro.addConnection("Sir Vishweshwaraiah Moti Bagh", "Bhikaji Cama Place", 2, "Pink");
    metro.addConnection("Bhikaji Cama Place", "Sarojini Nagar", 2, "Pink");
    metro.addConnection("Sarojini Nagar", "INA", 2, "Pink");
    metro.addConnection("INA", "South Extension", 2, "Pink");
    metro.addConnection("South Extension", "Lajpat Nagar", 2, "Pink");
    metro.addConnection("Lajpat Nagar", "Vinobapuri", 2, "Pink");
    metro.addConnection("Vinobapuri", "Ashram", 2, "Pink");
    metro.addConnection("Ashram", "Hazrat Nizamuddin", 2, "Pink");
    metro.addConnection("Hazrat Nizamuddin", "Mayur Vihar Pocket 1", 2, "Pink");
    metro.addConnection("Mayur Vihar Pocket 1", "Trilokpuri", 2, "Pink");
    metro.addConnection("Trilokpuri", "East Vinod Nagar", 2, "Pink");
    metro.addConnection("East Vinod Nagar", "Mandawali", 2, "Pink");
    metro.addConnection("Mandawali", "IP Extension", 2, "Pink");
    metro.addConnection("IP Extension", "Anand Vihar", 2, "Pink");
    metro.addConnection("Anand Vihar", "Karkarduma", 2, "Pink");
    metro.addConnection("Karkarduma", "Karkarduma Court", 2, "Pink");
    metro.addConnection("Karkarduma Court", "Krishna Nagar", 2, "Pink");
    metro.addConnection("Krishna Nagar", "East Azad Nagar", 2, "Pink");
    metro.addConnection("East Azad Nagar", "Welcome", 2, "Pink");
    metro.addConnection("Welcome", "Jafrabad", 2, "Pink");
    metro.addConnection("Jafrabad", "Maujpur", 2, "Pink");
    metro.addConnection("Maujpur", "Gokulpuri", 2, "Pink");
    metro.addConnection("Gokulpuri", "Johri Enclave", 2, "Pink");
    metro.addConnection("Johri Enclave", "Shiv Vihar", 2, "Pink");

    // MAGENTA LINE (Line 8)
    metro.addConnection("Janakpuri West", "Dabri Mor", 2, "Magenta");
    metro.addConnection("Dabri Mor", "Dashrath Puri", 2, "Magenta");
    metro.addConnection("Dashrath Puri", "Palam", 2, "Magenta");
    metro.addConnection("Palam", "Sadar Bazar Cantonment", 2, "Magenta");
    metro.addConnection("Sadar Bazar Cantonment", "Terminal 1 IGI Airport", 2, "Magenta");
    metro.addConnection("Terminal 1 IGI Airport", "Shankar Vihar", 2, "Magenta");
    metro.addConnection("Shankar Vihar", "Vasant Vihar", 2, "Magenta");
    metro.addConnection("Vasant Vihar", "Munirka", 2, "Magenta");
    metro.addConnection("Munirka", "RK Puram", 2, "Magenta");
    metro.addConnection("RK Puram", "IIT Delhi", 2, "Magenta");
    metro.addConnection("IIT Delhi", "Hauz Khas", 2, "Magenta");
    metro.addConnection("Hauz Khas", "Panchsheel Park", 2, "Magenta");
    metro.addConnection("Panchsheel Park", "Chirag Delhi", 2, "Magenta");
    metro.addConnection("Chirag Delhi", "Greater Kailash", 2, "Magenta");
    metro.addConnection("Greater Kailash", "Nehru Enclave", 2, "Magenta");
    metro.addConnection("Nehru Enclave", "Kalkaji Mandir", 2, "Magenta");
    metro.addConnection("Kalkaji Mandir", "Okhla NSIC", 2, "Magenta");
    metro.addConnection("Okhla NSIC", "Sukhdev Vihar", 2, "Magenta");
    metro.addConnection("Sukhdev Vihar", "Jamia Millia Islamia", 2, "Magenta");
    metro.addConnection("Jamia Millia Islamia", "Okhla Vihar", 2, "Magenta");
    metro.addConnection("Okhla Vihar", "Jasola Vihar Shaheen Bagh", 2, "Magenta");
    metro.addConnection("Jasola Vihar Shaheen Bagh", "Kalindi Kunj", 2, "Magenta");
    metro.addConnection("Kalindi Kunj", "Okhla Bird Sanctuary", 2, "Magenta");
    metro.addConnection("Okhla Bird Sanctuary", "Botanical Garden", 2, "Magenta");

    // GREY LINE (Line 9)
    metro.addConnection("Dwarka", "Nangli", 2, "Grey");
    metro.addConnection("Nangli", "Najafgarh", 2, "Grey");
    metro.addConnection("Najafgarh", "Dhansa Bus Stand", 2, "Grey");

    // AQUA LINE (Noida-Greater Noida)
    metro.addConnection("Noida Sector 51", "Noida Sector 50", 2, "Aqua");
    metro.addConnection("Noida Sector 50", "Noida Sector 76", 2, "Aqua");
    metro.addConnection("Noida Sector 76", "Noida Sector 101", 2, "Aqua");
    metro.addConnection("Noida Sector 101", "Noida Sector 81", 2, "Aqua");
    metro.addConnection("Noida Sector 81", "NSEZ", 2, "Aqua");
    metro.addConnection("NSEZ", "Noida Sector 83", 2, "Aqua");
    metro.addConnection("Noida Sector 83", "Noida Sector 137", 2, "Aqua");
    metro.addConnection("Noida Sector 137", "Noida Sector 142", 2, "Aqua");
    metro.addConnection("Noida Sector 142", "Noida Sector 143", 2, "Aqua");
    metro.addConnection("Noida Sector 143", "Noida Sector 144", 2, "Aqua");
    metro.addConnection("Noida Sector 144", "Noida Sector 145", 2, "Aqua");
    metro.addConnection("Noida Sector 145", "Noida Sector 146", 2, "Aqua");
    metro.addConnection("Noida Sector 146", "Noida Sector 147", 2, "Aqua");
    metro.addConnection("Noida Sector 147", "Noida Sector 148", 2, "Aqua");
    metro.addConnection("Noida Sector 148", "Knowledge Park II", 2, "Aqua");
    metro.addConnection("Knowledge Park II", "Pari Chowk", 2, "Aqua");
    metro.addConnection("Pari Chowk", "Alpha 1", 2, "Aqua");
    metro.addConnection("Alpha 1", "Delta 1", 2, "Aqua");
    metro.addConnection("Delta 1", "GNIDA Office", 2, "Aqua");
    metro.addConnection("GNIDA Office", "Depot Station", 2, "Aqua");

    // RAPID METRO (Gurgaon)
    metro.addConnection("Sikanderpur", "Phase 1", 2, "Rapid");
    metro.addConnection("Phase 1", "Phase 2", 2, "Rapid");
    metro.addConnection("Phase 2", "Phase 3", 2, "Rapid");
    metro.addConnection("Phase 3", "Cyber City", 2, "Rapid");

    // ORANGE LINE (Airport Express)
    metro.addConnection("New Delhi", "Shivaji Stadium", 3, "Orange");
    metro.addConnection("Shivaji Stadium", "Dhaula Kuan", 3, "Orange");
    metro.addConnection("Dhaula Kuan", "IGI Airport", 4, "Orange");
    metro.addConnection("IGI Airport", "Dwarka Sector 21", 4, "Orange");
}
