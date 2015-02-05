#include <vector>
#include <tuple>
#include <iostream>
#include <iomanip>      // std::setw
#include <queue>
#include <opencv2/features2d/features2d.hpp>  // DMatch

using namespace std;
using namespace cv;

static const int NOT_ENGAGED = -1;

/**
 * Returns a stable marriage in the form an int array v,
 * where v[i] is the man married to woman i.
 */
void marriageMatch(const vector<vector<DMatch>>& acceptor_table,
                   const vector<vector<DMatch>>& proposor_table,
                   const int k,
                   vector<DMatch>& matches)
{
    // Indicates that acceptor i is currently enganged
    // to the proposer v[i]
    vector<int> engagements(proposor_table.size(), NOT_ENGAGED);

    // queue of proposers that are not currently engaged
    queue<int> free_proposers;
    for (int i = 0; i < proposor_table.size(); i++) {
        free_proposers.push(i);
    }

    // next[i] is the index of the accecptor to whom
    // proposer[i] has not yet proposed
    vector<int> next(k, 0);

    while (!free_proposers.empty()) {

        int p = free_proposers.front();
        free_proposers.pop();

        if (next[p] >= proposor_table[p].size()) {
            // cout << "Debug! :) " << free_proposers.size() << flush << endl;
            continue;
        }

        // cout << "next[p] = " << next[p] << endl;
        int a = proposor_table[p][next[p]].trainIdx;
        next[p] = next[p] + 1;

        // cout << "next[p] = " << next[p] << endl;

        // if the acceptor has not been engaged yet,
        // the proposal get immediately accepted
        if (engagements[a] == NOT_ENGAGED) {
            engagements[a] = p;
        } else {
            // the current fiance of the 
            int fiance = engagements[a];

            for (int i = 0; i < acceptor_table[a].size(); i++) {
                assert(a == acceptor_table[a][i].queryIdx);

                int preference = acceptor_table[a][i].trainIdx;

                // the fiance has a higher preference
                // for the acceptor
                if (preference == fiance) {
                    free_proposers.push(p);
                    break;
                }
                // the current proposer has a higher preference
                // than the fiance
                else {
                    engagements[a] = p;
                    free_proposers.push(fiance);
                    break;
                }
            }
        }
    }

    matches.clear();

    cout << "Resolve matches ... " << endl;

    for (int i = 0; i < engagements.size(); i++) {
        if (engagements[i] != NOT_ENGAGED) {
            // ensure that the acceptor has this number of
            // neighbors
            if (i < acceptor_table[i].size()) {
                // search for the DMatch where the related
                // train index (index of the keypoint in the other
                // image) equals the index of the husband
                for (int j = 0; j < acceptor_table[i].size(); j++) {
                    if (acceptor_table[i][j].trainIdx == engagements[i]) {
                        matches.push_back(acceptor_table[i][j]);
                        break;
                    }
                }
            }
        }
    }

    cout << "Done" << flush << endl;
}