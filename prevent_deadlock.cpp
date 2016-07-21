#include <algorithm>

#include "prevent_deadlock.h"
#include "find_back_edges.h"
#include "if.h"

using namespace std;
using namespace bscotch;

// Reorder predecessor blocks to prevent deadlock.
void bscotch::prevent_deadlock(if_func &f) {
  map<if_bb*, int> pos;
  int i = 0;
  for (auto &b : f.bbs) pos[b] = i++;
  
  for (auto &b : f.bbs) {
    map<if_bb*, int> score;
    vector<pair<if_bb*, int> > perm;
    int i = 0;
    for (auto &p : b->pred) {
      perm.push_back(make_pair(p, i++));
      score[p] = (pos[b] - pos[p]);
      if (score[p] <= 0) score[p] += 1000000;
    }

    std::sort(
      perm.begin(), perm.end(),
      [&score](pair<if_bb *, int> a, pair<if_bb *, int> b)->bool {
        return score[a.first] > score[b.first];
      }
    );

    cout << "BB " << b->id << " predecessor reordering:";
    for (auto &x : perm)
      cout << ' ' << x.first->id << '(' << score[x.first] << ')';
    cout << endl;

    // Reorder predicate vector and phi args
    i = 0;
    for (auto &p : perm)
      b->pred[i++] = p.first;

    for (auto &v : b->vals) {
      if (v->op == VAL_PHI) {
        vector<if_val *> new_args;
        for (auto &p : perm)
          new_args.push_back(v->args[p.second]);
        v->args = new_args;
      }
    }
  }
}
