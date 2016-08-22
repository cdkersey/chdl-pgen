#ifndef CHDL_PGEN_REMOVE_COPIES
#define CHDL_PGEN_REMOVE_COPIES

#include <set>
#include <utility>

#include "if.h"

namespace pgen {

  // Remove generated "x = cat y" instructions. Needed for proper handling of
  // 1-cycle latency array reads.
  void remove_copies(if_func &f);
};

#endif
