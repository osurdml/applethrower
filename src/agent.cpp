#include <algorithm>
#include <cmath>
#include <cstddef>
#include <climits>
#include "params.hpp"
#include "agent.hpp"

Agent::Agent(int i, Coordinate c)
{
    id = i;
    curLoc = c;
    targetLoc = Coordinate(0, 0);
    curBinId = -1;
    targetBinId = -1;
}

Agent::~Agent()
{
    curBinId = -1;
    targetBinId = -1;
}

int Agent::getBinIndexById(std::vector<AppleBin> bins, int id)
{
    for (int b = 0; b < (int) bins.size(); ++b) {
        if (bins[b].id == id)
            return b;
    }
    return -1;
}

std::vector<int> Agent::getIdleBins(std::vector<AppleBin> bins, std::vector<Agent> agents)
{
    std::vector<int> idleBins;
    
    if (bins.size() == 0)
        return idleBins;
    
    for (int b = 0; b < (int) bins.size(); ++b)
            idleBins.push_back(b);
    
    for (int a = 0; a < (int) agents.size(); ++a) {
        if (agents[a].id == id)
            continue;
        int bIdx = getBinIndexById(bins, agents[a].getCurBinId());
        if (bIdx != -1)
            idleBins[bIdx] = -1;
        int tIdx = getBinIndexById(bins, agents[a].getTargetBinId());
        if (tIdx != -1)
            idleBins[tIdx] = -1;
    }
    
    for (int i = idleBins.size() - 1; i >= 0; --i) {
        if (idleBins[i] == -1)
            idleBins.erase(idleBins.begin() + i);
    }
    
    printf("Idle bins indexes: ");
    for (int i = 0; i < (int) idleBins.size(); ++i)
        printf("%d(%d) ", bins[idleBins[i]].id, idleBins[i]);
    printf("\n");
    
    return idleBins;
}

float Agent::getDistance(Coordinate loc1, Coordinate loc2)
{
    return abs(loc1.x - loc2.x) + abs(loc1.y - loc2.y);
}

int Agent::getFirstEstFullBin(std::vector<int> indexes, std::vector<AppleBin> bins)
{
    int maxBinIdx = -1;
    float maxEstCap = 0;
    
    for (int i = 0; i < (int) indexes.size(); ++i) {
        float estTime = getDistance(curLoc, bins[indexes[i]].loc) / AGENT_SPEED_L;
        float estIncrease = estTime * bins[indexes[i]].fillRate;
        float estCap = bins[indexes[i]].capacity + estIncrease;
        if (estCap > maxEstCap) {
            maxBinIdx = indexes[i];
            maxEstCap = estCap;
        }
    }
    
    return maxBinIdx;
}

int Agent::getClosestFullBin(std::vector<int> indexes, std::vector<AppleBin> bins)
{
    if (indexes.size() == 0)
        return -1;
    
    int minBinIdx = -1;
    int minDist = INT_MAX;
    for (int b = 0; b < (int) indexes.size(); ++b) {
        if (bins[indexes[b]].capacity < BIN_CAPACITY)
            continue;
        int dist = getDistance(curLoc, bins[indexes[b]].loc);
        if (dist < minDist) {
            minBinIdx = indexes[b];
            minDist = dist;
        }
    }
    return minBinIdx;
}

bool Agent::isLocationValid(Coordinate loc)
{
    return (loc.x >= 0 && loc.x < ORCH_COLS && loc.y >= 0 && loc.y < ORCH_ROWS);
}

