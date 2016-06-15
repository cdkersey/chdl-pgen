#ifndef BSCOTCH_BREAK_CYCLES
#define BSCOTCH_BREAK_CYCLES

#include "if.h"

namespace bscotch {
  // Add a cycle_breaker tag to one basic block in every cycle in the CFG
  void break_cycles(if_func &f);
};

#endif
