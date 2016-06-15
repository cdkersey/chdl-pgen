#include "break_cycles.h"
#include "find_back_edges.h"

using namespace bscotch;
using namespace std;

void bscotch::break_cycles(if_func &f) {
  set<cfg_edge_t> back_edges;
  find_back_edges(back_edges, f);

  for (auto &x : back_edges)
    x.first->cycle_breaker = true;
}
