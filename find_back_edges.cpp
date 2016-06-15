#include "find_back_edges.h"

#include <iostream>

using namespace std;
using namespace bscotch;

// This is probably exponential in the number of branches in the worst case; its
// performance may be an issue in the future.
void fbe_dfs(if_bb *b, set<cfg_edge_t> &e, set<if_bb *> &a) {
  a.insert(b);

  for (auto &p : b->suc)
    if (!a.count(p))
      fbe_dfs(p, e, a);
    else
      e.insert(make_pair(b, p));

  a.erase(b);
}

// Use depth-first search to find back edges.
void bscotch::find_back_edges(set<cfg_edge_t> &e, if_func &f) {
  set<if_bb *> a; // Ancenstor blocks
  fbe_dfs(&f.bbs[0], e, a);
}
