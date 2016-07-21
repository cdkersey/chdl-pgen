#include "prevent_deadlock.h"
#include "find_back_edges.h"
#include "if.h"

using namespace std;
using namespace bscotch;

void find_loop_blocks(vector<set<if_bb*> > &loops,
                 const set<cfg_edge_t> &back_edges,
                 if_func &f)
{
}

// Reorder predecessor blocks to prevent deadlock.
void bscotch::prevent_deadlock(if_func &f) {
  // Find back edges
  set<cfg_edge_t> back_edges;
  find_back_edges(back_edges, f);

  cout << "Back edges:";
  for (auto &x : back_edges)
    cout << ' ' << x.first << " -> " << x.second;
  cout << endl;

  // Find all blocks in loops.
  vector<set<if_bb*> > loops;
  find_loop_blocks(loops, back_edges, f);

  // Sort loops by size.

  // Find loop nest structure.
}