void Agent::move(AppleBin curBin)
{
    // Check if all locations are valid
    if (!isLocationValid(curLoc) || !isLocationValid(targetLoc))
        return;
    
    int speed = AGENT_SPEED_H;
    if (curBinId != -1 && curBin.capacity > 0)
        speed = AGENT_SPEED_L;
    
    if (targetLoc.x == 0) { // Target location is the repo
        if (curLoc.x != targetLoc.x)
            curLoc.x = (curLoc.x - speed <= targetLoc.x) ? targetLoc.x : curLoc.x - speed; // Move left
    } else {
        if (curLoc.y == targetLoc.y) {
            if (targetLoc.x > curLoc.x)
                curLoc.x = (curLoc.x + speed >= targetLoc.x) ? targetLoc.x : curLoc.x + speed; // Move right
            else
                curLoc.x = (curLoc.x - speed <= targetLoc.x) ? targetLoc.x : curLoc.x - speed; // Move left
        } else { // Need to travel between rows
            if (curLoc.x == 0 || curLoc.x == ORCH_COLS - 1) { // At left/rightmost column; can travel between rows
                if (targetLoc.y > curLoc.y)
                    curLoc.y = (curLoc.y + speed >= targetLoc.y) ? targetLoc.y : curLoc.y + speed; // Move down
                else
                    curLoc.y = (curLoc.y - speed <= targetLoc.y) ? targetLoc.y : curLoc.y - speed; // Move up
            } else {
                int leftDist = curLoc.x;
                int rightDist = ORCH_COLS - 1 - curLoc.x;
                if (leftDist < rightDist) // Travel between rows through the leftmost column
                    curLoc.x = (curLoc.x - speed <= targetLoc.x) ? targetLoc.x : curLoc.x - speed; // Move left
                else
                    curLoc.x = (curLoc.x + speed >= targetLoc.x) ? targetLoc.x : curLoc.x + speed; // Move right
            }
        }
    }
}

