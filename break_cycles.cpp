#include "break_cycles.h"
#include "find_back_edges.h"

using namespace pgen;
using namespace std;

void pgen::break_cycles(if_func &f) {
  set<cfg_edge_t> back_edges;
  find_back_edges(back_edges, f);

  for (auto &x : back_edges)
    x.second->cycle_breaker = true;
}
