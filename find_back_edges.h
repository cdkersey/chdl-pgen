#ifndef BSCOTCH_FIND_BACK_EDGES
#define BSCOTCH_FIND_BACK_EDGES

#include <set>
#include <utility>

#include "if.h"

namespace bscotch {
  typedef std::pair<if_bb*, if_bb*> cfg_edge_t;

  // Find back edges (identified as pairs of basic block pointers) in CFG
  void find_back_edges(std::set<cfg_edge_t> &e, if_func &f);
};

#endif
