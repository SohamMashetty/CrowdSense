// CrowdPredictor.h

#ifndef CROWD_PREDICTOR_H
#define CROWD_PREDICTOR_H

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <set>
#include <queue>
#include <unordered_map>

using namespace std;

class CrowdPredictor {
public:
    CrowdPredictor();
    void loadRoutes(const string& filename);
    void loadStopTimes(const string& filename);
    void buildGraph();
    void addWalkingEdges(int maxWalkKm = 1);
    vector<tuple<string, int, string>> findFastestPath(
        const string& start,
        const string& end,
        const string& earliest,
        const string& deadline,
        int maxTransfers = -1
    );
    static int timeDiffMinutes(const string& t1, const string& t2);
    static string addMinutes(const string& timeStr, int minutes);
    static bool isNearby(const string& stop1, const string& stop2);

private:
    map<string, vector<string>> routes;
    map<string, map<string, vector<string>>> stopTimes;
    unordered_map<string, vector<tuple<string, int, string>>> graph;  // node -> [(next_node, weight, route)]
};

#endif