void Agent::takeAction(int *binCounter, std::vector<AppleBin> &bins, std::vector<AppleBin> &repo, 
    std::vector<Agent> &agents, Orchard env, std::vector<Coordinate> &newLocs)
{
    if (targetBinId == -1) { // Agent is idle
        // Find an idle bin to be picked up
        std::vector<int> idleBins = getIdleBins(bins, agents);
        if (idleBins.size() > 0) { // There are idle bins
            printf("A%d(%d,%d) sees %d idle bins.\n", id, curLoc.x, curLoc.y, (int) idleBins.size());
            // Choose an existing bin to pick up
            int tmpIdx = getClosestFullBin(idleBins, bins);
            if (tmpIdx != -1) {
                targetBinId = bins[tmpIdx].id;
                targetLoc = bins[tmpIdx].loc;
            } else {
                tmpIdx = getFirstEstFullBin(idleBins, bins);
                if (tmpIdx != -1) {
                    targetBinId = bins[tmpIdx].id;
                    targetLoc = bins[tmpIdx].loc;
                }
            }
            if (targetBinId != -1) {
                int tIdx = getBinIndexById(bins, targetBinId);
                float estTime = ceil(getDistance(curLoc, bins[tIdx].loc) / AGENT_SPEED_L);
                float estApples = env.getEstApplesRemaining(bins[tIdx].loc, estTime, bins[tIdx].fillRate);
                if (estApples > 0) { // There are still apples in target location; need a new bin
                    curBinId = (*binCounter);
                    printf("A%d takes a new bin B%d.\n", id, curBinId);
                    bins.push_back(AppleBin(curBinId, curLoc.x, curLoc.y));
                    (*binCounter)++;
                }
                int cIdx = getBinIndexById(bins, curBinId);
                move(bins[cIdx]);
                if (cIdx != -1)
                    bins[cIdx].loc = curLoc;
                printf("A%d(%d,%d) moves to pick up B%d at (%d,%d).\n", id, curLoc.x, curLoc.y, targetBinId, 
                    targetLoc.x, targetLoc.y);
            }
        } else if (newLocs.size() > 0) { // There's a new harvest location without bin
            printf("A%d sees %d new locations without bins.\n", id, (int) newLocs.size());
            Coordinate newLoc = newLocs[0];
            bool newLocFlag = false;
            for (int a = 0; a < (int) agents.size() && !newLocFlag; ++a) {
                Coordinate tmp = agents[a].getTargetLoc();
                int tmpBinId = agents[a].getCurBinId();
                if (tmp.x == newLoc.x && tmp.y == newLoc.y && tmpBinId != -1 && bins[tmpBinId].capacity == 0)
                    newLocFlag = true; // An agent is already on the way to the new location with empty bin
            }
            if (!newLocFlag) { // No agent is targeting the new location
                curBinId = (*binCounter);
                bins.push_back(AppleBin(curBinId, curLoc.x, curLoc.y));
                targetBinId = -1;
                targetLoc = newLocs[0];
                newLocs.erase(newLocs.begin());
                int cIdx = getBinIndexById(bins, curBinId);
                move(bins[cIdx]);
                if (cIdx != -1)
                    bins[cIdx].loc = curLoc;
                printf("A%d(%d,%d) carries new bin to (%d,%d).\n", id, curLoc.x, curLoc.y, targetLoc.x, targetLoc.y);
            }
        }
    } else {
        /* Agent is not idle (i.e. moving towards a bin or waiting for a bin) */
        int curBinIdx = getBinIndexById(bins, curBinId);
        if (curBinId == targetBinId || (curBinId != -1 && bins[curBinIdx].capacity >= BIN_CAPACITY)) {
            // Agent is carrying the target bin, go to repo (column 0 at every row)
            targetLoc = getRepoLocation();
            int cIdx = getBinIndexById(bins, curBinId);
            move(bins[cIdx]);
            if (cIdx != -1)
                bins[cIdx].loc = curLoc;
            printf("A%d moves to (%d,%d). Destination: REPO.\n", id, curLoc.x, curLoc.y);
        } else { // Agent is on the way to pick up the target bin; it may or may not be carrying an empty bin
            if (curLoc.x == targetLoc.x && curLoc.y == targetLoc.y) { // Arrived at target location
                printf("A%d arrives at target (%d,%d).\n", id, targetLoc.x, targetLoc.y);
                int tIdx = getBinIndexById(bins, targetBinId);
                float cap = bins[tIdx].capacity;
                if (cap >= BIN_CAPACITY) { // Target bin is full
                    if (curBinId != -1) { // Agent is carrying an empty bin
                        int eIdx = getBinIndexById(bins, curBinId);
                        bins[eIdx].loc = curLoc;
                    }
                    int cIdx = getBinIndexById(bins, curBinId);
                    curBinId = targetBinId; // pick up target bin
                    printf("A%d(%d,%d) picks up B%d(%d,%d).\n", id, curLoc.x, curLoc.y, curBinId, 
                        bins[cIdx].loc.x, bins[cIdx].loc.y);
                    targetLoc = getRepoLocation();
                    move(bins[cIdx]);
                    if (cIdx != -1)
                        bins[cIdx].loc = curLoc;
                    printf("A%d moves to (%d,%d). Target: B%d.\n", id, curLoc.x, curLoc.y, targetBinId);
                } else { // else, if agent has arrived at target location, but target bin is not full yet, agent waits
                    int tIdx = getBinIndexById(bins, targetBinId);
                    printf("A%d(%d,%d) is waiting for B%d(%d,%d) to be filled.\n", id, curLoc.x, curLoc.y, targetBinId, 
                        bins[tIdx].loc.x, bins[tIdx].loc.y);
                }
            } else {
                int cIdx = getBinIndexById(bins, curBinId);
                move(bins[cIdx]);
                if (cIdx != -1)
                    bins[cIdx].loc = curLoc;
                printf("A%d moves to (%d,%d). Target: B%d.\n", id, curLoc.x, curLoc.y, targetBinId);
            }
        }
    }
    
    if (curLoc.x == 0) {
        // Arrived at REPO
        if (curBinId != -1 && bins[getBinIndexById(bins, curBinId)].capacity >= BIN_CAPACITY) { // Carrying a full bin
            int idx = getBinIndexById(bins, curBinId);
            if (idx >= 0 && idx < (int) bins.size()) {
                repo.push_back(copyBin(bins[idx])); // Put carried bin in repo
                printf("A%d(%d,%d) put B%d in REPO.\n", id, curLoc.x, curLoc.y, curBinId);
                bins.erase(bins.begin() + idx);
                // Reset all
                curBinId = -1;
                targetBinId = -1;
                targetLoc = Coordinate(-1, -1);
            } else {
                printf("A%d current bin ID B%d, index %d?\n", id, curBinId, idx);
            }
        }
    }
}

AppleBin Agent::copyBin(AppleBin ab)
{
    AppleBin nb(ab.id, ab.loc.x, ab.loc.y);
    nb.capacity = ab.capacity;
    return nb;
}



