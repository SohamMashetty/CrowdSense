// predictor.cpp (Fixed JSON-style parser for your stop_times.json and routes.json)

#include "predictor.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <cstring>

using namespace std;

CrowdPredictor::CrowdPredictor() {}

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    size_t end = s.find_last_not_of(" \t\n\r");
    return (start == string::npos || end == string::npos) ? "" : s.substr(start, end - start + 1);
}

void CrowdPredictor::loadRoutes(const string& filename) {
    ifstream in(filename);
    if (!in) {
        cerr << "Failed to open routes file." << endl;
        return;
    }

    string line, route;
    while (getline(in, line)) {
        if (line.find(": [") != string::npos) {
            size_t route_start = line.find('"') + 1;
            size_t route_end = line.find('"', route_start);
            route = line.substr(route_start, route_end - route_start);
            size_t bracket_start = line.find('[');
            size_t bracket_end = line.find(']');
            string stops_str = line.substr(bracket_start + 1, bracket_end - bracket_start - 1);
            istringstream ss(stops_str);
            string stop;
            vector<string> stops;
            while (getline(ss, stop, ',')) {
                stop.erase(remove(stop.begin(), stop.end(), '"'), stop.end());
                stops.push_back(trim(stop));
            }
            routes[route] = stops;
        }
    }
    in.close();
}

void CrowdPredictor::loadStopTimes(const string& filename) {
    ifstream in(filename);
    if (!in) {
        cerr << "Failed to open stop times file." << endl;
        return;
    }

    string line, currentRoute, currentStop;
    while (getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;

        if (line.back() == '{' && line.find('"') != string::npos) {
            size_t start = line.find('"') + 1;
            size_t end = line.find('"', start);
            currentRoute = line.substr(start, end - start);
        }
        else if (line.back() == '[') {
            size_t start = line.find('"') + 1;
            size_t end = line.find('"', start);
            currentStop = line.substr(start, end - start);

            vector<string> times;
            while (getline(in, line)) {
                line = trim(line);
                if (line.find(']') != string::npos) break;
                line.erase(remove(line.begin(), line.end(), '"'), line.end());
                line.erase(remove(line.begin(), line.end(), ','), line.end());
                times.push_back(trim(line));
            }
            stopTimes[currentRoute][currentStop] = times;
        }
    }
    in.close();
}

void CrowdPredictor::buildGraph() {
    for (map<string, vector<string>>::iterator rit = routes.begin(); rit != routes.end(); ++rit) {
        string route = rit->first;
        vector<string>& stops = rit->second;
        map<string, vector<string>>& times = stopTimes[route];

        for (size_t i = 0; i + 1 < stops.size(); ++i) {
            string from = stops[i];
            string to = stops[i + 1];
            vector<string>& fromTimes = times[from];
            vector<string>& toTimes = times[to];

            size_t count = min(fromTimes.size(), toTimes.size());
            for (size_t j = 0; j < count; ++j) {
                string fromNode = from + "@" + fromTimes[j];
                string toNode = to + "@" + toTimes[j];
                int duration = timeDiffMinutes(fromTimes[j], toTimes[j]);
                if (duration >= 0)
                    graph[fromNode].push_back(make_tuple(toNode, duration, route));
            }
        }
    }

    vector<string> allNodes;
    for (auto& kv : graph)
        allNodes.push_back(kv.first);

    for (size_t i = 0; i < allNodes.size(); ++i) {
        string stop1 = allNodes[i].substr(0, allNodes[i].find('@'));
        string t1 = allNodes[i].substr(allNodes[i].find('@') + 1);
        for (size_t j = 0; j < allNodes.size(); ++j) {
            if (i == j) continue;
            string stop2 = allNodes[j].substr(0, allNodes[j].find('@'));
            string t2 = allNodes[j].substr(allNodes[j].find('@') + 1);
            if (stop1 == stop2) {
                int delta = timeDiffMinutes(t1, t2);
                if (delta > 0 && delta <= 30) {
                    int penalty = delta + 5;
                    graph[allNodes[i]].push_back(make_tuple(allNodes[j], penalty, "transfer"));
                }
            }
        }
    }
}

