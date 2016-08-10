#ifndef CHDL_PGEN_PREVENT_DEADLOCK
#define CHDL_PGEN_PREVENT_DEADLOCK

#include "if.h"

namespace pgen {
  // Reorder predecessor blocks to prevent deadlock.
  void prevent_deadlock(if_func &f);
};

#endif
