#ifndef CHDL_PGEN_CONVERT_PHIS
#define CHDL_PGEN_CONVERT_PHIS

#include <set>
#include <utility>

#include "if.h"

namespace pgen {
  typedef std::pair<if_bb*, if_bb*> cfg_edge_t;

  // Replace phi inputs in live sets with the phi values themselves
  void convert_phis(if_func &f);
};

#endif