vector<tuple<string, int, string>> CrowdPredictor::findFastestPath(
    const string& start,
    const string& end,
    const string& earliest,
    const string& deadline,
    int maxTransfers
) {
    vector<tuple<string, int, string>> result;
    set<string> visited;
    typedef tuple<int, string, vector<tuple<string, int, string>>, int, string> State;
    priority_queue<State, vector<State>, greater<State>> pq;

    for (auto& kv : graph) {
        string node = kv.first;
        string stop = node.substr(0, node.find('@'));
        string time = node.substr(node.find('@') + 1);
        if (stop == start && time >= earliest && time <= deadline) {
            pq.push(make_tuple(0, node, vector<tuple<string, int, string>>(), 0, ""));
        }
    }

    int bestScore = 1e9;
    while (!pq.empty()) {
        int cost = get<0>(pq.top());
        string current = get<1>(pq.top());
        vector<tuple<string, int, string>> path = get<2>(pq.top());
        int transfers = get<3>(pq.top());
        string lastRoute = get<4>(pq.top());
        pq.pop();

        if (visited.count(current)) continue;
        visited.insert(current);

        string stop = current.substr(0, current.find('@'));
        string time = current.substr(current.find('@') + 1);

        if (stop == end && time <= deadline) {
            path.push_back(make_tuple(current, cost, lastRoute));
            return path;
        }

        for (size_t i = 0; i < graph[current].size(); ++i) {
            string neighbor = get<0>(graph[current][i]);
            int weight = get<1>(graph[current][i]);
            string route = get<2>(graph[current][i]);

            if (visited.count(neighbor)) continue;
            string ntime = neighbor.substr(neighbor.find('@') + 1);
            if (ntime < earliest || ntime > deadline) continue;

            int newTransfers = transfers;
            if (route != lastRoute && !lastRoute.empty() && route != "transfer" && route != "walk") {
                newTransfers++;
            }
            if (maxTransfers != -1 && newTransfers > maxTransfers) continue;

            double tieBreaker = (route == lastRoute || route == "transfer" || route == "walk") ? 0.0 : 0.01;
            vector<tuple<string, int, string>> newPath = path;
            newPath.push_back(make_tuple(current, cost, lastRoute));
            pq.push(make_tuple(cost + weight + tieBreaker, neighbor, newPath, newTransfers, route));
        }
    }

    return result;
}

void CrowdPredictor::addWalkingEdges(int maxWalkKm) {
    map<string, vector<pair<string, string>>> stopTimesByStop;
    for (auto& kv : graph) {
        string stop = kv.first.substr(0, kv.first.find('@'));
        string time = kv.first.substr(kv.first.find('@') + 1);
        stopTimesByStop[stop].push_back(make_pair(kv.first, time));
    }

    vector<string> allStops;
    for (map<string, vector<pair<string, string>>>::iterator it = stopTimesByStop.begin(); it != stopTimesByStop.end(); ++it)
        allStops.push_back(it->first);

    for (size_t i = 0; i + 1 < allStops.size(); ++i) {
        for (size_t j = i + 1; j < allStops.size(); ++j) {
            string a = allStops[i], b = allStops[j];
            if (!isNearby(a, b)) continue;
            for (size_t u = 0; u < stopTimesByStop[a].size(); ++u) {
                string nodeA = stopTimesByStop[a][u].first;
                string t = stopTimesByStop[a][u].second;
                string arrivalB = addMinutes(t, 15);
                string nodeB = b + "@" + arrivalB;
                if (graph.find(nodeB) != graph.end()) {
                    graph[nodeA].push_back(make_tuple(nodeB, 15, "walk"));
                }
            }
        }
    }
}

int CrowdPredictor::timeDiffMinutes(const string& t1, const string& t2) {
    int h1, m1, h2, m2;
    sscanf(t1.c_str(), "%d:%d", &h1, &m1);
    sscanf(t2.c_str(), "%d:%d", &h2, &m2);
    int diff = (h2 * 60 + m2) - (h1 * 60 + m1);
    return diff >= 0 ? diff : -1;
}

string CrowdPredictor::addMinutes(const string& timeStr, int minutes) {
    int h, m;
    sscanf(timeStr.c_str(), "%d:%d", &h, &m);
    int total = h * 60 + m + minutes;
    char buf[6];
    sprintf(buf, "%02d:%02d", (total / 60) % 24, total % 60);
    return string(buf);
}

bool CrowdPredictor::isNearby(const string& stop1, const string& stop2) {
    set<pair<string, string>> nearby = {
        {"BHEL", "RC_Puram"},
        {"RC_Puram", "Patancheru"},
        {"Lingampally", "BHEL"},
        {"Lingampally", "Miyapur"},
        {"Gachibowli", "Lingampally"}
    };
    return nearby.count(make_pair(stop1, stop2)) || nearby.count(make_pair(stop2, stop1));
}
