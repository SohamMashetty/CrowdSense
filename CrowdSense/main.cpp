// main.cpp

#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include "predictor.h"

using namespace std;

int main() {
    CrowdPredictor cp;
    cp.loadRoutes("routes.json");
    cp.loadStopTimes("stop_times.json");
    cp.buildGraph();
    cp.addWalkingEdges();

    string start, end, earliest, deadline, transferCap;
    cout << "Enter starting stop: ";
    getline(cin, start);
    cout << "Enter destination stop: ";
    getline(cin, end);
    cout << "Enter earliest departure time (HH:MM): ";
    getline(cin, earliest);
    cout << "Enter latest acceptable arrival time (HH:MM): ";
    getline(cin, deadline);
    cout << "Enter max transfers allowed (or * for unlimited): ";
    getline(cin, transferCap);

    int maxTransfers = -1;
    if (transferCap != "*") {
        maxTransfers = stoi(transferCap);
    }

    vector<tuple<string, int, string>> path = cp.findFastestPath(start, end, earliest, deadline, maxTransfers);

    if (path.empty()) {
        cout << "No route found within the deadline.\n";
    } else {
        cout << "\nRecommended Route:\n\n";

        bool prevWasTransfer = false;
        for (size_t i = 0; i + 1 < path.size(); ++i) {
            string node = get<0>(path[i]);
            int cost = get<1>(path[i]);
            string route = get<2>(path[i]);

            string nextNode = get<0>(path[i + 1]);
            string stop = node.substr(0, node.find('@'));
            string time = node.substr(node.find('@') + 1);
            string nextStop = nextNode.substr(0, nextNode.find('@'));

            if (route == "transfer") {
                cout << "\nTransfer at " << stop << " to " << nextStop << " at " << time << "\n";
                prevWasTransfer = true;
            } else if (route == "walk") {
                cout << "Walk from " << stop << " at " << time << " to " << nextStop << "\n";
                prevWasTransfer = false;
            } else {
                if (prevWasTransfer) cout << "\n";  // print extra line only after transfer
                cout << "Take Route " << route << " from " << stop << " at " << time << " to " << nextStop << "\n";
                prevWasTransfer = false;
            }
        }

        string lastNode = get<0>(path.back());
        string finalStop = lastNode.substr(0, lastNode.find('@'));
        string finalTime = lastNode.substr(lastNode.find('@') + 1);
        int totalTime = get<1>(path.back());

        cout << "\nArrive at " << finalStop << " by " << finalTime << "\n";
        cout << "Total travel time: " << totalTime << " minutes\n";
    }

    return 0;
}
