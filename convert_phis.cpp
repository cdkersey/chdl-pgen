#include <algorithm>

#include "if.h"
#include "convert_phis.h"

using namespace std;
using namespace bscotch;

static void live_in_phi_adj(if_bb &b) {
  for (auto &v : b.vals) {
    if (v->op == VAL_PHI) {
      for (auto &a : v->args) {
        auto it = find(b.live_in.begin(), b.live_in.end(), a);
        if (it != b.live_in.end())
          b.live_in.erase(it);
      }
      b.live_in.push_back(v);
    }
  }
}

void bscotch::convert_phis(if_func &f) {
  for (auto &b : f.bbs)
    live_in_phi_adj(*b);
}
